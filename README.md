XWinMosaic
==========

X11 window switcher that draws windows list as colour mosaic
------------------------------------------------------------

Inspired by [XMonad.Actions.GridSelect](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Actions-GridSelect.html), but written in C + GTK+2, uses nice-looking colours and has some set of helpful features.

![xwinmosaic's screenshot](http://i.imgur.com/zubWi.png "Screenshot")

Use arrow keys (also `C-n`, `C-p`, `C-f`, `C-b` in default mode, `hjkl` in vim mode) or mouse to navigate through windows.
Start typing to search for required window.

Config file is created automatically on first program run and stored in `~/.config/xwinmosaic/config`.

### Usage:
    Usage:
      xwinmosaic [OPTION...]  - show X11 windows as colour mosaic

    Help Options:
      -h, --help                      Show help options
      --help-all                      Show all help options
      --help-gtk                      Show GTK+ Options

    Application Options:
      -r, --read-stdin                Read items from stdin (and print selected item to stdout)
      -p, --permissive                Lets search entry text to be used as individual item.
      -V, --vim-mode                  Turn on vim-like navigation (hjkl, search on /)
      -C, --no-colors                 Turn off box colorizing
      -I, --no-icons                  Turn off showing icons
      -D, --no-desktops               Turn off showing desktop number
      -S, --screenshot                Get screenshot and set it as a background (for WMs that do not support XShape)
      -P, --at-pointer                Place center of mosaic at pointer position.
      -W, --box-width=<int>           Width of the boxes (default: 200)
      -H, --box-height=<int>          Height of the boxes (default: 40)
      -i, --icon-size=<int>           Size of window icons (default: 16)
      -f, --font-name="font name"     Which font to use for displaying widgets. (default: Sans)
      -s, --font-size=<int>           Font size (default: 10)
      -o, --hue-offset=<int>          Set color hue offset (from 0 to 255)
      --display=DISPLAY               X display to use

### Dependencies:

* [EWMH compatible Window Manager](http://en.wikipedia.org/wiki/Extended_Window_Manager_Hints)
* GTK+2
* CMake

### Building:

	cd xwinmosaic
	mkdir build
	cd build
	cmake ..         # or, if you wish to install it, cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
	make
	./src/xwinmosaic # or sudo make install, if you trust me. :)
