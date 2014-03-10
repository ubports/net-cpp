/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include "easy.h"

#include "shared.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stack>
#include <thread>

namespace easy = ::curl::easy;

namespace
{

struct Init
{
    Init()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~Init()
    {
        curl_global_cleanup();
    }
} init;

template<std::size_t capacity>
struct Pool
{
    static Pool<capacity>& instance()
    {
        static Pool<capacity> inst;
        return inst;
    }

    std::shared_ptr<CURL> acquire_or_wait_for(const std::chrono::microseconds& timeout)
    {
        std::shared_ptr<CURL> result = nullptr;
        std::unique_lock<std::mutex> ul(guard);
        if (!handles.empty())
        {
            result = handles.top();
            handles.pop();
        } else if (size < capacity)
        {
            auto handle = curl_easy_init();
            if (handle)
            {
                curl_easy_setopt(handle, curl::option::sharing, shared.native());
                size++;
                result = std::shared_ptr<CURL>(handle, [this](CURL* ptr)
                {
                    curl_easy_cleanup(ptr);
                });
                handles.push(result);
            }
        } else
        {
            if (wait_condition.wait_for(ul, timeout, [this]() { return !handles.empty(); }))
            {
                result = handles.top();
                handles.pop();
            }
        }

        // We are reusing handles, thus we need to make sure to reset options.
        if (result)
            curl_easy_reset(result.get());

        return result;
    }

    void release(const std::shared_ptr<CURL>& handle)
    {
        if (!handle)
            return;

        std::unique_lock<std::mutex> ul(guard);
        handles.push(handle);
        wait_condition.notify_one();
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    std::atomic<std::size_t> size;
    std::stack<std::shared_ptr<CURL>> handles;
    ::curl::shared::Handle shared;
};
}

std::ostream& curl::operator<<(std::ostream& out, curl::Code code)
{
    return out << curl_easy_strerror(static_cast<CURLcode>(code));
}

struct easy::Handle::Private
{
    Private() : handle(Pool<100>::instance().acquire_or_wait_for(std::chrono::microseconds{1000}))
    {
    }

    ~Private()
    {
        Pool<100>::instance().release(handle);
    }

    std::shared_ptr<CURL> handle;

    easy::Handle::OnFinished on_finished_cb;
    easy::Handle::OnProgress on_progress;
    easy::Handle::OnReadData on_read_data_cb;
    easy::Handle::OnWriteData on_write_data_cb;
    easy::Handle::OnWriteHeader on_write_header_cb;
};

int easy::Handle::progress_cb(void* data, double dltotal, double dlnow, double ultotal, double ulnow)
{
    static const int continue_operation = 0;

    auto thiz = static_cast<easy::Handle::Private*>(data);

    if (thiz && thiz->on_progress)
    {
        return thiz->on_progress(data, dltotal, dlnow, ultotal, ulnow);
    }

    return continue_operation;
}

std::size_t easy::Handle::write_data_cb(char* data, size_t size, size_t nmemb, void* cookie)
{
    static const std::size_t did_not_consume_any_data = 0;

    auto thiz = static_cast<easy::Handle::Private*>(cookie);

    if (thiz && thiz->on_write_data_cb)
    {
        return thiz->on_write_data_cb(data, size, nmemb);
    }

    return did_not_consume_any_data;
}

std::size_t easy::Handle::write_header_cb(void* data, size_t size, size_t nmemb, void* cookie)
{
    static const std::size_t did_not_consume_any_data = 0;

    auto thiz = static_cast<easy::Handle::Private*>(cookie);

    if (thiz && thiz->on_write_header_cb)
    {
        return thiz->on_write_header_cb(data, size, nmemb);
    }

    return did_not_consume_any_data;
}

std::size_t easy::Handle::read_data_cb(void* data, std::size_t size, std::size_t nmemb, void *cookie)
{
    static const std::size_t did_not_consume_any_data = 0;

    auto thiz = static_cast<easy::Handle::Private*>(cookie);

    if (thiz && thiz->on_read_data_cb)
    {
        return thiz->on_read_data_cb(data, size, nmemb);
    }

    return did_not_consume_any_data;
}

easy::Handle::Handle() : d(new Private())
{
}

easy::Handle& easy::Handle::url(const char* url)
{
    curl_easy_setopt(
                d->handle.get(),
                option::url,
                url);

    return *this;
}

easy::Handle& easy::Handle::user_agent(const char* user_agent)
{
    curl_easy_setopt(
                d->handle.get(),
                option::user_agent,
                user_agent);

    return *this;
}

easy::Handle& easy::Handle::http_credentials(const std::string& username, const std::string& pwd)
{
    curl_easy_setopt(
                d->handle.get(),
                option::username,
                username.c_str());

    curl_easy_setopt(
                d->handle.get(),
                option::password,
                pwd.c_str());

    return *this;
}

easy::Handle& easy::Handle::on_finished(const easy::Handle::OnFinished& on_finished)
{
    d->on_finished_cb = on_finished;
    return *this;
}

easy::Handle& easy::Handle::on_progress(const easy::Handle::OnProgress& on_progress)
{
    curl_easy_setopt(
                d->handle.get(),
                option::no_progress,
                curl::easy::disable);

    curl_easy_setopt(
                d->handle.get(),
                option::progress_function,
                Handle::progress_cb);

    curl_easy_setopt(
                d->handle.get(),
                option::progress_data,
                d.get());

    d->on_progress = on_progress;

    return *this;
}

easy::Handle& easy::Handle::on_read_data(const easy::Handle::OnReadData& on_read_data, std::size_t size)
{
    curl_easy_setopt(
                d->handle.get(),
                option::read_function,
                Handle::read_data_cb
                );

    curl_easy_setopt(
                d->handle.get(),
                option::read_data,
                d.get());

    curl_easy_setopt(
                d->handle.get(),
                option::in_file_size,
                size);

    d->on_read_data_cb = on_read_data;

    return *this;
}

easy::Handle& easy::Handle::on_write_data(const easy::Handle::OnWriteData& on_new_data)
{
    curl_easy_setopt(
                d->handle.get(),
                option::write_function,
                Handle::write_data_cb);

    curl_easy_setopt(
                d->handle.get(),
                option::write_data,
                d.get());

    d->on_write_data_cb = on_new_data;

    return *this;
}

easy::Handle& easy::Handle::on_write_header(const easy::Handle::OnWriteHeader& on_new_header)
{
    curl_easy_setopt(
                d->handle.get(),
                option::header_function,
                Handle::write_header_cb
                );

    curl_easy_setopt(
                d->handle.get(),
                option::header_data,
                d.get());

    d->on_write_header_cb = on_new_header;

    return *this;
}

easy::Handle& easy::Handle::method(core::net::http::Method method)
{
    switch(method)
    {
    case core::net::http::Method::get:
        curl_easy_setopt(d->handle.get(), option::http_get, enable);
        break;
    case core::net::http::Method::head:
        curl_easy_setopt(d->handle.get(), option::http_get, disable);
        curl_easy_setopt(d->handle.get(), option::http_put, disable);
        curl_easy_setopt(d->handle.get(), option::http_post, disable);
        break;
    case core::net::http::Method::post:
        curl_easy_setopt(d->handle.get(), option::http_post, enable);
        break;
    case core::net::http::Method::put:
        curl_easy_setopt(d->handle.get(), option::http_put, enable);
        break;
    default: throw core::net::http::Client::Errors::HttpMethodNotSupported{method};
    }

    return *this;
}

easy::Handle& easy::Handle::post_data(const std::string& data, const core::net::http::ContentType&)
{
    long content_length = data.size();
    curl_easy_setopt(d->handle.get(), option::post_field_size, content_length);
    curl_easy_setopt(d->handle.get(), option::copy_postfields, data.c_str());

    return *this;
}

core::net::http::Status easy::Handle::status()
{
    long result;
    curl_easy_getinfo(
                d->handle.get(),
                info::response_code,
                &result);
    return static_cast<core::net::http::Status>(result);
}

curl::Native easy::Handle::native() const
{
    return d->handle.get();
}

curl::Code easy::Handle::perform()
{
    return static_cast<curl::Code>(curl_easy_perform(d->handle.get()));
}

void easy::Handle::notify_finished(curl::Code code)
{
    if (d->on_finished_cb)
        d->on_finished_cb(code);
}
