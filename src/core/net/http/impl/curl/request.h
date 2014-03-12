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
#ifndef CORE_NET_HTTP_IMPL_CURL_REQUEST_H_
#define CORE_NET_HTTP_IMPL_CURL_REQUEST_H_

#include <core/net/http/request.h>

#include <core/net/http/error.h>
#include <core/net/http/response.h>

#include "curl.h"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <sstream>

namespace core
{
namespace net
{
namespace http
{
namespace impl
{
namespace curl
{
// Make sure that we switch the state back to idle whenever an instance
// of StateGuard goes out of scope.
struct StateGuard
{
    StateGuard(std::atomic<core::net::http::Request::State>& state) : state(state)
    {
        state.store(core::net::http::Request::State::active);
    }

    ~StateGuard()
    {
        state.store(core::net::http::Request::State::idle);
    }

    std::atomic<core::net::http::Request::State>& state;
};

class Request : public core::net::http::Request
{
public:
    Request(::curl::multi::Handle& multi,
            const ::curl::easy::Handle& easy)
        : atomic_state(core::net::http::Request::State::idle),
          multi(multi),
          easy(easy)
    {
    }

    State state()
    {
        return atomic_state.load();
    }

    Response execute(const Request::ProgressHandler& ph)
    {
        if (atomic_state.load() == core::net::http::Request::State::active)
            throw core::net::http::Request::Errors::AlreadyActive{CORE_FROM_HERE()};

        StateGuard sg{atomic_state};
        Context context;

        if (ph)
        {
            easy.on_progress([&](void*, double dltotal, double dlnow, double ultotal, double ulnow)
            {
                Request::Progress progress;
                progress.download.total = dltotal;
                progress.download.current = dlnow;
                progress.upload.total = ultotal;
                progress.upload.current = ulnow;

                int result{-1};

                switch(ph(progress))
                {
                case Request::Progress::Next::abort_operation: result = 1; break;
                case Request::Progress::Next::continue_operation: result = 0; break;
                }

                return result;
            });
        }

        easy.on_write_data(
                    [&](char* data, std::size_t size, std::size_t nmemb)
                    {
                        context.body.write(data, size * nmemb);
                        return size * nmemb;
                    });
        easy.on_write_header(
                    [&](void* data, std::size_t size, std::size_t nmemb)
                    {
                        const char* begin = static_cast<const char*>(data);
                        const char* end = begin + size*nmemb;
                        auto position = std::find(begin, end, ':');

                        if (position != begin && position < end)
                        {
                            context.result.header.add(
                                        std::string{begin, position},
                                        std::string{position+1, end});
                        }

                        return size * nmemb;
                    });

        try
        {
            easy.perform();
        } catch(const std::system_error& se)
        {
            throw core::net::http::Error(se.what(), CORE_FROM_HERE());
        }

        context.result.status = easy.status();
        context.result.body = context.body.str();

        return context.result;
    }

    void async_execute(
            const Request::ProgressHandler& ph,
            const Request::ResponseHandler& rh,
            const Request::ErrorHandler& eh)
    {
        if (atomic_state.load() == core::net::http::Request::State::active)
            throw core::net::http::Request::Errors::AlreadyActive{CORE_FROM_HERE()};

        auto sg = std::make_shared<StateGuard>(atomic_state);
        auto context = std::make_shared<Context>();

        easy.on_finished([=](::curl::Code code)
        {
            if (code == ::curl::Code::ok)
            {
                context->result.status = easy.status();
                context->result.body = context->body.str();

                rh(context->result);
            } else
            {
                std::stringstream ss; ss << code;
                eh(core::net::http::Error(ss.str(), CORE_FROM_HERE()));
            }
        });

        if (ph)
        {
            easy.on_progress([=](void*, double dltotal, double dlnow, double ultotal, double ulnow)
            {
                Request::Progress progress;
                progress.download.total = dltotal;
                progress.download.current = dlnow;
                progress.upload.total = ultotal;
                progress.upload.current = ulnow;

                int result{-1};

                switch(ph(progress))
                {
                case Request::Progress::Next::abort_operation: result = 1; break;
                case Request::Progress::Next::continue_operation: result = 0; break;
                }

                return result;
            });
        }

        easy.on_write_data(
                    [=](char* data, std::size_t size, std::size_t nmemb)
                    {
                        context->body.write(data, size * nmemb);
                        return size * nmemb;
                    });

        easy.on_write_header(
                    [=](void* data, std::size_t size, std::size_t nmemb)
                    {
                        const char* begin = static_cast<const char*>(data);
                        const char* end = begin + size*nmemb;
                        auto position = std::find(begin, end, ':');

                        if (position != begin && position < end)
                        {
                            context->result.header.add(
                                        std::string{begin, position},
                                        std::string{position+1, end});
                        }

                        return size * nmemb;
                    });

        multi.add(easy);
    }

private:
    std::atomic<core::net::http::Request::State> atomic_state;
    ::curl::multi::Handle& multi;
    ::curl::easy::Handle easy;

    struct Context
    {
        Response result;
        std::stringstream body;
    };
};
}
}
}
}
}

#endif // CORE_NET_HTTP_IMPL_CURL_REQUEST_H_
