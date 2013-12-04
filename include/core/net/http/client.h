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
#ifndef CORE_NET_HTTP_CLIENT_H_
#define CORE_NET_HTTP_CLIENT_H_

#include <core/net/visibility.h>

#include <core/net/http/method.h>

#include <memory>

namespace core
{
namespace net
{
namespace http
{
class Request;
class CORE_NET_DLL_PUBLIC Client
{
public:
    /** @brief Summarizes error conditions. */
    struct Errors
    {
        Errors() = delete;

        /** @brief HttpMethodNotSupported is thrown if the underlying impl.
         * does not support the requested HTTP method.
         */
        struct HttpMethodNotSupported
        {
            Method method;
        };
    };

    Client(const Client&) = delete;
    virtual ~Client() = default;

    Client& operator=(const Client&) = delete;
    bool operator==(const Client&) const = delete;

    /**
     * @brief request creates a Request for the provided URI and the given HTTP method.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param method The HTTP method to use when requesting the resource.
     * @param uri The uri describing the resource to be requested.
     * @return An instance of a Request ready to be executed.
     */
    // virtual std::shared_ptr<Request> request(Method method, const std::string& uri) = 0;

    /**
     * @brief get is a convenience method for issueing a GET request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param uri The URI to issue a GET request for.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> get(const std::string& uri) = 0;

    /**
     * @brief head is a convenience method for issueing a HEAD request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param uri The URI to issue a HEAD request for.
     * @return An executable instance of class Request.
     */
    // virtual std::shared_ptr<Request> head(const std::string& uri) = 0;

    /**
     * @brief put is a convenience method for issuing a PUT request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param uri The URI to issue a PUT request for.
     * @param payload The data to be transmitted as part of the PUT request.
     * @return An executable instance of class Request.
     */
    // virtual std::shared_ptr<Request> put(const std::string& uri, const std::string& payload) = 0;

    /**
     * @brief post is a convenience method for issuing a POST request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param uri The URI to issue a POST request for.
     * @param payload The data to be transmitted as part of the PUT request.
     * @return An executable instance of class Request.
     */
    // virtual std::shared_ptr<Request> post(const std::string& uri, const std::string& payload) = 0;

protected:
    Client() = default;
};

/** @brief Dispatches to the default implementation and returns a client instance. */
CORE_NET_DLL_PUBLIC std::shared_ptr<Client> make_client();
}
}
}

#endif // CORE_NET_HTTP_CLIENT_H_
