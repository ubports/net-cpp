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
#ifndef CORE_NET_HTTP_IMPL_CURL_CURL_H_
#define CORE_NET_HTTP_IMPL_CURL_CURL_H_

#include <core/net/http/client.h>
#include <core/net/http/method.h>
#include <core/net/http/status.h>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <curl/curl.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stack>
#include <thread>

namespace
{
boost::asio::io_service dispatcher;
boost::asio::io_service::work work{dispatcher};
}

namespace curl
{
struct Init
{
    Init()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~Init()
    {
        curl_global_cleanup();
    }
} init;

struct Shared
{
    Shared() : d(new Private())
    {
    }

    CURLSH* handle() const
    {
        return d->handle;
    }

    struct Private
    {
        Private() : handle(curl_share_init())
        {
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
            curl_share_setopt(handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
        }

        ~Private()
        {
            curl_share_cleanup(handle);
        }

        CURLSH* handle;
    };
    std::shared_ptr<Private> d;
};



enum class Code
{
    ok = CURLE_OK
};

namespace info
{
static const CURLINFO response_code = CURLINFO_RESPONSE_CODE;
}

namespace option
{
static const CURLoption header_function = CURLOPT_HEADERFUNCTION;
static const CURLoption header_data = CURLOPT_HEADERDATA;
static const CURLoption write_function = CURLOPT_WRITEFUNCTION;
static const CURLoption write_data = CURLOPT_WRITEDATA;
static const CURLoption read_function = CURLOPT_READFUNCTION;
static const CURLoption read_data = CURLOPT_READDATA;
static const CURLoption url = CURLOPT_URL;
static const CURLoption user_agent = CURLOPT_USERAGENT;
static const CURLoption http_get = CURLOPT_HTTPGET;
static const CURLoption http_post = CURLOPT_POST;
static const CURLoption http_put = CURLOPT_PUT;
static const CURLoption copy_postfields = CURLOPT_COPYPOSTFIELDS;
static const CURLoption post_field_size = CURLOPT_POSTFIELDSIZE;
static const CURLoption upload = CURLOPT_UPLOAD;
static const CURLoption in_file_size = CURLOPT_INFILESIZE;
static const CURLoption sharing = CURLOPT_SHARE;
}
namespace easy
{
static const long disable = 0;
static const long enable = 1;
class Handle
{
private:
    template<std::size_t capacity>
    struct Pool
    {
        static Pool<capacity>& instance()
        {
            static Pool<capacity> inst;
            return inst;
        }

        std::shared_ptr<CURL> acquire_or_wait_for(const std::chrono::microseconds& timeout)
        {
            std::shared_ptr<CURL> result = nullptr;
            std::unique_lock<std::mutex> ul(guard);
            if (!handles.empty())
            {
                result = handles.top();
                handles.pop();
            } else if (size < capacity)
            {
                auto handle = curl_easy_init();
                if (handle)
                {
                    curl_easy_setopt(handle, option::sharing, shared.handle());
                    size++;
                    result = std::shared_ptr<CURL>(handle, [this](CURL* ptr)
                    {
                        curl_easy_cleanup(ptr);
                    });
                    handles.push(result);
                }
            } else
            {
                if (wait_condition.wait_for(ul, timeout, [this]() { return !handles.empty(); }))
                {
                    result = handles.top();
                    handles.pop();
                }
            }

            // We are reusing handles, thus we need to make sure to reset options.
            if (result)
                curl_easy_reset(result.get());

            return result;
        }

        void release(const std::shared_ptr<CURL>& handle)
        {
            if (!handle)
                return;

            std::unique_lock<std::mutex> ul(guard);
            handles.push(handle);
            wait_condition.notify_one();
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        std::atomic<std::size_t> size;
        std::stack<std::shared_ptr<CURL>> handles;
        ::curl::Shared shared;
    };

public:
    typedef std::function<void(curl::Code)> OnFinished;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnReadData;
    typedef std::function<std::size_t(char*, std::size_t, std::size_t)> OnWriteData;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnWriteHeader;

    static std::size_t write_data_cb(char* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz && thiz->on_write_data_cb)
        {
            return thiz->on_write_data_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    static std::size_t write_header_cb(void* data, size_t size, size_t nmemb, void* cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz && thiz->on_write_header_cb)
        {
            return thiz->on_write_header_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    static std::size_t read_data_cb(void* data, std::size_t size, std::size_t nmemb, void *cookie)
    {
        static const std::size_t did_not_consume_any_data = 0;
        auto thiz = static_cast<Private*>(cookie);

        if (thiz && thiz->on_read_data_cb)
        {
            return thiz->on_read_data_cb(data, size, nmemb);
        }

        return did_not_consume_any_data;
    }

    Handle() : d(new Private())
    {
    }

    Handle& url(const char* url)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::url,
                    url);

        return *this;
    }

    Handle& user_agent(const char* user_agent)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::user_agent,
                    user_agent);

        return *this;
    }

    Handle& on_finished(const OnFinished& on_finished)
    {
        d->on_finished_cb = on_finished;
        return *this;
    }

    Handle& on_write_data(const OnWriteData& on_new_data)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::write_function,
                    Handle::write_data_cb);

        curl_easy_setopt(
                    d->handle.get(),
                    option::write_data,
                    d.get());

        d->on_write_data_cb = on_new_data;

        return *this;
    }

    Handle& on_write_header(const OnWriteHeader& on_new_header)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::header_function,
                    Handle::write_header_cb
                    );

        curl_easy_setopt(
                    d->handle.get(),
                    option::header_data,
                    d.get());

        d->on_write_header_cb = on_new_header;

        return *this;
    }

    Handle& on_read_data(const OnReadData& on_read_data, std::size_t size)
    {
        curl_easy_setopt(
                    d->handle.get(),
                    option::read_function,
                    Handle::read_data_cb
                    );
        curl_easy_setopt(
                    d->handle.get(),
                    option::read_data,
                    d.get());
        curl_easy_setopt(
                    d->handle.get(),
                    option::in_file_size,
                    size);
        d->on_read_data_cb = on_read_data;
        return *this;
    }

    Handle& method(core::net::http::Method method)
    {
        switch(method)
        {
        case core::net::http::Method::get:
            curl_easy_setopt(d->handle.get(), option::http_get, enable);
            break;
        case core::net::http::Method::head:
            curl_easy_setopt(d->handle.get(), option::http_get, disable);
            curl_easy_setopt(d->handle.get(), option::http_put, disable);
            curl_easy_setopt(d->handle.get(), option::http_post, disable);
            break;
        case core::net::http::Method::post:
            curl_easy_setopt(d->handle.get(), option::http_post, enable);
            break;
        case core::net::http::Method::put:
            curl_easy_setopt(d->handle.get(), option::http_put, enable);
            break;
        default: throw core::net::http::Client::Errors::HttpMethodNotSupported{method};
        }

        return *this;
    }

    Handle& post_data(const std::string& data, const core::net::http::ContentType&)
    {
        long content_length = data.size();
        curl_easy_setopt(d->handle.get(), option::post_field_size, content_length);
        curl_easy_setopt(d->handle.get(), option::copy_postfields, data.c_str());

        return *this;
    }

    core::net::http::Status status()
    {
        long result;
        curl_easy_getinfo(
                    d->handle.get(),
                    info::response_code,
                    &result);
        return static_cast<core::net::http::Status>(result);
    }

    operator CURL*() const
    {
        return d->handle.get();
    }

    CURL* handle() const
    {
        return d->handle.get();
    }

    Code perform()
    {
        return static_cast<Code>(curl_easy_perform(d->handle.get()));
    }

    void notify_finished(curl::Code code)
    {
        if (d->on_finished_cb)
            d->on_finished_cb(code);
    }

private:
    struct Private
    {
        Private() : handle(Pool<100>::instance().acquire_or_wait_for(std::chrono::microseconds{1000}))
        {
        }

        ~Private()
        {
            Pool<100>::instance().release(handle);
        }

        std::shared_ptr<CURL> handle;

        OnFinished on_finished_cb;
        OnWriteData on_write_data_cb;
        OnWriteHeader on_write_header_cb;
        OnReadData on_read_data_cb;
    };
    std::shared_ptr<Private> d;
};
}
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
struct Handle
{
    struct SynchronizedHandleStore
    {
        std::mutex guard;
        std::map<CURL*, easy::Handle> handles;

        void add(easy::Handle easy)
        {
            std::lock_guard<std::mutex> lg(guard);
            handles.insert(std::make_pair(easy.handle(), easy));
            curl_multi_add_handle(easy.handle());
        }

        void remove(easy::Handle easy)
        {
            std::lock_guard<std::mutex> lg(guard);
            handles.erase(easy.handle());
            curl_multi_remove_handle(d->handle, easy.handle());
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

    static Handle& instance()
    {
        static std::thread worker{[]() { dispatcher.run(); }};
        static Handle inst;
        return inst;
    }

    template<typename T>
    struct Holder
    {
        std::shared_ptr<T> value;
    };

    Handle() : d(new Private())
    {
        auto holder = new Holder<Private>{d};
        curl_multi_setopt(d->handle, option::socket_function, Private::curl_socket_callback);
        curl_multi_setopt(d->handle, option::socket_data, holder);
        curl_multi_setopt(d->handle, option::timer_function, Private::curl_timer_callback);
        curl_multi_setopt(d->handle, option::timer_data, holder);
    }

    void add(curl::easy::Handle easy)
    {
        curl_multi_add_handle(d->handle, easy.handle());
        handle_store().add(easy);
    }

    void remove(curl::easy::Handle easy)
    {
        curl_multi_remove_handle(d->handle, easy.handle());
        handle_store().remove(easy);
    }

    CURLM* handle() const
    {
        return d->handle;
    }

    struct Private
    {

        static void process_multi_info(CURLM* handle)
        {
            CURLMsg* msg{nullptr};

            int msg_count;
            while ((msg = curl_multi_info_read(handle, &msg_count)))
            {
                if (msg->msg == CURLMSG_DONE)
                {
                    auto easy = msg->easy_handle;

                    auto it = known_handles().find(easy);
                    if (it != known_handles().end())
                    {
                        it->second.notify_finished(static_cast<curl::Code>(msg->data.result));
                        known_handles().erase(it);
                    }
                }
            }
        }

        struct Timeout
        {
            Timeout() : timer(dispatcher)
            {
            }

            ~Timeout()
            {
                timer.cancel();
            }

            void async_wait_for(const std::shared_ptr<Private>& context, const std::chrono::milliseconds& ms)
            {
                timer.expires_from_now(boost::posix_time::milliseconds{ms.count()});
                timer.async_wait([context](const boost::system::error_code& ec)
                {

                    if (ec)
                        return;

                    int still_running;
                    curl_multi_socket_action(context->handle, CURL_SOCKET_TIMEOUT, 0, &still_running);

                    Private::process_multi_info(context->handle);
                });
            }

            boost::asio::deadline_timer timer;
        };

        static int curl_timer_callback(
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

        struct Socket
        {
            ~Socket()
            {
                sd.cancel();
            }

            void async_wait_for_readable(const std::shared_ptr<Private>& context)
            {
                sd.async_read_some(boost::asio::null_buffers{}, [this, context](const boost::system::error_code& ec, std::size_t)
                {
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

                    Private::process_multi_info(context->handle);

                    // Restart
                    async_wait_for_readable(context);
                });
            }

            void async_wait_for_writeable(const std::shared_ptr<Private>& context)
            {
                boost::asio::async_write(sd, boost::asio::null_buffers{}, [this, context](const boost::system::error_code& ec, std::size_t)
                {
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

                    Private::process_multi_info(context->handle);

                    // Restart
                    async_wait_for_writeable(context);
                });
            }

            boost::asio::posix::stream_descriptor sd;
        };

        static int curl_socket_callback(
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
                socket = new Socket
                {
                    std::move(boost::asio::posix::stream_descriptor
                    {
                        dispatcher,
                        s
                    })
                };

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
                delete socket;
                break;
            }
            }

            return doc_tells_we_must_return_0;
        }

        Private()
            : handle(curl_multi_init())
        {
        }

        ~Private()
        {
            curl_multi_cleanup(handle);
        }

        CURLM* handle;
        Timeout timeout;
    };

    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_CURL_H_
