/* Copyright (c) 2012, Anton S. Lobashev
 * x_interaction.h - layer to talk with X11.
 */
#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#endif
#ifdef WIN32
#include <windows.h>
#define Window HWND
#endif

#ifndef X_INTERACTION_H
#define X_INTERACTION_H

#ifdef X11
Atom a_UTF8_STRING;

Atom a_WM_CLASS;
Atom a_WM_NAME;
Atom a_WM_WINDOW_ROLE;

Atom a_NET_SUPPORTING_WM_CHECK;
Atom a_NET_CLIENT_LIST;
Atom a_NET_DESKTOP_VIEWPORT;
Atom a_NET_CURRENT_DESKTOP;
Atom a_NET_ACTIVE_WINDOW;
Atom a_NET_WORKAREA;

Atom a_NET_WM_DESKTOP;
Atom a_NET_WM_NAME;
Atom a_NET_WM_VISIBLE_NAME;
Atom a_NET_WM_ICON;
Atom a_NET_WM_USER_TIME;

Atom a_NET_WM_WINDOW_TYPE;

Atom a_NET_WM_WINDOW_TYPE_DESKTOP;
Atom a_NET_WM_WINDOW_TYPE_DOCK;
Atom a_NET_WM_WINDOW_TYPE_TOOLBAR;
Atom a_NET_WM_WINDOW_TYPE_MENU;
Atom a_NET_WM_WINDOW_TYPE_UTILITY;
Atom a_NET_WM_WINDOW_TYPE_SPLASH;
Atom a_NET_WM_WINDOW_TYPE_DIALOG;
Atom a_NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
Atom a_NET_WM_WINDOW_TYPE_POPUP_MENU;
Atom a_NET_WM_WINDOW_TYPE_TOOLTIP;
Atom a_NET_WM_WINDOW_TYPE_NOTIFICATION;
Atom a_NET_WM_WINDOW_TYPE_COMBO;
Atom a_NET_WM_WINDOW_TYPE_DND;
Atom a_NET_WM_WINDOW_TYPE_NORMAL;
#endif

#ifdef X11
void atoms_init ();
#endif
#ifdef X11
void* property (Window win, Atom prop, Atom type, int *nitems);
#endif
void climsg(Window win, long type, long l0, long l1, long l2, long l3, long l4);
#ifdef X11
int wm_supports_ewmh ();
#endif
char* get_window_name (Window win);
char* get_window_class (Window win);
int get_window_desktop (Window win);
Window* sorted_windows_list (Window *myown, Window *active_win, int *nitems);
void switch_to_window (Window win);
GdkPixbuf *get_window_icon (Window win, guint req_width, guint req_height);
gboolean already_opened ();

#endif /* X_INTERACTION_H */
