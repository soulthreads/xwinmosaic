XWinMosaic
==========

X11 window switcher that draws windows list as colour mosaic
------------------------------------------------------------

Inspired by [XMonad.Actions.GridSelect](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Actions-GridSelect.html), but written in C + GTK+2, uses nice-looking colours and has some set of helpful features.

![xwinmosaic's screenshot](http://i.imgur.com/UoMDO.png "Screenshot")

Use arrow keys or mouse to navigate through windows.
Start typing to search for required window.

### Usage:

	xwinmosaic [OPTIONS]
	Options:
		-h                Show this help
		-r                Read items from stdin (and print selected item to stdout)
		-C                Turns off box colorizing
		-I                Turns off showing icons
		-D                Turns off showing desktop number

		-W <int>          Width of the boxes (default: 200)
		-H <int>          Height of the boxes (default: 40)
		-i <int>          Size of window icons (default: 16)
		-f "font name"    Which font to use for displaying widgets. (default: Sans)
		-s <int>          Font size (default: 10)

### Dependencies:

* [EWMH compatible Window Manager](http://en.wikipedia.org/wiki/Extended_Window_Manager_Hints)
* GTK+2
* CMake

### Building:

	cd xwinmosaic
	mkdir build
	cmake ..         # or, if you wish to install it, cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
	make
	./src/xwinmosaic # or sudo make install, if you trust me. :)
