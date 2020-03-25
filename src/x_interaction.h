/* Copyright (c) 2012, Anton S. Lobashev
 * x_interaction.h - layer to talk with X11.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>

#ifndef X_INTERACTION_H
#define X_INTERACTION_H

extern Atom a_UTF8_STRING;

extern Atom a_WM_CLASS;
extern Atom a_WM_NAME;
extern Atom a_WM_WINDOW_ROLE;

extern Atom a_NET_SUPPORTING_WM_CHECK;
extern Atom a_NET_CLIENT_LIST;
extern Atom a_NET_DESKTOP_VIEWPORT;
extern Atom a_NET_CURRENT_DESKTOP;
extern Atom a_NET_ACTIVE_WINDOW;
extern Atom a_NET_WORKAREA;

extern Atom a_NET_WM_DESKTOP;
extern Atom a_NET_WM_NAME;
extern Atom a_NET_WM_VISIBLE_NAME;
extern Atom a_NET_WM_ICON;
extern Atom a_NET_WM_USER_TIME;

extern Atom a_NET_WM_WINDOW_TYPE;

extern Atom a_NET_WM_WINDOW_TYPE_DESKTOP;
extern Atom a_NET_WM_WINDOW_TYPE_DOCK;
extern Atom a_NET_WM_WINDOW_TYPE_TOOLBAR;
extern Atom a_NET_WM_WINDOW_TYPE_MENU;
extern Atom a_NET_WM_WINDOW_TYPE_UTILITY;
extern Atom a_NET_WM_WINDOW_TYPE_SPLASH;
extern Atom a_NET_WM_WINDOW_TYPE_DIALOG;
extern Atom a_NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
extern Atom a_NET_WM_WINDOW_TYPE_POPUP_MENU;
extern Atom a_NET_WM_WINDOW_TYPE_TOOLTIP;
extern Atom a_NET_WM_WINDOW_TYPE_NOTIFICATION;
extern Atom a_NET_WM_WINDOW_TYPE_COMBO;
extern Atom a_NET_WM_WINDOW_TYPE_DND;
extern Atom a_NET_WM_WINDOW_TYPE_NORMAL;

extern Atom a_NET_WM_STATE;
extern Atom a_NET_WM_STATE_SKIP_TASKBAR;


void atoms_init ();
void* property (Window win, Atom prop, Atom type, int *nitems);
void climsg(Window win, long type, long l0, long l1, long l2, long l3, long l4);
int wm_supports_ewmh ();
char* get_window_name (Window win);
char* get_window_class (Window win);
int get_window_desktop (Window win);
Window* sorted_windows_list (Window *myown, Window *active_win, int *nitems);
void switch_to_window (Window win);
GdkPixbuf *get_window_icon (Window win, guint req_width, guint req_height);
gboolean already_opened ();

#endif /* X_INTERACTION_H */
