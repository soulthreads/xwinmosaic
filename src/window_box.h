#ifndef WINDOW_BOX_H
#define WINDOW_BOX_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "x_interaction.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_BOX            (window_box_get_type ())
#define WINDOW_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_TYPE_BOX, WindowBox))
#define WINDOW_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_TYPE_BOX, WindowBoxClass))
#define WINDOW_IS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_TYPE_BOX))
#define WINDOW_IS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_TYPE_BOX))
#define WINDOW_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_TYPE_BOX, WindowBoxClass))

typedef struct _WindowBox      WindowBox;
typedef struct _WindowBoxClass WindowBoxClass;

struct _WindowBox
{
  GtkDrawingArea parent;

  /*< private >*/
  gboolean is_window;
  Window xwindow;
  gchar *name;
  gchar *xclass;
  gboolean show_desktop;
  gint desktop;

  gboolean on_box;
  gboolean box_down;

  gboolean colorize;
  gdouble r, g, b;

  gboolean has_icon;
  GdkPixbuf *icon_pixbuf;
  cairo_t *icon_context;
  cairo_surface_t *icon_surface;

  gchar *font_name;
  guint font_size;

  guint x, y, width, height;
};

struct _WindowBoxClass
{
  GtkDrawingAreaClass parent_class;

  void (* pressed)  (WindowBox *box);
  void (* released) (WindowBox *box);
  void (* clicked)  (WindowBox *box);
  void (* enter)    (WindowBox *box);
  void (* leave)    (WindowBox *box);
  void (* activate) (WindowBox *box);

};

GType window_box_get_type (void);
GtkWidget* window_box_new (void);
GtkWidget* window_box_new_with_xwindow (Window win);
GtkWidget* window_box_new_with_name (gchar *name);
void window_box_set_is_window (WindowBox *box, gboolean is_window);
gboolean window_box_get_is_window (WindowBox *box);
void window_box_set_xwindow (WindowBox *box, guint window);
guint window_box_get_xwindow (WindowBox *box);
void window_box_set_name (WindowBox *box, const gchar *name);
const gchar *window_box_get_name (WindowBox *box);
void window_box_set_xclass (WindowBox *box, const gchar *xclass);
const gchar *window_box_get_xclass (WindowBox *box);
void window_box_update_name (WindowBox *box);
void window_box_update_xclass (WindowBox *box);
void window_box_setup_icon (WindowBox *box, guint req_width, guint req_height);
void window_box_set_colorize (WindowBox *box, gboolean colorize);
void window_box_set_show_desktop (WindowBox *box, gboolean show_desktop);
void window_box_set_font (WindowBox *box, const gchar *font, guint size);

void window_box_set_inner (WindowBox *box, int x, int y, int width, int height);
G_END_DECLS

#endif /* WINDOW_BOX_H */
