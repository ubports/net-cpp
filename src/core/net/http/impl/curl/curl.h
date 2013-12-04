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

namespace curl
{
typedef size_t (*OnNewDataFunction)(char* ptr, size_t size, size_t nmemb, void* userdata);
typedef size_t (*OnNewHeaderFunction)(void* ptr, size_t size, size_t nmemb, void* userdata);

struct Init
{
    Init()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
} init;

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
static const CURLoption url = CURLOPT_URL;
static const CURLoption user_agent = CURLOPT_USERAGENT;
static const CURLoption http_get = CURLOPT_HTTPGET;
}
namespace easy
{
static const long enable = 1;
struct Handle
{
    typedef std::function<std::size_t(char*, std::size_t, std::size_t)> OnNewData;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnNewHeader;

    static std::size_t on_new_data_function(char* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;

        auto thiz = static_cast<Handle*>(cookie);

        if (thiz && thiz->on_new_data)
        {
            return thiz->on_new_data(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    static std::size_t on_new_header_function(void* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;

        auto thiz = static_cast<Handle*>(cookie);

        if (thiz && thiz->on_new_header)
        {
            return thiz->on_new_header(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    Handle() : handle(curl_easy_init())
    {
    }

    Handle& url(const char* url)
    {
        curl_easy_setopt(
                    handle,
                    option::url,
                    url);

        return *this;
    }

    Handle& user_agent(const char* user_agent)
    {
        curl_easy_setopt(
                    handle,
                    option::user_agent,
                    user_agent);

        return *this;
    }

    Handle& on_data_function(const OnNewData& on_new_data)
    {
        curl_easy_setopt(
                    handle,
                    option::write_function,
                    Handle::on_new_data_function);

        curl_easy_setopt(
                    handle,
                    option::write_data,
                    this);

        this->on_new_data = on_new_data;

        return *this;
    }

    Handle& on_header_function(const OnNewHeader& on_new_header)
    {
        curl_easy_setopt(
                    handle,
                    option::header_function,
                    Handle::on_new_header_function
                    );

        curl_easy_setopt(
                    handle,
                    option::header_data,
                    this);

        this->on_new_header = on_new_header;

        return *this;
    }

    Handle& method(core::net::http::Method method)
    {
        switch(method)
        {
        case core::net::http::Method::get: curl_easy_setopt(handle, option::http_get, enable); break;
        default: throw core::net::http::Client::Errors::HttpMethodNotSupported{method};
        }

        return *this;
    }

    core::net::http::Status status()
    {
        long result;
        curl_easy_getinfo(
                    handle,
                    info::response_code,
                    &result);
        return static_cast<core::net::http::Status>(result);
    }

    operator CURL*() const
    {
        return handle;
    }

    Code perform()
    {
        return static_cast<Code>(curl_easy_perform(handle));
    }

    CURL* handle;
    OnNewData on_new_data;
    OnNewHeader on_new_header;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_CURL_H_
