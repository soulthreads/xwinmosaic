cmake_minimum_required (VERSION 2.8)
project (XWinMosaic C)

ADD_DEFINITIONS ("-std=c99 -Wall")
set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_C_FLAGS_DEBUG "-g")

add_subdirectory (src)
