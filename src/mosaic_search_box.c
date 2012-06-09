/* Copyright (c) 2012, Anton S. Lobashev
 * mosaic_seach_box.c - search box widget.
 */

#include "mosaic_search_box.h"

#define BOX_DEFAULT_WIDTH 200

enum {
  CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TEXT,
  N_PROPERTIES
};

static GObject*	mosaic_search_box_constructor (GType gtype,
					       guint n_properties,
					       GObjectConstructParam *properties);
static void mosaic_search_box_dispose (GObject *gobject);

static void mosaic_search_box_set_property (GObject *gobject,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void mosaic_search_box_get_property (GObject *gobject,
					    guint prop_id,
					    GValue *value,
					    GParamSpec *pspec);

static gboolean mosaic_search_box_expose_event (GtkWidget *widget, GdkEventExpose *event);
static void mosaic_search_box_paint (MosaicSearchBox *box, cairo_t *cr, gint width, gint height);
static void mosaic_search_box_size_request (GtkWidget *widget, GtkRequisition *requisition);

static guint search_box_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE (MosaicSearchBox, mosaic_search_box, MOSAIC_TYPE_BOX);

static void
mosaic_search_box_class_init (MosaicSearchBoxClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->constructor = mosaic_search_box_constructor;
  gobject_class->dispose = mosaic_search_box_dispose;
  gobject_class->set_property = mosaic_search_box_set_property;
  gobject_class->get_property = mosaic_search_box_get_property;

  widget_class->expose_event = mosaic_search_box_expose_event;
  widget_class->size_request = mosaic_search_box_size_request;

  obj_properties[PROP_TEXT] =
    g_param_spec_string ("text",
			 "Text in the search entry",
			 "Text, displayed in search entry",
			 NULL,
			 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class, PROP_TEXT, obj_properties [PROP_TEXT]);

  search_box_signals [CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (MosaicSearchBoxClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

}

static void
mosaic_search_box_init (MosaicSearchBox *box)
{
  gtk_widget_set_can_focus (GTK_WIDGET (box), FALSE);
  gtk_widget_set_receives_default (GTK_WIDGET (box), FALSE);
}

static GObject*	mosaic_search_box_constructor (GType gtype,
					guint n_properties,
					GObjectConstructParam *properties)
{
  GObject *obj;
  MosaicSearchBox *box;

  obj = G_OBJECT_CLASS (mosaic_search_box_parent_class)->constructor (gtype, n_properties, properties);

  box = MOSAIC_SEARCH_BOX (obj);

  MOSAIC_BOX (box)->name = g_strdup("\0");
  box->cursor = g_strdup ("|");

  return obj;
}

GtkWidget* mosaic_search_box_new (void)
{
  return g_object_new (MOSAIC_TYPE_SEARCH_BOX, NULL);
}

static void
mosaic_search_box_dispose (GObject *gobject)
{
  MosaicSearchBox *box = MOSAIC_SEARCH_BOX (gobject);

  if (box->cursor)
    g_free (box->cursor);
  box->cursor = NULL;

  G_OBJECT_CLASS (mosaic_search_box_parent_class)->dispose (gobject);
}

static void mosaic_search_box_set_property (GObject *gobject,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec)
{
  MosaicSearchBox *box = MOSAIC_SEARCH_BOX (gobject);

  switch (prop_id) {
  case PROP_TEXT:
    mosaic_search_box_set_text (box, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void mosaic_search_box_get_property (GObject *gobject,
					    guint prop_id,
					    GValue *value,
					    GParamSpec *pspec)
{
  MosaicSearchBox *box = MOSAIC_SEARCH_BOX (gobject);

  switch (prop_id) {
  case PROP_TEXT:
    g_value_set_string (value, MOSAIC_BOX(box)->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static gboolean
mosaic_search_box_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  g_return_val_if_fail (MOSAIC_IS_SEARCH_BOX (widget), FALSE);

  cairo_t *cr;
  cr = gdk_cairo_create (widget->window);
  cairo_rectangle (cr,
		   event->area.x, event->area.y,
		   event->area.width, event->area.height);
  cairo_clip (cr);
  mosaic_search_box_paint (MOSAIC_SEARCH_BOX (widget), cr, widget->allocation.width, widget->allocation.height);
  cairo_destroy (cr);
  return TRUE;
}

static void
mosaic_search_box_paint (MosaicSearchBox *box, cairo_t *cr, gint width, gint height)
{
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  PangoLayout *pl;
  PangoFontDescription *pfd;
  pl = pango_cairo_create_layout (cr);
  pango_layout_set_text (pl, MOSAIC_BOX (box)->name, -1);
  pfd = pango_font_description_from_string (MOSAIC_BOX (box)->font);
  pango_layout_set_font_description (pl, pfd);
  pango_font_description_free (pfd);

  int pwidth, pheight;
  pango_layout_get_pixel_size (pl, &pwidth, &pheight);

  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);

  if ((width-pwidth) < 10)
    cairo_move_to (cr, width-pwidth-5, (height-pheight)/2);
  else
    cairo_move_to (cr, 5, (height-pheight)/2);

  pango_cairo_show_layout (cr, pl);
  g_object_unref (pl);

  if ((width-pwidth) < 10)
    cairo_rectangle (cr, width-4, 5, 2, height-10);
  else
    cairo_rectangle (cr, pwidth+6, 5, 2, height-10);
  cairo_fill (cr);

  mosaic_box_paint (MOSAIC_BOX (box), cr, width, height);
}

static void mosaic_search_box_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  PangoLayout *pl = gtk_widget_create_pango_layout (widget, "|");
  PangoFontDescription *pfd = pango_font_description_from_string (MOSAIC_BOX (widget)->font);
  pango_layout_set_font_description (pl, pfd);
  pango_font_description_free (pfd);

  int pwidth, pheight;
  pango_layout_get_pixel_size (pl, &pwidth, &pheight);

  requisition->width = BOX_DEFAULT_WIDTH;
  requisition->height = pheight + 10;
  g_object_unref (pl);
}

void
mosaic_search_box_set_text (MosaicSearchBox *box, const gchar *text)
{
  mosaic_box_set_name (MOSAIC_BOX (box), text);
  g_signal_emit (box, search_box_signals [CHANGED], 0);
}

const gchar *
mosaic_search_box_get_text (MosaicSearchBox *box)
{
  g_return_val_if_fail (MOSAIC_IS_SEARCH_BOX (box), NULL);

  return MOSAIC_BOX (box)->name;
}

void mosaic_search_box_append_text (MosaicSearchBox *box, const gchar *text)
{
  gchar *new_text = g_strjoin (NULL, mosaic_search_box_get_text (box), text, NULL);
  mosaic_search_box_set_text (box, new_text);
  g_free (new_text);
}

void mosaic_search_box_remove_symbols (MosaicSearchBox *box, guint size)
{
  gchar *text = g_strdup (mosaic_search_box_get_text (box));
  if (strlen (text)) {
    gchar *p = text + strlen (text);
    for (int i = 0; i < size; i++) {
      p = g_utf8_find_prev_char (text, p);
      *p = '\0';
    }
    mosaic_search_box_set_text (box, text);
  }
  g_free (text);
}
