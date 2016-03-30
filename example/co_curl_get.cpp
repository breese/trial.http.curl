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
#include <boost/program_options.hpp>
#include <trial/http/curl/message.hpp>
#include <trial/http/curl/socket.hpp>
#include "message_io.hpp"

void download(boost::asio::io_service& io,
              std::string url,
              boost::asio::yield_context yield)
{
    boost::system::error_code error;
    trial::http::curl::socket socket(io);
    trial::http::curl::endpoint endpoint(url);
    socket.async_write_get(endpoint, yield[error]);
    if (error)
    {
        std::cout << "error = " << error.message() << std::endl;
    }
    else
    {
        trial::http::curl::message message;
        while (socket.is_open())
        {
            message.clear();
            socket.async_read_response(message, yield[error]);
            if (error)
            {
                std::cout << "error = " << error.message() << std::endl;
            }
            if (error != boost::asio::error::in_progress)
                break;
            std::cout << message;
        }
        if (!error)
        {
            std::cout << message;
        }
    }
}

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    po::options_description description(argv[0]);
    description.add_options()
        ("help,h", "Show this help")
        ("url", po::value<std::string>(), "URL");
    po::positional_options_description position;
    position.add("url", 1);
    po::variables_map options;
    po::store(po::command_line_parser(argc, argv).options(description).positional(position).run(),
              options);
    po::notify(options);

    if (options.count("url") == 0)
    {
        std::cerr << description << std::endl;
        return 1;
    }

    std::string url(options["url"].as<std::string>());

    boost::asio::io_service io;
    boost::asio::spawn(io,
                       boost::bind(download,
                                   boost::ref(io),
                                   url,
                                   _1));
    io.run();
    return 0;
}
