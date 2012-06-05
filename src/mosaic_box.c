#include "mosaic_box.h"

enum {
  CLICKED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  N_PROPERTIES
};

static GObject *mosaic_box_constructor (GType gtype,
					guint n_properties,
					GObjectConstructParam *properties);
static void mosaic_box_dispose (GObject *gobject);
static void mosaic_box_set_property (GObject *gobject,
					    guint prop_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void mosaic_box_get_property (GObject *gobject,
					    guint prop_id,
					    GValue *value,
					    GParamSpec *pspec);
static void mosaic_box_realize (GtkWidget *widget);
static void mosaic_box_size_request (GtkWidget * widget, GtkRequisition * requisition);
static void mosaic_box_size_allocate (GtkWidget * widget,GtkAllocation * allocation);

static gboolean mosaic_box_button_press (GtkWidget *widget, GdkEventButton *event);
static gboolean mosaic_box_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean mosaic_box_key_press (GtkWidget *widget, GdkEventKey *event);
static gboolean mosaic_box_key_release (GtkWidget *widget, GdkEventKey *event);
static gboolean mosaic_box_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean mosaic_box_leave_notify (GtkWidget *widget, GdkEventCrossing *event);

static void mosaic_box_clicked (MosaicBox *box);

static guint box_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE (MosaicBox, mosaic_box, GTK_TYPE_DRAWING_AREA);

static void
mosaic_box_class_init (MosaicBoxClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->constructor = mosaic_box_constructor;
  gobject_class->dispose = mosaic_box_dispose;
  gobject_class->set_property = mosaic_box_set_property;
  gobject_class->get_property = mosaic_box_get_property;

  widget_class->realize = mosaic_box_realize;
  widget_class->size_request = mosaic_box_size_request;
  widget_class->size_allocate = mosaic_box_size_allocate;
  widget_class->button_press_event = mosaic_box_button_press;
  widget_class->button_release_event = mosaic_box_button_release;
  widget_class->key_press_event = mosaic_box_key_press;
  widget_class->key_release_event = mosaic_box_key_release;
  widget_class->enter_notify_event = mosaic_box_enter_notify;
  widget_class->leave_notify_event = mosaic_box_leave_notify;

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
			 "Name in the box",
			 "Name displayed in the box",
			 NULL,
			 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
				     N_PROPERTIES,
				     obj_properties);

  klass->clicked = NULL;

  box_signals [CLICKED] =
    g_signal_new ("clicked",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (MosaicBoxClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
mosaic_box_init (MosaicBox *box)
{
  gtk_widget_set_can_focus (GTK_WIDGET (box), TRUE);
  gtk_widget_set_receives_default (GTK_WIDGET (box), TRUE);

  box->name = NULL;
}

static GObject*	mosaic_box_constructor (GType gtype,
					guint n_properties,
					GObjectConstructParam *properties)
{
  GObject *obj;
  MosaicBox *box;

  obj = G_OBJECT_CLASS (mosaic_box_parent_class)->constructor (gtype, n_properties, properties);

  box = MOSAIC_BOX (obj);

  box->font_name = g_strdup ("Sans");
  box->font_size = 10;
  box->on_box = FALSE;
  box->box_down = FALSE;

  return obj;
}

GtkWidget* mosaic_box_new (void)
{
  return g_object_new (MOSAIC_TYPE_BOX, NULL);
}

GtkWidget* mosaic_box_new_with_name (gchar *name)
{
  return g_object_new (MOSAIC_TYPE_BOX, "name", name, NULL);
}

static void
mosaic_box_dispose (GObject *gobject)
{
  MosaicBox *box = MOSAIC_BOX (gobject);
  if (box->name)
    g_free (box->name);
  box->name = NULL;

  if (box->font_name)
    g_free (box->font_name);
  box->font_name = NULL;

  G_OBJECT_CLASS (mosaic_box_parent_class)->dispose (gobject);
}

static void mosaic_box_set_property (GObject *gobject,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  MosaicBox *box = MOSAIC_BOX (gobject);

  switch (prop_id) {
  case PROP_NAME:
    mosaic_box_set_name (box, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void mosaic_box_get_property (GObject *gobject,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  MosaicBox *box = MOSAIC_BOX (gobject);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string (value, box->name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void mosaic_box_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (MOSAIC_IS_BOX (widget));

  gtk_widget_set_realized (widget, TRUE);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK);

  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);

  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);
}

static void mosaic_box_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  requisition->width = BOX_DEFAULT_WIDTH;
  requisition->height = BOX_DEFAULT_HEIGHT;
}

static void mosaic_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  g_return_if_fail (MOSAIC_IS_BOX (widget));
  g_return_if_fail (allocation != NULL);

  GTK_WIDGET_CLASS (mosaic_box_parent_class)->size_allocate (widget, allocation);

  widget->allocation = *allocation;
  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);
  }
}

static gboolean mosaic_box_button_press (GtkWidget *widget, GdkEventButton *event)
{
  MosaicBox *box;

  if (event->type == GDK_BUTTON_PRESS && gtk_widget_get_can_focus (widget))
  {
    box = MOSAIC_BOX (widget);
    if (event->button == 1) {
      box->box_down = TRUE;
      gtk_widget_grab_focus (widget);
    }
  }

  return TRUE;
}

static gboolean mosaic_box_button_release (GtkWidget *widget, GdkEventButton *event)
{
  MosaicBox *box;

  if (event->type == GDK_BUTTON_RELEASE && gtk_widget_get_can_focus (widget))
  {
    box = MOSAIC_BOX (widget);
    if (event->button == 1 && box->on_box && box->box_down)
      mosaic_box_clicked (MOSAIC_BOX (widget));
    box->box_down = FALSE;
  }

  return TRUE;
}
static gboolean mosaic_box_key_press (GtkWidget *widget, GdkEventKey *event)
{
  MosaicBox *box = MOSAIC_BOX (widget);
  if ((event->keyval == GDK_Return ||
      event->keyval == GDK_KP_Enter ||
      event->keyval == GDK_ISO_Enter) &&
      gtk_widget_get_can_focus (widget))
  {
    box->box_down = TRUE;
  }
  return FALSE;
}

static gboolean mosaic_box_key_release (GtkWidget *widget, GdkEventKey *event)
{
  MosaicBox *box = MOSAIC_BOX (widget);
  if ((event->keyval == GDK_Return ||
       event->keyval == GDK_KP_Enter ||
       event->keyval == GDK_ISO_Enter) &&
      box->box_down &&
      gtk_widget_get_can_focus (widget))
  {
    mosaic_box_clicked (box);
    box->box_down = FALSE;
  }
  return FALSE;
}

static gboolean mosaic_box_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  MosaicBox *box;
  GtkWidget *event_widget;

  box = MOSAIC_BOX (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) && (event->detail != GDK_NOTIFY_INFERIOR)) {
    box->on_box = TRUE;
    gtk_widget_queue_draw (widget);
  }

  return FALSE;
}

static gboolean mosaic_box_leave_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  MosaicBox *box;
  GtkWidget *event_widget;

  box = MOSAIC_BOX (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) && (event->detail != GDK_NOTIFY_INFERIOR)) {
    box->on_box = FALSE;
    gtk_widget_queue_draw (widget);
  }

  return FALSE;
}

void mosaic_box_clicked (MosaicBox *box)
{
  g_signal_emit (box, box_signals [CLICKED], 0);
}

void
mosaic_box_set_name (MosaicBox *box, const gchar *name)
{
  gchar *new_name;
  g_return_if_fail (MOSAIC_IS_BOX (box));

  new_name = g_strdup (name);
  g_free (box->name);
  box->name = new_name;

  g_object_notify (G_OBJECT (box), "name");
  gtk_widget_queue_draw (GTK_WIDGET (box));
}

const gchar *
mosaic_box_get_name (MosaicBox *box)
{
  g_return_val_if_fail (MOSAIC_IS_BOX (box), NULL);

  return box->name;
}

void mosaic_box_paint (MosaicBox *box, cairo_t *cr, gint width, gint height, gboolean ralign)
{

}
