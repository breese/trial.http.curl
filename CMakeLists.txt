###############################################################################
#
# Copyright (C) 2016 Bjorn Reese <breese@users.sourceforge.net>
#
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################

cmake_minimum_required(VERSION 2.8)
project(trial.http.curl)

set(TRIAL_HTTP_CURL_BUILD_DIR ${CMAKE_BINARY_DIR})
set(TRIAL_HTTP_CURL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARY_OUTPUT_PATH ${TRIAL_HTTP_CURL_BUILD_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${TRIAL_HTTP_CURL_BUILD_DIR}/bin)

#add_definitions(-Wall -Wextra -Wshadow)
#add_definitions(-DTRIAL_HTTP_CURL_DEBUG=1)
#add_definitions(-std=c++11)

###############################################################################
# Boost package
###############################################################################

find_package(Boost 1.55.0 COMPONENTS chrono system)
if (NOT ${Boost_FOUND})
  message(FATAL_ERROR "${Boost_ERROR_REASON}")
endif()

include_directories(BEFORE ${Boost_INCLUDE_DIR})

###############################################################################
# Curl package
###############################################################################

include(FindPkgConfig)
pkg_check_modules(Curl REQUIRED libcurl)

###############################################################################
# Trial.Protocol package
###############################################################################

set(TRIAL_HTTP_CURL_DEPENDENT_LIBRARIES
  ${Boost_CHRONO_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Curl_LIBRARIES})

include_directories(BEFORE include)

add_subdirectory(example EXCLUDE_FROM_ALL)
