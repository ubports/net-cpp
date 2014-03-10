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
#include <core/net/http/request.h>

#include <iosfwd>
#include <memory>

namespace core
{
namespace net
{
namespace http
{
class ContentType;
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

    /** @brief Execute the client and any impl-specific thread-pool or runtime. */
    virtual void run() = 0;

    /** @brief Stop the client and any impl-specific thread-pool or runtime. */
    virtual void stop() = 0;

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
     * @param configuration The configuration to issue a get request for.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> get(const Request::Configuration& configuration) = 0;

    /**
     * @brief head is a convenience method for issueing a HEAD request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> head(const Request::Configuration& configuration) = 0;

    /**
     * @brief put is a convenience method for issuing a PUT request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param payload The data to be transmitted as part of the PUT request.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> put(const Request::Configuration& configuration, std::istream& payload, std::size_t size) = 0;

    /**
     * @brief post is a convenience method for issuing a POST request for the given URI.
     * @throw Errors::HttpMethodNotSupported if the underlying implementation does not support the provided HTTP method.
     * @param configuration The configuration to issue a get request for.
     * @param payload The data to be transmitted as part of the PUT request.
     * @return An executable instance of class Request.
     */
    virtual std::shared_ptr<Request> post(const Request::Configuration& configuration, const std::string& payload, const ContentType& type) = 0;

protected:
    Client() = default;
};

/** @brief Dispatches to the default implementation and returns a client instance. */
CORE_NET_DLL_PUBLIC std::shared_ptr<Client> make_client();
}
}
}

#endif // CORE_NET_HTTP_CLIENT_H_
