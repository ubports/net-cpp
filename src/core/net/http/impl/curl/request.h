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
#ifndef CORE_NET_HTTP_IMPL_CURL_REQUEST_H_
#define CORE_NET_HTTP_IMPL_CURL_REQUEST_H_

#include <core/net/http/request.h>
#include <core/net/http/response.h>

#include "curl.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace core
{
namespace net
{
namespace http
{
namespace impl
{
namespace curl
{
class Request : public core::net::http::Request
{
public:
    Request(const ::curl::easy::Handle& handle) : handle(handle)
    {
    }

    Response execute()
    {
        Response result;
        std::stringstream body;

        handle.on_data_function(
                    [&body](char* data, std::size_t size, std::size_t nmemb)
                    {
                        body.write(data, size * nmemb);
                        return size * nmemb;
                    });

        handle.on_header_function(
                    [&result](void* data, std::size_t size, std::size_t nmemb)
                    {
                        const char* begin = static_cast<const char*>(data);
                        const char* end = begin + size*nmemb;
                        auto position = std::find(begin, end, ':');

                        if (position != begin && position < end)
                        {
                            result.headers.emplace(
                                        Header::Key{begin, position},
                                        Header::Value{position+1, end});
                        }

                        return size * nmemb;
                    });

        auto rc = handle.perform();

        if (rc != ::curl::Code::ok)
        {
            throw Errors::DidNotReceiveResponse{};
        }

        result.status = handle.status();
        result.body = body.str();
        return std::move(result);
    }

private:
    ::curl::easy::Handle handle;
};
}
}
}
}
}

#endif // CORE_NET_HTTP_IMPL_CURL_REQUEST_H_
