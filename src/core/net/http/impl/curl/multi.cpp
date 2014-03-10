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

#include "multi.h"

#include "easy.h"

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>

namespace easy = curl::easy;
namespace multi = curl::multi;

namespace
{
struct SynchronizedHandleStore
{
    std::mutex guard;
    std::map<CURL*, easy::Handle> handles;

    void add(easy::Handle easy)
    {
        std::lock_guard<std::mutex> lg(guard);
        handles.insert(std::make_pair(easy.native(), easy));
    }

    void remove(easy::Handle easy)
    {
        std::lock_guard<std::mutex> lg(guard);
        handles.erase(easy.native());
    }

    easy::Handle lookup_native(CURL* native)
    {
        std::lock_guard<std::mutex> lg(guard);
        auto it = handles.find(native);

        if (it == handles.end())
            throw std::runtime_error("SynchronizedHandleStore::lookup_native: No such handle.");

        return it->second;
    }
};

template<typename T>
struct Holder
{
    std::shared_ptr<T> value;
};
}

namespace curl
{
namespace multi
{
enum class Code
{
    call_multi_perform = CURLM_CALL_MULTI_PERFORM,
    ok = CURLM_OK,
    bad_handle = CURLM_BAD_HANDLE,
    easy_handle = CURLM_BAD_EASY_HANDLE,
    out_of_memory = CURLM_OUT_OF_MEMORY,
    internal_error = CURLM_INTERNAL_ERROR,
    bad_socket = CURLM_BAD_SOCKET,
    unknown_option = CURLM_UNKNOWN_OPTION,
    added_already = CURLM_ADDED_ALREADY
};

inline std::ostream& operator<<(std::ostream& out, Code code)
{
    switch (code)
    {
    case Code::call_multi_perform: out << "curl::multi::Code::call_multi_perform"; break;
    case Code::ok: out << "curl::multi::Code::ok"; break;
    case Code::bad_handle: out << "curl::multi::Code::bad_handle"; break;
    case Code::easy_handle: out << "curl::multi::Code::easy_handle"; break;
    case Code::out_of_memory: out << "curl::multi::Code::out_of_memory"; break;
    case Code::internal_error: out << "curl::multi::Code::internal_error"; break;
    case Code::bad_socket: out << "curl::multi::Code::bad_socket"; break;
    case Code::unknown_option: out << "curl::multi::Code::unknown_option"; break;
    case Code::added_already: out << "curl::multi::Code::added_already"; break;
    }

    return out;
}

namespace option
{
    const CURLMoption socket_function = CURLMOPT_SOCKETFUNCTION;
    const CURLMoption socket_data = CURLMOPT_SOCKETDATA;
    const CURLMoption timer_function = CURLMOPT_TIMERFUNCTION;
    const CURLMoption timer_data = CURLMOPT_TIMERDATA;
}
}
}

struct multi::Handle::Private
{
    void process_multi_info();

    void remove(curl::easy::Handle easy);

    static int curl_timer_callback(
            CURLM*,
            long timeout_ms,
            void* cookie);

    struct Timeout
    {
        Timeout(boost::asio::io_service& dispatcher);

        void cancel();
        void async_wait_for(
                const std::shared_ptr<Handle::Private>& context,
                const std::chrono::milliseconds& ms);

        struct Private : public std::enable_shared_from_this<Private>
        {
            Private(boost::asio::io_service& dispatcher);

            void cancel();

            void async_wait_for(
                    const std::shared_ptr<multi::Handle::Private>& context,
                    const std::chrono::milliseconds& ms);

            void handle_timeout(const std::shared_ptr<multi::Handle::Private>& context);

            boost::asio::deadline_timer timer;
        };
        std::shared_ptr<Private> d;
    };

    static int curl_socket_callback(
            CURL*,
            curl_socket_t s,
            int action,
            void* cookie,
            void* socket_cookie);

    struct Socket
    {
        Socket(boost::asio::io_service& dispatcher,
               curl_socket_t native);
        ~Socket();

        void async_wait_for_readable(const std::shared_ptr<Handle::Private>& context);
        void async_wait_for_writeable(const std::shared_ptr<Handle::Private>& context);

        struct Private : public std::enable_shared_from_this<Private>
        {
            Private(boost::asio::io_service& dispatcher,
                    curl_socket_t native);
            ~Private();

            void cancel();
            void async_wait_for_readable(const std::shared_ptr<Handle::Private>& context);
            void async_wait_for_writeable(std::shared_ptr<Handle::Private> context);

            bool cancel_requested;
            boost::asio::posix::stream_descriptor sd;
        };
        std::shared_ptr<Private> d;
    };

    Private();
    ~Private();

    CURLM* handle;
    boost::asio::io_service dispatcher;
    boost::asio::io_service::work keep_alive;
    SynchronizedHandleStore handle_store;
    Timeout timeout;
};

multi::Handle::Handle() : d(new Private())
{
    auto holder = new Holder<Private>{d};
    curl_multi_setopt(d->handle, option::socket_function, Private::curl_socket_callback);
    curl_multi_setopt(d->handle, option::socket_data, holder);
    curl_multi_setopt(d->handle, option::timer_function, Private::curl_timer_callback);
    curl_multi_setopt(d->handle, option::timer_data, holder);
}

void multi::Handle::run()
{
    d->dispatcher.run();
}

void multi::Handle::stop()
{
    d->dispatcher.stop();
}

void multi::Handle::add(easy::Handle easy)
{
    d->handle_store.add(easy);
    curl_multi_add_handle(native(), easy.native());
}

void multi::Handle::remove(easy::Handle easy)
{
    d->handle_store.remove(easy);
    curl_multi_remove_handle(native(), easy.native());
}

curl::easy::Handle multi::Handle::easy_handle_from_native(CURL* native)
{
    return d->handle_store.lookup_native(native);
}

multi::Native multi::Handle::native() const
{
    return d->handle;
}

void multi::Handle::Private::process_multi_info()
{
    CURLMsg* msg{nullptr};

    int msg_count;
    while ((msg = curl_multi_info_read(handle, &msg_count)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            auto native_easy = msg->easy_handle;
            auto rc = static_cast<curl::Code>(msg->data.result);
            try
            {
                auto easy = handle_store.lookup_native(native_easy);
                easy.notify_finished(rc);
                remove(easy);
            } catch(...)
            {
            }
        }
    }
}

void multi::Handle::Private::remove(curl::easy::Handle easy)
{
    handle_store.remove(easy);
    curl_multi_remove_handle(handle, easy.native());
}

multi::Handle::Private::Timeout::Timeout(boost::asio::io_service& dispatcher) : d(new Private(dispatcher))
{
}

void multi::Handle::Private::Timeout::cancel()
{
}

void multi::Handle::Private::Timeout::async_wait_for(const std::shared_ptr<Handle::Private>& context, const std::chrono::milliseconds& ms)
{
    d->async_wait_for(context, ms);
}

multi::Handle::Private::Timeout::Private::Private(boost::asio::io_service& dispatcher) : timer(dispatcher)
{
}

void multi::Handle::Private::Timeout::Private::cancel()
{
    timer.cancel();
}

void multi::Handle::Private::Timeout::Private::async_wait_for(const std::shared_ptr<Handle::Private>& context, const std::chrono::milliseconds& ms)
{
    if (ms.count() > 0)
    {
        auto self(shared_from_this());
        timer.expires_from_now(boost::posix_time::milliseconds{ms.count()});
        timer.async_wait([this, self, context](const boost::system::error_code& ec)
        {
            if (ec)
                return;

            handle_timeout(context);
        });
    } else if (ms.count() == 0)
    {
        handle_timeout(context);
    }
}

void multi::Handle::Private::Timeout::Private::handle_timeout(const std::shared_ptr<Handle::Private>& context)
{
    int still_running{0};
    curl_multi_socket_action(context->handle, CURL_SOCKET_TIMEOUT, 0, &still_running);
    context->process_multi_info();
}

int multi::Handle::Private::curl_timer_callback(
        CURLM*,
        long timeout_ms,
        void* cookie)
{
    if (timeout_ms < 0)
        return -1;

    auto holder = static_cast<Holder<Private>*>(cookie);

    if (!holder)
        return 0;

    auto thiz = holder->value;

    thiz->timeout.async_wait_for(thiz, std::chrono::milliseconds{timeout_ms});

    return 0;
}

multi::Handle::Private::Socket::Socket(boost::asio::io_service& dispatcher,
                                       curl_socket_t native)
    : d(new Private(dispatcher, native))
{
}

multi::Handle::Private::Socket::~Socket()
{
    d->cancel();
}

void multi::Handle::Private::Socket::async_wait_for_readable(const std::shared_ptr<multi::Handle::Private>& context)
{
    d->async_wait_for_readable(context);
}

void multi::Handle::Private::Socket::async_wait_for_writeable(const std::shared_ptr<multi::Handle::Private>& context)
{
    d->async_wait_for_writeable(context);
}

multi::Handle::Private::Socket::Socket::Private::Private(boost::asio::io_service& dispatcher,
                                                         curl_socket_t native)
    : cancel_requested(false),
      sd(dispatcher, static_cast<int>(native))
{
}

multi::Handle::Private::Socket::Socket::Private::~Private()
{
    sd.release();
}

void multi::Handle::Private::Socket::Socket::Private::cancel()
{
    cancel_requested = true;
    sd.release();
}

void multi::Handle::Private::Socket::Socket::Private::async_wait_for_readable(const std::shared_ptr<multi::Handle::Private>& context)
{
    auto self(shared_from_this());
    sd.async_read_some(boost::asio::null_buffers{}, [this, self, context](const boost::system::error_code& ec, std::size_t)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (cancel_requested)
            return;

        int bitmask{0};

        if (ec)
            bitmask = CURL_CSELECT_ERR;
        else
            bitmask = CURL_CSELECT_IN;

        int still_running;
        curl_multi_socket_action(
                    context->handle,
                    sd.native_handle(),
                    bitmask,
                    &still_running);

        context->process_multi_info();

        // Restart
        async_wait_for_readable(context);

        if (still_running <= 0)
            context->timeout.cancel();
    });
}

void multi::Handle::Private::Socket::Socket::Private::async_wait_for_writeable(
        std::shared_ptr<multi::Handle::Private> context)
{
    auto self(shared_from_this());
    sd.async_write_some(boost::asio::null_buffers{}, [this, self, context](const boost::system::error_code& ec, std::size_t)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        if (cancel_requested)
            return;

        int bitmask{0};

        if (ec)
            bitmask = CURL_CSELECT_ERR;
        else
            bitmask = CURL_CSELECT_OUT;

        int still_running;
        curl_multi_socket_action(
                    context->handle,
                    sd.native_handle(),
                    bitmask,
                    &still_running);

        context->process_multi_info();

        // Restart
        async_wait_for_writeable(context);

        if (still_running <= 0)
            context->timeout.cancel();
    });
}

int multi::Handle::Private::curl_socket_callback(
        CURL*,
        curl_socket_t s,
        int action,
        void* cookie,
        void* socket_cookie)
{
    static const int doc_tells_we_must_return_0{0};

    auto holder = static_cast<Holder<Private>*>(cookie);

    if (!holder)
        return doc_tells_we_must_return_0;

    auto thiz = holder->value;
    auto socket = static_cast<Socket*>(socket_cookie);

    if (!socket)
    {
        socket = new Socket{thiz->dispatcher, s};
        curl_multi_assign(thiz->handle, s, socket);
    }

    switch (action)
    {
    case CURL_POLL_NONE:
    {
        break;
    }
    case CURL_POLL_IN:
        socket->async_wait_for_readable(thiz);
        break;
    case CURL_POLL_OUT:
        socket->async_wait_for_writeable(thiz);
        break;
    case CURL_POLL_INOUT:
        socket->async_wait_for_readable(thiz);
        socket->async_wait_for_writeable(thiz);
        break;
    case CURL_POLL_REMOVE:
    {
        curl_multi_assign(thiz->handle, s, nullptr);
        delete socket;
        break;
    }
    }

    return doc_tells_we_must_return_0;
}

multi::Handle::Private::Private()
    : handle(curl_multi_init()),
      keep_alive(dispatcher),
      timeout(dispatcher)
{
}

multi::Handle::Private::~Private()
{
    curl_multi_cleanup(handle);
}
