#ifndef TRIAL_HTTP_CURL_SOCKET_HPP
#define TRIAL_HTTP_CURL_SOCKET_HPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <curl/curl.h>
#include <string>
#include <map>
#include <boost/system/error_code.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/detail/socket_option.hpp>
#include <boost/utility/string_ref.hpp>
#include <trial/http/curl/endpoint.hpp>
#include <trial/http/curl/detail/service.hpp>

namespace trial
{
namespace http
{
namespace curl
{

class socket
    : public boost::asio::socket_base,
      public boost::asio::basic_io_object<curl::detail::service>
{
    friend class detail::service;
    typedef curl::detail::service service_type;
    typedef boost::system::error_code error_code;
    typedef boost::string_ref view_type;

public:
    socket(boost::asio::io_service&);
    ~socket();

    // Send a HTTP GET request.
    template <typename CompletionToken>
    typename boost::asio::async_result<
        typename boost::asio::handler_type<CompletionToken,
                                           void(error_code)>::type
        >::type
    async_write_get(const endpoint&,
                    BOOST_ASIO_MOVE_ARG(CompletionToken) token);

    // Send a HTTP HEAD request.
    template <typename CompletionToken>
    typename boost::asio::async_result<
        typename boost::asio::handler_type<CompletionToken,
                                           void(error_code)>::type
        >::type
    async_write_head(const endpoint&,
                     BOOST_ASIO_MOVE_ARG(CompletionToken) token);

    // Send a HTTP PUT request.
    template <typename Message, typename CompletionToken>
    typename boost::asio::async_result<
        typename boost::asio::handler_type<CompletionToken,
                                           void(error_code)>::type
        >::type
    async_write_put(const Message&,
                    const endpoint&,
                    BOOST_ASIO_MOVE_ARG(CompletionToken) token);

    // Receive a HTTP response.
    //
    // The handler is called with:
    // - no error when the full response has been received
    // - asio::error::in_progress if a partial response has  been received
    //   (e.g. during HTTP chunking)
    // - asio::error::operation_aborted when the operation is terminated
    // - an other error when an error occurred
    template <typename Message, typename CompletionToken>
    typename boost::asio::async_result<
        typename boost::asio::handler_type<CompletionToken,
                                           void(error_code)>::type
        >::type
    async_read_response(Message& message,
                        BOOST_ASIO_MOVE_ARG(CompletionToken) token);

    bool is_open() const;

private:
    static curl_socket_t curl_open_callback(void *, curlsocktype, struct curl_sockaddr *);
    static int curl_close_callback(void *, curl_socket_t);
    static std::size_t curl_header_callback(char *, std::size_t, std::size_t, void *);
    static std::size_t curl_download_callback(char *, std::size_t, std::size_t, void *);

    template <typename WriteHandler>
    void do_async_write_get(const endpoint&,
                            const WriteHandler& handler);
    template <typename WriteHandler>
    void do_async_write_head(const endpoint&,
                             const WriteHandler& handler);
    template <typename Message, typename WriteHandler>
    void do_async_write_put(const Message&,
                            const endpoint&,
                            const WriteHandler& handler);
    template <typename Message, typename ReadHandler>
    void do_async_read_response(Message&,
                                const ReadHandler& handler);

    bool perform();
    void header(const view_type& key, const view_type& value);
    std::size_t body(const view_type&);

    template <typename WriteHandler>
    void async_wait_writable(BOOST_ASIO_MOVE_ARG(WriteHandler) handler);
    template <typename WriteHandler>
    void process_write(const error_code&,
                       BOOST_ASIO_MOVE_ARG(WriteHandler) handler);
    template <typename WriteHandler>
    void process_expiration(const error_code&,
                            BOOST_ASIO_MOVE_ARG(WriteHandler) handler);

    template <typename Message, typename ReadHandler>
    void async_wait_readable(Message&, BOOST_ASIO_MOVE_ARG(ReadHandler) handler);
    template <typename Message, typename ReadHandler>
    void process_read(const error_code&,
                      Message&,
                      BOOST_ASIO_MOVE_ARG(ReadHandler) handler);

    template <typename Handler>
    void post_handler(BOOST_ASIO_MOVE_ARG(Handler) handler,
                      const error_code& error);
    template <typename Handler>
    void invoke_handler(BOOST_ASIO_MOVE_ARG(Handler) handler,
                        const error_code& error);

private:
    boost::asio::ip::tcp::socket real_socket;
    CURL *easy;
    CURLM *multi;
    boost::asio::steady_timer timer;

    struct state
    {
        enum value
        {
            waiting,
            writing,
            reading,
            done
        };
    };
    struct
    {
        socket::state::value state;
        error_code code;
        curl::message *message;
        curl::message storage;
        struct curl_slist *header;
        curl::message::body_type::const_iterator position;
        curl::message::body_type::const_iterator ending;
    } current;
};

} // namespace curl
} // namespace http
} // namespace trial

#include <trial/http/curl/detail/socket.ipp>

#endif // TRIAL_HTTP_CURL_SOCKET_HPP
