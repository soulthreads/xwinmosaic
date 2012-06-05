#ifndef MOSAIC_WINDOW_BOX_H
#define MOSAIC_WINDOW_BOX_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "mosaic_box.h"
#include "x_interaction.h"

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

  gchar *xclass;
  gboolean show_desktop;
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
void mosaic_window_box_set_xwindow (MosaicWindowBox *box, guint window);
guint mosaic_window_box_get_xwindow (MosaicWindowBox *box);
void mosaic_window_box_set_name (MosaicWindowBox *box, const gchar *name);
const gchar *mosaic_window_box_get_name (MosaicWindowBox *box);
void mosaic_window_box_set_xclass (MosaicWindowBox *box, const gchar *xclass);
const gchar *mosaic_window_box_get_xclass (MosaicWindowBox *box);
void mosaic_window_box_update_name (MosaicWindowBox *box);
void mosaic_window_box_update_xclass (MosaicWindowBox *box);
void mosaic_window_box_setup_icon (MosaicWindowBox *box, guint req_width, guint req_height);
void mosaic_window_box_set_colorize (MosaicWindowBox *box, gboolean colorize);
void mosaic_window_box_set_show_desktop (MosaicWindowBox *box, gboolean show_desktop);
void mosaic_window_box_set_color_offset (MosaicWindowBox *box, guchar color_offset);

G_END_DECLS

#endif /* MOSAIC_WINDOW_BOX_H */
