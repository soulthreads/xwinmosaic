/* Copyright (c) 2012, Timur S. Sultanov
 * win32_interaction.c - layer to talk with WinAPI.
 */

#include "win32_interaction.h"
#include <winuser.h>

WINUSERAPI VOID WINAPI SwitchToThisWindow(HWND,BOOL);


static HWND window_list[1024];

char* get_window_name(HWND win)
{
  TCHAR title[256];
  GetWindowText(win, title, 255);
  return g_locale_to_utf8(title, lstrlen(title), NULL, NULL, NULL);
}

char* get_window_class(HWND win)
{
  TCHAR class[256];
  RealGetWindowClass(win, class, 255);
  return g_locale_to_utf8(class, lstrlen(class), NULL, NULL, NULL);
}

BOOL CALLBACK EnumWindowsProc(
  HWND hwnd,
  LPARAM num
)
{
  WINDOWINFO pwi;
  pwi.cbSize = sizeof(WINDOWINFO);
  GetWindowInfo(hwnd, &pwi);

  if(((pwi.dwStyle)&WS_VISIBLE) && (pwi.dwStyle&WS_TILEDWINDOW) && (pwi.dwStyle&WS_TABSTOP)){
    if(g_strcmp0(get_window_name(hwnd), "XWinMosaic")){
      window_list[(*((int*)num))++] = hwnd;
    }
  }
  return 1;
}

HWND* get_windows_list()
{
  int num = 0;
  EnumWindows(EnumWindowsProc, (int)&num);
  window_list[num] = NULL;
  return window_list;
}

gboolean already_opened()
{
  gboolean opened = FALSE;
  HWND *win_list = get_windows_list();
  HWND *_win_list;
  _win_list = win_list;
  while(*win_list++)
  {
    gchar* wmclass = get_window_class(*win_list);
    if(wmclass && !g_strcmp0(wmclass, "xwinmosaic"))
    {
      opened = TRUE;
      break;
    }
    if(wmclass)
      g_free(wmclass);
  }
  free(_win_list);
  return opened;
}

GdkPixbuf* get_window_icon(HWND win, guint req_width, guint req_height)
{
  GdkPixbuf* gicon;
  HICON icon = (HICON)SendMessage(win,WM_GETICON,ICON_SMALL,0);
  if(!icon)
  {
    icon = (HICON)GetClassLongPtr(win, GCL_HICON);
  }
  if(!icon)
  {
    icon = (HICON)GetClassLongPtr(win, GCL_HICONSM);
  }
  if(!icon)
  {
    icon = (HICON)SendMessage(win,WM_GETICON,ICON_BIG,0);
  }
  gicon = gdk_win32_icon_to_pixbuf_libgtk_only(icon);
  gicon = gdk_pixbuf_scale_simple(gicon, req_width, req_height, GDK_INTERP_BILINEAR);
  return gicon;
}

HWND* sorted_windows_list(HWND *myown, HWND *active_win, int *nitems)
{
  WINDOWINFO pwi;
  HWND* pre_win_list = get_windows_list();
  int size = 0;
  while(*(pre_win_list + size)) {
    GetWindowInfo(*(pre_win_list + size), &pwi);
    printf("%s, %s, %x\n", get_window_name(*(pre_win_list + size)),
           get_window_class(*(pre_win_list + size)),
           pwi.dwStyle);
    size++;
  }
  *nitems = size;
  myown = pre_win_list;
  active_win = pre_win_list+1;
  return pre_win_list;
}

void switch_to_window(HWND win)
{
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(win, &wp);
  if((wp.showCmd == SW_MINIMIZE) || (wp.showCmd == SW_SHOWMINIMIZED) ||
     (wp.showCmd == SW_SHOWMINNOACTIVE))
    ShowWindow(win, SW_RESTORE);
  else
    SwitchToThisWindow(win, FALSE);
}
