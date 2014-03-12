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
#ifndef CORE_NET_HTTP_RESPONSE_H_
#define CORE_NET_HTTP_RESPONSE_H_

#include <core/net/visibility.h>

#include <core/net/http/header.h>
#include <core/net/http/status.h>

#include <map>
#include <sstream>
#include <string>

namespace core
{
namespace net
{
namespace http
{
/**
 * @brief The Response struct models a response to a core::net::http::Request.
 */
struct CORE_NET_DLL_PUBLIC Response
{
    // TODO(tvoss): This really should be a stringstream, but libstdc++ is broken and
    // does not define move operators correctly.
    /** @brief The body of the response is a string. */
    typedef std::string Body;

    /** @brief The HTTP status as sent by the server. */
    Status status{Status::bad_request};
    /** @brief The header fields of the response. */
    Header header{};
    /** @brief The body of the response. */
    Body body{};
};
}
}
}

#endif // CORE_NET_HTTP_RESPONSE_H_
