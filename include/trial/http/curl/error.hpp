#ifndef TRIAL_HTTP_CURL_ERROR_HPP
#define TRIAL_HTTP_CURL_ERROR_HPP

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
#include <boost/system/system_error.hpp>

namespace trial
{
namespace http
{
namespace curl
{

struct error
{
    enum value
    {
        success = 0,

        unknown,
        invalid_state
    };

    static const boost::system::error_category& category();

    static boost::system::error_code make_error_code(error::value e = error::success);
};

class exception : public boost::system::system_error
{
public:
    exception(boost::system::error_code ec)
        : system_error(ec)
    {}
};

} // namespace curl
} // namespace http
} // namespace trial

namespace boost
{
namespace system
{

template<> struct is_error_code_enum<trial::http::curl::error::value>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

#include <trial/http/curl/detail/error.ipp>

#endif // TRIAL_HTTP_CURL_ERROR_HPP
