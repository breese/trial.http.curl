#ifndef TRIAL_HTTP_CURL_ENDPOINT_HPP
#define TRIAL_HTTP_CURL_ENDPOINT_HPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <string>

namespace trial
{
namespace http
{
namespace curl
{

class endpoint
{
public:
    explicit endpoint(const std::string&);
    endpoint(const endpoint&);

    const std::string& url() const;

private:
    std::string location;
};

} // namespace curl
} // namespace http
} // namespace trial

#include <trial/http/curl/detail/endpoint.ipp>

#endif // TRIAL_HTTP_CURL_ENDPOINT_HPP
