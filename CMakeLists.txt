cmake_minimum_required (VERSION 2.8)
project (XWinMosaic C)

OPTION (WITH_SCRIPTS "Install extra scripts" OFF)

ADD_DEFINITIONS ("-std=c99 -Wall")
set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_C_FLAGS_DEBUG "-g")

add_subdirectory (src)

if (WITH_SCRIPTS)
  add_subdirectory (contrib)
ENDIF (WITH_SCRIPTS)
