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

#include <memory>

namespace core
{
namespace net
{
namespace http
{
class Response;
class CORE_NET_DLL_PUBLIC Request
{
public:
    enum class State
    {
        idle,
        active
    };

    struct Errors
    {
        Errors() = delete;

        struct DidNotReceiveResponse : public std::runtime_error
        {
            inline DidNotReceiveResponse(const std::string& what) : std::runtime_error(what.c_str())
            {
            }
        };

        struct AlreadyActive : public std::runtime_error
        {
            inline AlreadyActive(const std::string& location) : std::runtime_error(location.c_str())
            {
            }
        };
    };

    struct Credentials
    {
        std::string username;
        std::string password;
    };

    typedef std::function<Credentials(const std::string&)> AuthenicationHandler;

    struct Configuration
    {
        inline static Configuration from_uri_as_string(const std::string& uri)
        {
            Configuration result;
            result.uri = uri;

            return result;
        }

        std::string uri;
        struct
        {
            AuthenicationHandler for_http;
            AuthenicationHandler for_proxy;
        } authentication_handler;
    };

    struct Progress
    {
        enum class Next
        {
            continue_operation,
            abort_operation
        };

        struct
        {
            double total{-1.};
            double current{-1.};
        } download{};

        struct
        {
            double total{-1.};
            double current{-1.};
        } upload{};
    };

    typedef std::function<void()> ErrorHandler;
    typedef std::function<Progress::Next(const Progress&)> ProgressHandler;
    typedef std::function<void(const Response&)> ResponseHandler;

    Request(const Request&) = delete;
    virtual ~Request() = default;

    Request& operator=(const Request&) = delete;
    bool operator==(const Request&) const = delete;

    virtual State state() = 0;

    virtual Response execute(const ProgressHandler&) = 0;
    virtual void async_execute(const ProgressHandler&, const ResponseHandler&, const ErrorHandler&) = 0;

protected:
    Request() = default;
};
}
}
}

#endif // CORE_NET_HTTP_REQUEST_H_
