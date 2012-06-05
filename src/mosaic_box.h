#ifndef MOSAIC_BOX_H
#define MOSAIC_BOX_H

#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

G_BEGIN_DECLS

#define MOSAIC_TYPE_BOX            (mosaic_box_get_type ())
#define MOSAIC_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOSAIC_TYPE_BOX, MosaicBox))
#define MOSAIC_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOSAIC_TYPE_BOX, MosaicBoxClass))
#define MOSAIC_IS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOSAIC_TYPE_BOX))
#define MOSAIC_IS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOSAIC_TYPE_BOX))
#define MOSAIC_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOSAIC_TYPE_BOX, MosaicBoxClass))

typedef struct _MosaicBox      MosaicBox;
typedef struct _MosaicBoxClass MosaicBoxClass;

struct _MosaicBox
{
  GtkDrawingArea parent;

  /*< private >*/
  gchar *name;

  gboolean on_box;
  gboolean box_down;

  gchar *font_name;
  guint font_size;
};

struct _MosaicBoxClass
{
  GtkDrawingAreaClass parent_class;

  void (* clicked)  (MosaicBox *box);
};

#define BOX_DEFAULT_WIDTH 200
#define BOX_DEFAULT_HEIGHT 40

GType mosaic_box_get_type (void);
GtkWidget* mosaic_box_new (void);
void mosaic_box_set_name (MosaicBox *box, const gchar *name);
const gchar *mosaic_box_get_name (MosaicBox *box);

void mosaic_box_paint (MosaicBox *box, cairo_t *cr, gint width, gint height, gboolean ralign);

G_END_DECLS

#endif /* MOSAIC_BOX_H */
