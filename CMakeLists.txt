cmake_minimum_required(VERSION 3.13)

project(http-server)

# Chalo ji, sare source files dhund lao - nested folders vich vi dekho!
# GLOB_RECURSE traverses every level below src/, so this picks up
# src/main.cpp and src/<layer>/<module>.cpp alike.
# CONFIGURE_DEPENDS re-runs configure when the file set changes, so a newly
# added module does not need a manual `cmake -B build -S .`.
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(http-server ${SOURCE_FILES})

# Headers live next to their translation units under src/, so `src` itself is
# the include root: #include "http/HttpRequest.hpp" resolves from anywhere.
target_include_directories(http-server PRIVATE src)

target_link_libraries(http-server PRIVATE Threads::Threads ZLIB::ZLIB)
