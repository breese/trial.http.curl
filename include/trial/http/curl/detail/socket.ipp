#ifndef TRIAL_HTTP_CURL_SOCKET_IPP
#define TRIAL_HTTP_CURL_SOCKET_IPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <algorithm>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <trial/http/curl/detail/service.hpp>
#include <trial/http/curl/error.hpp>
#include <trial/http/curl/status.hpp>

#if defined(TRIAL_HTTP_CURL_DEBUG)
#include <iostream>
#define TRIAL_HTTP_CURL_LOG(x) std::cout << x << std::endl;
#else
#define TRIAL_HTTP_CURL_LOG(x)
#endif

namespace trial
{
namespace http
{
namespace curl
{

namespace detail
{

inline boost::system::error_code make_error_code(CURLcode code)
{
    using namespace boost;
    switch (code)
    {
    case CURLE_OK:
        return error::make_error_code();
    case CURLE_UNSUPPORTED_PROTOCOL:
        return system::errc::make_error_code(system::errc::protocol_not_supported);
    case CURLE_FAILED_INIT:
        return error::make_error_code(error::invalid_state);
    case CURLE_URL_MALFORMAT:
    case CURLE_BAD_FUNCTION_ARGUMENT:
    case CURLE_UNKNOWN_OPTION:
        return system::errc::make_error_code(system::errc::invalid_argument);
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
        return asio::error::make_error_code(asio::error::host_not_found);
    case CURLE_COULDNT_CONNECT:
        return system::errc::make_error_code(system::errc::connection_refused);
    case CURLE_REMOTE_ACCESS_DENIED:
        return system::errc::make_error_code(system::errc::permission_denied);
    case CURLE_PARTIAL_FILE:
        return system::errc::make_error_code(system::errc::message_size);
    case CURLE_OUT_OF_MEMORY:
        return system::errc::make_error_code(system::errc::not_enough_memory);
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_ABORTED_BY_CALLBACK:
        return asio::error::make_error_code(asio::error::operation_aborted);
    case CURLE_BAD_DOWNLOAD_RESUME:
        return system::errc::make_error_code(system::errc::bad_address);
    case CURLE_GOT_NOTHING:
    case CURLE_READ_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_SEND_ERROR:
    case CURLE_CHUNK_FAILED:
        return system::errc::make_error_code(system::errc::broken_pipe);
    case CURLE_FILESIZE_EXCEEDED:
        return system::errc::make_error_code(system::errc::value_too_large);
    case CURLE_AGAIN:
        return system::errc::make_error_code(system::errc::resource_unavailable_try_again);
    default:
        TRIAL_HTTP_CURL_LOG("Unknown CURLcode: " << code);
        return error::make_error_code(error::unknown);
    }
}

struct status_code_type
{
    explicit status_code_type(int v) : value(v) {}
    int value;
};

inline boost::system::error_code make_error_code(status_code_type code)
{
    switch (code.value)
    {
    case 0:
    case 200:
    case 201:
    case 202:
        return error::make_error_code();
    case 203:
        return status::make_error_code(status::non_authoritative_information);
    case 204:
        return status::make_error_code(status::no_content);
    case 205:
        return status::make_error_code(status::reset_content);
    case 206:
        return status::make_error_code(status::partial_content);
    case 301:
        return status::make_error_code(status::redirect_moved_permanently);
    case 302:
        return status::make_error_code(status::redirect_found);
    case 400:
        return status::make_error_code(status::bad_request);
    case 401:
        return status::make_error_code(status::payment_required);
    case 402:
        return status::make_error_code(status::unauthorized);
    case 403:
        return status::make_error_code(status::forbidden);
    case 404:
        return status::make_error_code(status::not_found);
    case 405:
        return status::make_error_code(status::method_not_allowed);
    case 406:
        return status::make_error_code(status::not_acceptable);
    case 411:
        return status::make_error_code(status::length_required);
    default:
        TRIAL_HTTP_CURL_LOG("Unknown status code: " << code.value);
        return error::make_error_code(error::unknown);
    }
}

} // namespace detail

inline socket::socket(boost::asio::io_service& io)
    : basic_io_object<service_type>(io),
      real_socket(io),
      easy(0),
      multi(0),
      timer(io)
{
    current.state = state::done;
    current.message = 0;
    current.header = 0;
    http_200_aliases = 0;

    easy = ::curl_easy_init();
    if (!easy)
    {
        throw curl::exception(boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor));
    }
    // FIXME: Move multi handle to service
    multi = ::curl_multi_init();
    if (!multi)
    {
        ::curl_easy_cleanup(easy);
        throw curl::exception(boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor));
    }

    ::curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, &socket::curl_open_callback);
    ::curl_easy_setopt(easy, CURLOPT_OPENSOCKETDATA, this);
    ::curl_easy_setopt(easy, CURLOPT_CLOSESOCKETFUNCTION, &socket::curl_close_callback);
    ::curl_easy_setopt(easy, CURLOPT_CLOSESOCKETDATA, this);
    ::curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &socket::curl_header_callback);
    ::curl_easy_setopt(easy, CURLOPT_HEADERDATA, this);
    ::curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &socket::curl_download_callback);
    ::curl_easy_setopt(easy, CURLOPT_WRITEDATA, this);

    ::curl_easy_setopt(easy, CURLOPT_USERAGENT, "trial.http.curl/0.1");
    ::curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
    ::curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);
#if defined(TRIAL_HTTP_CURL_DEBUG)
    ::curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
#endif

    ::curl_multi_add_handle(multi, easy);
}

inline socket::~socket()
{
    assert(multi);
    assert(easy);

    ::curl_slist_free_all(current.header);
    current.header = 0;
    ::curl_slist_free_all(http_200_aliases);
    http_200_aliases = 0;
    ::curl_multi_remove_handle(multi, easy);
    ::curl_multi_cleanup(multi);
    ::curl_easy_cleanup(easy);
}

inline void socket::add_http_200_aliases(const std::list<std::string>& http200AliasList)
{
    if (!http200AliasList.empty())
    {
        for (std::list<std::string>::const_iterator it = http200AliasList.begin(); it != http200AliasList.end(); ++it)
        {
            http_200_aliases = curl_slist_append(http_200_aliases, (*it).c_str());
        }
        ::curl_easy_setopt(easy, CURLOPT_HTTP200ALIASES, http_200_aliases);
    }
}

template <typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_get(const endpoint& remote,
                        BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    TRIAL_HTTP_CURL_LOG("async_write_get");

    get_io_service().post(boost::bind(&socket::do_async_write_custom<handler_type>,
                                      this,
                                      "GET",
                                      remote,
                                      BOOST_ASIO_MOVE_CAST(handler_type)(handler)));

    return result.get();
}

template <typename Message, typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_get(const endpoint& remote,
                        const Message& msg,
                        BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    TRIAL_HTTP_CURL_LOG("async_write_get");

    get_io_service().post(boost::bind(&socket::do_async_write_custom<Message, handler_type>,
                                      this,
                                      "GET",
                                      boost::cref(msg),
                                      remote,
                                      BOOST_ASIO_MOVE_CAST(handler_type)(handler)));

    return result.get();
}

template <typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_head(const endpoint& remote,
                         BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    get_io_service().post(boost::bind(&socket::do_async_write_custom<handler_type>,
                                      this,
                                      "HEAD",
                                      remote,
                                      BOOST_ASIO_MOVE_CAST(handler_type)(handler)));
    return result.get();
}

template <typename Message, typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_put(const Message& msg,
                        const endpoint& remote,
                        BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    get_io_service().post(boost::bind(&socket::do_async_write_custom<Message, handler_type>,
                                      this,
                                      "PUT",
                                      boost::cref(msg),
                                      remote,
                                      BOOST_ASIO_MOVE_CAST(handler_type)(handler)));
    return result.get();
}

template <typename Message, typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_post(const Message& msg,
                         const endpoint& remote,
                         BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    get_io_service().post(boost::bind(&socket::do_async_write_custom<Message, handler_type>,
                                      this,
                                      "POST",
                                      boost::cref(msg),
                                      remote,
                                      handler));
    return result.get();
}

template <typename WriteHandler>
void socket::do_async_write_custom(const std::string& method,
                                   const endpoint& remote,
                                   WriteHandler handler)
{
    TRIAL_HTTP_CURL_LOG("do_async_write_custom: " << method);

    current.storage.clear();

    if (current.state == state::done)
    {
        current.state = state::waiting;

        ::curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.c_str());
        ::curl_easy_setopt(easy, CURLOPT_URL, remote.url().c_str());
        if (method == "HEAD")
        {
            ::curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
        }

        if (perform())
        {
            async_wait_writable(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
        }
        else
        {
            error_code success;
            invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                           success);
        }
    }
    else
    {
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                       error::make_error_code(curl::error::invalid_state));
    }
}

template <typename Message, typename WriteHandler>
void socket::do_async_write_custom(const std::string& method,
                                   const Message& msg,
                                   const endpoint& remote,
                                   WriteHandler handler)
{
    TRIAL_HTTP_CURL_LOG("do_async_write_custom: " << method);

    current.storage.clear();
    current.message = 0;

    if (current.state == state::done)
    {
        current.state = state::waiting;

        // Use CURLOPT_CUSTOMREQUEST instead of CURLOPT_UPLOAD to have better
        // control over which HTTP headers are added.
        ::curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.c_str());
        ::curl_easy_setopt(easy, CURLOPT_URL, remote.url().c_str());
        if (!msg.body().empty())
        {
            ::curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, msg.body().size());
            ::curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, msg.body().data());
        }
        if (!msg.headers().empty())
        {
            current.header = 0; // FIXME: Free old?
            for (message::headers_type::const_iterator it = msg.headers().begin();
                 it != msg.headers().end();
                 ++it)
            {
                std::string line = it->first + ": " + it->second;
                current.header = curl_slist_append(current.header, line.c_str());
            }
            ::curl_easy_setopt(easy, CURLOPT_HTTPHEADER, current.header);
        }

        if (perform())
        {
            async_wait_writable(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
        }
        else
        {
            error_code success;
            invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                           success);
        }
    }
    else
    {
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                       error::make_error_code(curl::error::invalid_state));
    }
}

template <typename Message, typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_read_response(Message& msg,
                            BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    TRIAL_HTTP_CURL_LOG("async_read_response");

    get_io_service().post(boost::bind(&socket::do_async_read_response<Message, handler_type>,
                                      this,
                                      boost::ref(msg),
                                      handler));
    return result.get();
}

template <typename Message, typename ReadHandler>
void socket::do_async_read_response(Message& msg,
                                    ReadHandler handler)
{
    TRIAL_HTTP_CURL_LOG("do_async_read_response");

    if (!current.storage.headers().empty() ||
        !current.storage.body().empty())
    {
        TRIAL_HTTP_CURL_LOG("read got data");
        // FIXME: move
        msg.headers() = current.storage.headers();
        current.storage.headers().clear();
        if (!current.storage.body().empty())
        {
            msg.body().assign(current.storage.body().begin(),
                              current.storage.body().end());
            current.storage.body().clear();
            error_code success;
            invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler), success);
            return;
        }
    }

    if (current.state == state::reading)
    {
        current.message = &msg;
        if (perform())
        {
            switch (current.state)
            {
            case state::reading:
                async_wait_readable(handler);
                break;

            default:
                break;
            }
        }
        else
        {
            switch (current.state)
            {
            case state::done:
                {
                    error_code success;
                    invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler),
                                   success);
                }
                break;

            case state::reading:
            case state::writing:
            case state::waiting:
                assert(false);
                break;
            }
        }
    }
    else
    {
        invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler),
                       error::make_error_code(curl::error::invalid_state));
    }
}

inline bool socket::is_open() const
{
    TRIAL_HTTP_CURL_LOG("is_open: " << current.state);
    switch (current.state)
    {
    case state::waiting:
        return true;
    case state::writing:
        if (!current.storage.body().empty())
            return true;
    case state::reading:
    case state::done:
        break;
    }
    return real_socket.is_open();
}

inline bool socket::perform()
{
    TRIAL_HTTP_CURL_LOG("perform");

    int running_handles = 0;
    CURLMcode mcode = CURLM_OK;
    do
    {
        mcode = ::curl_multi_perform(multi, &running_handles);
    } while (mcode == CURLM_CALL_MULTI_PERFORM);
    TRIAL_HTTP_CURL_LOG("perform: " << mcode);

    const bool keep_running = (running_handles > 0);
    if (keep_running)
    {
        fd_set fdread;
        fd_set fdwrite;
        int maxfd = -1;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        mcode = ::curl_multi_fdset(multi, &fdread, &fdwrite, NULL, &maxfd);
        if (mcode == CURLM_OK)
        {
            if (maxfd == -1)
            {
                current.state = state::waiting;
            }
            else if (real_socket.native_handle() > maxfd)
            {
                // FIXME
            }
            else if (FD_ISSET(real_socket.native_handle(), &fdwrite))
            {
                current.state = state::writing;
            }
            else if (FD_ISSET(real_socket.native_handle(), &fdread))
            {
                current.state = state::reading;
            }
        }
    }
    else
    {
        int queue_count = 0;
        CURLMsg *info = ::curl_multi_info_read(multi, &queue_count);
        if (info && (info->msg == CURLMSG_DONE))
        {
            current.state = state::done;
            current.code = detail::make_error_code(info->data.result);
            TRIAL_HTTP_CURL_LOG("perform: " << info->data.result << ", " << current.code.message());
        }
    }
    return keep_running;
}

inline void socket::header(const view_type& key,
                           const view_type& value)
{
    curl::message& buffer = current.message ? *current.message : current.storage;
    const bool is_trailer = !buffer.body().empty();
    if (is_trailer)
    {
        buffer.trailers().emplace(std::string(key.data(), key.size()),
                                  std::string(value.data(), value.size()));
    }
    else
    {
        buffer.headers().emplace(std::string(key.data(), key.size()),
                                 std::string(value.data(), value.size()));
    }
}

inline std::size_t socket::body(const view_type& view)
{
    curl::message& buffer = current.message ? *current.message : current.storage;
    buffer.body().insert(buffer.body().end(), view.begin(), view.end());
    return view.size();
}

template <typename ReadHandler>
void socket::async_wait_readable(ReadHandler handler)
{
    TRIAL_HTTP_CURL_LOG("async_wait_readable");

    switch (current.state)
    {
    case state::waiting:
        // FIXME: Redirect
        assert(false);
        break;

    case state::writing:
        assert(false);
        break;

    case state::reading:
        real_socket.async_read_some(boost::asio::null_buffers(),
                                    boost::bind(&socket::process_read<ReadHandler>,
                                                this,
                                                _1,
                                                handler));
        break;

    case state::done:
        {
            error_code success;
            invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler), success);
        }
        break;
    }
}

template <typename ReadHandler>
void socket::process_read(const error_code& error,
                          ReadHandler handler)
{
    TRIAL_HTTP_CURL_LOG("async_wait_readable: " << error.message());

    if (!is_open())
    {
        // Socket has been closed
        invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler), error);
    }
    else if (perform())
    {
        switch (current.state)
        {
        case state::reading:
            {
                invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler),
                               boost::asio::error::make_error_code(boost::asio::error::in_progress));
            }
            break;

        case state::done:
            assert(false);
            break;
        case state::writing:
            assert(false);
            break;
        case state::waiting:
            // Redirect takes us here
            // FIXME: Start timer (to handle additional DNS lookup)?
            async_wait_writable(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
            break;
        }
    }
    else
    {
        switch (current.state)
        {
        case state::done:
            {
                error_code success;
                invoke_handler(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler),
                               success);
            }
            break;

        case state::reading:
        case state::writing:
        case state::waiting:
            assert(false);
            break;
        }
    }
}

template <typename WriteHandler>
void socket::async_wait_writable(WriteHandler handler)
{
    TRIAL_HTTP_CURL_LOG("async_wait_writable");

    switch (current.state)
    {
    case state::waiting:
        // Avoid busy-looping during address resolution
        timer.expires_from_now(boost::chrono::milliseconds(100));
        timer.async_wait(boost::bind(&socket::process_expiration<WriteHandler>,
                                     this,
                                     _1,
                                     handler));
        break;

    case state::writing:
        real_socket.async_write_some(boost::asio::null_buffers(),
                                     boost::bind(&socket::process_write<WriteHandler>,
                                                 this,
                                                 _1,
                                                 handler));
        break;

    case state::reading:
    case state::done:
        {
            error_code success;
            invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler), success);
        }
        break;
    }
}

template <typename WriteHandler>
void socket::process_write(const error_code& error,
                           WriteHandler handler)
{
    TRIAL_HTTP_CURL_LOG("process_write: " << error.message());

    if (!is_open())
    {
        // Socket has been closed
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler), error);
    }
    else if (perform())
    {
        async_wait_writable(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
    }
    else
    {
        error_code success;
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                       success);
    }
}

template <typename WriteHandler>
void socket::process_expiration(const error_code& error,
                                WriteHandler handler)
{
    TRIAL_HTTP_CURL_LOG("process_expiration: " << error.message());

    // FIXME: expire after N repetitions

    if (error)
    {
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler), error);
    }
    else if (perform())
    {
        switch (current.state)
        {
        case state::waiting:
            // Avoid busy-looping during address resolution
            timer.expires_from_now(boost::chrono::milliseconds(100));
            timer.async_wait(boost::bind(&socket::process_expiration<WriteHandler>,
                                         this,
                                         _1,
                                         handler));
            break;

        case state::writing:
            async_wait_writable(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
            break;

        case state::reading:
            {
                error_code success;
                invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                               success);
            }
            break;

        case state::done:
            assert(false);
            break;
        }
    }
    else
    {
        error_code success;
        invoke_handler(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler),
                       success);
    }
}

template <typename Handler>
void socket::invoke_handler(BOOST_ASIO_MOVE_ARG(Handler) handler,
                            const error_code& error)
{
    error_code code = error;
    if ((!error) || (error == boost::asio::error::in_progress))
    {
        if (current.code)
        {
            code = current.code;
            current.code.clear();
        }
        else
        {
            long status_code = 0L;
            CURLcode rc = ::curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status_code);
            if (rc == CURLE_OK)
            {
                code = detail::make_error_code(detail::status_code_type(status_code));
            }
        }
    }
    TRIAL_HTTP_CURL_LOG("invoke_handler: " << code.message());
    using boost::asio::asio_handler_invoke;
    asio_handler_invoke(boost::bind<void>(handler, code), &handler);
}

inline curl_socket_t socket::curl_open_callback(void *closure,
                                                curlsocktype purpose,
                                                struct curl_sockaddr *address)
{
    if (purpose == CURLSOCKTYPE_IPCXN)
    {
        assert(closure);
        assert(address);
        // assert(address->socktype == SOCK_STREAM);

        socket *s = reinterpret_cast<socket *>(closure);

        s->current.state = state::writing;

        switch (address->family)
        {
        case AF_INET: // IPv4
            {
                error_code error;
                s->real_socket.open(boost::asio::ip::tcp::v4(), error);
                if (!error)
                    return s->real_socket.native_handle();
            }
            break;

        case AF_INET6: // IPv6
            {
                error_code error;
                s->real_socket.open(boost::asio::ip::tcp::v6(), error);
                if (!error)
                    return s->real_socket.native_handle();
            }
            break;

        default:
            break;
        }
    }
    return CURL_SOCKET_BAD;
}

inline int socket::curl_close_callback(void *closure,
                                       curl_socket_t)
{
    assert(closure);

    socket *s = reinterpret_cast<socket *>(closure);
    s->real_socket.close();
    return 0;
}

inline std::size_t socket::curl_header_callback(char *buffer,
                                                std::size_t size,
                                                std::size_t nitems,
                                                void *closure)
{
    assert(closure);

    socket *s = reinterpret_cast<socket *>(closure);

    // FIXME: Use better HTTP header line parser
    for (std::size_t i = 0; i < size * nitems; ++i)
    {
        if (buffer[i] == ':')
        {
            view_type key(buffer, i);
            view_type value(&buffer[i+1], size * nitems - i);
            s->header(key, value);
            break;
        }
    }
    return size * nitems;
}

inline std::size_t socket::curl_download_callback(char *buffer,
                                                  std::size_t size,
                                                  std::size_t nitems,
                                                  void *closure)
{
    assert(closure);
    assert(size == sizeof(buffer[0]));

    socket *s = reinterpret_cast<socket *>(closure);
    view_type view(buffer, nitems);
    return s->body(view);
}

} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_SOCKET_IPP
