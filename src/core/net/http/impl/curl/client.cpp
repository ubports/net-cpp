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

#include "client.h"
#include "curl.h"
#include "request.h"

#include <core/net/http/method.h>

namespace net = core::net;
namespace http = core::net::http;



http::impl::curl::Client::Client()
{
}

std::shared_ptr<http::Request> http::impl::curl::Client::head(const std::string& uri)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::head)
            .url(uri.c_str());
    return std::shared_ptr<http::Request>{new http::impl::curl::Request{handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::get(const std::string& uri)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::get)
            .url(uri.c_str());
    return std::shared_ptr<http::Request>{new http::impl::curl::Request{handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::post(const std::string& uri,
                              const std::string& payload,
                              const http::ContentType& ct)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::post)
            .url(uri.c_str())
            .post_data(payload.c_str(), ct);
    return std::shared_ptr<http::Request>{new http::impl::curl::Request{handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::put(
        const std::string& uri,
        std::istream& payload,
        std::size_t size)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::put)
            .url(uri.c_str())
            .on_read_data([&payload, size](void* dest, std::size_t /*in_size*/, std::size_t /*nmemb*/)
            {
                auto result = payload.readsome(static_cast<char*>(dest), size);
                return result;
            }, size);
    return std::shared_ptr<http::Request>{new http::impl::curl::Request{handle}};
}

std::shared_ptr<http::Client> http::make_client()
{
    return std::make_shared<http::impl::curl::Client>();
}
