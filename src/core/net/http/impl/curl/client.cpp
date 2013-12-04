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

std::shared_ptr<http::Request> http::impl::curl::Client::get(const std::string& uri)
{
    ::curl::easy::Handle handle = ::curl::easy::Handle()
            .method(http::Method::get)
            .url(uri.c_str());
    handle.user_agent("lalelu");
    return std::shared_ptr<http::Request>{new http::impl::curl::Request{handle}};
}

std::shared_ptr<http::Client> http::make_client()
{
    return std::make_shared<http::impl::curl::Client>();
}
