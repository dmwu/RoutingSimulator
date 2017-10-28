cmake_minimum_required(VERSION 3.0.2)
project(sim)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES_FATTREE datacentre/fattree)

set(SOURCE_FILES_MPTCP mptcp)

add_subdirectory(${SOURCE_FILES_FATTREE})
add_subdirectory(${SOURCE_FILES_MPTCP})