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
#ifndef CORE_NET_HTTP_IMPL_CURL_CURL_H_
#define CORE_NET_HTTP_IMPL_CURL_CURL_H_

#include <core/net/http/client.h>
#include <core/net/http/method.h>
#include <core/net/http/status.h>

#include <curl/curl.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stack>

namespace curl
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

struct Shared
{
    Shared() : d(new Private())
    {
    }

    CURLSH* handle() const
    {
        return d->handle;
    }

    struct Private
    {
        Private() : handle(curl_share_init())
        {
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
        }

        ~Private()
        {
            curl_share_cleanup(handle);
        }

        CURLSH* handle;
    };
    std::shared_ptr<Private> d;
};

enum class Code
{
    ok = CURLE_OK
};

namespace info
{
static const CURLINFO response_code = CURLINFO_RESPONSE_CODE;
}

namespace option
{
static const CURLoption header_function = CURLOPT_HEADERFUNCTION;
static const CURLoption header_data = CURLOPT_HEADERDATA;
static const CURLoption write_function = CURLOPT_WRITEFUNCTION;
static const CURLoption write_data = CURLOPT_WRITEDATA;
static const CURLoption read_function = CURLOPT_READFUNCTION;
static const CURLoption read_data = CURLOPT_READDATA;
static const CURLoption url = CURLOPT_URL;
static const CURLoption user_agent = CURLOPT_USERAGENT;
static const CURLoption http_get = CURLOPT_HTTPGET;
static const CURLoption http_post = CURLOPT_POST;
static const CURLoption http_put = CURLOPT_PUT;
static const CURLoption copy_postfields = CURLOPT_COPYPOSTFIELDS;
static const CURLoption post_field_size = CURLOPT_POSTFIELDSIZE;
static const CURLoption upload = CURLOPT_UPLOAD;
static const CURLoption in_file_size = CURLOPT_INFILESIZE;
static const CURLoption sharing = CURLOPT_SHARE;
}
namespace easy
{
static const long disable = 0;
static const long enable = 1;
class Handle
{
private:
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
                    curl_easy_setopt(handle, option::sharing, shared.handle());
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
        ::curl::Shared shared;
    };

public:
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnReadData;
    typedef std::function<std::size_t(char*, std::size_t, std::size_t)> OnWriteData;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnWriteHeader;

    static std::size_t write_data_cb(char* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz && thiz->on_write_data_cb)
        {
            return thiz->on_write_data_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    static std::size_t write_header_cb(void* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz && thiz->on_write_header_cb)
        {
            return thiz->on_write_header_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    static std::size_t read_data_cb(void* data, std::size_t size, std::size_t nmemb, void *cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz)
        {
            return thiz->on_read_data_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    Handle() : d(new Private())
    {
    }

    Handle& url(const char* url)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::url,
                    url);

        return *this;
    }

    Handle& user_agent(const char* user_agent)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::user_agent,
                    user_agent);

        return *this;
    }

    Handle& on_write_data(const OnWriteData& on_new_data)
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

    Handle& on_write_header(const OnWriteHeader& on_new_header)
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

    Handle& on_read_data(const OnReadData& on_read_data, std::size_t size)
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

    Handle& method(core::net::http::Method method)
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

    Handle& post_data(const std::string& data, const core::net::http::ContentType&)
    {
        long content_length = data.size();
        curl_easy_setopt(d->handle.get(), option::post_field_size, content_length);
        curl_easy_setopt(d->handle.get(), option::copy_postfields, data.c_str());

        return *this;
    }

    core::net::http::Status status()
    {
        long result;
        curl_easy_getinfo(
                    d->handle.get(),
                    info::response_code,
                    &result);
        return static_cast<core::net::http::Status>(result);
    }

    operator CURL*() const
    {
        return d->handle.get();
    }

    Code perform()
    {
        return static_cast<Code>(curl_easy_perform(d->handle.get()));
    }

private:
    struct Private
    {
        Private() : handle(Pool<100>::instance().acquire_or_wait_for(std::chrono::microseconds{1000}))
        {
        }

        ~Private()
        {
            Pool<100>::instance().release(handle);
        }

        std::shared_ptr<CURL> handle;

        OnWriteData on_write_data_cb;
        OnWriteHeader on_write_header_cb;
        OnReadData on_read_data_cb;
    };
    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_CURL_H_
