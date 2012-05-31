XWinMosaic
==========

EWMH-compatible window switcher that arranges them in a sort-of colorful mosaic
-------------------------------------------------------------------------------

Inspired by [XMonad.Actions.GridSelect](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Actions-GridSelect.html), written in C + GTK+2.

![xwinmosaic's screenshot](http://i.imgur.com/UoMDO.png "Screenshot")

Use arrow keys or mouse to navigate through windows.
Start typing to search for required window.

### Usage:

	xwinmosaic [OPTIONS]
	Actions:
		-h          Show this help
		-C          Turns off box colorizing
		-I          Turns off showing icons
		-D          Turns off showing desktop number

		-W <int>    Width of the boxes (default: 200)
		-H <int>    Height of the boxes (default: 40)
		-i <int>    Size of window icons (default: 16)

### Dependencies:

* GTK+2
* CMake

### Building:

	cd xwinmosaic
	mkdir build
	cmake ..         # or, if you wish to install it, cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
	make
	./src/xwinmosaic # or sudo make install, if you trust me. :)
