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
#include <boost/bind.hpp>
#include <trial/http/curl/detail/service.hpp>
#include <trial/http/curl/error.hpp>

namespace trial
{
namespace http
{
namespace curl
{

inline socket::socket(boost::asio::io_service& io,
                      const endpoint& remote_endpoint)
    : basic_io_object<service_type>(io),
      real_socket(io),
      easy(0),
      multi(0),
      remote_endpoint(remote_endpoint),
      timer(io)
{
    current.state = state::done;
    current.status_code = 0;
    current.message = 0;

    easy = ::curl_easy_init();
    if (!easy)
    {
        throw curl::error(boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor));
    }
    // FIXME: Move multi handle to service
    multi = ::curl_multi_init();
    if (!multi)
    {
        ::curl_easy_cleanup(easy);
        throw curl::error(boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor));
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
    ::curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);

    ::curl_multi_add_handle(multi, easy);
}

inline socket::~socket()
{
    assert(multi);
    assert(easy);

    ::curl_multi_remove_handle(multi, easy);
    ::curl_multi_cleanup(multi);
    ::curl_easy_cleanup(easy);
}

template <typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_get(BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    current.storage.clear();
    current.status_code = 0;

    if (current.state == state::done)
    {
        current.state = state::waiting;

        ::curl_easy_setopt(easy, CURLOPT_HTTPGET, 1L);
        ::curl_easy_setopt(easy, CURLOPT_URL, remote_endpoint.url().c_str());

        if (perform())
        {
            async_wait_writable(BOOST_ASIO_MOVE_CAST(handler_type)(handler));
        }
    }
    else
    {
        post_handler(curl::make_error_code(curl::invalid_state),
                     BOOST_ASIO_MOVE_CAST(handler_type)(handler));
    }

    return result.get();
}

template <typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_write_head(BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    current.storage.clear();
    current.status_code = 0;

    if (current.state == state::done)
    {
        current.state = state::waiting;

        ::curl_easy_setopt(easy, CURLOPT_HEADER, 1L);
        ::curl_easy_setopt(easy, CURLOPT_NOBODY, 1L); 
        ::curl_easy_setopt(easy, CURLOPT_URL, remote_endpoint.url().c_str());

        if (perform())
        {
            async_wait_writable(BOOST_ASIO_MOVE_CAST(handler_type)(handler));
        }
    }
    else
    {
        post_handler(curl::make_error_code(curl::invalid_state),
                     BOOST_ASIO_MOVE_CAST(handler_type)(handler));
    }

    return result.get();
}

template <typename Message, typename CompletionToken>
typename boost::asio::async_result<
    typename boost::asio::handler_type<CompletionToken,
                                       void(socket::error_code)>::type
    >::type
socket::async_read_response(Message& message,
                            BOOST_ASIO_MOVE_ARG(CompletionToken) token)
{
    typedef typename boost::asio::handler_type<CompletionToken,
                                               void(error_code)>::type
        handler_type;
    handler_type handler(BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
    boost::asio::async_result<handler_type> result(handler);

    if (!current.storage.headers().empty())
    {
        // FIXME: move
        message.headers() = current.storage.headers();
        current.storage.headers().clear();
        if (!current.storage.body().empty())
        {
            message.body() = current.storage.body();
            current.storage.body().clear();
            error_code success;
            post_handler(success,
                         BOOST_ASIO_MOVE_CAST(handler_type)(handler));
            return result.get();
        }
    }

    if (current.state == state::reading)
    {
        current.message = &message;
        if (perform())
        {
            switch (current.state)
            {
            case state::reading:
                async_wait_readable(message,
                                    BOOST_ASIO_MOVE_CAST(handler_type)(handler));
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
                    post_handler(success,
                                 BOOST_ASIO_MOVE_CAST(handler_type)(handler));
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
        post_handler(curl::make_error_code(curl::invalid_state),
                     BOOST_ASIO_MOVE_CAST(handler_type)(handler));
    }

    return result.get();
}

inline bool socket::is_open() const
{
    return ((current.state == state::waiting) || real_socket.is_open());
}

inline int socket::status_code() const
{
    return current.status_code;
}

inline bool socket::perform()
{
    int running_handles = 0;
    CURLMcode mcode = CURLM_OK;
    do
    {
        mcode = ::curl_multi_perform(multi, &running_handles);
    } while (mcode == CURLM_CALL_MULTI_PERFORM);

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
        }
        // FIXME: else call handler with error
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
        if (current.status_code == 0)
        {
            ::curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &current.status_code);
        }
    }
}

inline std::size_t socket::body(const view_type& view)
{
    curl::message& buffer = current.message ? *current.message : current.storage;
    buffer.body().assign(view.begin(), view.end());
    return view.size();
}

template <typename Message, typename ReadHandler>
void socket::async_wait_readable(Message& message,
                                 BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
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
                                    boost::bind(&socket::process_read<Message, ReadHandler>,
                                                this,
                                                _1,
                                                message,
                                                handler));
        break;

    case state::done:
        {
            error_code success;
            dispatch_handler(success, BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
        }
        break;
    }
}

template <typename Message, typename ReadHandler>
void socket::process_read(const error_code& error,
                          Message& message,
                          BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
    if (!is_open())
    {
        // Socket has been closed
        dispatch_handler(error, BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
    }
    else if (perform())
    {
        switch (current.state)
        {
        case state::reading:
            {
                dispatch_handler(boost::asio::error::make_error_code(boost::asio::error::in_progress),
                                 BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
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
                dispatch_handler(success,
                                 BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
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
void socket::async_wait_writable(BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
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
        real_socket.async_write_some(boost::asio::null_buffers(),
                                     boost::bind(&socket::process_write<WriteHandler>,
                                                 this,
                                                 _1,
                                                 handler));
        break;

    case state::reading:
        {
            error_code success;
            dispatch_handler(success, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
        }
        break;

    case state::done:
        break;
    }
}

template <typename WriteHandler>
void socket::process_write(const error_code& error,
                           BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    if (!is_open())
    {
        // Socket has been closed
        dispatch_handler(error, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
    }
    else if (perform())
    {
        async_wait_writable(BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
    }
}

template <typename WriteHandler>
void socket::process_expiration(const error_code& error,
                                BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
{
    // FIXME: expire after N repetitions

    if (error)
    {
        dispatch_handler(error, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
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
                dispatch_handler(success, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
            }
            break;

        case state::done:
            assert(false);
            break;
        }
    }
    // FIXME: else dispatch_handler with error
}

template <typename Handler>
void socket::post_handler(const error_code& error,
                          BOOST_ASIO_MOVE_ARG(Handler) handler)
{
    // Invoke on socket thread
    get_io_service().post(boost::bind(&socket::dispatch_handler<Handler>,
                                      this,
                                      error,
                                      handler));
}

template <typename Handler>
void socket::dispatch_handler(const error_code& error,
                              Handler handler)
{
    handler(error);
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
                                       curl_socket_t item)
{
    assert(closure);

    socket *s = reinterpret_cast<socket *>(closure);
    s->real_socket.close();
    return 0;
}

inline int socket::curl_header_callback(char *buffer,
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

inline int socket::curl_download_callback(char *buffer,
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
