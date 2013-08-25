/* Copyright (c) 2012, Anton S. Lobashev
 * mosaic_window_box.h - headers for window box widget
 */

#ifndef MOSAIC_WINDOW_BOX_H
#define MOSAIC_WINDOW_BOX_H

#include <stdlib.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "mosaic_box.h"
#ifdef X11
#include "x_interaction.h"
#endif
#ifdef WIN32
#include "win32_interaction.h"
#endif

G_BEGIN_DECLS

#define MOSAIC_TYPE_WINDOW_BOX            (mosaic_window_box_get_type ())
#define MOSAIC_WINDOW_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOSAIC_TYPE_WINDOW_BOX, MosaicWindowBox))
#define MOSAIC_WINDOW_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOSAIC_TYPE_WINDOW_BOX, MosaicWindowBoxClass))
#define MOSAIC_IS_WINDOW_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOSAIC_TYPE_WINDOW_BOX))
#define MOSAIC_IS_WINDOW_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOSAIC_TYPE_WINDOW_BOX))
#define MOSAIC_WINDOW_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOSAIC_TYPE_WINDOW_BOX, MosaicWindowBoxClass))

typedef struct _MosaicWindowBox      MosaicWindowBox;
typedef struct _MosaicWindowBoxClass MosaicWindowBoxClass;

struct _MosaicWindowBox
{
  MosaicBox parent;

  /*< private >*/
  gboolean is_window;
  Window xwindow;

  gchar *opt_name;
  gboolean show_desktop;
  gboolean show_titles;
  gint desktop;

  gboolean colorize;
  gdouble r, g, b;
  guchar color_offset;

  gboolean has_icon;
  GdkPixbuf *icon_pixbuf;
  cairo_t *icon_context;
  cairo_surface_t *icon_surface;
};

struct _MosaicWindowBoxClass
{
  MosaicBoxClass parent_class;

  void (* pressed)  (MosaicWindowBox *box);
  void (* released) (MosaicWindowBox *box);
  void (* clicked)  (MosaicWindowBox *box);
  void (* enter)    (MosaicWindowBox *box);
  void (* leave)    (MosaicWindowBox *box);
  void (* activate) (MosaicWindowBox *box);
};

GType mosaic_window_box_get_type (void);
GtkWidget* mosaic_window_box_new (void);
GtkWidget* mosaic_window_box_new_with_xwindow (Window win);
GtkWidget* mosaic_window_box_new_with_name (gchar *name);
void mosaic_window_box_set_is_window (MosaicWindowBox *box, gboolean is_window);
gboolean mosaic_window_box_get_is_window (MosaicWindowBox *box);
void mosaic_window_box_set_xwindow (MosaicWindowBox *box, Window window);
Window mosaic_window_box_get_xwindow (MosaicWindowBox *box);
void mosaic_window_box_set_name (MosaicWindowBox *box, const gchar *name);
const gchar *mosaic_window_box_get_name (MosaicWindowBox *box);
void mosaic_window_box_set_opt_name (MosaicWindowBox *box, const gchar *opt_name);
const gchar *mosaic_window_box_get_opt_name (MosaicWindowBox *box);
void mosaic_window_box_update_xwindow_name (MosaicWindowBox *box);
void mosaic_window_box_update_opt_name (MosaicWindowBox *box);
void mosaic_window_box_setup_icon_from_wm (MosaicWindowBox *box, guint req_width, guint req_height);
void mosaic_window_box_setup_icon_from_theme (MosaicWindowBox *box, const gchar *name, guint req_width, guint req_height);
void mosaic_window_box_setup_icon_from_file (MosaicWindowBox *box, const gchar *file, guint req_width, guint req_height);
void mosaic_window_box_set_colorize (MosaicWindowBox *box, gboolean colorize);
void mosaic_window_box_set_show_desktop (MosaicWindowBox *box, gboolean show_desktop);
void mosaic_window_box_set_show_titles (MosaicWindowBox *box, gboolean show_titles);
void mosaic_window_box_set_color_offset (MosaicWindowBox *box, guchar color_offset);
void mosaic_window_box_set_color_from_string (MosaicWindowBox *box, const gchar *color);
gint mosaic_window_box_get_desktop (MosaicWindowBox *box);
void mosaic_window_box_set_desktop (MosaicWindowBox *box, gint desktop);

G_END_DECLS

#endif /* MOSAIC_WINDOW_BOX_H */
