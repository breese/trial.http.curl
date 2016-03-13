#ifndef TRIAL_HTTP_CURL_ERROR_IPP
#define TRIAL_HTTP_CURL_ERROR_IPP

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

class error_category : public boost::system::error_category
{
public:
    const char *name() const BOOST_SYSTEM_NOEXCEPT
    {
        return "trial.http.curl.error";
    }

    std::string message(int value) const
    {
        switch (value)
        {
        case error::success:
            return "success";
        case error::unknown:
            return "unknown";
        case error::invalid_state:
            return "invalid state";
        }
        return "trial.http.curl error";
    }
};

} // namespace detail

inline const boost::system::error_category& error::category()
{
    static detail::error_category instance;
    return instance;
}

inline boost::system::error_code error::make_error_code(error::value e)
{
    return boost::system::error_code(static_cast<int>(e),
                                     curl::error::category());
}

} // namespace curl
} // namespace http
} // namespace trial

#endif // TRIAL_HTTP_CURL_ERROR_IPP
