#ifndef MOSAIC_SEARCH_BOX_H
#define MOSAIC_SEARCH_BOX_H

#include <glib.h>
#include <string.h>
#include "mosaic_box.h"

G_BEGIN_DECLS

#define MOSAIC_TYPE_SEARCH_BOX            (mosaic_search_box_get_type ())
#define MOSAIC_SEARCH_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOSAIC_TYPE_SEARCH_BOX, MosaicSearchBox))
#define MOSAIC_SEARCH_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MOSAIC_TYPE_SEARCH_BOX, MosaicSearchBoxClass))
#define MOSAIC_IS_SEARCH_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOSAIC_TYPE_SEARCH_BOX))
#define MOSAIC_IS_SEARCH_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MOSAIC_TYPE_SEARCH_BOX))
#define MOSAIC_SEARCH_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MOSAIC_TYPE_SEARCH_BOX, MosaicSearchBoxClass))

typedef struct _MosaicSearchBox      MosaicSearchBox;
typedef struct _MosaicSearchBoxClass MosaicSearchBoxClass;

struct _MosaicSearchBox
{
  MosaicBox parent;

  /*< private >*/
  gchar cursor;
};

struct _MosaicSearchBoxClass
{
  MosaicBoxClass parent_class;

  void (* changed) (MosaicSearchBox *box);
};

GType mosaic_search_box_get_type (void);
GtkWidget *mosaic_search_box_new (void);
void mosaic_search_box_set_text (MosaicSearchBox *box, const gchar *text);
const gchar *mosaic_search_box_get_text (MosaicSearchBox *box);
void mosaic_search_box_append_text (MosaicSearchBox *box, const gchar *text);
void mosaic_search_box_remove_symbols (MosaicSearchBox *box, guint size);

G_END_DECLS

#endif /* MOSAIC_SEARCH_BOX_H */
