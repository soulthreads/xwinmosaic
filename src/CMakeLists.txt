find_package (PkgConfig)

IF(UNIX)
  pkg_check_modules (DEPS REQUIRED gtk+-2.0 x11)
ENDIF(UNIX)

IF(WIN32)
  pkg_check_modules (DEPS REQUIRED gtk+-2.0)
ENDIF(WIN32)

set (CFLAGS ${DEPS_CFLAGS}
    	    ${DEPS_CFLAGS_OTHER})
set (LIB_PATHS ${DEPS_LIBRARY_DIRS})
link_directories (${LIB_PATHS})
add_definitions (${CFLAGS})

IF(UNIX)
  add_executable (xwinmosaic x_interaction.c mosaic_box.c mosaic_window_box.c mosaic_search_box.c main.c)
ENDIF(UNIX)
  add_executable (xwinmosaic win32_interaction.c mosaic_box.c mosaic_window_box.c mosaic_search_box.c main.c)
IF(WIN32)

ENDIF(WIN32)

target_link_libraries (xwinmosaic ${DEPS_LIBRARIES})

install (TARGETS xwinmosaic
	RUNTIME DESTINATION bin)
