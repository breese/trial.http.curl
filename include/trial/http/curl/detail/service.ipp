#ifndef TRIAL_HTTP_CURL_DETAIL_SERVICE_IPP
#define TRIAL_HTTP_CURL_DETAIL_SERVICE_IPP

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
namespace detail
{

inline service::service(boost::asio::io_service& io)
    : boost::asio::detail::service_base<detail::service>(io)
{
}

inline service::~service()
{
}

} // namespace detail
} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_DETAIL_SERVICE_IPP
