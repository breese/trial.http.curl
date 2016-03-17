#ifndef TRIAL_HTTP_CURL_EXAMPLE_MESSAGE_IO_HPP
#define TRIAL_HTTP_CURL_EXAMPLE_MESSAGE_IO_HPP

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
#include <trial/http/curl/message.hpp>

inline std::ostream& operator << (std::ostream& stream,
                                  const trial::http::curl::message& msg)
{
    for (trial::http::curl::message::headers_type::const_iterator it = msg.headers().begin();
         it != msg.headers().end();
         ++it)
    {
        stream << it->first << ": " << it->second;
    }
    stream << "\n";
    stream.write(reinterpret_cast<const std::ostream::char_type*>(msg.body().data()),
                 msg.body().size());
    stream << "\n";
    return stream;
}

#endif // TRIAL_HTTP_CURL_EXAMPLE_MESSAGE_IO_HPP
