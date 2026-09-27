cmake_minimum_required(VERSION 3.13)

project(http-server)

# Chalo ji, sare source files dhund lao - nested folders vich vi dekho!
# GLOB_RECURSE traverses every level below src/, so this picks up
# src/main.cpp and src/<layer>/<module>.cpp alike.
# CONFIGURE_DEPENDS re-runs configure when the file set changes, so a newly
# added module does not need a manual `cmake -B build -S .`.
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

# Everything except main() goes in a library so the unit tests can drive the
# real code instead of a copy of it. The `http-server` target is unchanged:
# it is still main() plus all of src.
add_library(httpcore STATIC ${SOURCE_FILES})
# Headers live next to their translation units under src/, so `src` itself is
# the include root: #include "http/HttpRequest.hpp" resolves from anywhere.
target_include_directories(httpcore PUBLIC src)
target_link_libraries(httpcore PUBLIC Threads::Threads ZLIB::ZLIB)

add_executable(http-server src/main.cpp)
target_link_libraries(http-server PRIVATE httpcore)

# ---- tests -----------------------------------------------------------------
# tests/run.sh is the entrypoint and does the build itself; these targets exist
# so `ctest` works for anyone who expects it to.
# See tests/README.md for what the suites do and do not cover.
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)

foreach(suite route_matcher negotiation string_utils)
  add_executable(${suite}_test tests/unit/${suite}_test.cpp)
  target_link_libraries(${suite}_test PRIVATE httpcore)
  add_test(NAME ${suite} COMMAND ${suite}_test)
endforeach()

add_test(NAME server_behaviour
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/server_behaviour_test.py
                 $<TARGET_FILE:http-server>)

add_test(NAME suite_size
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_size.py
                 ${CMAKE_CURRENT_SOURCE_DIR}/tests)
