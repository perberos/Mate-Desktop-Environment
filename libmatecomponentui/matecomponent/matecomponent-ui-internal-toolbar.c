#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <gtk/gtk.h>
#include <matecomponent/matecomponent-ui-private.h>

typedef struct {
	GtkToolbar     parent;

	gboolean       got_size;
	GtkRequisition full_size;
} InternalToolbar;

typedef struct {
	GtkToolbarClass parent_class;
} InternalToolbarClass;

GType internal_toolbar_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE(InternalToolbar, internal_toolbar, GTK_TYPE_TOOLBAR)

enum {
	PROP_0,
	PROP_IS_FLOATING,
	PROP_ORIENTATION,
	PROP_PREFERRED_WIDTH,
	PROP_PREFERRED_HEIGHT
};

static void
get_full_size (InternalToolbar *toolbar)
{
	if (!toolbar->got_size) {
		gboolean show_arrow;

		toolbar->got_size = TRUE;

		show_arrow = gtk_toolbar_get_show_arrow (GTK_TOOLBAR (toolbar));
		if (show_arrow) /* Not an elegant approach, sigh. */
			g_object_set (toolbar, "show_arrow", FALSE, NULL);

		gtk_widget_size_request (GTK_WIDGET (toolbar), &toolbar->full_size);

		if (show_arrow)
			g_object_set (toolbar, "show_arrow", TRUE, NULL);
	}
}

static void
invalidate_size (InternalToolbar *toolbar)
{
	toolbar->got_size = FALSE;
}

GList *
matecomponent_ui_internal_toolbar_get_children (GtkWidget *toolbar)
{
	int i, n_items = 0;
	GList *ret = NULL;

	n_items = gtk_toolbar_get_n_items (GTK_TOOLBAR (toolbar));

	for (i = 0; i < n_items; i++) {
		GtkWidget *child;
		GtkToolItem *item = gtk_toolbar_get_nth_item  (GTK_TOOLBAR (toolbar), i);
		if ((child = GTK_BIN (item)->child) && MATECOMPONENT_IS_UI_TOOLBAR_ITEM (child))
			ret = g_list_prepend (ret, child);
		else
			ret = g_list_prepend (ret, item);
	}

	return g_list_reverse (ret);
}

static void
set_attributes_on_child (MateComponentUIToolbarItem *item,
                         GtkOrientation       orientation,
                         GtkToolbarStyle      style)
{
	matecomponent_ui_toolbar_item_set_orientation (item, orientation);

	switch (style) {
	case GTK_TOOLBAR_BOTH_HORIZ:
		if (! matecomponent_ui_toolbar_item_get_want_label (item))
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		else if (orientation == GTK_ORIENTATION_HORIZONTAL)
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	case GTK_TOOLBAR_BOTH:
		if (orientation == GTK_ORIENTATION_VERTICAL)
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	case GTK_TOOLBAR_ICONS:
		matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		break;
	case GTK_TOOLBAR_TEXT:
		matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY);
		break;
	default:
		g_assert_not_reached ();
	}
}

static void
impl_get_property (GObject    *object,
		   guint       property_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	InternalToolbar *toolbar = (InternalToolbar *) object;

	get_full_size (toolbar);

	switch (property_id) {
	case PROP_PREFERRED_WIDTH:
		g_value_set_uint (value, toolbar->full_size.width);
		break;
	case PROP_PREFERRED_HEIGHT:
		g_value_set_uint (value, toolbar->full_size.height);
		break;
	default:
		break;
	};
}

static void
impl_set_property (GObject      *object,
		   guint         property_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	GtkToolbar *toolbar = GTK_TOOLBAR (object);

	invalidate_size ((InternalToolbar *) toolbar);

	switch (property_id) {
	case PROP_ORIENTATION:
		gtk_toolbar_set_orientation (toolbar, 
					     g_value_get_enum (value));
		break;
	case PROP_IS_FLOATING:
		gtk_toolbar_set_show_arrow (toolbar, !g_value_get_boolean (value));
		break;
	default:
		break;
	};
}

static void
impl_orientation_changed (GtkToolbar *widget,
			  GtkOrientation orientation)
{
	InternalToolbar *toolbar = (InternalToolbar *) widget;

	toolbar->got_size = FALSE;

	GTK_TOOLBAR_CLASS (internal_toolbar_parent_class)->orientation_changed (widget, orientation);
}

static void
impl_style_changed (GtkToolbar *toolbar,
		    GtkToolbarStyle style)
{
	GList *items, *l;
	GtkOrientation orientation;

	invalidate_size ((InternalToolbar *) toolbar);

	items = matecomponent_ui_internal_toolbar_get_children (GTK_WIDGET (toolbar));

	orientation = gtk_toolbar_get_orientation (GTK_TOOLBAR (toolbar));

	for (l = items; l != NULL; l = l->next) {

		if (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (l->data))
			set_attributes_on_child (l->data, orientation, style);
	}

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));

	GTK_TOOLBAR_CLASS (internal_toolbar_parent_class)->style_changed (toolbar, style);
	g_list_free (items);
}

static void
internal_toolbar_class_init (InternalToolbarClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;
	GtkToolbarClass *toolbar_class = (GtkToolbarClass *) klass;

	gobject_class->get_property = impl_get_property;
	gobject_class->set_property = impl_set_property;

	toolbar_class->orientation_changed = impl_orientation_changed;
	toolbar_class->style_changed = impl_style_changed;

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_WIDTH,
		g_param_spec_uint ("preferred_width", NULL, NULL,
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_HEIGHT,
		g_param_spec_uint ("preferred_height",
				   NULL, NULL,
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_IS_FLOATING,
		g_param_spec_boolean ("is_floating",
				      NULL, NULL,
				      FALSE, G_PARAM_WRITABLE));
}

static void
internal_toolbar_init (InternalToolbar *toolbar)
{
	g_signal_connect (toolbar, "add",
			  G_CALLBACK (invalidate_size), NULL);
	g_signal_connect (toolbar, "remove",
			  G_CALLBACK (invalidate_size), NULL);
}

GtkWidget *
matecomponent_ui_internal_toolbar_new (void)
{
	return g_object_new (internal_toolbar_get_type(), NULL);
}
