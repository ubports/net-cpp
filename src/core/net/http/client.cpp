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

#include <core/net/http/client.h>

#include <core/net/http/content_type.h>

namespace net = core::net;
namespace http = net::http;

http::Client::Errors::HttpMethodNotSupported::HttpMethodNotSupported(
        http::Method method,
        const core::Location& loc)
    : http::Error("Http method not suppored", loc),
      method(method)
{

}

std::shared_ptr<http::Request> http::Client::post_form(
        const http::Request::Configuration& configuration,
        const net::Uri::Values& values)
{
    std::stringstream ss;
    bool first{true};

    for (const auto& pair : values)
    {
        ss << (first ? "" : "&") << url_escape(pair.first) << "=" << url_escape(pair.second);
        first = false;
    }

    return post(configuration, ss.str(), http::ContentType::x_www_form_urlencoded);
}
