/* Copyright (c) 2012, Timur S. Sultanov
 * win32_interaction.h - layer to talk with WinAPI.
 */

#include <windows.h>
#include <gdk/gdkwin32.h>
#include <gtk/gtk.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>

#ifndef WIN32_INTERACTION_H
#define WIN32_INTERACTION_H

#define Window HWND

char *get_window_name(HWND win);
char *get_window_class(HWND win);
HWND* sorted_windows_list(HWND *myown, HWND *active_win, int *nitems);
void switch_to_window(HWND win);
GdkPixbuf* get_window_icon(HWND win, guint req_width, guint req_height);

gboolean already_opened ();

HWND* get_windows_list();

#endif /* WIN32_INTERACTION_H */
