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
namespace shared = ::curl::shared;

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
}

std::ostream& curl::operator<<(std::ostream& out, curl::Code code)
{
    return out << curl_easy_strerror(static_cast<CURLcode>(code));
}

void curl::easy::native::perform(curl::easy::native::Handle handle)
{
    curl::easy::throw_if_not<curl::Code::ok>(
                static_cast<curl::Code>(
                    curl_easy_perform(handle)));
}

struct easy::Handle::Private
{
    Private() : handle(curl_easy_init(), [](CURL* ptr) { curl_easy_cleanup(ptr); })
    {
    }

    std::shared_ptr<CURL> handle;

    shared::Handle shared;

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
    set_option(Option::http_auth, CURLAUTH_ANY);
    set_option(Option::sharing, d->shared.native());
    set_option(Option::no_signal, easy::enable);
}

void easy::Handle::reset()
{
    d.reset();
}

easy::Handle::Timings easy::Handle::timings()
{
    easy::Handle::Timings result;
    double value;

    get_option(::curl::Info::namelookup_time, &value);
    result.name_look_up = easy::Handle::Timings::Seconds{value};
    get_option(::curl::Info::connect_time, &value);
    result.connect = easy::Handle::Timings::Seconds{value};
    get_option(::curl::Info::appconnect_time, &value);
    result.app_connect = easy::Handle::Timings::Seconds{value};
    get_option(::curl::Info::pretransfer_time, &value);
    result.pre_transfer = easy::Handle::Timings::Seconds{value};
    get_option(::curl::Info::starttransfer_time, &value);
    result.start_transfer = easy::Handle::Timings::Seconds{value};
    get_option(::curl::Info::total_time, &value);
    result.total = easy::Handle::Timings::Seconds{value};

    return result;
}

easy::Handle& easy::Handle::url(const char* url)
{
    set_option(Option::url, url);
    return *this;
}

easy::Handle& easy::Handle::user_agent(const char* user_agent)
{
    set_option(Option::user_agent, user_agent);
    return *this;
}

easy::Handle& easy::Handle::http_credentials(const std::string& username, const std::string& pwd)
{
    set_option(Option::username, username.c_str());
    set_option(Option::password, pwd.c_str());
    return *this;
}

easy::Handle& easy::Handle::on_finished(const easy::Handle::OnFinished& on_finished)
{
    d->on_finished_cb = on_finished;
    return *this;
}

easy::Handle& easy::Handle::on_progress(const easy::Handle::OnProgress& on_progress)
{
    set_option(Option::no_progress, curl::easy::disable);
    set_option(Option::progress_function, Handle::progress_cb);
    set_option(Option::progress_data, d.get());

    d->on_progress = on_progress;

    return *this;
}

easy::Handle& easy::Handle::on_read_data(const easy::Handle::OnReadData& on_read_data, std::size_t size)
{
    set_option(Option::read_function, Handle::read_data_cb);
    set_option(Option::read_data, d.get());
    set_option(Option::in_file_size, size);

    d->on_read_data_cb = on_read_data;

    return *this;
}

easy::Handle& easy::Handle::on_write_data(const easy::Handle::OnWriteData& on_new_data)
{
    set_option(Option::write_function, Handle::write_data_cb);
    set_option(Option::write_data, d.get());

    d->on_write_data_cb = on_new_data;

    return *this;
}

easy::Handle& easy::Handle::on_write_header(const easy::Handle::OnWriteHeader& on_new_header)
{
    set_option(Option::header_function, Handle::write_header_cb);
    set_option(Option::header_data, d.get());

    d->on_write_header_cb = on_new_header;

    return *this;
}

easy::Handle& easy::Handle::method(core::net::http::Method method)
{
    switch(method)
    {
    case core::net::http::Method::get:
        set_option(Option::http_get, enable);
        break;
    case core::net::http::Method::head:
        set_option(Option::http_get, disable);
        set_option(Option::http_put, disable);
        set_option(Option::http_post, disable);
        break;
    case core::net::http::Method::post:
        set_option(Option::http_post, enable);
        break;
    case core::net::http::Method::put:
        set_option(Option::http_put, enable);
        break;
    default: throw core::net::http::Client::Errors::HttpMethodNotSupported{method, CORE_FROM_HERE()};
    }

    return *this;
}

easy::Handle& easy::Handle::post_data(const std::string& data, const std::string&)
{
    long content_length = data.size();
    set_option(Option::post_field_size, content_length);
    set_option(Option::copy_postfields, data.c_str());

    return *this;
}

core::net::http::Status easy::Handle::status()
{
    long result;
    get_option(curl::Info::response_code, &result);
    return static_cast<core::net::http::Status>(result);
}

easy::native::Handle easy::Handle::native() const
{
    return d->handle.get();
}

void easy::Handle::perform()
{
    easy::native::perform(d->handle.get());
}

void easy::Handle::notify_finished(curl::Code code)
{
    if (d->on_finished_cb)
        d->on_finished_cb(code);
}
