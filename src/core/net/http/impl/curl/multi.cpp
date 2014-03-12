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

namespace easy = ::curl::easy;
namespace multi = ::curl::multi;

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

std::ostream& multi::operator<<(std::ostream& out, multi::Code code)
{
    switch (code)
    {
    case multi::Code::call_multi_perform: out << "curl::multi::Code::call_multi_perform"; break;
    case multi::Code::ok: out << "curl::multi::Code::ok"; break;
    case multi::Code::bad_handle: out << "curl::multi::Code::bad_handle"; break;
    case multi::Code::bad_easy_handle: out << "curl::multi::Code::easy_handle"; break;
    case multi::Code::out_of_memory: out << "curl::multi::Code::out_of_memory"; break;
    case multi::Code::internal_error: out << "curl::multi::Code::internal_error"; break;
    case multi::Code::bad_socket: out << "curl::multi::Code::bad_socket"; break;
    case multi::Code::unknown_option: out << "curl::multi::Code::unknown_option"; break;
    case multi::Code::added_already: out << "curl::multi::Code::added_already"; break;
    }

    return out;
}

multi::native::Handle multi::native::init()
{
    return curl_multi_init();
}

multi::Code multi::native::cleanup(multi::native::Handle handle)
{
    return static_cast<multi::Code>(curl_multi_cleanup(handle));
}

multi::Code multi::native::assign(multi::native::Handle handle, multi::native::Socket socket, void* cookie)
{
    return static_cast<multi::Code>(curl_multi_assign(handle, socket, cookie));
}

multi::Code multi::native::add_handle(multi::native::Handle m, easy::native::Handle e)
{
    return static_cast<multi::Code>(curl_multi_add_handle(m, e));
}

multi::Code multi::native::remove_handle(multi::native::Handle m, easy::native::Handle e)
{
    return static_cast<multi::Code>(curl_multi_remove_handle(m, e));
}

std::pair<multi::native::Message, int> multi::native::read_info(multi::native::Handle handle)
{
    int count{-1};
    auto msg = curl_multi_info_read(handle, &count);

    return std::make_pair(msg, count);
}

std::pair<multi::Code, int> multi::native::socket_action(multi::native::Handle handle, multi::native::Socket socket, int events)
{
    int count{-1};
    auto rc = static_cast<multi::Code>(
                curl_multi_socket_action(
                    handle,
                    socket,
                    events,
                    &count));

    return std::make_pair(rc, count);
}

struct multi::Handle::Private
{
    void process_multi_info();

    static int timer_callback(
            multi::native::Handle,
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

    static int socket_callback(
            easy::native::Handle,
            multi::native::Socket s,
            int action,
            void* cookie,
            void* socket_cookie);

    struct Socket
    {
        Socket(boost::asio::io_service& dispatcher,
               multi::native::Socket native);
        ~Socket();

        void async_wait_for_readable(const std::shared_ptr<Handle::Private>& context);
        void async_wait_for_writeable(const std::shared_ptr<Handle::Private>& context);

        struct Private : public std::enable_shared_from_this<Private>
        {
            Private(boost::asio::io_service& dispatcher,
                    multi::native::Socket native);
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

    multi::native::Handle handle;
    boost::asio::io_service dispatcher;
    boost::asio::io_service::work keep_alive;
    SynchronizedHandleStore handle_store;
    Timeout timeout;
};

multi::Handle::Handle() : d(new Private())
{
    auto holder = new Holder<Private>{d};

    set_option(Option::socket_function, Private::socket_callback);
    set_option(Option::socket_data, holder);
    set_option(Option::timer_function, Private::timer_callback);
    set_option(Option::timer_data, holder);
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
    multi::throw_if_not<multi::Code::ok>(
                multi::native::add_handle(
                    native(),
                    easy.native()));
}

void multi::Handle::remove(easy::Handle easy)
{
    d->handle_store.remove(easy);
    multi::throw_if_not<multi::Code::ok>(
                multi::native::remove_handle(
                    native(),
                    easy.native()));
}

curl::easy::Handle multi::Handle::easy_handle_from_native(easy::native::Handle native)
{
    return d->handle_store.lookup_native(native);
}

multi::native::Handle multi::Handle::native() const
{
    return d->handle;
}

void multi::Handle::Private::process_multi_info()
{
    while (true)
    {
        multi::native::Message msg;
        int count;

        std::tie(msg, count) = multi::native::read_info(handle);

        if (!msg)
            break;

        if (msg->msg == CURLMSG_DONE)
        {
            auto native_easy = msg->easy_handle;
            auto rc = static_cast<curl::Code>(msg->data.result);
            try
            {
                handle_store.lookup_native(native_easy).notify_finished(rc);
                multi::native::remove_handle(handle, native_easy);
            } catch(...)
            {
            }
        }
    }
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
    auto result = multi::native::socket_action(context->handle, CURL_SOCKET_TIMEOUT, 0);
    multi::throw_if_not<multi::Code::ok>(result.first);

    context->process_multi_info();
}

int multi::Handle::Private::timer_callback(
        multi::native::Handle,
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
                                       multi::native::Socket native)
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
                                                         multi::native::Socket native)
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

        auto result = multi::native::socket_action(context->handle, sd.native_handle(), bitmask);
        multi::throw_if_not<multi::Code::ok>(result.first);

        context->process_multi_info();

        // Restart
        async_wait_for_readable(context);

        if (result.second <= 0)
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

        auto result = multi::native::socket_action(context->handle, sd.native_handle(), bitmask);
        multi::throw_if_not<multi::Code::ok>(result.first);

        context->process_multi_info();

        // Restart
        async_wait_for_writeable(context);

        if (result.second <= 0)
            context->timeout.cancel();
    });
}

int multi::Handle::Private::socket_callback(
        easy::native::Handle,
        multi::native::Socket s,
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
        multi::throw_if_not<multi::Code::ok>(multi::native::assign(thiz->handle, s, socket));
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
        multi::native::assign(thiz->handle, s, nullptr);
        delete socket;
        break;
    }
    }

    return doc_tells_we_must_return_0;
}

multi::Handle::Private::Private()
    : handle(multi::native::init()),
      keep_alive(dispatcher),
      timeout(dispatcher)
{
}

multi::Handle::Private::~Private()
{
    multi::native::cleanup(handle);
}
