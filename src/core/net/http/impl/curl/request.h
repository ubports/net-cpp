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

#include "client.h"
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
// See http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
// We expect the line to exclude \r\n.
std::tuple<std::string, std::string> parse_header_line(const char* line, std::size_t size)
{
    static constexpr const char key_value_delimiter{':'};

    const char* begin = line;
    const char* end = begin + size;
    const char* position = std::find(begin, end, key_value_delimiter);

    if (position != begin && position < end)
    {
        // Advance until we find
        const char* trimmed = position+1;
        while (trimmed != end && std::isspace(*trimmed))
            trimmed++;

        return std::make_tuple(
                    (position != begin ? std::string{begin, position} : std::string{}),
                    (trimmed != end ? std::string{trimmed, end} : std::string{}));
    }

    return std::make_tuple(std::string{}, std::string{});
}

std::tuple<std::string, std::string, std::size_t> handle_header_line(void* data, std::size_t size, std::size_t nmemb)
{
    std::size_t length = size * nmemb;

    // Either an empty line only containing \r\n or
    // an invalid header line. We bail out in both cases.
    if (length <= 2)
        return std::tuple_cat(std::make_tuple(std::string{}, std::string{}), std::make_tuple(length));

    return std::tuple_cat(parse_header_line(static_cast<const char*>(data), length-2), std::make_tuple(length));
}



// Make sure that we switch the state back to idle whenever an instance
// of StateGuard goes out of scope.
struct StateGuard
{
    StateGuard(std::atomic<core::net::http::Request::State>& state)
          : state(state)
    {
        state.store(core::net::http::Request::State::active);
    }

    ~StateGuard()
    {
        state.store(core::net::http::Request::State::done);
    }

    std::atomic<core::net::http::Request::State>& state;
};

class Request : public core::net::http::Request,
                public std::enable_shared_from_this<Request>
{
public:

    static std::shared_ptr<Request> create(::curl::multi::Handle multi,
                                           ::curl::easy::Handle easy)
    {
        return std::make_shared<Request>(multi, easy);
    }

    Request(::curl::multi::Handle multi,
            ::curl::easy::Handle easy)
        : atomic_state(core::net::http::Request::State::ready),
          multi(multi),
          easy(easy)
    {
    }

    State state()
    {
        return atomic_state.load();
    }

    void set_timeout(const std::chrono::milliseconds& timeout)
    {
        if (atomic_state.load() != core::net::http::Request::State::ready)
            throw core::net::http::Request::Errors::AlreadyActive{CORE_FROM_HERE()};

        easy.set_option(::curl::Option::timeout_ms, timeout.count());
    }

    Response execute(const Request::ProgressHandler& ph)
    {
        if (atomic_state.load() != core::net::http::Request::State::ready)
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
                        static constexpr const std::size_t key_index{0};
                        static constexpr const std::size_t value_index{1};
                        static constexpr const std::size_t size_index{2};

                        auto kvs = handle_header_line(data, size, nmemb);

                        if (not std::get<key_index>(kvs).empty())
                            context.result.header.add(std::get<key_index>(kvs), std::get<value_index>(kvs));

                        return std::get<size_index>(kvs);
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

    void async_execute(const Request::Handler& handler)
    {
        if (atomic_state.load() != core::net::http::Request::State::ready)
            throw core::net::http::Request::Errors::AlreadyActive{CORE_FROM_HERE()};

        auto sg = std::make_shared<StateGuard>(atomic_state);
        auto context = std::make_shared<Context>();

        auto thiz = shared_from_this();

        easy.on_finished([thiz, handler, context](::curl::Code code)
        {
            if (code == ::curl::Code::ok)
            {
                context->result.status = thiz->easy.status();
                context->result.body = context->body.str();

                if (handler.on_response())
                    handler.on_response()(context->result);
            } else
            {
                if (handler.on_error())
                {
                    std::stringstream ss; ss << code;
                    handler.on_error()(core::net::http::Error(ss.str(), CORE_FROM_HERE()));
                }
            }

            thiz->easy.release();
        });

        if (handler.on_progress())
        {
            easy.on_progress([handler](void*, double dltotal, double dlnow, double ultotal, double ulnow)
            {
                Request::Progress progress;
                progress.download.total = dltotal;
                progress.download.current = dlnow;
                progress.upload.total = ultotal;
                progress.upload.current = ulnow;

                int result{-1};

                switch(handler.on_progress()(progress))
                {
                case Request::Progress::Next::abort_operation: result = 1; break;
                case Request::Progress::Next::continue_operation: result = 0; break;
                }

                return result;
            });
        }

        easy.on_write_data(
                    [context](char* data, std::size_t size, std::size_t nmemb)
                    {
                        context->body.write(data, size * nmemb);
                        return size * nmemb;
                    });

        easy.on_write_header(
                    [context](void* data, std::size_t size, std::size_t nmemb)
                    {
                        static constexpr const std::size_t key_index{0};
                        static constexpr const std::size_t value_index{1};
                        static constexpr const std::size_t size_index{2};

                        auto kvs = handle_header_line(data, size, nmemb);

                        if (not std::get<key_index>(kvs).empty())
                            context->result.header.add(std::get<key_index>(kvs), std::get<value_index>(kvs));

                        return std::get<size_index>(kvs);
                    });

        multi.add(easy);
    }

    std::string url_escape(const std::string& s)
    {
        return easy.escape(s);
    }

    std::string url_unescape(const std::string& s)
    {
        return easy.unescape(s);
    }
private:
    std::atomic<core::net::http::Request::State> atomic_state;
    ::curl::multi::Handle multi;
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
