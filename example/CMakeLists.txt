###############################################################################
# Coroutine example
###############################################################################

find_package(Boost 1.55.0 COMPONENTS coroutine context thread)
if (NOT ${Boost_FOUND})
  message(FATAL_ERROR "${Boost_ERROR_REASON}")
endif()

add_executable(co_curl
  co_curl.cpp
)
target_link_libraries(co_curl
  ${Boost_COROUTINE_LIBRARY}
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${TRIAL_HTTP_CURL_DEPENDENT_LIBRARIES}
)

###############################################################################
# All examples
###############################################################################

add_custom_target(examples
  DEPENDS co_curl
)
