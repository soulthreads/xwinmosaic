#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include "x_interaction.h"
#include "window_box.h"

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

static struct {
  guint box_width;
  guint box_height;
  gboolean colorize;
  gboolean show_icons;
  gboolean show_desktop;
  guint icon_size;
  gchar *font_name;
  guint font_size;
  gboolean read_stdin;
  gboolean launcher_mode;
  gboolean screenshot;
  guchar color_offset;
  gint screenshot_offset_x;
  gint screenshot_offset_y;
  gboolean vim_mode;
  gboolean at_pointer;
  gint center_x;
  gint center_y;
} options;

static GOptionEntry entries [] =
{
  { "read-stdin", 'r', 0, G_OPTION_ARG_NONE, &options.read_stdin,
    "Read items from stdin (and print selected item to stdout)", NULL },
  { "launcher-mode", 'L', 0, G_OPTION_ARG_NONE, &options.launcher_mode,
    "Let xwinmosaic act like dmenu", NULL},
  { "vim-mode", 'V', 0, G_OPTION_ARG_NONE, &options.vim_mode,
    "Turn on vim-like navigation (hjkl, search on /)", NULL },
  { "no-colors", 'C', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.colorize,
    "Turn off box colorizing", NULL },
  { "no-icons", 'I', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.show_icons,
    "Turn off showing icons", NULL },
  { "no-desktops", 'D', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options.show_desktop,
    "Turn off showing desktop number", NULL },
  { "screenshot", 'S', 0, G_OPTION_ARG_NONE, &options.screenshot,
    "Get screenshot and set it as a background (for WMs that do not support XShape)", NULL },
  { "at-pointer", 'P', 0, G_OPTION_ARG_NONE, &options.at_pointer,
    "Place center of mosaic at pointer position.", NULL },

  { "box-width", 'W', 0, G_OPTION_ARG_INT, &options.box_width,
    "Width of the boxes (default: 200)", "<int>" },
  { "box-height", 'H', 0, G_OPTION_ARG_INT, &options.box_height,
    "Height of the boxes (default: 40)", "<int>" },
  { "icon-size", 'i', 0, G_OPTION_ARG_INT, &options.icon_size,
    "Size of window icons (default: 16)", "<int>" },
  { "font-name", 'f', 0, G_OPTION_ARG_STRING, &options.font_name,
    "Which font to use for displaying widgets. (default: Sans)", "\"font name\"" },
  { "font-size", 's', 0, G_OPTION_ARG_INT, &options.font_size,
    "Font size (default: 10)", "<int>" },
  { "hue-offset", 'o', 0, G_OPTION_ARG_INT, &options.color_offset,
    "Set color hue offset (from 0 to 255)", "<int>" },
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
static GdkFilterReturn event_filter (XEvent *xevent, GdkEvent *event, gpointer data);
static void refilter (GtkEditable *editable, gpointer data);
static void draw_mask (GdkDrawable *bitmap, GtkWidget **wdgts, guint size);
static void read_stdin ();
static GdkPixbuf* get_screenshot ();
static void read_config ();
static void write_default_config ();
static void launch(gchar* command);

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
    g_print ("option parsing failed: %s\n", error->message);
    exit (1);
  }
  g_option_context_free (context);

  atoms_init ();

  if (options.read_stdin) {
    options.show_icons = FALSE;
    options.show_desktop = FALSE;
    read_stdin ();
  } else if(options.launcher_mode) {
    options.show_icons = FALSE;
    options.show_desktop = FALSE;
    options.read_stdin = TRUE;
    read_stdin ();
  } else {
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
  }


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "XWinMosaic");

  GdkRectangle rect = current_monitor_size ();
  width = rect.width;
  height = rect.height;

  if (options.at_pointer) {
    gdk_display_get_pointer (gdk_display_get_default (), NULL, &options.center_x, &options.center_y, NULL);
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
  gtk_window_set_decorated (GTK_WINDOW (window), False);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), True);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), True);
/**/
  gtk_widget_add_events (GTK_WIDGET (window), GDK_FOCUS_CHANGE);
  g_signal_connect (G_OBJECT (window), "focus-out-event",
		    G_CALLBACK (gtk_main_quit), NULL);
/**/
  layout = gtk_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), layout);

  if (options.screenshot) {
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

  search = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (search), 20);
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
    draw_mask (window_shape_bitmap, boxes, 0);
    gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);
  }

  gtk_widget_show_all (window);
  gtk_widget_hide (search);
  gtk_window_present (GTK_WINDOW (window));

  GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));
  myown_window = GDK_WINDOW_XID (gdk_window);

  if ((!options.read_stdin)&&(!options.launcher_mode)) {
    // Get PropertyNotify events from root window.
    XSelectInput (gdk_x11_get_default_xdisplay (),
		  gdk_x11_get_default_root_xwindow (),
		  PropertyChangeMask);
    gdk_window_add_filter (NULL, (GdkFilterFunc) event_filter, NULL);
  }
  update_box_list ();
  draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, 0,
	       options.box_width, options.box_height);

  // Window wil be shown on all desktops (and so hidden in windows list)
  unsigned int desk = 0xFFFFFFFF; // -1
  XChangeProperty(gdk_x11_get_default_xdisplay (), myown_window, a_NET_WM_DESKTOP, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char *)&desk, 1);

  gtk_main ();

  if ((!options.read_stdin)&&(!options.launcher_mode))
    XFree (wins);

  return 0;
}

static GdkRectangle current_monitor_size ()
{
  // Where is the pointer now?
  int x, y;
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
  int monitor = gdk_screen_get_monitor_at_point (gdk_screen_get_default (), x, y);
  GdkRectangle rect;
  gdk_screen_get_monitor_geometry (gdk_screen_get_default (), monitor, &rect);

  return rect;
}

static void draw_mosaic (GtkLayout *where,
		  GtkWidget **widgets, int rsize,
		  int focus_on,
		  int rwidth, int rheight)
{
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
	if (cur_x > 0 && cur_x+rwidth < width && cur_y > 0 && cur_y+rheight < height) {
	  offset = 0;
	  if (gtk_widget_get_parent (widgets[i]))
	    gtk_layout_move (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	  else
	    gtk_layout_put (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	  gtk_widget_set_size_request (widgets[i], rwidth, rheight);
	  window_box_set_inner (WINDOW_BOX (widgets[i]), cur_x, cur_y, rwidth, rheight);
	  gtk_widget_show (widgets[i]);
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
    draw_mask (window_shape_bitmap, widgets, rsize);
    gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);
  }
}

static void on_rect_click (GtkWidget *widget, gpointer data)
{
  WindowBox *box = WINDOW_BOX (widget);
  if ((!options.read_stdin)&&(!options.launcher_mode)) {
    Window win = box->xwindow;
    switch_to_window (win);
  } else {
    if(options.read_stdin) puts(box->name);
    if(options.launcher_mode)
      {
        gchar* searchbox = (gchar*)gtk_entry_get_text(GTK_ENTRY (search));
        if(g_strcmp0(searchbox, box->name))
          {
            launch(box->name);
          }
        else launch(searchbox);
      }
  }
  gtk_main_quit ();
}


static void update_box_list ()
{
  if (!options.read_stdin) {
    if (wsize) {
      for (int i = 0; i < wsize; i++) {
	gtk_widget_destroy (boxes[i]);
      }
      free (boxes);
      XFree (wins);
    }
    wins = sorted_windows_list (&myown_window, active_window, &wsize);
    if (wins) {
      // Get PropertyNotify events from each relevant window.
      for (int i = 0; i < wsize; i++)
	XSelectInput (gdk_x11_get_default_xdisplay (),
		      wins[i],
		      PropertyChangeMask);
    }
  }

  if (wsize) {
    boxes = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    for (int i = 0; i < wsize; i++) {
      if (!options.read_stdin) {
	boxes[i] = window_box_new_with_xwindow (wins[i]);
	window_box_set_show_desktop (WINDOW_BOX (boxes[i]), options.show_desktop);
	if (options.show_icons)
	  window_box_setup_icon (WINDOW_BOX(boxes[i]), options.icon_size, options.icon_size);
      } else {
	boxes[i] = window_box_new_with_name (in_items[i]);
      }
      window_box_set_font (WINDOW_BOX (boxes [i]), options.font_name, options.font_size);
      window_box_set_colorize (WINDOW_BOX (boxes[i]), options.colorize);
      window_box_set_color_offset (WINDOW_BOX (boxes[i]), options.color_offset);
      g_signal_connect (G_OBJECT (boxes[i]), "clicked",
			G_CALLBACK (on_rect_click), NULL);
    }
  }
}

static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  switch (event->keyval) {
  case GDK_KEY_Escape:
  {
    int text_length = gtk_entry_get_text_length (GTK_ENTRY (search));
    if (text_length > 0 || gtk_widget_get_visible (search)) {
      gtk_widget_hide (search);
      gtk_editable_delete_text (GTK_EDITABLE (search), 0, -1);
      g_signal_emit_by_name (G_OBJECT (search), "changed", NULL);
    } else {
      gtk_main_quit ();
    }
    break;
  }
  case GDK_KEY_Return:
    {
      if(gtk_entry_get_text_length(GTK_ENTRY (search))&&(!filtered_size)&&options.launcher_mode)
        {
          launch( (gchar*)gtk_entry_get_text(GTK_ENTRY (search)));
          gtk_main_quit();
        }
    }
    break;
  case GDK_KEY_Left:
  case GDK_KEY_Up:
  case GDK_KEY_Right:
  case GDK_KEY_Down:
  case GDK_KEY_Tab:
    break;
  case GDK_KEY_End:
    if(options.launcher_mode){
      WindowBox* box = WINDOW_BOX (gtk_window_get_focus(GTK_WINDOW (window)));
      gtk_entry_set_text(GTK_ENTRY (search), box->name);
      gtk_widget_show(search);
    }
    break;
  case GDK_KEY_BackSpace:
  {
    int text_length = gtk_entry_get_text_length (GTK_ENTRY (search));
    if (text_length > 0)
      gtk_editable_delete_text (GTK_EDITABLE (search), text_length - 1, -1);
    if (text_length == 1 || text_length == 0) {
      if (!options.vim_mode)
	gtk_widget_hide (search);
      g_signal_emit_by_name (G_OBJECT (search), "changed", NULL);
    }
    break;
  }
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
	default:
	  return FALSE;
	}
	return TRUE;
      }
    }

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
      case GDK_KEY_slash:
	gtk_widget_show (search);
	g_signal_emit_by_name (G_OBJECT (search), "changed", NULL);
	break;
      }
      return TRUE;
    }

    char *key = event->string;
    if (strlen(key)) {
      int text_length = gtk_entry_get_text_length (GTK_ENTRY (search));
      gtk_editable_insert_text (GTK_EDITABLE (search), key, strlen (key), &text_length);
      if (text_length)
	gtk_widget_show (search);
      return TRUE;
    }
  }
  }
  return FALSE;
}

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
	if (gtk_entry_get_text_length (GTK_ENTRY (search))) {
	  refilter (GTK_EDITABLE (search), NULL);
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
	    window_box_update_name (WINDOW_BOX (boxes[i]));
	    break;
	  }
      }
      if (atom == a_NET_WM_ICON && options.show_icons) {
	// Search for appropriate widget to update icon.
	for (int i = 0; i < wsize; i++)
	  if (wins [i] == win) {
	    window_box_setup_icon (WINDOW_BOX (boxes[i]), options.icon_size, options.icon_size);
	    break;
	  }
      }
    }
  }

  return GDK_FILTER_CONTINUE;
}

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

static void refilter (GtkEditable *entry, gpointer data)
{
  if (filtered_size) {
    free (filtered_boxes);
  }
  filtered_size = 0;

  for (int i = 0; i < wsize; i++)
      gtk_widget_hide (boxes [i]);

  gchar *search_for = g_utf8_casefold (GTK_ENTRY(entry)->text, -1);
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
      gchar *wclass_cmp = NULL;
      int wn_size = 0;
      int wc_size = 0;

      wname_cmp = g_utf8_casefold (window_box_get_name (WINDOW_BOX (boxes[i])), -1);
      wn_size = strlen (wname_cmp);
      if (!options.read_stdin) {
	wclass_cmp = g_utf8_casefold (window_box_get_xclass (WINDOW_BOX (boxes[i])), -1);
	wc_size = strlen (wclass_cmp);
      }
      gboolean found = FALSE;
      if (g_str_has_prefix (wname_cmp, search_for)) {
	found = TRUE;
	priority1 [p1size++] = boxes [i];
      }
      if (!found && ((g_strstr_len (wname_cmp, wn_size, search_for) != NULL) ||
		     (!options.read_stdin && g_str_has_prefix (wclass_cmp, search_for)))) {
	found = TRUE;
	priority2 [p2size++] = boxes [i];
      }
      if (!found && ((search_by_letters (wname_cmp, wn_size, search_for, s_size)) ||
	  (!options.read_stdin && g_strstr_len (wclass_cmp, wc_size, search_for) != NULL))) {
	found = TRUE;
	priority3 [p3size++] = boxes [i];
      }
      g_free (wname_cmp);
      g_free (wclass_cmp);
    }
    for (int i = 0; i < p1size; i++)
      filtered_boxes [filtered_size++] = priority1 [i];
    for (int i = 0; i < p2size; i++)
      filtered_boxes [filtered_size++] = priority2 [i];
    for (int i = 0; i < p3size; i++)
      filtered_boxes [filtered_size++] = priority3 [i];

    free (priority1);
    free (priority2);
    free (priority3);

    draw_mosaic (GTK_LAYOUT (layout), filtered_boxes, filtered_size, 0,
		 options.box_width, options.box_height);
  } else {
    draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, 0,
		 options.box_width, options.box_height);
  }
  g_free (search_for);
}

static void draw_mask (GdkDrawable *bitmap, GtkWidget **wdgts, guint size)
{
  cairo_t *cr;

  cr = gdk_cairo_create (bitmap);

  cairo_save (cr);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill (cr);
  cairo_restore (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  // Show each window_box.
  for (int i = 0; i < size; i++) {
    WindowBox *box = WINDOW_BOX (wdgts[i]);
    cairo_rectangle (cr,
		     box->x, box->y,
		     box->width, box->height);
    cairo_fill (cr);
  }

  // show search entry if it is active.
  if (search) {
    if (gtk_widget_get_visible (search)) {
      // Stupid thing to show search entry at top level..
      gtk_widget_hide (search);
      gtk_widget_show (search);
    }
    if (gtk_entry_get_text_length (GTK_ENTRY (search)) ||
	(options.vim_mode && gtk_widget_get_visible (search))) {
      cairo_rectangle (cr,
		       search->allocation.x,
		       search->allocation.y,
		       search->allocation.width,
		       search->allocation.height);
      cairo_fill (cr);
    }
  }

  cairo_destroy (cr);
}

static void read_stdin ()
{
  char buffer [BUFSIZ];
  char *p;
  in_items = calloc (1, sizeof (char *));
  wsize = 0;
  while (fgets (buffer, BUFSIZ, stdin)) {
    in_items = realloc (in_items, (wsize+1) * sizeof (char *));
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

  gdk_drawable_get_size (root_window, &swidth, &sheight);
  gdk_window_get_origin (root_window, &x, &y);

  return gdk_pixbuf_get_from_drawable (NULL, root_window, NULL,
				       x + options.screenshot_offset_x,
				       y + options.screenshot_offset_y,
				       0, 0,
				       swidth - options.screenshot_offset_x,
				       sheight - options.screenshot_offset_y);
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
  options.icon_size = 16;
  options.font_name = g_strdup ("Sans");
  options.font_size = 10;
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
    if (g_key_file_has_key (config, group, "icon_size", &error))
      options.icon_size = g_key_file_get_integer (config, group, "icon_size", &error);
    if (g_key_file_has_key (config, group, "font_name", &error))
      options.font_name = g_key_file_get_string (config, group, "font_name", &error);
    if (g_key_file_has_key (config, group, "font_size", &error))
      options.font_size = g_key_file_get_integer (config, group, "font_size", &error);
    if (g_key_file_has_key (config, group, "screenshot", &error))
      options.screenshot = g_key_file_get_boolean (config, group, "screenshot", &error);
    if (g_key_file_has_key (config, group, "screenshot_offset_x", &error))
      options.screenshot_offset_x = g_key_file_get_integer (config, group, "screenshot_offset_x", &error);
    if (g_key_file_has_key (config, group, "screenshot_offset_y", &error))
      options.screenshot_offset_y = g_key_file_get_integer (config, group, "screenshot_offset_y", &error);
    if (g_key_file_has_key (config, group, "at_pointer", &error))
      options.at_pointer = g_key_file_get_boolean (config, group, "at_pointer", &error);
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
      fprintf (config,
	       "[default]\n\
vim_mode = %s\n\
box_width = %d\n\
box_height = %d\n\
colorize = %s\n\
color_offset = %d\n\
show_icons = %s\n\
show_desktop = %s\n\
icon_size = %d\n\
font_name = %s\n\
font_size = %d\n\
screenshot = %s\n\
screenshot_offset_x = %d\n\
screenshot_offset_y = %d\n\
at_pointer = %s\n\
",
	       (options.vim_mode) ? "true" : "false",
	       options.box_width,
	       options.box_height,
	       (options.colorize) ? "true" : "false",
	       options.color_offset,
	       (options.show_icons) ? "true" : "false",
	       (options.show_desktop) ? "true" : "false",
	       options.icon_size,
	       options.font_name,
	       options.font_size,
	       (options.screenshot) ? "true" : "false",
	       options.screenshot_offset_x,
	       options.screenshot_offset_y,
	       (options.at_pointer) ? "true" : "false");

      fclose (config);
      }
  }

  g_print ("created new config in %s\n", confdir);
}

static void launch(gchar* command) {
  //stub!
  puts(command);
}

