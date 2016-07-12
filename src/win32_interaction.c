/* Copyright (c) 2012, Timur S. Sultanov
 * win32_interaction.c - layer to talk with WinAPI.
 */

#include "win32_interaction.h"
#include <winuser.h>
#include <winable.h>

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
if ((pwi.dwStyle & WS_VISIBLE) && !(pwi.dwStyle & WS_DISABLED) && (pwi.dwStyle & WS_SYSMENU))
{
    if(g_strcmp0(get_window_name(hwnd), "XWinMosaic")){
        window_list[(*((int*)num))++] = hwnd;
    }
  }
  return 1;
}

HWND* get_windows_list()
{
  int num = 0;
  EnumWindows(EnumWindowsProc, (LPARAM)&num);
  window_list[num] = NULL;
  return window_list;
}

gboolean already_opened()
{
  gboolean opened = FALSE;
  HWND *win_list = get_windows_list();
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
  return opened;
}

GdkPixbuf* get_window_icon(HWND win, guint req_width, guint req_height)
{
  GdkPixbuf* gicon;
  HICON icon = (HICON)SendMessage(win,WM_GETICON,(req_width > 16) ? ICON_BIG : ICON_SMALL,0);
  if(!icon)
  {
    icon = (HICON)SendMessage(win,WM_GETICON,ICON_SMALL,0);
  }
  if(!icon)
  {
    icon = (HICON)GetClassLongPtr(win, GCLP_HICON);
  }
  if(!icon)
  {
    icon = (HICON)GetClassLongPtr(win, GCLP_HICONSM);
  }
  gicon = gdk_win32_icon_to_pixbuf_libgtk_only(icon);
  if (gdk_pixbuf_get_width (gicon) > req_width)
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
#ifdef DEBUG
    g_printerr ("%s, %s, %x\n", get_window_name(*(pre_win_list + size)),
           get_window_class(*(pre_win_list + size)),
           pwi.dwStyle);
#endif
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
    SetForegroundWindow (win);
}

void tab_event (gboolean shift);

LRESULT CALLBACK alt_tab_hook (INT nCode, WPARAM wParam, LPARAM lParam)
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
  KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;
  BOOL isShiftPressed = FALSE;
  switch (nCode)
    {
    case HC_ACTION:
      {
        isShiftPressed = GetAsyncKeyState (VK_SHIFT) >> ((sizeof(SHORT) * 8) - 1);

        if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN) {
          if (!(pkbhs->flags & LLKHF_UP))
            tab_event(isShiftPressed);
          return 1;
        }
        break;
      }
      
    default:
      break;
    }
  return CallNextHookEx (NULL, nCode, wParam, lParam);
}

void install_alt_tab_hook ()
{
  if(!SetWindowsHookEx(WH_KEYBOARD_LL, alt_tab_hook, NULL, 0))
    g_printerr("Alt+Tab hook setup failed");
}
