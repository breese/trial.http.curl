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

void upload(boost::asio::io_service& io,
            std::string url,
            trial::http::curl::message::body_type body,
            boost::asio::yield_context yield)
{
    trial::http::curl::socket socket(io);
    trial::http::curl::endpoint endpoint(url);
    trial::http::curl::message message;
    if (!body.empty())
    {
        message.headers().emplace("Content-Type", "application/x-www-form-urlencoded");
        message.body() = body;
    }
    socket.async_write_put(message, endpoint, yield);

    boost::system::error_code error;
    while (socket.is_open())
    {
        socket.async_read_response(message, yield[error]);
        std::cout << "status = " << socket.status_code() << std::endl;
        std::cout << "error = " << error.message() << std::endl;
        if (error != boost::asio::error::in_progress)
            break;
        std::cout << message;
    }
    if (!error)
    {
        std::cout << message;
    }
}

int main(int argc, char *argv[])
{
    namespace po = boost::program_options;
    po::options_description description(argv[0]);
    description.add_options()
        ("help,h", "Show this help")
        ("data", po::value<std::string>()->default_value(""), "")
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
    std::string data(options["data"].as<std::string>());

    trial::http::curl::message::body_type body;
    body.assign(data.begin(), data.end());
    body.push_back(0); // Terminating zero (that libcurl wants)

    boost::asio::io_service io;
    boost::asio::spawn(io,
                       boost::bind(upload,
                                   boost::ref(io),
                                   url,
                                   body,
                                   _1));
    io.run();
    return 0;
}
