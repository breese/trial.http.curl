###############################################################################
# Coroutine example
###############################################################################

find_package(Boost 1.55.0 COMPONENTS program_options coroutine context thread)
if (NOT ${Boost_FOUND})
  message(FATAL_ERROR "${Boost_ERROR_REASON}")
endif()

set(TRIAL_HTTP_CURL_EXAMPLE_LIBRARIES
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_COROUTINE_LIBRARY}
  ${Boost_CONTEXT_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${TRIAL_HTTP_CURL_DEPENDENT_LIBRARIES}
)

add_executable(co_curl_get
  co_curl_get.cpp
)
target_link_libraries(co_curl_get
  ${TRIAL_HTTP_CURL_EXAMPLE_LIBRARIES}
)

add_executable(co_curl_head
  co_curl_head.cpp
)
target_link_libraries(co_curl_head
  ${TRIAL_HTTP_CURL_EXAMPLE_LIBRARIES}
)

add_executable(co_curl_put
  co_curl_put.cpp
)
target_link_libraries(co_curl_put
  ${TRIAL_HTTP_CURL_EXAMPLE_LIBRARIES}
)

add_executable(co_curl_post
  co_curl_post.cpp
)
target_link_libraries(co_curl_post
  ${TRIAL_HTTP_CURL_EXAMPLE_LIBRARIES}
)

###############################################################################
# All examples
###############################################################################

add_custom_target(examples
  DEPENDS co_curl_get
  DEPENDS co_curl_head
  DEPENDS co_curl_put
  DEPENDS co_curl_post
)
