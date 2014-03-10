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

void http::impl::curl::Client::run()
{
    multi.run();
}

void http::impl::curl::Client::stop()
{
    multi.stop();
}

std::shared_ptr<http::Request> http::impl::curl::Client::head(const http::Request::Configuration& configuration)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::head)
            .url(configuration.uri.c_str());

    if (configuration.authentication_handler.for_http)
    {
        auto credentials = configuration.authentication_handler.for_http(configuration.uri);
        handle.http_credentials(credentials.username, credentials.password);
    }

    return std::shared_ptr<http::Request>{new http::impl::curl::Request{multi, handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::get(const http::Request::Configuration& configuration)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::get)
            .url(configuration.uri.c_str());

    if (configuration.authentication_handler.for_http)
    {
        auto credentials = configuration.authentication_handler.for_http(configuration.uri);
        handle.http_credentials(credentials.username, credentials.password);
    }

    return std::shared_ptr<http::Request>{new http::impl::curl::Request{multi, handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::post(
        const Request::Configuration& configuration,
        const std::string& payload,
        const http::ContentType& ct)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::post)
            .url(configuration.uri.c_str())
            .post_data(payload.c_str(), ct);

    if (configuration.authentication_handler.for_http)
    {
        auto credentials = configuration.authentication_handler.for_http(configuration.uri);
        handle.http_credentials(credentials.username, credentials.password);
    }

    return std::shared_ptr<http::Request>{new http::impl::curl::Request{multi, handle}};
}

std::shared_ptr<http::Request> http::impl::curl::Client::put(
        const Request::Configuration& configuration,
        std::istream& payload,
        std::size_t size)
{
    ::curl::easy::Handle handle;
    handle.method(http::Method::put)
            .url(configuration.uri.c_str())
            .on_read_data([&payload, size](void* dest, std::size_t /*in_size*/, std::size_t /*nmemb*/)
            {
                auto result = payload.readsome(static_cast<char*>(dest), size);
                return result;
            }, size);

    if (configuration.authentication_handler.for_http)
    {
        auto credentials = configuration.authentication_handler.for_http(configuration.uri);
        handle.http_credentials(credentials.username, credentials.password);
    }

    return std::shared_ptr<http::Request>{new http::impl::curl::Request{multi, handle}};
}

std::shared_ptr<http::Client> http::make_client()
{
    return std::make_shared<http::impl::curl::Client>();
}
