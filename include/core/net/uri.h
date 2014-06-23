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
    typedef std::string Base;

    typedef std::vector<std::string> Endpoint;

    typedef std::vector<std::pair<std::string, std::string>> Parameters;

    /**
     * @brief The base is the first part of the URI, including the protocol
     *
     * e.g.
     * \code{.cpp}
     * "http://www.ubuntu.com"
     * \endcode
     */
    Base base;

    /**
     * @brief the endpoint components
     *
     * e.g.
     * \code{.cpp}
     * {"api", "v3", "search"}
     * \endcode
     */
    Endpoint endpoint;

    /**
     * @brief The CGI parameters as key value pairs
     *
     * e.g.
     * \code{.cpp}
     * {{"key1", "value1"}, {"key2", "value2"}}
     * \endcode
     */
    Parameters parameters;
};

/**
 * @brief Build a URI from its components
 *
 * e.g.
 * \code{.cpp}
 * std::string query = "banana";
 * core::net::make_uri("https://api.mydomain.com", {"api", "v3", "search"}, {{"query", query}})
 * \endcode
 *
 * When converted to a std::string with core::net::http::client::uri_to_string()
 * the endpoint and parameters will be URL-escaped.
 */
CORE_NET_DLL_PUBLIC
Uri make_uri (const Uri::Base& base, const Uri::Endpoint& endpoint = Uri::Endpoint(),
           const Uri::Parameters& parameters = Uri::Parameters());

}
}

#endif // CORE_NET_URI_H_
