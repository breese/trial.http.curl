#ifndef TRIAL_HTTP_CURL_STATUS_IPP
#define TRIAL_HTTP_CURL_STATUS_IPP

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

class status_category : public boost::system::error_category
{
public:
    const char *name() const BOOST_SYSTEM_NOEXCEPT
    {
        return "trial.http.curl.status";
    }

    std::string message(int value) const
    {
        switch (value)
        {
        case status::success:
            return "success";
        case status::continue_request:
            return "continue";
        case status::non_authoritative_information:
            return "non-authorative information";
        case status::no_content:
            return "no content";
        case status::reset_content:
            return "reset content";
        case status::partial_content:
            return "partial content";
        case status::redirect_moved_permanently:
            return "redirect moved permanently";
        case status::redirect_found:
            return "redirect found";
        case status::bad_request:
            return "bad request";
        case status::unauthorized:
            return "unauthorized";
        case status::payment_required:
            return "payment required";
        case status::forbidden:
            return "forbidden";
        case status::not_found:
            return "not found";
        case status::method_not_allowed:
            return "method not allowed";
        case status::not_acceptable:
            return "not acceptable";
        case status::length_required:
            return "length required";
        }
        return "trial.http.curl status";
    }
};

} // namespace detail

inline const boost::system::error_category& status::category()
{
    static detail::status_category instance;
    return instance;
}

inline boost::system::error_code status::make_error_code(status::value e)
{
    return boost::system::error_code(static_cast<int>(e),
                                     status::category());
}

} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_STATUS_IPP
