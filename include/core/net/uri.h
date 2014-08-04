/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Pete Woods <pete.woods@canonical.com>
 */

#ifndef CORE_NET_URI_H_
#define CORE_NET_URI_H_

#include <string>
#include <vector>

#include <core/net/visibility.h>

namespace core
{
namespace net
{


/**
 * @brief The Uri class encapsulates the components of a URI
 */
struct Uri
{
    typedef std::string Host;

    typedef std::vector<std::string> Path;

    typedef std::vector<std::pair<std::string, std::string>> QueryParameters;

    /**
     * @brief The host is the first part of the URI, including the protocol
     *
     * e.g.
     * \code{.cpp}
     * "http://www.ubuntu.com"
     * \endcode
     */
    Host host;

    /**
     * @brief the path components
     *
     * e.g.
     * \code{.cpp}
     * {"api", "v3", "search"}
     * \endcode
     */
    Path path;

    /**
     * @brief The CGI query parameters as ordered key-value pairs
     *
     * e.g.
     * \code{.cpp}
     * {{"key1", "value1"}, {"key2", "value2"}}
     * \endcode
     */
    QueryParameters query_parameters;
};

/**
 * @brief Build a URI from its components
 *
 * e.g.
 * \code{.cpp}
 * std::string query = "banana";
 * auto uri = core::net::make_uri("https://api.mydomain.com", {"api", "v3", "search"}, {{"query", query}});
 * \endcode
 *
 * When converted to a std::string with core::net::http::client::uri_to_string()
 * the endpoint and parameters will be URL-escaped.
 */
CORE_NET_DLL_PUBLIC
Uri make_uri (const Uri::Host& host, const Uri::Path& path = Uri::Path(),
           const Uri::QueryParameters& query_parameters = Uri::QueryParameters());

}
}

#endif // CORE_NET_URI_H_
