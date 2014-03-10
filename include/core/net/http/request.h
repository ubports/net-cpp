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
#ifndef CORE_NET_HTTP_REQUEST_H_
#define CORE_NET_HTTP_REQUEST_H_

#include <core/net/visibility.h>

#include <core/net/uri.h>

#include <core/net/http/error.h>

#include <memory>

namespace core
{
namespace net
{
namespace http
{
class Response;

/**
 * @brief The Request class encapsulates a request for a web resource.
 */
class CORE_NET_DLL_PUBLIC Request
{
public:
    /**
     * @brief The State enum describes the different states a request can be in.
     */
    enum class State
    {
        idle, ///< The request is idle and needs execution.
        active ///< The request is active and is actively being executed.
    };

    /**
     * @brief The Errors struct collects the Request-specific exceptions and error modes.
     */
    struct Errors
    {
        Errors() = delete;

        /**
         * @brief AlreadyActive is thrown when *execute is called on an active request.
         */
        struct AlreadyActive : public core::net::http::Error
        {
            /**
             * @brief AlreadyActive creates a new instance with a location hint.
             * @param loc The location that the call originates from.
             */
            inline AlreadyActive(const core::Location& loc)
                : core::net::http::Error("Request is already active.", loc)
            {
            }
        };
    };

    /**
     * @brief The Credentials struct encapsulates username and password for basic & digest authentication.
     */
    struct Credentials
    {
        std::string username;
        std::string password;
    };

    /** Function signature for querying credentials for a given URL. */
    typedef std::function<Credentials(const std::string&)> AuthenicationHandler;

    /**
     * @brief The Configuration struct encapsulates all options for creating requests.
     */
    struct Configuration
    {
        /**
         * @brief from_uri_as_string creates a new instance of Configuration for a url.
         * @param uri The url of the web resource to issue a request for.
         * @return A new Configuration instance.
         */
        inline static Configuration from_uri_as_string(const std::string& uri)
        {
            Configuration result;
            result.uri = uri;

            return result;
        }

        /** Uri of the web resource to issue a request for. */
        std::string uri;

        /** Encapsulates proxy and http authentication handlers. */
        struct
        {
            /** Invoked for querying user credentials to do basic/digest auth. */
            AuthenicationHandler for_http;
            /** Invoked for querying user credentials to authenticate proxy accesses. */
            AuthenicationHandler for_proxy;
        } authentication_handler;
    };

    /**
     * @brief The Progress struct encapsulates progress information for web-resource requests.
     */
    struct Progress
    {
        /**
         * @brief The Next enum summarizes the available return-types for the progress callback.
         */
        enum class Next
        {
            continue_operation, ///< Continue the request.
            abort_operation ///< Abort the request.
        };

        struct
        {
            double total{-1.}; ///< Total number of bytes to be downloaded.
            double current{-1.}; ///< Current number of bytes already downloaded.
        } download{};

        struct
        {
            double total{-1.}; ///< Total number of bytes to be uploaded.
            double current{-1.}; ///< Current number of bytes already uploaded.
        } upload{};
    };

    /**
     * @brief ErrorHandler is invoked in case of errors arising while executing the request.
     */
    typedef std::function<void(const core::net::Error&)> ErrorHandler;

    /**
     * @brief ProgressHandler is invoked for progress updates while executing the request.
     */
    typedef std::function<Progress::Next(const Progress&)> ProgressHandler;

    /**
     * @brief ResponseHandler is invoked when a request completes.
     */
    typedef std::function<void(const Response&)> ResponseHandler;

    Request(const Request&) = delete;
    virtual ~Request() = default;

    Request& operator=(const Request&) = delete;
    bool operator==(const Request&) const = delete;

    /**
     * @brief state queries the current state of the operation.
     * @return A value from the State enumeration.
     */
    virtual State state() = 0;

    /**
     * @brief Synchronously executes the request.
     * @throw core::net::http::Error in case of http-related errors.
     * @throw core::net::Error in case of network-related errors.
     * @return The response to the request.
     */
    virtual Response execute(const ProgressHandler&) = 0;

    /**
     * @brief Asynchronously executes the request, reporting errors, progress and completion to the given handlers.
     * @param ph Function to call for reporting the progress of the operation.
     * @param rh Function to call for reporting completion of the operation.
     * @param eh Function to call for reporting errors during the operation.
     * @return The response to the request.
     */
    virtual void async_execute(const ProgressHandler& ph, const ResponseHandler& rh, const ErrorHandler&) = 0;

protected:
    Request() = default;
};
}
}
}

#endif // CORE_NET_HTTP_REQUEST_H_
