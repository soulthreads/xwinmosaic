/* Copyright (c) 2012, Anton S. Lobashev
 * x_interaction.c - layer to talk with X11.
 */

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "x_interaction.h"

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

Atom a_NET_WM_STATE;
Atom a_NET_WM_STATE_SKIP_TASKBAR;

// Initialize Xatoms values.
void atoms_init ()
{
  Display *dpy = (Display *)gdk_x11_get_default_xdisplay ();

  a_UTF8_STRING = XInternAtom (dpy, "UTF8_STRING", 0);

  a_WM_CLASS = XInternAtom (dpy, "WM_CLASS", 0);
  a_WM_NAME = XInternAtom (dpy, "WM_NAME", 0);
  a_WM_WINDOW_ROLE = XInternAtom (dpy, "WM_WINDOW_ROLE", 0);

  a_NET_SUPPORTING_WM_CHECK = XInternAtom (dpy, "_NET_SUPPORTING_WM_CHECK", 0);
  a_NET_CLIENT_LIST = XInternAtom (dpy, "_NET_CLIENT_LIST", 0);
  a_NET_DESKTOP_VIEWPORT = XInternAtom (dpy, "_NET_DESKTOP_VIEWPORT", 0);
  a_NET_CURRENT_DESKTOP = XInternAtom (dpy, "_NET_CURRENT_DESKTOP", 0);
  a_NET_ACTIVE_WINDOW = XInternAtom (dpy, "_NET_ACTIVE_WINDOW", 0);
  a_NET_WORKAREA = XInternAtom (dpy, "_NET_WORKAREA", 0);

  a_NET_WM_DESKTOP = XInternAtom (dpy, "_NET_WM_DESKTOP", 0);
  a_NET_WM_NAME = XInternAtom (dpy, "_NET_WM_NAME", 0);
  a_NET_WM_VISIBLE_NAME = XInternAtom (dpy, "_NET_WM_VISIBLE_NAME", 0);
  a_NET_WM_ICON = XInternAtom (dpy, "_NET_WM_ICON", 0);
  a_NET_WM_USER_TIME = XInternAtom (dpy, "_NET_WM_USER_TIME", 0);


  a_NET_WM_WINDOW_TYPE = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", 0);
  a_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
  a_NET_WM_WINDOW_TYPE_DOCK = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
  a_NET_WM_WINDOW_TYPE_TOOLBAR = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", 0);
  a_NET_WM_WINDOW_TYPE_MENU = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", 0);
  a_NET_WM_WINDOW_TYPE_UTILITY = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
  a_NET_WM_WINDOW_TYPE_SPLASH = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
  a_NET_WM_WINDOW_TYPE_DIALOG = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
  a_NET_WM_WINDOW_TYPE_DROPDOWN_MENU = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", 0);
  a_NET_WM_WINDOW_TYPE_POPUP_MENU = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", 0);
  a_NET_WM_WINDOW_TYPE_TOOLTIP = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", 0);
  a_NET_WM_WINDOW_TYPE_NOTIFICATION = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", 0);
  a_NET_WM_WINDOW_TYPE_COMBO = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_COMBO", 0);
  a_NET_WM_WINDOW_TYPE_DND = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DND", 0);
  a_NET_WM_WINDOW_TYPE_NORMAL = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

  a_NET_WM_STATE = XInternAtom (dpy, "_NET_WM_STATE", 0);
  a_NET_WM_STATE_SKIP_TASKBAR = XInternAtom (dpy, "_NET_WM_STATE_SKIP_TASKBAR", 0);
}

// Get property for a window.
void* property (Window win, Atom prop, Atom type, int *nitems)
{
  Display *dpy = (Display *)gdk_x11_get_default_xdisplay ();
  Atom type_ret;
  int format_ret;
  unsigned long items_ret;
  unsigned long after_ret;
  unsigned char *prop_data = NULL;

  if (Success == XGetWindowProperty (dpy, win, prop, 0, 0xffffffff,
				    False, type, &type_ret, &format_ret,
				    &items_ret, &after_ret, &prop_data))
    if (nitems)
      *nitems = items_ret;

  return prop_data;
}

// Send a message to a window.
void climsg(Window win, long type, long l0, long l1, long l2, long l3, long l4)
{
    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = win;
    xev.message_type = type;
    xev.format = 32;
    xev.data.l[0] = l0;
    xev.data.l[1] = l1;
    xev.data.l[2] = l2;
    xev.data.l[3] = l3;
    xev.data.l[4] = l4;

    XSendEvent(gdk_x11_get_default_xdisplay (), gdk_x11_get_default_root_xwindow (), False,
          (SubstructureNotifyMask | SubstructureRedirectMask),
          (XEvent *)&xev);
}

// Pretty obvious function name, I think.
int wm_supports_ewmh ()
{
  int supports = 0;
  Window *wm = (Window *) property (gdk_x11_get_default_root_xwindow (),
				    a_NET_SUPPORTING_WM_CHECK,
				    XA_WINDOW, NULL);
  if (wm) {
    unsigned char *win_manager = property (*wm, a_NET_WM_NAME, a_UTF8_STRING, NULL);
    if (win_manager)
      supports = 1;
    XFree (win_manager);
  }
  XFree (wm);
  return supports;
}

char* get_window_name (Window win)
{
  int length = 0;
  char *net_wm_visible_name = (char *) property (win, a_NET_WM_VISIBLE_NAME, a_UTF8_STRING, &length);
  if (net_wm_visible_name && length)
    return net_wm_visible_name;
  XFree (net_wm_visible_name);

  char *net_wm_name = (char *) property (win, a_NET_WM_NAME, a_UTF8_STRING, &length);
  if (net_wm_name && length)
    return net_wm_name;
  XFree (net_wm_name);

 char *wm_name_utf8 = (char *) property (win, a_WM_NAME, a_UTF8_STRING, &length);
 if (wm_name_utf8 && length)
   return wm_name_utf8;
  XFree (wm_name_utf8);

  char *wm_name = (char *) property (win, a_WM_NAME, XA_STRING, &length);
  if (wm_name && length)
    return wm_name;
  XFree (wm_name);

  return g_strdup ("<empty>");
}

char* get_window_class (Window win)
{
  char *wm_class = (char *) property (win, a_WM_CLASS, XA_STRING, NULL);
  if (wm_class)
    return wm_class;

  return g_strdup ("<empty>");
}

// What desktop does window belong to.
int get_window_desktop (Window win)
{
  int32_t *desktop = property (win,
			   a_NET_WM_DESKTOP,
			   XA_CARDINAL, NULL);
  int32_t result = (desktop) ? *desktop : 0;
  XFree (desktop);
  return result;
}

// If window type is "normal" or "dialog" (or null) then show it.
static gboolean show_window (Window win)
{
  int num = 0;
  gboolean type_ok = TRUE;
  Atom *type = (Atom *) property (win, a_NET_WM_WINDOW_TYPE, XA_ATOM, &num);
  for (int i = 0; i < num; i++)
    if (type[i] == a_NET_WM_WINDOW_TYPE_DESKTOP ||
	type[i] == a_NET_WM_WINDOW_TYPE_DOCK ||
	type[i] == a_NET_WM_WINDOW_TYPE_TOOLBAR ||
	type[i] == a_NET_WM_WINDOW_TYPE_MENU ||
	type[i] == a_NET_WM_WINDOW_TYPE_UTILITY ||
	type[i] == a_NET_WM_WINDOW_TYPE_SPLASH ||
	type[i] == a_NET_WM_WINDOW_TYPE_DROPDOWN_MENU ||
	type[i] == a_NET_WM_WINDOW_TYPE_POPUP_MENU ||
	type[i] == a_NET_WM_WINDOW_TYPE_TOOLTIP ||
	type[i] == a_NET_WM_WINDOW_TYPE_NOTIFICATION ||
	type[i] == a_NET_WM_WINDOW_TYPE_COMBO ||
	type[i] == a_NET_WM_WINDOW_TYPE_DND ||
	get_window_desktop(win) == -1) {
      type_ok = FALSE;
      break;
    }
  num = 0;
  Atom *state = (Atom *) property (win, a_NET_WM_STATE, XA_ATOM, &num);
  for (int i = 0; i < num; i++)
    if (state[i] == a_NET_WM_STATE_SKIP_TASKBAR) {
      type_ok = FALSE;
      break;
    }

  return type_ok;
}

// Returns a list of windows (except panels and other "non-normal" windows)
Window* sorted_windows_list (Window *myown, Window *active_win, int *nitems)
{
  Window root_win = (Window)gdk_x11_get_default_root_xwindow ();
  int pre_size = 0;

  Window *pre_win_list = (Window *) property (root_win, a_NET_CLIENT_LIST, XA_WINDOW, &pre_size);
  if (pre_size) {
    int size = 0;
    // Do not show panels and all-desktop applications in list.
    for (int i = 0; i < pre_size; i++)
      if ((show_window (pre_win_list[i])) && (pre_win_list[i] != *myown))
	size++;

    Window *win_list = (Window *) malloc (size * sizeof (Window));
    // That's actually kinda stupid…
    int offset = 0;
    for (int i = 0; i < pre_size; i++) {
      if (!(show_window (pre_win_list [i])) || (pre_win_list[i] == *myown)) {
	offset++;
	continue;
      }
      win_list[i-offset] = pre_win_list [i];
    }
    XFree (pre_win_list);

    // active window may not update it's user time.
    int sort_from = 0;
    if (active_win != NULL)
      for (int i = 0; i < size; i++)
	if (win_list [i] == *active_win) {
	  win_list [i] = win_list [0];
	  win_list [0] = *active_win;
	  sort_from = 1;
	  break;
	}

    if (sort_from == 0) // Active window does not exist anymore.
      active_win = win_list;

    unsigned int *time_list = (unsigned int *)malloc (size * sizeof (unsigned int));
    for (int i = 0; i < size; i++) {
      unsigned int *time = (unsigned int *) property (win_list [i], a_NET_WM_USER_TIME, XA_CARDINAL, NULL);
      if (time)
	time_list [i] = *time;
      else
	time_list [i] = 0;
      XFree (time);
    }

    // dumb sorting
    for (int i = sort_from; i < size; i++)
      for (int j = i+1; j < size; j++)
	if (time_list[i] < time_list [j]) {
	  unsigned int time_tmp = time_list[j];
	  time_list [j] = time_list [i];
	  time_list [i] = time_tmp;

	  Window win_tmp = win_list [j];
	  win_list [j] = win_list [i];
	  win_list [i] = win_tmp;
	}

    free (time_list);

    *nitems = size;
    return win_list;
  }

  *nitems = 0;
  return NULL;
}

// Switch to window and it's desktop.
void switch_to_window (Window win)
{
  Window root_window = gdk_x11_get_default_root_xwindow ();
  int32_t desktop = get_window_desktop (win);
  climsg (win, a_NET_ACTIVE_WINDOW, 2, CurrentTime, 0, 0, 0);
  if (desktop > -1) {
    climsg (root_window, a_NET_CURRENT_DESKTOP, desktop, CurrentTime, 0, 0, 0);
    climsg (win, a_NET_ACTIVE_WINDOW, 2, CurrentTime, 0, 0, 0);
  }
}

GdkPixbuf *get_window_icon (Window win, guint req_width, guint req_height)
{
  GdkPixbuf *pixmap = NULL;

  /* get the _NET_WM_ICON property */
  gint nitems = 0;
  gulong *data = (gulong *) property (win, a_NET_WM_ICON, XA_CARDINAL, &nitems);
  if (data) {
    gulong *pdata = data;
    gulong *pdata_end = data + nitems;
    gulong *bicon = NULL;
    gulong bwidth = 0;
    gulong bheight = 0;

    while ((pdata+2) < pdata_end) {
      gulong w = pdata [0];
      gulong h = pdata [1];
      gulong size = w * h;
      pdata += 2;

      if (pdata + size > pdata_end)
	break;

      bicon = pdata;
      bwidth = w;
      bheight = h;

      if ((w >= req_width) && (h >= req_height))
	break;

      pdata += size;
    }

    if (bicon != NULL) {
      gulong len = bwidth * bheight;
      guchar *pixdata = g_new (guchar, len * 4);

      guchar *p = pixdata;
      for (int i = 0; i < len; p += 4, i++) {
	guint argb = bicon [i];
	guint rgba = (argb << 8) | (argb >> 24);
	p [0] = (rgba >> 24) & 0xff;
	p [1] = (rgba >> 16) & 0xff;
	p [2] = (rgba >>  8) & 0xff;
	p [3] = (rgba >>  0) & 0xff;
      }

      GdkPixbuf *pre_pixmap = gdk_pixbuf_new_from_data
	(pixdata,
	 GDK_COLORSPACE_RGB,
	 TRUE, 8,
	 bwidth, bheight, bwidth * 4,
	 (GdkPixbufDestroyNotify) g_free,
	  NULL);

      if (bwidth > req_width || bheight > req_height) {
	pixmap = gdk_pixbuf_scale_simple (pre_pixmap,
					  req_width,
					  req_height,
					  GDK_INTERP_BILINEAR);
	g_object_unref (pre_pixmap);
      } else {
	pixmap = pre_pixmap;
      }
    }
  }
  XFree (data);

  return pixmap;
}

// If xwinmosaic is already opened, exit.
gboolean already_opened ()
{
  int size = 0;
  gboolean opened = FALSE;
  Window *win_list = (Window *) property (gdk_x11_get_default_root_xwindow (), a_NET_CLIENT_LIST, XA_WINDOW, &size);
  if (size) {
    for (int i = 0; i < size; i++) {
      gchar *wmclass = get_window_class (win_list [i]);
      if (wmclass && show_window (win_list[i]) && !g_strcmp0 (wmclass, "xwinmosaic")) {
	opened = TRUE;
	break;
      }
      if (wmclass)
	g_free (wmclass);
    }
  }
  XFree (win_list);
  return opened;
}
