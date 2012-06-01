#include "window_box.h"

#define BOX_DEFAULT_WIDTH 200
#define BOX_DEFAULT_HEIGHT 40

enum {
  CLICKED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_IS_WINDOW,
  PROP_XWINDOW,
  PROP_NAME,
  PROP_XCLASS,
  N_PROPERTIES
};

static GObject*	window_box_constructor (GType gtype,
					guint n_properties,
					GObjectConstructParam *properties);
static void window_box_dispose (GObject *gobject);
static void window_box_set_property (GObject *gobject,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec);
static void window_box_get_property (GObject *gobject,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec);
static void window_box_realize (GtkWidget *widget);
static void window_box_size_request (GtkWidget * widget, GtkRequisition * requisition);
static void window_box_size_allocate (GtkWidget * widget,GtkAllocation * allocation);

static gboolean window_box_button_press (GtkWidget *widget, GdkEventButton *event);
static gboolean window_box_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean window_box_key_press (GtkWidget *widget, GdkEventKey *event);
static gboolean window_box_key_release (GtkWidget *widget, GdkEventKey *event);
static gboolean window_box_enter_notify (GtkWidget * widget, GdkEventCrossing * event);
static gboolean window_box_leave_notify (GtkWidget * widget, GdkEventCrossing * event);

static void window_box_clicked (WindowBox *box);

static gboolean window_box_expose (GtkWidget *box, GdkEventExpose *event);
static void window_box_paint (WindowBox *box, cairo_t *cr, gint width, gint height);

static void window_box_create_colors (WindowBox *box);

static guint box_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *obj_properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE (WindowBox, window_box, GTK_TYPE_DRAWING_AREA);

static void
window_box_class_init (WindowBoxClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->constructor = window_box_constructor;
  gobject_class->dispose = window_box_dispose;
  gobject_class->set_property = window_box_set_property;
  gobject_class->get_property = window_box_get_property;

  widget_class->realize = window_box_realize;
  widget_class->size_request = window_box_size_request;
  widget_class->size_allocate = window_box_size_allocate;
  widget_class->expose_event = window_box_expose;
  widget_class->button_press_event = window_box_button_press;
  widget_class->button_release_event = window_box_button_release;
  widget_class->key_press_event = window_box_key_press;
  widget_class->key_release_event = window_box_key_release;
  widget_class->enter_notify_event = window_box_enter_notify;
  widget_class->leave_notify_event = window_box_leave_notify;

  obj_properties[PROP_IS_WINDOW] =
    g_param_spec_boolean ("is-window",
			  "Is window",
			  "If set, the box stores XWindow information",
			  FALSE,
			  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  obj_properties[PROP_XWINDOW] =
    g_param_spec_uint ("xwindow",
		       "XWindow",
		       "XWindow ID",
		       0, G_MAXUINT,
		       0,
		       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
			 "Name in the box",
			 "Name displayed in the box",
			 NULL,
			 G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  obj_properties[PROP_XCLASS] =
    g_param_spec_string ("xclass",
			 "WM_CLASS",
			 "Class of the XWindow",
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
		  G_STRUCT_OFFSET (WindowBoxClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
window_box_init (WindowBox *box)
{
  gtk_widget_set_can_focus (GTK_WIDGET (box), TRUE);
  gtk_widget_set_receives_default (GTK_WIDGET (box), TRUE);

  box->name = NULL;
  box->xclass = NULL;
  box->icon_pixbuf = NULL;
  box->icon_surface = NULL;
  box->icon_context = NULL;
  box->font_name = NULL;
}

static GObject*	window_box_constructor (GType gtype,
					guint n_properties,
					GObjectConstructParam *properties)
{
  GObject *obj;
  WindowBox *box;

  obj = G_OBJECT_CLASS (window_box_parent_class)->constructor (gtype, n_properties, properties);

  box = WINDOW_BOX (obj);

  if (box->is_window) {
    box->name = get_window_name (box->xwindow);
    box->xclass = get_window_class (box->xwindow);
    box->desktop = get_window_desktop (box->xwindow);
  }
  box->font_name = g_strdup ("Sans");
  box->font_size = 10;
  box->show_desktop = FALSE;
  box->has_icon = FALSE;
  box->colorize = TRUE;
  box->color_offset = 0;
  window_box_create_colors (box);
  box->on_box = FALSE;
  box->box_down = FALSE;

  return obj;
}

GtkWidget* window_box_new (void)
{
  return g_object_new (WINDOW_TYPE_BOX, "is-window", FALSE, NULL);
}

GtkWidget* window_box_new_with_xwindow (Window win)
{
  return g_object_new (WINDOW_TYPE_BOX, "is-window", TRUE, "xwindow", win, NULL);
}

GtkWidget* window_box_new_with_name (gchar *name)
{
  return g_object_new (WINDOW_TYPE_BOX, "is-window", FALSE, "name", name, NULL);
}

static void
window_box_dispose (GObject *gobject)
{
  WindowBox *box = WINDOW_BOX (gobject);
  if (box->name) {
    g_free (box->name);
    box->name = NULL;
    if (box->xclass)
      g_free (box->xclass);
    box->xclass = NULL;

    if (box->icon_context) {
      cairo_destroy (box->icon_context);
      cairo_surface_destroy (box->icon_surface);
      box->icon_context = NULL;
      box->icon_surface = NULL;
    }
    if (box->icon_pixbuf)
      g_object_unref (box->icon_pixbuf);
    box->icon_pixbuf = NULL;

    if (box->font_name)
      g_free (box->font_name);
    box->font_name = NULL;
  }
  G_OBJECT_CLASS (window_box_parent_class)->dispose (gobject);
}

static void window_box_set_property (GObject *gobject,
				     guint prop_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
  WindowBox *box = WINDOW_BOX (gobject);

  switch (prop_id) {
  case PROP_IS_WINDOW:
    window_box_set_is_window (box, g_value_get_boolean (value));
    break;
  case PROP_XWINDOW:
    window_box_set_xwindow (box, g_value_get_uint (value));
    break;
  case PROP_NAME:
    window_box_set_name (box, g_value_get_string (value));
    break;
  case PROP_XCLASS:
    window_box_set_xclass (box, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void window_box_get_property (GObject *gobject,
				     guint prop_id,
				     GValue *value,
				     GParamSpec *pspec)
{
  WindowBox *box = WINDOW_BOX (gobject);

  switch (prop_id) {
  case PROP_IS_WINDOW:
    g_value_set_boolean (value, box->is_window);
    break;
  case PROP_XWINDOW:
    g_value_set_uint (value, box->xwindow);
    break;
  case PROP_NAME:
    g_value_set_string (value, box->name);
    break;
  case PROP_XCLASS:
    g_value_set_string (value, box->xclass);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    break;
  }
}

static void window_box_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (WINDOW_IS_BOX (widget));

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

static void window_box_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  requisition->width = BOX_DEFAULT_WIDTH;
  requisition->height = BOX_DEFAULT_HEIGHT;
}

static void window_box_size_allocate (GtkWidget * widget,GtkAllocation * allocation)
{
  g_return_if_fail (WINDOW_IS_BOX (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (widget->window,
			    allocation->x, allocation->y,
			    allocation->width, allocation->height);
  }
}

static gboolean window_box_button_press (GtkWidget *widget, GdkEventButton *event)
{
  WindowBox *box;

  if (event->type == GDK_BUTTON_PRESS)
  {
    box = WINDOW_BOX (widget);
    if (event->button == 1) {
      box->box_down = TRUE;
      gtk_widget_grab_focus (widget);
    }
  }

  return TRUE;
}

static gboolean window_box_button_release (GtkWidget *widget, GdkEventButton *event)
{
  WindowBox *box;

  if (event->type == GDK_BUTTON_RELEASE)
  {
    box = WINDOW_BOX (widget);
    if (event->button == 1 && box->on_box && box->box_down)
      window_box_clicked (WINDOW_BOX (widget));
    box->box_down = FALSE;
  }

  return TRUE;
}
static gboolean window_box_key_press (GtkWidget *widget, GdkEventKey *event)
{
  WindowBox *box = WINDOW_BOX (widget);
  if (event->keyval == GDK_Return ||
      event->keyval == GDK_KP_Enter ||
      event->keyval == GDK_ISO_Enter)
  {
    box->box_down = TRUE;
  }
  return FALSE;
}

static gboolean window_box_key_release (GtkWidget *widget, GdkEventKey *event)
{
  WindowBox *box = WINDOW_BOX (widget);
  if ((event->keyval == GDK_Return ||
       event->keyval == GDK_KP_Enter ||
       event->keyval == GDK_ISO_Enter) && box->box_down)
  {
    window_box_clicked (box);
    box->box_down = FALSE;
  }
  return FALSE;
}

static gboolean window_box_enter_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  WindowBox *box;
  GtkWidget *event_widget;

  box = WINDOW_BOX (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) && (event->detail != GDK_NOTIFY_INFERIOR)) {
    box->on_box = TRUE;
    gtk_widget_queue_draw (widget);
  }

  return FALSE;
}

static gboolean window_box_leave_notify (GtkWidget * widget, GdkEventCrossing * event)
{
  WindowBox *box;
  GtkWidget *event_widget;

  box = WINDOW_BOX (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) && (event->detail != GDK_NOTIFY_INFERIOR)) {
    box->on_box = FALSE;
    gtk_widget_queue_draw (widget);
  }

  return FALSE;
}

void window_box_clicked (WindowBox *box)
{
  g_signal_emit (box, box_signals [CLICKED], 0);
}

static gboolean
window_box_expose (GtkWidget *box, GdkEventExpose *event)
{
  g_return_val_if_fail (WINDOW_IS_BOX (box), FALSE);

  cairo_t *cr;
  cr = gdk_cairo_create (box->window);
  cairo_rectangle (cr,
		   event->area.x, event->area.y,
		   event->area.width, event->area.height);
  cairo_clip (cr);
  window_box_paint (WINDOW_BOX (box), cr, box->allocation.width, box->allocation.height);
  cairo_destroy (cr);
  return FALSE;
}

static void
window_box_paint (WindowBox *box, cairo_t *cr, gint width, gint height)
{
  gboolean has_focus = gtk_widget_has_focus (GTK_WIDGET (box));
  if (box->on_box)
    cairo_set_source_rgb (cr, box->r-0.2, box->g-0.2, box->b-0.2);
  else if (has_focus)
    cairo_set_source_rgb (cr, box->r-0.4, box->g-0.4, box->b-0.4);
  else
    cairo_set_source_rgb (cr, box->r, box->g, box->b);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill_preserve (cr);

  // Draw border
  if (has_focus) {
//    cairo_set_source_rgb (cr, box->b, box->g, box->r);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_set_line_width (cr, 4);
    cairo_stroke_preserve (cr);
  }
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  cairo_text_extents_t extents;

  /* Shall we draw the desktop number */
  if (box->is_window && box->show_desktop) {
    gchar desk [4] = { 0 };
    sprintf (desk, "%d", box->desktop+1);
    if (has_focus)
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
    else
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.5);

    cairo_select_font_face (cr, "Sans",
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, height);
    cairo_text_extents (cr, desk, &extents);
    cairo_move_to (cr, (width - extents.width)/2, (height + extents.height)/2);
    cairo_show_text (cr, desk);
  }

  if (has_focus)
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
  else
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
  cairo_select_font_face (cr, box->font_name,
			  CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, box->font_size);
  cairo_text_extents (cr, box->name, &extents);


  if (box->has_icon) {
    if (box->icon_pixbuf) {
      guint iwidth = gdk_pixbuf_get_width (box->icon_pixbuf);
      guint iheight = gdk_pixbuf_get_height (box->icon_pixbuf);
      cairo_save (cr);
      cairo_set_source_surface (cr, box->icon_surface, 5, (height-iheight)/2);
      cairo_rectangle (cr, 0, 0,
		       iwidth+5,
		       (height+iheight)/2);
      cairo_clip (cr);
      cairo_paint (cr);
      cairo_restore (cr);

      if ((width-extents.width)/2 > iwidth+5)
	cairo_move_to (cr, (width - extents.width)/2, (height + extents.height)/2);
      else
	cairo_move_to (cr, 10+iwidth, (height + extents.height)/2);
    }
  } else {
    if (width-5 > extents.width)
      cairo_move_to (cr, (width - extents.width)/2, (height + extents.height)/2);
    else
      cairo_move_to (cr, 5, (height + extents.height)/2);
  }
  cairo_show_text (cr, box->name);
}

void
window_box_set_is_window (WindowBox *box, gboolean is_window)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  box->is_window = is_window;

  g_object_notify (G_OBJECT (box), "is-window");
}

gboolean
window_box_get_is_window (WindowBox *box)
{
  g_return_val_if_fail (WINDOW_IS_BOX (box), FALSE);

  return box->is_window;
}

void
window_box_set_xwindow (WindowBox *box, guint window)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  if (box->is_window) {
    box->xwindow = window;

    g_object_notify (G_OBJECT (box), "xwindow");
    box->desktop = get_window_desktop (box->xwindow);
    window_box_update_name (box);
    window_box_update_xclass (box);
  }
}

guint
window_box_get_xwindow (WindowBox *box)
{
  g_return_val_if_fail (WINDOW_IS_BOX (box), 0);

  return box->xwindow;
}

void
window_box_set_name (WindowBox *box, const gchar *name)
{
  gchar *new_name;
  g_return_if_fail (WINDOW_IS_BOX (box));

  new_name = g_strdup (name);
  g_free (box->name);
  box->name = new_name;

  g_object_notify (G_OBJECT (box), "name");
  gtk_widget_queue_draw (GTK_WIDGET (box));
}

const gchar *
window_box_get_name (WindowBox *box)
{
  g_return_val_if_fail (WINDOW_IS_BOX (box), NULL);

  return box->name;
}

void
window_box_set_xclass (WindowBox *box, const gchar *xclass)
{
  gchar *new_xclass;
  g_return_if_fail (WINDOW_IS_BOX (box));

  new_xclass = g_strdup (xclass);
  g_free (box->xclass);
  box->xclass = new_xclass;

  g_object_notify (G_OBJECT (box), "xclass");
  window_box_create_colors (box);
  gtk_widget_queue_draw (GTK_WIDGET (box));
}

const gchar *
window_box_get_xclass (WindowBox *box)
{
  g_return_val_if_fail (WINDOW_IS_BOX (box), NULL);

  return box->xclass;
}

void window_box_update_name (WindowBox *box)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  if (box->is_window) {
    gchar *wname = get_window_name (box->xwindow);
    window_box_set_name (box, wname);
    g_free (wname);
  }
}

void window_box_update_xclass (WindowBox *box)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  if (box->is_window) {
    gchar *xclass = get_window_class (box->xwindow);
    window_box_set_xclass (box, xclass);
    g_free (xclass);
  }
}

static gushort get_crc16 (gchar *octets, guint len)
{
  gushort crc16_poly = 0x8408;

  gushort data;
  gushort crc = 0xffff;
  if (len == 0)
    return (~crc);

  do {
    for (int i =0, data= (guint)0xff & *octets++;
	 i < 8;
	 i++, data >>= 1) {
      if ((crc & 0x0001) ^ (data & 0x0001))
	crc = (crc >> 1) ^ crc16_poly;
      else
	crc >>= 1;
    }
  } while (--len);

  crc = ~crc;
  data = crc;
  crc = (crc << 8) | (data >> 8 & 0xff);

  return (crc);
}

static gdouble hue2rgb (gdouble p, gdouble q, gdouble t)
{
  if (t < 0.0)
    t += 1.0;
  if (t > 1.0)
    t -= 1.0;
  if (t < 1.0/6.0)
    return (p + (q - p) * 6.0 * t);
  if (t < 1.0/2.0)
    return q;
  if (t < 2.0/3.0)
    return (p + (q - p) * (2.0/3.0 - t) * 6.0);
  return p;
}

static void window_box_create_colors (WindowBox *box)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  gchar *source = (box->is_window) ? box->xclass : box->name;
  if (box->colorize && source) {
    gdouble h, s, l;
    gulong crc = get_crc16 (source, strlen (source));
    guchar pre_h = (((crc >> 8) & 0xFF) + box->color_offset) % 256;
    guchar pre_s = ((crc << 0) & 0xFF);
    h = pre_h / 255.0;
    s = 0.5 + pre_s / 512.0;
    l = 0.6;

    gdouble q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    gdouble p = 2.0 * l - q;
    box->r = hue2rgb (p, q, h + 1.0/3.0);
    box->g = hue2rgb (p, q, h);
    box->b = hue2rgb (p, q, h - 1.0/3.0);
  } else {
    box->r = box->g = box->b = 0.6;
  }
}

void window_box_setup_icon (WindowBox *box, guint req_width, guint req_height)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  if (box->icon_pixbuf)
    g_object_unref (box->icon_pixbuf);
  if (box->icon_surface)
    cairo_surface_destroy (box->icon_surface);
  if (box->icon_context)
    cairo_destroy (box->icon_context);

  box->icon_pixbuf = get_window_icon (box->xwindow, req_width, req_height);
  if (box->icon_pixbuf) {
    box->has_icon = TRUE;
    box->icon_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						    gdk_pixbuf_get_width (box->icon_pixbuf),
						    gdk_pixbuf_get_height (box->icon_pixbuf));
    box->icon_context = cairo_create (box->icon_surface);
    gdk_cairo_set_source_pixbuf (box->icon_context, box->icon_pixbuf, 0, 0);
    cairo_paint (box->icon_context);
  } else {
    box->has_icon = FALSE;
  }
}

void window_box_set_colorize (WindowBox *box, gboolean colorize)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  box->colorize = colorize;
  window_box_create_colors (box);
}

void window_box_set_show_desktop (WindowBox *box, gboolean show_desktop)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  box->show_desktop = show_desktop;
}

void window_box_set_font (WindowBox *box, const gchar *font, guint size)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  if (box->font_name)
    g_free (box->font_name);

  box->font_name = g_strdup (font);
  box->font_size = size;
}

void window_box_set_inner (WindowBox *box, int x, int y, int width, int height)
{
  box->x = x;
  box->y = y;
  box->width = width;
  box->height = height;
}

void window_box_set_color_offset (WindowBox *box, guchar color_offset)
{
  g_return_if_fail (WINDOW_IS_BOX (box));

  box->color_offset = color_offset;
  window_box_create_colors (box);
}
