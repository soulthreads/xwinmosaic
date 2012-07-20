#include "win32_interaction.h"

static HWND* window_list;

char* get_window_name(HWND win)
{
  TCHAR* title;
  title = malloc(256*sizeof(TCHAR));
  GetWindowText(win, title, 255);
//  _tprintf(TEXT("%s\n"), title);
  if(sizeof(TCHAR) == sizeof(wchar_t))
  {
    return g_utf16_to_utf8((gunichar2*)title, lstrlen(title), NULL, NULL, NULL);
  } else
  {
    return title;
  }
}

char* get_window_class(HWND win)
{
  TCHAR* class;
  class = malloc(256*sizeof(TCHAR));
  RealGetWindowClass(win, class, 255);
  if(sizeof(TCHAR) == sizeof(wchar_t))
  {
    return g_utf16_to_utf8((gunichar2*)class, lstrlen(class), NULL, NULL, NULL);
  } else {
    return class;
  }
}

BOOL CALLBACK EnumWindowsProc(
  HWND hwnd,
  LPARAM list
)
{
  WINDOWINFO pwi;
  pwi.cbSize = sizeof(WINDOWINFO);
  GetWindowInfo(hwnd, &pwi);

  if(((pwi.dwStyle)&WS_VISIBLE) && !(pwi.dwStyle&WS_POPUP)){
    *window_list = hwnd;
    window_list++;
  }
  return 1;
}

HWND* get_windows_list()
{
  HWND *list;
  window_list = malloc(sizeof(HWND)*1024);
  list = window_list;
  memset(window_list, 0, sizeof(HWND)*1024);
  EnumWindows(EnumWindowsProc, 0);
  return list;
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
  return NULL;
}

void switch_wo_window(HWND win)
{
  
}

HWND* sorted_windows_list(HWND *myown, HWND *active_win, int *nitems)
{
  HWND* pre_win_list = get_windows_list();
  int size = 0;
  while(1) {
    if(*(pre_win_list + size))
    {
//      printf("%s\n", get_window_name(*(pre_win_list + size)));
      size++;
    }
    else
      break;
  }
  *nitems = size;
  myown = pre_win_list;
  active_win = pre_win_list+1;
  printf("%d windows\n", size);
  /* Looks like WinAPI returns already sorted by last access time
     window list */
  return pre_win_list;
}

void switch_to_window(HWND win)
{
  printf("%s\n", get_window_name(win));
}
