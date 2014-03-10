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
#ifndef CORE_NET_HTTP_IMPL_CURL_EASY_H_
#define CORE_NET_HTTP_IMPL_CURL_EASY_H_

#include <core/net/http/client.h>
#include <core/net/http/method.h>
#include <core/net/http/status.h>

#include "shared.h"

#include <curl/curl.h>

#include <iosfwd>
#include <system_error>

namespace curl
{
typedef CURL* Native;

enum class Code
{
    ok = CURLE_OK,
    unsupported_protocol = CURLE_UNSUPPORTED_PROTOCOL,
    failed_init = CURLE_FAILED_INIT,
    url_malformat = CURLE_URL_MALFORMAT,
    not_built_in = CURLE_NOT_BUILT_IN,
    could_not_resolve_proxy = CURLE_COULDNT_RESOLVE_PROXY,
    could_not_resolve_host = CURLE_COULDNT_RESOLVE_HOST,
    could_not_connect = CURLE_COULDNT_CONNECT,
    remote_access_denied = CURLE_REMOTE_ACCESS_DENIED,
    quote_error = CURLE_QUOTE_ERROR,
    http_returned_error = CURLE_HTTP_RETURNED_ERROR,
    write_error = CURLE_WRITE_ERROR,
    upload_failed = CURLE_UPLOAD_FAILED,
    read_error = CURLE_READ_ERROR,
    out_of_memory = CURLE_OUT_OF_MEMORY,
    operation_timed_out = CURLE_OPERATION_TIMEDOUT,
    range_error = CURLE_RANGE_ERROR,
    http_post_error = CURLE_HTTP_POST_ERROR,
    ssl_connect_error = CURLE_SSL_CONNECT_ERROR,
    bad_download_resume = CURLE_BAD_DOWNLOAD_RESUME,
    function_not_found = CURLE_FUNCTION_NOT_FOUND,
    aborted_by_callback = CURLE_ABORTED_BY_CALLBACK,
    bad_function_argument = CURLE_BAD_FUNCTION_ARGUMENT,
    interface_failed = CURLE_INTERFACE_FAILED,
    too_many_redirects = CURLE_TOO_MANY_REDIRECTS,
    unknown_option = CURLE_UNKNOWN_OPTION,
    peer_failed_verification = CURLE_PEER_FAILED_VERIFICATION,
    ssl_engine_not_found = CURLE_SSL_ENGINE_NOTFOUND,
    ssl_engine_set_failed = CURLE_SSL_ENGINE_SETFAILED,
    send_error = CURLE_SEND_ERROR,
    receive_error = CURLE_RECV_ERROR,
    ssl_cert_problem = CURLE_SSL_CERTPROBLEM,
    ssl_cipher = CURLE_SSL_CIPHER,
    ssl_ca_cert = CURLE_SSL_CACERT,
    bad_content_encoding = CURLE_BAD_CONTENT_ENCODING,
    filesize_exceeded = CURLE_FILESIZE_EXCEEDED,
    send_fail_while_rewinding = CURLE_SEND_FAIL_REWIND,
    ssl_engine_init_failed = CURLE_SSL_ENGINE_INITFAILED,
    login_denied = CURLE_LOGIN_DENIED,
    remote_disk_full = CURLE_REMOTE_DISK_FULL,
    remote_file_exists = CURLE_REMOTE_FILE_EXISTS,
    conversion_failed = CURLE_CONV_FAILED,
    caller_must_register_conversion = CURLE_CONV_REQD,
    ssl_cacert_bad_file = CURLE_SSL_CACERT_BADFILE,
    remote_file_not_found = CURLE_REMOTE_FILE_NOT_FOUND,
    ssl_shutdown_failed = CURLE_SSL_SHUTDOWN_FAILED,
    again = CURLE_AGAIN,
    ssl_crl_bad_file = CURLE_SSL_CRL_BADFILE,
    ssl_issuer_error = CURLE_SSL_ISSUER_ERROR,
    chunk_failed = CURLE_CHUNK_FAILED,
    no_connection_available = CURLE_NO_CONNECTION_AVAILABLE
};

std::ostream& operator<<(std::ostream& out, Code code);

enum class Info
{
    response_code = CURLINFO_RESPONSE_CODE
};

enum class Option
{
    header_function = CURLOPT_HEADERFUNCTION,
    header_data = CURLOPT_HEADERDATA,
    progress_function = CURLOPT_PROGRESSFUNCTION,
    progress_data = CURLOPT_PROGRESSDATA,
    no_progress = CURLOPT_NOPROGRESS,
    write_function = CURLOPT_WRITEFUNCTION,
    write_data = CURLOPT_WRITEDATA,
    read_function = CURLOPT_READFUNCTION,
    read_data = CURLOPT_READDATA,
    url = CURLOPT_URL,
    user_agent = CURLOPT_USERAGENT,
    http_auth = CURLOPT_HTTPAUTH,
    http_get = CURLOPT_HTTPGET,
    http_post = CURLOPT_POST,
    http_put = CURLOPT_PUT,
    copy_postfields = CURLOPT_COPYPOSTFIELDS,
    post_field_size = CURLOPT_POSTFIELDSIZE,
    upload = CURLOPT_UPLOAD,
    in_file_size = CURLOPT_INFILESIZE,
    sharing = CURLOPT_SHARE,
    username = CURLOPT_USERNAME,
    password = CURLOPT_PASSWORD
};

namespace easy
{
constexpr static const long disable = 0;
constexpr static const long enable = 1;

void perform(Native handle);

template<typename T>
inline Code set(Native handle, Option option, T value)
{
    return static_cast<Code>(curl_easy_setopt(handle, static_cast<CURLoption>(option), value));
}

template<typename T>
inline Code get(Native handle, Info info, T value)
{
    return static_cast<Code>(curl_easy_getinfo(handle, static_cast<CURLINFO>(info), value));
}

template<Code ref>
inline void throw_if(Code code)
{
    if (code == ref)
    {
        std::stringstream ss; ss << code;
        throw std::system_error(static_cast<int>(code), std::generic_category(), ss.str());
    }
}

template<Code ref>
inline void throw_if_not(Code code)
{
    if (code != ref)
    {
        std::stringstream ss; ss << code;
        throw std::system_error(static_cast<int>(code), std::generic_category(), ss.str());
    }
}

class Handle
{
public:
    typedef std::function<void(curl::Code)> OnFinished;
    typedef std::function<int(void*, double, double, double, double)> OnProgress;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnReadData;
    typedef std::function<std::size_t(char*, std::size_t, std::size_t)> OnWriteData;
    typedef std::function<std::size_t(void*, std::size_t, std::size_t)> OnWriteHeader;

    Handle();

    template<typename T, typename U>
    inline void get_option(T option, U value)
    {
        throw_if_not<Code::ok>(get(native(), option, value));
    }

    template<typename T>
    inline void set_option(Option option, T value)
    {
        throw_if_not<Code::ok>(set(native(), option, value));
    }

    Handle& url(const char* url);
    Handle& user_agent(const char* user_agent);
    Handle& http_credentials(const std::string& username, const std::string& pwd);
    Handle& on_finished(const OnFinished& on_finished);
    Handle& on_progress(const OnProgress& on_progress);
    Handle& on_read_data(const OnProgress& on_progress);
    Handle& on_read_data(const OnReadData& on_read_data, std::size_t size);
    Handle& on_write_data(const OnWriteData& on_new_data);
    Handle& on_write_header(const OnWriteHeader& on_new_header);

    Handle& method(core::net::http::Method method);
    Handle& post_data(const std::string& data, const core::net::http::ContentType&);

    core::net::http::Status status();

    Native native() const;

    void perform();

    void notify_finished(curl::Code code);

private:
    static int progress_cb(void* data, double dltotal, double dlnow, double ultotal, double ulnow);
    static std::size_t read_data_cb(void* data, std::size_t size, std::size_t nmemb, void *cookie);
    static std::size_t write_data_cb(char* data, size_t size, size_t nmemb, void* cookie);
    static std::size_t write_header_cb(void* data, size_t size, size_t nmemb, void* cookie);

    struct Private;
    std::shared_ptr<Private> d;
};
}
}
#endif // CORE_NET_HTTP_IMPL_CURL_EASY_H_
