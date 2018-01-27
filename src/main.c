/* Copyright (c) 2012, Anton S. Lobashev
 * main.c - main program file
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef X11
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include "x_interaction.h"
#endif

#ifdef WIN32
#include <gdk/gdkwin32.h>
#include "win32_interaction.h"
#endif

#include "mosaic_window_box.h"
#include "mosaic_search_box.h"

static GtkWidget *window;
static Window myown_window;
static Window *active_window; // For displaying it first in windows list.
static Window *wins;
static gchar **in_items; // If we read from stdin.
static int wsize = 0;
static GtkWidget **boxes;
static GtkWidget *layout;
static GtkWidget *search;
static GtkWidget **filtered_boxes;
static int filtered_size;
static int width, height;
static GdkDrawable *window_shape_bitmap;

/* for window mask */
typedef struct {
  gint x, y, width, height;
} rect;

static rect *box_rects;
static guint boxes_drawn;

/* for screenshot mode */
static gboolean key_pressed;

static GKeyFile *color_config;
static gchar **fallback_colors;
static gsize fallback_size;

static struct {
  guint box_width;
  guint box_height;
  gboolean colorize;
  gboolean show_icons;
  gboolean show_desktop;
  gboolean show_titles;
  guint icon_size;
  gchar *font;
  gboolean read_stdin;
  gboolean permissive;
  gboolean persistent;
  gboolean format;
  gboolean screenshot;
  guchar color_offset;
  gint screenshot_offset_x;
  gint screenshot_offset_y;
  gboolean vim_mode;
  gboolean at_pointer;
  gint center_x;
  gint center_y;
  gchar *color_file;
  gint selected;
} options;

typedef struct {
  gint desktop;
  gchar *iconpath;
  gchar *color;
  gchar *label;
  gchar *opt_name;
} Entry;

static GOptionEntry entries [] =
{
  { "persistent", 'R', 0, G_OPTION_ARG_NONE, &options.persistent,
    "Make XWinMosaic acts like Alt-Tab switcher", NULL },
  { "read-stdin", 'r', 0, G_OPTION_ARG_NONE, &options.read_stdin,
    "Read items from stdin (and print selected item to stdout)", NULL },
  { "permissive", 'p', 0, G_OPTION_ARG_NONE, &options.permissive,
    "Lets search entry text to be used as individual item.", NULL},
  { "format", 't', 0, G_OPTION_ARG_NONE, &options.format,
    "Read items from stdin in next format: <desktop_num>, <box_color>, <icon>, <label>, <opt-name>.", NULL},
  { "vim-mode", 'V', 0, G_OPTION_ARG_NONE, &options.vim_mode,
    "Turn on vim-like navigation (hjkl, search on /)", NULL },
  { "no-colors", 'C', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.colorize,
    "Turn off box colorizing", NULL },
  { "no-icons", 'I', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.show_icons,
    "Turn off showing icons", NULL },
  { "no-desktops", 'D', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.show_desktop,
    "Turn off showing desktop number", NULL },
    { "no-titles", 'T', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.show_titles,
    "Turn off showing titles", NULL },
  { "screenshot", 'S', 0, G_OPTION_ARG_NONE, &options.screenshot,
    "Get screenshot and set it as a background (for WMs that do not support XShape)", NULL },
  { "at-pointer", 'P', 0, G_OPTION_ARG_NONE, &options.at_pointer,
    "Place center of mosaic at pointer position.", NULL },
  { "box-width", 'W', 0, G_OPTION_ARG_INT, &options.box_width,
    "Width of the boxes (default: 200)", "<int>" },
  { "selected", 's', 0, G_OPTION_ARG_INT, &options.selected,
    "Initially selected box", "<int>" },
  { "box-height", 'H', 0, G_OPTION_ARG_INT, &options.box_height,
    "Height of the boxes (default: 40)", "<int>" },
  { "icon-size", 'i', 0, G_OPTION_ARG_INT, &options.icon_size,
    "Size of window icons (default: 16)", "<int>" },
  { "font", 'f', 0, G_OPTION_ARG_STRING, &options.font,
    "Which font to use for displaying widgets. (default: \"Sans 10\")", "\"font [size]\"" },
  { "hue-offset", 'o', 0, G_OPTION_ARG_INT, &options.color_offset,
    "Set color hue offset (from 0 to 255)", "<int>" },
  { "color-file", 'F', 0, G_OPTION_ARG_FILENAME, &options.color_file,
    "Pick colors from file", "<file>" },
  { NULL }
};

static GdkRectangle current_monitor_size ();
static void draw_mosaic (GtkLayout *where,
		  GtkWidget **widgets, int rsize,
		  int focus_on,
		  int rwidth, int rheight);
static void on_rect_click (GtkWidget *widget, gpointer data);
static void update_box_list ();
static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data);
#ifdef X11
static GdkFilterReturn event_filter (XEvent *xevent, GdkEvent *event, gpointer data);
#endif
static void refilter (MosaicSearchBox *search_box, gpointer data);
static void draw_mask (GdkDrawable *bitmap, guint size);
static void read_stdin ();
static GdkPixbuf* get_screenshot ();
static void read_config ();
static void write_default_config ();
static void on_focus_change (GtkWidget *widget, GdkEventFocus *event, gpointer data);
static void read_colors ();
static gboolean parse_format (Entry *entry, gchar *data);
void tab_event (gboolean shift);

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  read_config ();

  // Read options from command-line arguments.
  GError *error = NULL;
  GOptionContext *context;
  context = g_option_context_new (" - show X11 windows as colour mosaic");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("option parsing failed: %s\n", error->message);
    exit (1);
  }
  g_option_context_free (context);

  if(options.format && !options.read_stdin) {
    g_printerr("You must provide option --read-stdin!");
    exit(1);
  }

#ifdef X11
  atoms_init ();
#endif

  if (already_opened ()) {
    g_printerr ("Another instance of xwinmosaic is opened.\n");
    exit (1);
  }

  if (options.read_stdin) {
    if(!options.format) {
      options.show_icons = FALSE;
      options.show_desktop = FALSE;
    }
    read_stdin ();
  } else {
#ifdef X11
    // Checks whether WM supports EWMH specifications.
    if (!wm_supports_ewmh ()) {
      GtkWidget *dialog = gtk_message_dialog_new
	(NULL,
	 GTK_DIALOG_MODAL,
	 GTK_MESSAGE_ERROR,
	 GTK_BUTTONS_CLOSE,
	 "Error: your WM does not support EWMH specifications.");

      gtk_dialog_run (GTK_DIALOG (dialog));
      g_signal_connect_swapped (dialog, "response",
				G_CALLBACK (gtk_main_quit), NULL);
      return 1;
    }

    active_window = (Window *) property (gdk_x11_get_default_root_xwindow (),
					 a_NET_ACTIVE_WINDOW,
					 XA_WINDOW,
					 NULL);
#endif
  }

  if (options.color_file)
    read_colors ();

#ifdef WIN32
  if (options.persistent) {
#ifdef DEBUG
      g_printerr ("Installing Alt-Tab hook");
#endif
      install_alt_tab_hook();
  }
#endif

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "XWinMosaic");

  GdkRectangle rect = current_monitor_size ();
  width = rect.width;
  height = rect.height;

  if (options.at_pointer) {
    gdk_display_get_pointer (gdk_display_get_default (), NULL, &options.center_x, &options.center_y, NULL);

    gint monitors = gdk_screen_get_n_monitors (gdk_screen_get_default ());
    if (monitors > 1) {
      guint xm = 0, ym = 0;
      gint current_monitor = gdk_screen_get_monitor_at_point (gdk_screen_get_default (),
							      options.center_x, options.center_y);
      for (int i = 0; i < current_monitor; i++) {
	GdkRectangle mon_rect;
	gdk_screen_get_monitor_geometry (gdk_screen_get_default (), i, &mon_rect);
	xm += mon_rect.width;
	ym += mon_rect.height;
      }
      if (xm && ym) {
	options.center_x %= xm;
	options.center_y %= ym;
      }
    }

    if (options.center_x < options.box_width/2)
      options.center_x = options.box_width/2 + 1;
    else if (options.center_x > width - options.box_width/2)
      options.center_x = width - options.box_width/2 - 1;
    if (options.center_y < options.box_height/2)
      options.center_y = options.box_height/2 + 1;
    else if (options.center_y > height - options.box_height/2)
      options.center_y = height - options.box_height/2 - 1;
  } else {
    options.center_x = width/2;
    options.center_y = height/2;
  }

  gtk_window_set_default_size (GTK_WINDOW (window), width, height);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_decorated (GTK_WINDOW (window), 0);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), 1);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), 1);
/**/
  gtk_widget_add_events (GTK_WIDGET (window), GDK_FOCUS_CHANGE);
  g_signal_connect (G_OBJECT (window), "focus-out-event",
        	    G_CALLBACK (on_focus_change), NULL);
/**/
  layout = gtk_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), layout);

  if (options.screenshot) {
    gtk_window_fullscreen (GTK_WINDOW (window));

    GdkPixbuf *screenshot;
    GdkPixmap *background = NULL;
    GtkStyle *style = NULL;
    screenshot = get_screenshot ();

    gdk_pixbuf_render_pixmap_and_mask (screenshot, &background, NULL, 0);
    style = gtk_style_new ();
    style->bg_pixmap [0] = background;

    gtk_widget_set_style (window, style);
    gtk_widget_set_style (layout, style);
  }

  search = mosaic_search_box_new ();
  mosaic_box_set_font (MOSAIC_BOX (search), options.font);
  gtk_widget_set_can_focus (search, FALSE);
  GtkRequisition s_req;
  gtk_widget_size_request (search, &s_req);
  gtk_layout_put (GTK_LAYOUT (layout), search,
		  (width - s_req.width)/2, height - s_req.height - options.box_height);
  g_signal_connect (G_OBJECT (search), "changed",
		    G_CALLBACK (refilter), NULL);

  g_signal_connect (G_OBJECT (window), "key-press-event",
		    G_CALLBACK (on_key_press), NULL);
  g_signal_connect_swapped(G_OBJECT (window), "destroy",
        		   G_CALLBACK(gtk_main_quit), NULL);

  if (!options.screenshot) {
    window_shape_bitmap = (GdkDrawable *) gdk_pixmap_new (NULL, width, height, 1);
    draw_mask (window_shape_bitmap, 0);
    gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);
  }

  gtk_widget_show_all (window);
  gtk_widget_hide (search);
  gtk_window_present (GTK_WINDOW (window));
  gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
  
  if (options.persistent)
    gtk_widget_hide (window);

  GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));
#ifdef X11
  myown_window = GDK_WINDOW_XID (gdk_window);

  if (!options.read_stdin) {
    // Get PropertyNotify events from root window.
    XSelectInput (gdk_x11_get_default_xdisplay (),
		  gdk_x11_get_default_root_xwindow (),
		  PropertyChangeMask);
    gdk_window_add_filter (NULL, (GdkFilterFunc) event_filter, NULL);
  }
#endif
#ifdef WIN32
  myown_window = GDK_WINDOW_HWND (gdk_window);
#endif
  update_box_list ();

  draw_mosaic (GTK_LAYOUT (layout), boxes, wsize,
               options.selected >= wsize ? 0 : options.selected,
	       options.box_width, options.box_height);

#ifdef X11
  // Window will be shown on all desktops (and so hidden in windows list)
  unsigned int desk = 0xFFFFFFFF; // -1
  XChangeProperty(gdk_x11_get_default_xdisplay (), myown_window, a_NET_WM_DESKTOP, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char *)&desk, 1);
#endif

  gtk_main ();

#ifdef X11
  if (!options.read_stdin)
    XFree (wins);
#endif

  return 0;
}

static GdkRectangle current_monitor_size ()
{
  // Where is the pointer now?
  gint x, y;
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
  gint monitor = gdk_screen_get_monitor_at_point (gdk_screen_get_default (), x, y);
  GdkRectangle rect;
  gdk_screen_get_monitor_geometry (gdk_screen_get_default (), monitor, &rect);

  return rect;
}

static void draw_mosaic (GtkLayout *where,
		  GtkWidget **widgets, int rsize,
		  int focus_on,
		  int rwidth, int rheight)
{
  boxes_drawn = 0;
  int cur_x = options.center_x - rwidth/2;
  int cur_y = options.center_y - rheight/2;
  if (rsize) {
    int i = 0;
    int offset = 0;
    int max_offset = (width*2) / rwidth + (height*2) / rheight;
    int side = 0;

    while (i < rsize) {
      int j = 0;
      do {
	if (i == rsize)
	  break;
	if (cur_x >= 0 && cur_x+rwidth <= width && cur_y >= 0 && cur_y+rheight <= height) {
	  offset = 0;
	  if (gtk_widget_get_parent (widgets[i]))
	    gtk_layout_move (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	  else
	    gtk_layout_put (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	  gtk_widget_set_size_request (widgets[i], rwidth, rheight);
	  gtk_widget_show (widgets[i]);
	  if (!options.screenshot) {
	    box_rects[i].x = cur_x;
	    box_rects[i].y = cur_y;
	    boxes_drawn++;
	  }
	  i++;
	} else {
	  offset++;
	}
	if (side) {
	  if (j % (side * 4) < side || j % (side * 4) >= side * 3)
	    cur_x += rwidth;
	  else
	    cur_x -= rwidth;
	  if (j % (side * 4) < side * 2)
	    cur_y += rheight;
	  else
	    cur_y -= rheight;
	}
	j++;
      } while (j < side * 4);
      if (offset >= max_offset)
	break;
      side++;
      cur_x = options.center_x - rwidth/2;
      cur_y -= rheight;
    }
    if (focus_on >= rsize)
      // If some window was killed and focus was on the last element
      focus_on = rsize-1;
    gtk_widget_grab_focus (widgets[focus_on]);
  }
  if (!options.screenshot) {
    draw_mask (window_shape_bitmap, rsize);
    gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);
  }
}

static void on_rect_click (GtkWidget *widget, gpointer data)
{
  MosaicWindowBox *box = MOSAIC_WINDOW_BOX (widget);

  if (!options.read_stdin) {
    gtk_widget_hide (window);
    switch_to_window (mosaic_window_box_get_xwindow (box));
  } else {
    puts (mosaic_window_box_get_name (box));
  }

  if (options.persistent) {
    if (strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)))) {
      gtk_widget_hide (search);
      mosaic_search_box_set_text (MOSAIC_SEARCH_BOX (search), "\0");
    }
  } else {
    gtk_main_quit ();
  }
}

static void update_box_list ()
{
  if (!options.read_stdin) {
    if (wsize) {
      for (int i = 0; i < wsize; i++) {
	gtk_widget_destroy (boxes[i]);
      }
      free (boxes);
#ifdef X11
      XFree (wins);
#endif
    }
    wins = sorted_windows_list (&myown_window, active_window, &wsize);
#ifdef X11
    if (wins) {
      // Get PropertyNotify events from each relevant window.
      for (int i = 0; i < wsize; i++) {
	XSelectInput (gdk_x11_get_default_xdisplay (),
		      wins[i],
		      PropertyChangeMask);
      }
    }
#endif
  }

  if (!options.screenshot && box_rects) {
    free (box_rects);
    box_rects = NULL;
  }

  if (wsize) {
    boxes = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));

    if (!options.screenshot) {
      box_rects = (rect *) calloc (wsize, sizeof (rect));
      for (int i = 0; i < wsize; i++) {
	box_rects[i].width = options.box_width;
	box_rects[i].height = options.box_height;
      }
    }

    GError *col_error;
    Entry entry;
    for (int i = 0; i < wsize; i++) {
      if (!options.read_stdin) {
	boxes[i] = mosaic_window_box_new_with_xwindow (wins[i]);
#ifdef X11
	mosaic_window_box_set_show_desktop (MOSAIC_WINDOW_BOX (boxes[i]), options.show_desktop);
#endif
	mosaic_window_box_set_show_titles (MOSAIC_WINDOW_BOX (boxes[i]), options.show_titles);
	if (options.show_icons)
	  mosaic_window_box_setup_icon_from_wm (MOSAIC_WINDOW_BOX(boxes[i]), options.icon_size, options.icon_size);
      } else {
        if(!options.format)
          boxes[i] = mosaic_window_box_new_with_name (in_items[i]);
        else {
          if(parse_format(&entry, in_items[i])){
            boxes[i] = mosaic_window_box_new_with_name(entry.label);
            if((entry.desktop)>=0) {//g_printerr("Custom background digits not implemented yet\n");
              mosaic_window_box_set_desktop(MOSAIC_WINDOW_BOX(boxes[i]), entry.desktop-1);
              mosaic_window_box_set_show_desktop (MOSAIC_WINDOW_BOX(boxes[i]), TRUE);
            }
            if(options.show_icons && (entry.iconpath)[0]!='*') {
              if(strchr(entry.iconpath, '.')) {
                mosaic_window_box_setup_icon_from_file(MOSAIC_WINDOW_BOX(boxes[i]), entry.iconpath,
                                                         options.icon_size, options.icon_size);
              } else {
                mosaic_window_box_setup_icon_from_theme(MOSAIC_WINDOW_BOX(boxes[i]), entry.iconpath,
                                                        options.icon_size, options.icon_size);
              }
            }
            if(strlen(entry.opt_name)){
              g_printerr("%s\n", entry.opt_name);
              mosaic_window_box_set_opt_name(MOSAIC_WINDOW_BOX(boxes[i]), entry.opt_name);
            }
          } else {
            boxes[i] = mosaic_window_box_new_with_name("Parse error");
          }
        }
      }
      mosaic_box_set_font (MOSAIC_BOX (boxes [i]), options.font);
      mosaic_window_box_set_colorize (MOSAIC_WINDOW_BOX (boxes[i]), options.colorize);
      mosaic_window_box_set_color_offset (MOSAIC_WINDOW_BOX (boxes[i]), options.color_offset);
      if (options.colorize && options.color_file) {
	gchar *color = NULL;
	if (!options.read_stdin) {
	  const gchar *wm_class = mosaic_window_box_get_opt_name (MOSAIC_WINDOW_BOX (boxes[i]));
	  gchar *class1 = g_strdup (wm_class);
	  gchar *class2 = g_strdup (wm_class+strlen (class1)+1);
	  if (g_key_file_has_key (color_config, "colors", class1, &col_error))
	    color = g_key_file_get_string (color_config, "colors", class1, &col_error);
	  else if (g_key_file_has_key (color_config, "colors", class2, &col_error))
	    color = g_key_file_get_string (color_config, "colors", class2, &col_error);
	  g_free (class1);
	  g_free (class2);
	}

	if (!color && fallback_size)
	  color = g_strdup (fallback_colors [i % fallback_size]);

	if (color)
	  mosaic_window_box_set_color_from_string (MOSAIC_WINDOW_BOX (boxes[i]), color);

	g_free (color);
      }
      if(options.format) {
        if((entry.color)[0]=='#')
          mosaic_window_box_set_color_from_string(MOSAIC_WINDOW_BOX(boxes[i]), entry.color);
      }
      g_signal_connect (G_OBJECT (boxes[i]), "clicked",
			G_CALLBACK (on_rect_click), NULL);
    }
  }
}

static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  key_pressed = TRUE;
  switch (event->keyval) {
  case GDK_Escape:
    if (strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)))
	|| gtk_widget_get_visible (search)) {
      gtk_widget_hide (search);
      mosaic_search_box_set_text (MOSAIC_SEARCH_BOX (search), "\0");
    } else {
      if (options.persistent)
        gtk_widget_hide (window);
      else
        gtk_main_quit ();
    }
    break;
  case GDK_Return:
    if(strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))) && !filtered_size &&
       options.read_stdin && options.permissive) {
      puts (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)));
      gtk_main_quit();
    }
    break;
  case GDK_Left:
  case GDK_Up:
  case GDK_Right:
  case GDK_Down:
    break;
  case GDK_Tab:
    tab_event(FALSE);
    return TRUE;
  case GDK_ISO_Left_Tab:
    tab_event(TRUE);
    return TRUE;
    break;
  case GDK_End:
    if (options.permissive) {
      MosaicWindowBox* box = MOSAIC_WINDOW_BOX (gtk_window_get_focus (GTK_WINDOW (window)));
      mosaic_search_box_set_text (MOSAIC_SEARCH_BOX (search), mosaic_window_box_get_name (box));
      gtk_widget_show (search);
    }
    break;
  case GDK_BackSpace:
    mosaic_search_box_remove_symbols (MOSAIC_SEARCH_BOX (search), 1);
    if (!strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))))
      if (!options.vim_mode)
	gtk_widget_hide (search);
    break;
  default:
  {
    // Ignore Ctrl key.
    if (event->state & GDK_CONTROL_MASK) {
      if (!options.vim_mode) {
	switch (event->keyval) {
	case GDK_n:
	  gtk_widget_child_focus (layout, GTK_DIR_DOWN);
	  break;
	case GDK_p:
	  gtk_widget_child_focus (layout, GTK_DIR_UP);
	  break;
	case GDK_f:
	  gtk_widget_child_focus (layout, GTK_DIR_RIGHT);
	  break;
	case GDK_b:
	  gtk_widget_child_focus (layout, GTK_DIR_LEFT);
	  break;
	case GDK_m:
	  if(strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))) && !filtered_size &&
	     options.read_stdin && options.permissive) {
	    puts (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)));
	    gtk_main_quit();
	  } else {
	    g_signal_emit_by_name (gtk_window_get_focus (GTK_WINDOW (window)), "clicked", NULL);
	  }
	  break;
	case GDK_h:
	  mosaic_search_box_remove_symbols (MOSAIC_SEARCH_BOX (search), 1);
	  if (!strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))))
	    gtk_widget_hide (search);
	  break;
	case GDK_w:
	  mosaic_search_box_kill_word (MOSAIC_SEARCH_BOX (search));
	  if (!strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))))
	    gtk_widget_hide (search);
	  break;
	case GDK_g:
	  if (strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)))
	      || gtk_widget_get_visible (search)) {
	    gtk_widget_hide (search);
	    mosaic_search_box_set_text (MOSAIC_SEARCH_BOX (search), "\0");
	  } else {
	    gtk_main_quit ();
	  }
	  break;
	}
      }
      return FALSE;
    }
    
    /* if(event->state & GDK_SHIFT_MASK) { */
    /*   switch(event->keyval) { */
    /*   case GDK_Tab: */
    /*     { */
    /*       g_printerr("Shift-Tab\n"); */
    /*       tab_event(TRUE); */
    /*       return TRUE; */
    /*     } */
    /*   default: */
    /*     g_printerr("keyval = %d\n", event->keyval); */
    /*   } */
    /* } */
    
    if (options.vim_mode && !gtk_widget_get_visible (search)) {
      switch (event->keyval) {
      case GDK_h:
	gtk_widget_child_focus (layout, GTK_DIR_LEFT);
	break;
      case GDK_j:
	gtk_widget_child_focus (layout, GTK_DIR_DOWN);
	break;
      case GDK_k:
	gtk_widget_child_focus (layout, GTK_DIR_UP);
	break;
      case GDK_l:
	gtk_widget_child_focus (layout, GTK_DIR_RIGHT);
	break;
      case GDK_slash:
	gtk_widget_show (search);
	g_signal_emit_by_name (G_OBJECT (search), "changed", NULL);
	break;
      }
      return TRUE;
    }

    char *key = event->string;
    if (strlen(key)) {
      mosaic_search_box_append_text (MOSAIC_SEARCH_BOX (search), key);
      int text_length = strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)));
      if (text_length)
	gtk_widget_show (search);
      return TRUE;
    }
  }
  }
  return FALSE;
}

#ifdef X11
static GdkFilterReturn event_filter (XEvent *xevent, GdkEvent *event, gpointer data)
{
  if (xevent->type == PropertyNotify) {
    Atom atom = xevent->xproperty.atom;
    Window win = xevent->xproperty.window;
    if (win == gdk_x11_get_default_root_xwindow ()) {
      if (atom == a_NET_CLIENT_LIST) {
	int focus_on = 0;
	if (filtered_size && filtered_boxes) {
	  for (int i = 0; i < filtered_size; i++)
	    if (gtk_widget_is_focus (filtered_boxes [i])) {
	      focus_on = i;
	      break;
	    }
	} else {
	  for (int i = 0; i < wsize; i++)
	    if (gtk_widget_is_focus (boxes [i])) {
	      focus_on = i;
	      break;
	    }
	}
	update_box_list ();
	if (strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search)))) {
	  refilter (MOSAIC_SEARCH_BOX (search), NULL);
	  draw_mosaic (GTK_LAYOUT (layout), filtered_boxes, filtered_size, focus_on,
		       options.box_width, options.box_height);
	} else {
	  draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, focus_on,
		       options.box_width, options.box_height);
	}
      }
    } else {
      if (atom == a_WM_NAME || atom == a_NET_WM_NAME || atom == a_NET_WM_VISIBLE_NAME) {
	// Search for appropriate widget to change label.
	for (int i = 0; i < wsize; i++)
	  if (wins [i] == win) {
	    mosaic_window_box_update_xwindow_name (MOSAIC_WINDOW_BOX (boxes[i]));
	    break;
	  }
      }
      if (atom == a_NET_WM_ICON && options.show_icons) {
	// Search for appropriate widget to update icon.
	for (int i = 0; i < wsize; i++)
	  if (wins [i] == win) {
	    mosaic_window_box_setup_icon_from_wm (MOSAIC_WINDOW_BOX (boxes[i]), options.icon_size, options.icon_size);
	    break;
	  }
      }
    }
  }

  return GDK_FILTER_CONTINUE;
}
#endif

static gboolean search_by_letters (const gchar *source, gint s_len, const gchar *letters, gint l_len)
{
  gboolean found = FALSE;
  const gchar *p1 = letters;
  const gchar *p2 = source;
  while (p1 < letters + l_len) {
    gunichar c1 = g_utf8_get_char (p1);
    found = FALSE;
    while (p2 < source + s_len) {
      gunichar c2 = g_utf8_get_char (p2);
      if (c1 == c2) {
	found = TRUE;
	p2 = g_utf8_find_next_char (p2, NULL);
	break;
      }
      p2 = g_utf8_find_next_char (p2, NULL);
    }
    if (!found)
      break;
    p1 = g_utf8_find_next_char (p1, NULL);
  }

  return found;
}

static void refilter (MosaicSearchBox *search_box, gpointer data)
{
  if (filtered_size) {
    free (filtered_boxes);
  }
  filtered_size = 0;

  for (int i = 0; i < wsize; i++)
      gtk_widget_hide (boxes [i]);

  gchar *search_for = g_utf8_casefold (mosaic_search_box_get_text (search_box), -1);
  int s_size = strlen (search_for);
  if (s_size) {
    filtered_boxes = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));

    GtkWidget **priority1 = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    GtkWidget **priority2 = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    GtkWidget **priority3 = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    gint p1size = 0;
    gint p2size = 0;
    gint p3size = 0;

    for (int i = 0; i < wsize; i++) {
      gchar *wname_cmp = NULL;
      gchar *opt_name1_cmp = NULL;
      gchar *opt_name2_cmp = NULL;
      int wn_size = 0;
      int op1_size = 0;
      int op2_size = 0;

      wname_cmp = g_utf8_casefold (mosaic_window_box_get_name (MOSAIC_WINDOW_BOX (boxes[i])), -1);
      wn_size = strlen (wname_cmp);
      const gchar *opt_name = mosaic_window_box_get_opt_name (MOSAIC_WINDOW_BOX (boxes[i]));
      if (opt_name) {
	opt_name1_cmp = g_utf8_casefold (opt_name, -1);
	op1_size = strlen (opt_name1_cmp);
	if (!options.read_stdin) {
	  opt_name2_cmp = g_utf8_casefold (opt_name+op1_size+1, -1);
	  op2_size = strlen (opt_name2_cmp);
	}
      }
      gboolean found = FALSE;
      if (g_str_has_prefix (wname_cmp, search_for)) {
	found = TRUE;
	priority1 [p1size++] = boxes [i];
      }
      if (!found && ((g_strstr_len (wname_cmp, wn_size, search_for) != NULL) ||
		     (op1_size && g_str_has_prefix (opt_name1_cmp, search_for)) ||
		     (op2_size && g_str_has_prefix (opt_name2_cmp, search_for)))) {
	found = TRUE;
	priority2 [p2size++] = boxes [i];
      }
      if (!found && ((search_by_letters (wname_cmp, wn_size, search_for, s_size)) ||
		     (op1_size && g_strstr_len (opt_name1_cmp, op1_size, search_for) != NULL) ||
		     (op2_size && g_strstr_len (opt_name2_cmp, op2_size, search_for) != NULL))) {
	found = TRUE;
	priority3 [p3size++] = boxes [i];
      }
      g_free (wname_cmp);
      g_free (opt_name1_cmp);
      g_free (opt_name2_cmp);
      if (found)
	filtered_size++;
    }

    for (int i = 0; i < p1size; i++)
      filtered_boxes [i] = priority1 [i];
    for (int i = 0; i < p2size; i++)
      filtered_boxes [p1size+i] = priority2 [i];
    for (int i = 0; i < p3size; i++)
      filtered_boxes [p1size+p2size+i] = priority3 [i];

    free (priority1);
    free (priority2);
    free (priority3);

    draw_mosaic (GTK_LAYOUT (layout), filtered_boxes, filtered_size, 0,
		 options.box_width, options.box_height);
  } else {
    draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, 0,
		 options.box_width, options.box_height);
  }

  if (gtk_widget_get_visible (search)) {
    // Stupid thing to show search entry at top level..
    gtk_widget_hide (search);
    gtk_widget_show (search);
  }

  g_free (search_for);
}

static void draw_mask (GdkDrawable *bitmap, guint size)
{
  cairo_t *cr;

  cr = gdk_cairo_create (bitmap);

  cairo_save (cr);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_restore (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  // Show each mosaic_window_box.
  for (int i = 0; i < boxes_drawn; i++) {
    cairo_rectangle (cr,
		     box_rects[i].x, box_rects[i].y,
		     box_rects[i].width, box_rects[i].height);
    cairo_fill (cr);
  }

  // show search entry if it is active.
  if (search) {
    if (strlen (mosaic_search_box_get_text (MOSAIC_SEARCH_BOX (search))) ||
	(options.vim_mode && gtk_widget_get_visible (search))) {
      GtkAllocation alloc;
      gtk_widget_get_allocation (search, &alloc);
      cairo_rectangle (cr,
		       alloc.x,
		       alloc.y,
		       alloc.width,
		       alloc.height);
      cairo_fill (cr);
    }
  }

  cairo_destroy (cr);
}

static void read_stdin ()
{
  char buffer [BUFSIZ];
  char *p;
  int current_size = 1;
  in_items = calloc (1, sizeof (char *));
  wsize = 0;
  while (fgets (buffer, BUFSIZ, stdin)) {
    if (wsize == current_size) {
      current_size *= 2;
      in_items = realloc (in_items, current_size * sizeof (char *));
    }
    if ((p = strchr (buffer, '\n')))
      *p = '\0';
    in_items [wsize] = g_strdup (buffer);
    wsize++;
  }
}

static GdkPixbuf* get_screenshot ()
{
  GdkWindow *root_window = gdk_get_default_root_window ();
  gint x, y;
  gint swidth, sheight;

  guint m_offset_x = 0, m_offset_y = 0;
  gint monitors = gdk_screen_get_n_monitors (gdk_screen_get_default ());
  if (monitors > 1) {
    gint px = 0, py = 0;
    gdk_display_get_pointer (gdk_display_get_default (), NULL, &px, &py, NULL);
    gint current_monitor = gdk_screen_get_monitor_at_point (gdk_screen_get_default (), px, py);
    for (int i = 0; i < current_monitor; i++) {
      GdkRectangle mon_rect;
      gdk_screen_get_monitor_geometry (gdk_screen_get_default (), i, &mon_rect);
      m_offset_x += mon_rect.width;
      m_offset_y += mon_rect.height;
    }
  }

  gdk_drawable_get_size (root_window, &swidth, &sheight);
  gdk_window_get_origin (root_window, &x, &y);

  if (swidth <= m_offset_x)
    m_offset_x = 0;
  if (sheight <= m_offset_y)
    m_offset_y = 0;

  return gdk_pixbuf_get_from_drawable (NULL, root_window, NULL,
				       x + options.screenshot_offset_x + m_offset_x,
				       y + options.screenshot_offset_y + m_offset_y,
				       0, 0,
				       swidth - options.screenshot_offset_x - m_offset_x,
				       sheight - options.screenshot_offset_y - m_offset_y);
}

static void read_config ()
{
  // Set default options.
  options.vim_mode = FALSE;
  options.box_width = 200;
  options.box_height = 40;
  options.colorize = TRUE;
  options.color_offset = 0;
  options.show_icons = TRUE;
  options.show_desktop = TRUE;
  options.show_titles = TRUE;
  options.icon_size = 16;
  options.font = g_strdup ("Sans 10");
  options.read_stdin = FALSE;
  options.screenshot = FALSE;
  options.screenshot_offset_x = 0;
  options.screenshot_offset_y = 0;

  gchar *filename = g_strjoin ("/", g_get_user_config_dir (), "xwinmosaic/config", NULL);

  GError *error = NULL;
  GKeyFile *config = g_key_file_new ();

  if (!g_key_file_load_from_file (config, filename, 0, &error)) {
    write_default_config ();
    return;
  }

  const gchar *group = "default";
  if (g_key_file_has_group (config, group)) {
    if (g_key_file_has_key (config, group, "vim_mode", &error))
      options.vim_mode = g_key_file_get_boolean (config, group, "vim_mode", &error);
    if (g_key_file_has_key (config, group, "box_width", &error))
      options.box_width = g_key_file_get_integer (config, group, "box_width", &error);
    if (g_key_file_has_key (config, group, "box_height", &error))
      options.box_height = g_key_file_get_integer (config, group, "box_height", &error);
    if (g_key_file_has_key (config, group, "colorize", &error))
      options.colorize = g_key_file_get_boolean (config, group, "colorize", &error);
    if (g_key_file_has_key (config, group, "color_offset", &error))
      options.color_offset = g_key_file_get_integer (config, group, "color_offset", &error);
    if (g_key_file_has_key (config, group, "show_icons", &error))
      options.show_icons = g_key_file_get_boolean (config, group, "show_icons", &error);
    if (g_key_file_has_key (config, group, "show_desktop", &error))
      options.show_desktop = g_key_file_get_boolean (config, group, "show_desktop", &error);
    if (g_key_file_has_key (config, group, "show_titles", &error))
      options.show_titles = g_key_file_get_boolean (config, group, "show_titles", &error);
    if (g_key_file_has_key (config, group, "icon_size", &error))
      options.icon_size = g_key_file_get_integer (config, group, "icon_size", &error);
    if (g_key_file_has_key (config, group, "font", &error))
      options.font = g_key_file_get_string (config, group, "font", &error);
    if (g_key_file_has_key (config, group, "screenshot", &error))
      options.screenshot = g_key_file_get_boolean (config, group, "screenshot", &error);
    if (g_key_file_has_key (config, group, "screenshot_offset_x", &error))
      options.screenshot_offset_x = g_key_file_get_integer (config, group, "screenshot_offset_x", &error);
    if (g_key_file_has_key (config, group, "screenshot_offset_y", &error))
      options.screenshot_offset_y = g_key_file_get_integer (config, group, "screenshot_offset_y", &error);
    if (g_key_file_has_key (config, group, "at_pointer", &error))
      options.at_pointer = g_key_file_get_boolean (config, group, "at_pointer", &error);
    if (g_key_file_has_key (config, group, "color_file", &error))
      options.color_file = g_key_file_get_string (config, group, "color_file", &error);
  }

  g_key_file_free (config);
}

static void write_default_config ()
{
  gchar *confdir = g_strjoin ("/", g_get_user_config_dir (), "xwinmosaic", NULL);
  gchar *filename = g_strjoin ("/", g_get_user_config_dir (), "xwinmosaic/config", NULL);

  if (g_mkdir_with_parents (confdir, 0755) != -1) {
    FILE *config;
    if ((config = fopen (filename, "w")) != NULL) {
      fprintf (config, "[default]\n");
      fprintf (config, "vim_mode = %s\n", (options.vim_mode) ? "true" : "false");
      fprintf (config, "box_width = %d\n", options.box_width);
      fprintf (config, "box_height = %d\n", options.box_height);
      fprintf (config, "colorize = %s\n", (options.colorize) ? "true" : "false");
      fprintf (config, "color_offset = %d\n", options.color_offset);
      fprintf (config, "show_icons = %s\n", (options.show_icons) ? "true" : "false");
      fprintf (config, "show_desktop = %s\n", (options.show_desktop) ? "true" : "false");
      fprintf (config, "show_titles = %s\n", (options.show_titles) ? "true" : "false");
      fprintf (config, "icon_size = %d\n", options.icon_size);
      fprintf (config, "font = %s\n", options.font);
      fprintf (config, "screenshot = %s\n", (options.screenshot) ? "true" : "false");
      fprintf (config, "screenshot_offset_x = %d\n", options.screenshot_offset_x);
      fprintf (config, "screenshot_offset_y = %d\n", options.screenshot_offset_y);
      fprintf (config, "at_pointer = %s\n", (options.at_pointer) ? "true" : "false");
      fprintf (config, "# color_file = /path/to/file\n");
      fclose (config);
      }
  }

  g_printerr ("created new config in %s\n", confdir);
}

static void on_focus_change (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  if (event->in == FALSE) { /* focus out */
    /* workaround for awesome wm and its unexpected focus changes */
    if (options.screenshot && !key_pressed)
      return;
    if (options.persistent)
      gtk_widget_hide (window);
    else
      gtk_main_quit ();
  }
}

static void read_colors ()
{

  GError *error = NULL;
  color_config = g_key_file_new ();

  if (!g_key_file_load_from_file (color_config, options.color_file, 0, &error)) {
    g_printerr ("%s\n", error->message);
    return;
  }

  fallback_size = 0;

  const gchar *group = "colors";
  if (g_key_file_has_group (color_config, group)) {
    if (g_key_file_has_key (color_config, group, "fallback", &error))
      fallback_colors = g_key_file_get_string_list (color_config, group, "fallback", &fallback_size, &error);
  }
}
static gboolean parse_format (Entry* entry, char *data)
{
  gchar **opts = g_strsplit(data, ",", 5);
  if(!opts[1] + !opts[2] + !opts[3] + !opts[4]) { //What did i just write?
    g_printerr("Format error\n");
    return FALSE;
  }
  g_strchug(opts[0]);
  if(g_ascii_isdigit(opts[0][0])) {
    if(sscanf(opts[0], "%d", &(entry->desktop))==EOF) return FALSE;
  } else
      entry->desktop = -1;
  entry->color = opts[1];
  entry->iconpath = opts[2];
  entry->label = opts[3];
  entry->opt_name = opts[4];
  //g_strfreev(opts);
  g_strchug(entry->color);
  g_strchug(entry->iconpath);
  g_strchug(entry->label);
  g_strchug(entry->opt_name);
  return TRUE;
}

void tab_event (gboolean shift) //FIXME: put prototype for this function
                                //in some header file
{
  gboolean is_visible = FALSE;
  g_object_get (window, "visible", &is_visible, NULL);
  if(is_visible) {
    GtkWidget **bs;
    guint bsize = 0;
    if(gtk_widget_get_visible (search)) {
      bs = filtered_boxes;
      bsize = filtered_size;
    } else {
      bs = boxes;
      bsize = wsize;
    }
    if (bsize == 0) return; // nothing to switch between
    // Calculate current box by straightforward pointer comparison
    guint current_box = 0;
    MosaicWindowBox* box = MOSAIC_WINDOW_BOX (gtk_window_get_focus (GTK_WINDOW (window)));
    for (guint i = 0; i < bsize; i++)
      if (MOSAIC_WINDOW_BOX(box) == MOSAIC_WINDOW_BOX(bs[i])) {
	current_box = i;
	break;
      }
    if(!shift) {
	current_box < bsize-1 ? current_box++ : (current_box = 0);
    } else {
	current_box > 0 ? current_box-- : (current_box = bsize-1);
    }
    gtk_widget_grab_focus (bs[current_box]);
  } else {
    update_box_list();
    draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, 0,
                 options.box_width, options.box_height);
    gtk_window_present (GTK_WINDOW (window));
  }
}
