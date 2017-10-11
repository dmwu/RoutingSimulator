cmake_minimum_required(VERSION 3.8)
project(sim)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES_FATTREE datacentre/fattree)

set(SOURCE_FILES_MPTCP mptcp)

add_subdirectory(${SOURCE_FILES_FATTREE})
add_subdirectory(${SOURCE_FILES_MPTCP})