#ifndef TRIAL_HTTP_CURL_STATUS_HPP
#define TRIAL_HTTP_CURL_STATUS_HPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/system/error_code.hpp>

namespace trial
{
namespace http
{
namespace curl
{

struct status
{
    enum value
    {
        success = 0,

        // Informational 1xx
        continue_request = 100,

        // Redirection 3xx
        redirect_moved_permanently = 301,
        redirect_found = 302,

        // Client 
        bad_request = 400,
        unauthorized = 402,
        forbidden = 403,
        not_found = 404,
        method_not_allowed = 405
    };

    static const boost::system::error_category& category();

    static boost::system::error_code make_error_code(status::value e = status::success);
};

} // namespace curl
} // namespace http
} // namespace trial

namespace boost
{
namespace system
{

template<> struct is_error_code_enum<trial::http::curl::status::value>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

#include <trial/http/curl/detail/status.ipp>

#endif // TRIAL_HTTP_CURL_STATUS_HPP
