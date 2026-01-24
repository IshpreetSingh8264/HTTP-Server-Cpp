cmake_minimum_required(VERSION 3.13)

project(http-server-starter-cpp)

# Chalo ji, sare source files dhund lao - nested folders vich vi dekho!
# (Okay folks, find all source files - look in nested folders too!)
file(GLOB_RECURSE SOURCE_FILES 
  src/*.cpp 
  src/**/*.cpp
)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Include directory nu add karo taaki headers mil sakein
# (Add include directory so headers can be found)
include_directories(include)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(http-server ${SOURCE_FILES})

# Libraries link karo - threads te gzip compression lai
# (Link libraries - for threads and gzip compression)
target_link_libraries(http-server PRIVATE Threads::Threads ZLIB::ZLIB)
