/* Copyright (c) 2014 Vinícius dos Santos Oliveira

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt) */

// Adapted from https://github.com/BoostGSoC14/boost.http/

#ifndef TRIAL_HTTP_CURL_MESSAGE_HPP
#define TRIAL_HTTP_CURL_MESSAGE_HPP

#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/container/flat_map.hpp>

namespace trial
{
namespace http
{
namespace curl
{

typedef boost::container::flat_multimap<std::string, std::string> headers;

template<class Headers, class Body>
struct basic_message
{
    typedef Headers headers_type;
    typedef Body body_type;

    headers_type &headers()
    {
        return headers_;
    }

    const headers_type &headers() const
    {
        return headers_;
    }

    body_type &body()
    {
        return body_;
    }

    const body_type &body() const
    {
        return body_;
    }

    headers_type &trailers()
    {
        return trailers_;
    }

    const headers_type &trailers() const
    {
        return trailers_;
    }

    void clear()
    {
        headers_.clear();
        body_.clear();
        trailers_.clear();
    }

private:
    headers_type headers_;
    body_type body_;
    headers_type trailers_;
};

typedef basic_message< curl::headers, std::vector<boost::uint8_t> > message;

} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_MESSAGE_HPP
