#ifndef TRIAL_HTTP_CURL_DETAIL_ENDPOINT_IPP
#define TRIAL_HTTP_CURL_DETAIL_ENDPOINT_IPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

namespace trial
{
namespace http
{
namespace curl
{

inline endpoint::endpoint(const std::string& url)
    : location(url)
{
}

inline endpoint::endpoint(const endpoint& other)
    : location(other.location)
{
}

inline const std::string& endpoint::url() const
{
    return location;
}

} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_DETAIL_ENDPOINT_IPP
