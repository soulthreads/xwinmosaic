find_package (PkgConfig)
pkg_check_modules (DEPS REQUIRED gtk+-2.0 x11)
set (CFLAGS ${DEPS_CFLAGS}
    	    ${DEPS_CFLAGS_OTHER})
set (LIB_PATHS ${DEPS_LIBRARY_DIRS})
link_directories (${LIB_PATHS})
add_definitions (${CFLAGS})

add_executable (xwinmosaic x_interaction.c mosaic_window_box.c main.c)

target_link_libraries (xwinmosaic ${DEPS_LIBRARIES})

install (TARGETS xwinmosaic
	RUNTIME DESTINATION bin)
