#include "mosaic_search_box.h"

enum {
  PROP_0,
  PROP_NAME,
  N_PROPERTIES
};

static GObject*	mosaic_search_box_constructor (GType gtype,
					       guint n_properties,
					       GObjectConstructParam *properties);
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
  gobject_class->set_property = mosaic_search_box_set_property;
  gobject_class->get_property = mosaic_search_box_get_property;

  widget_class->expose_event = mosaic_search_box_expose_event;

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
			 "Name in the box",
			 "Name displayed in the box",
			 NULL,
			 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
				     N_PROPERTIES,
				     obj_properties);
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

  box->cursor = '|';

  return obj;
}

GtkWidget* mosaic_search_box_new (void)
{
  return g_object_new (MOSAIC_TYPE_SEARCH_BOX, NULL);
}

static void mosaic_search_box_set_property (GObject *gobject,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec)
{
  MosaicSearchBox *box = MOSAIC_SEARCH_BOX (gobject);

  switch (prop_id) {
  case PROP_NAME:
    mosaic_search_box_set_name (box, g_value_get_string (value));
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
  case PROP_NAME:
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
  return FALSE;
}

static void
mosaic_search_box_paint (MosaicSearchBox *box, cairo_t *cr, gint width, gint height)
{
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  mosaic_box_paint (MOSAIC_BOX (box), cr, width, height, 0, TRUE);
}

void
mosaic_search_box_set_name (MosaicSearchBox *box, const gchar *name)
{
  mosaic_box_set_name (MOSAIC_BOX (box), name);
}

const gchar *
mosaic_search_box_get_name (MosaicSearchBox *box)
{
  g_return_val_if_fail (MOSAIC_IS_SEARCH_BOX (box), NULL);

  return MOSAIC_BOX (box)->name;
}
