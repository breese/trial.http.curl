#ifndef TRIAL_HTTP_CURL_DETAIL_SERVICE_HPP
#define TRIAL_HTTP_CURL_DETAIL_SERVICE_HPP

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/asio/io_service.hpp>

namespace trial
{
namespace http
{
namespace curl
{
class socket;

namespace detail
{

class service : public boost::asio::io_service::service
{
    friend class curl::socket;

public:
    static boost::asio::io_service::id id;

    explicit service(boost::asio::io_service&);
    ~service();

    // Required by boost::asio::basic_io_object
    struct implementation_type {};
    void construct(implementation_type& impl) {}
    void destroy(implementation_type&) {}
    // Required for move construction
    void move_construct(implementation_type&, implementation_type&) {}
    void move_assign(implementation_type&, service&, implementation_type&) {}

private:
    virtual void shutdown_service() {}
};

} // namespace detail
} // namespace curl
} // namespace http
} // namespace trial

#include <trial/http/curl/detail/service.ipp>

#endif // TRIAL_HTTP_CURL_DETAIL_SERVICE_HPP
