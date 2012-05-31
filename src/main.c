#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <string.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include "x_interaction.h"
#include "window_box.h"

static GtkWidget *window;
static Window active_window; // For displaying it first in windows list.
static Window *wins;
static int wsize;
static GtkWidget **boxes;
static GtkWidget *layout;
static GtkWidget *search;
static Window *filtered_wins;
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
} options;

static GdkRectangle current_monitor_size ();
static void draw_mosaic (GtkLayout *where,
		  GtkWidget **widgets, int rsize,
		  int focus_on,
		  int rwidth, int rheight);
static void on_rect_click (GtkWidget *widget, gpointer data);
static void update_window_list ();
static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data);
static GdkFilterReturn event_filter (XEvent *xevent, GdkEvent *event, gpointer data);
static void refilter (GtkEditable *editable, gpointer data);
static void draw_mask (GdkDrawable *bitmap, GtkWidget **wdgts, guint size);
static void show_help ();

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  int opt;
  options.box_width = 200;
  options.box_height = 40;
  options.colorize = TRUE;
  options.show_icons = TRUE;
  options.show_desktop = TRUE;
  options.icon_size = 16;
  options.font_size = 10;
  while ((opt = getopt (argc, argv, "hCIDW:H:i:f:s:")) != -1) {
    switch (opt) {
    case 'h':
      show_help ();
      return 0;
    case 'C':
      options.colorize = FALSE;
      break;
    case 'I':
      options.show_icons = FALSE;
      break;
    case 'D':
      options.show_desktop = FALSE;
      break;
    case 'W':
      options.box_width = atoi (optarg);
      break;
    case 'H':
      options.box_height = atoi (optarg);
      break;
    case 'i':
      options.icon_size = atoi (optarg);
      break;
    case 'f':
      options.font_name = g_strdup (optarg);
      break;
    case 's':
      options.font_size = atoi (optarg);
      break;
    default:
      show_help ();
      return 1;
    }
  }
  if (!options.font_name)
    options.font_name = g_strdup ("Sans");

  atoms_init ();

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

  active_window = *((Window *) property (gdk_x11_get_default_root_xwindow (),
					 a_NET_ACTIVE_WINDOW,
					 XA_WINDOW,
					 NULL));
  update_window_list ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "XWinMosaic");

  GdkRectangle rect = current_monitor_size ();
  width = rect.width;
  height = rect.height;
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  gtk_window_set_decorated (GTK_WINDOW (window), False);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), True);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), True);
/**/
  gtk_widget_add_events (GTK_WIDGET (window), GDK_FOCUS_CHANGE);
  g_signal_connect (G_OBJECT (window), "focus-out-event",
		    G_CALLBACK (gtk_main_quit), NULL);
/**/
  layout = gtk_layout_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), layout);

  draw_mosaic (GTK_LAYOUT (layout), boxes, wsize, 0, options.box_width, options.box_height);

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

  window_shape_bitmap = (GdkDrawable *) gdk_pixmap_new (NULL, width, height, 1);
  draw_mask (window_shape_bitmap, boxes, wsize);
  gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);

  gtk_widget_show_all (window);
  gtk_widget_hide (search);

  // Get PropertyNotify events from root window.
  XSelectInput (gdk_x11_get_default_xdisplay (),
		gdk_x11_get_default_root_xwindow (),
		PropertyChangeMask);
  gdk_window_add_filter (NULL, (GdkFilterFunc) event_filter, NULL);

  // Window wil be shown on all desktops (and so hidden in windows list)
  GdkWindow *gdk_window = gtk_widget_get_window (GTK_WIDGET (window));
  Window mwin = GDK_WINDOW_XID (gdk_window);
  unsigned int desk = 0xFFFFFFFF; // -1
  XChangeProperty(gdk_x11_get_default_xdisplay (), mwin, a_NET_WM_DESKTOP, XA_CARDINAL,
		  32, PropModeReplace, (unsigned char *)&desk, 1);

  gtk_main ();

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
  int cur_x = (width - rwidth)/2;
  int cur_y = (height - rheight)/2;
  if (rsize) {
    int i = 0;

    if (gtk_widget_get_parent (widgets[i]))
      gtk_layout_move (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
    else
      gtk_layout_put (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
    gtk_widget_set_size_request (widgets[i], rwidth, rheight);
    window_box_set_inner (WINDOW_BOX (widgets[i]), cur_x, cur_y, rwidth, rheight);
    gtk_widget_show (widgets[i++]);
    int side = 1;
    while (i < rsize) {
      cur_x = (width - rwidth)/2;
      cur_y -= rheight;

      for (int j = 0; j < side * 4; j++) {
	if (i == rsize)
	  break;
	if (gtk_widget_get_parent (widgets[i]))
	  gtk_layout_move (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	else
	  gtk_layout_put (GTK_LAYOUT (where), widgets[i], cur_x, cur_y);
	gtk_widget_set_size_request (widgets[i], rwidth, rheight);
	window_box_set_inner (WINDOW_BOX (widgets[i]), cur_x, cur_y, rwidth, rheight);
	gtk_widget_show (widgets[i++]);
	if (j % (side * 4) < side || j % (side * 4) >= side * 3)
	  cur_x += rwidth;
	else
	  cur_x -= rwidth;
	if (j % (side * 4) < side * 2)
	  cur_y += rheight;
	else
	  cur_y -= rheight;
      }
      side++;
    }
    if (focus_on >= rsize)
      // If some window was killed and focus was on the last element
      focus_on = rsize-1;
    gtk_widget_grab_focus (widgets[focus_on]);
  }
  window_shape_bitmap = (GdkDrawable *) gdk_pixmap_new (NULL, width, height, 1);
  draw_mask (window_shape_bitmap, widgets, rsize);
  gtk_widget_shape_combine_mask (window, window_shape_bitmap, 0, 0);
}

static void on_rect_click (GtkWidget *widget, gpointer data)
{
  Window *win = (Window *) data;
  switch_to_window (*win);
  gtk_main_quit ();
}

static void update_window_list ()
{
  if (wsize) {
    for (int i = 0; i < wsize; i++) {
      gtk_widget_destroy (boxes[i]);
    }
    free (boxes);
    XFree (wins);
  }
  wins = sorted_windows_list (&active_window, &wsize);
  if (wins) {
    // Get PropertyNotify events from each relevant window.
    for (int i = 0; i < wsize; i++)
      XSelectInput (gdk_x11_get_default_xdisplay (),
		    wins[i],
		    PropertyChangeMask);

    boxes = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    for (int i = 0; i < wsize; i++) {
      boxes[i] = window_box_new_with_xwindow (wins[i]);
      window_box_set_font (WINDOW_BOX (boxes [i]), options.font_name, options.font_size);
      window_box_set_colorize (WINDOW_BOX (boxes[i]), options.colorize);
      window_box_set_show_desktop (WINDOW_BOX (boxes[i]), options.show_desktop);
      if (options.show_icons)
	window_box_setup_icon (WINDOW_BOX(boxes[i]), options.icon_size, options.icon_size);
      g_signal_connect (G_OBJECT (boxes[i]), "clicked",
			G_CALLBACK (on_rect_click), &(WINDOW_BOX(boxes [i])->xwindow));
    }
  }
}

static gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  switch (event->keyval) {
  case GDK_KEY_Escape:
    gtk_main_quit ();
    break;
  case GDK_KEY_Return:
  case GDK_KEY_Left:
  case GDK_KEY_Up:
  case GDK_KEY_Right:
  case GDK_KEY_Down:
  case GDK_KEY_Tab:
    break;
  case GDK_KEY_BackSpace:
  {
    int text_length = gtk_entry_get_text_length (GTK_ENTRY (search));
    if (text_length > 0)
      gtk_editable_delete_text (GTK_EDITABLE (search), text_length - 1, -1);
    if (text_length == 1 || text_length == 0)
      gtk_widget_hide (search);
    break;
  }
  default:
  {
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
	update_window_list ();
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

static void refilter (GtkEditable *entry, gpointer data)
{
  if (filtered_size) {
    XFree (filtered_wins);
    free (filtered_boxes);
  }
  filtered_size = 0;

  gchar *search_for = g_utf8_casefold (GTK_ENTRY(entry)->text, -1);
  int s_size = strlen (search_for);
  if (s_size) {
    for (int i = 0; i < wsize; i++)
      gtk_widget_hide (boxes [i]);

    filtered_wins = (Window *) malloc (wsize * sizeof (Window));
    filtered_boxes = (GtkWidget **) malloc (wsize * sizeof (GtkWidget *));
    for (int i = 0; i < wsize; i++) {
      char *wname = get_window_name (wins[i]);
      gchar *wname_cmp = g_utf8_casefold (wname, -1);
      int wn_size = strlen (wname_cmp);
      gchar *p1 = search_for;
      gchar *p2 = wname_cmp;
      gboolean found = FALSE;
      while (p1 < search_for + s_size) {
	gunichar c1 = g_utf8_get_char (p1);
	found = FALSE;
	while (p2 < wname_cmp + wn_size) {
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
      if (found) {
	filtered_wins [filtered_size] = wins [i];
	filtered_boxes [filtered_size] = boxes [i];
	filtered_size++;
      }
      g_free (wname_cmp);
      free (wname);
    }
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
    if (gtk_entry_get_text_length (GTK_ENTRY (search))) {
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

static void show_help ()
{
  fprintf (stderr, "\
Usage: xwinmosaic [OPTIONS]\n\
Options:\n\
  -h                Show this help\n\
  -C                Turns off box colorizing\n\
  -I                Turns off showing icons\n\
  -D                Turns off showing desktop number\n\
\n\
  -W <int>          Width of the boxes (default: 200)\n\
  -H <int>          Height of the boxes (default: 40)\n\
  -i <int>          Size of window icons (default: 16)\n\
  -f \"font name\"    Which font to use for displaying widgets. (default: Sans)\n\
  -s <int>          Font size (default: 10)\n\
");
}
