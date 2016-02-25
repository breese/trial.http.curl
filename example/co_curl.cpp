///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <trial/http/curl/message.hpp>
#include <trial/http/curl/socket.hpp>

void dump(const trial::http::curl::message& message)
{
    for (trial::http::curl::message::headers_type::const_iterator it = message.headers().begin();
         it != message.headers().end();
         ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    for (trial::http::curl::message::body_type::const_iterator it = message.body().begin();
         it != message.body().end();
         ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;
}

void download(boost::asio::io_service& io,
              std::string url,
              boost::asio::yield_context yield)
{
    trial::http::curl::endpoint endpoint(url);
    trial::http::curl::socket socket(io, endpoint);
    socket.async_write_get(yield);

    trial::http::curl::message message;
    boost::system::error_code error;
    while (socket.is_open())
    {
        socket.async_read_response(message, yield[error]);
        std::cout << "status = " << socket.status_code() << std::endl;
        std::cout << "error = " << error.message() << std::endl;
        if (error != boost::asio::error::in_progress)
            break;
        dump(message);
    }
    if (!error)
    {
        dump(message);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <url>" << std::endl;
        return 1;
    }

    boost::asio::io_service io;
    std::string url(argv[1]);
    boost::asio::spawn(io,
                       boost::bind(download,
                                   boost::ref(io),
                                   url,
                                   _1));
    io.run();
    return 0;
}
