/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-control-item.c: A special toolbar item for controls.
 *
 * Author:
 *	Jon K Hellan (hellan@acm.org)
 *
 * Copyright 2000 Jon K Hellan.
 * Copyright (C) 2001 Eazel, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-toolbar-control-item.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-property-bag-client.h>

G_DEFINE_TYPE (MateComponentUIToolbarControlItem,
	       matecomponent_ui_toolbar_control_item,
	       GTK_TYPE_TOOL_BUTTON)

static void
set_control_property_bag_value (MateComponentUIToolbarControlItem *item,
				const char                 *name,
				MateComponentArg                  *value)
{
	MateComponentControlFrame *frame;
	MateComponent_PropertyBag bag;

	if (!item->control)
		return;

	frame = matecomponent_widget_get_control_frame (item->control);
	if (!frame)
		return;

	bag = matecomponent_control_frame_get_control_property_bag (frame, NULL);
	if (bag == CORBA_OBJECT_NIL)
		return;

	matecomponent_pbclient_set_value (bag, name, value, NULL);

	matecomponent_object_release_unref (bag, NULL);
}

#define MAKE_SET_CONTROL_PROPERTY_BAG_VALUE(gtype, paramtype, capstype)	\
static void								\
set_control_property_bag_##gtype (MateComponentUIToolbarControlItem *item,	\
				  const char *name,			\
				  paramtype value)			\
{									\
	MateComponentArg *arg;							\
									\
	arg = matecomponent_arg_new (MATECOMPONENT_ARG_##capstype);			\
	MATECOMPONENT_ARG_SET_##capstype (arg, value);				\
	set_control_property_bag_value (item, name, arg);		\
	matecomponent_arg_release (arg);					\
}

MAKE_SET_CONTROL_PROPERTY_BAG_VALUE (gint,     gint,         INT)
     /* MAKE_SET_CONTROL_PROPERTY_BAG_VALUE (gboolean, gboolean,     BOOLEAN)
	MAKE_SET_CONTROL_PROPERTY_BAG_VALUE (string,   const char *, STRING) */

static GtkToolbar *
get_parent_toolbar (MateComponentUIToolbarControlItem *control_item)
{
	GtkWidget *toolbar;

	toolbar = GTK_WIDGET (control_item)->parent;
	if (!GTK_IS_TOOLBAR (toolbar)) {
		g_warning ("Non-toolbar parent '%s'", g_type_name_from_instance ((GTypeInstance *)toolbar));
		return NULL;
	}

	return GTK_TOOLBAR (toolbar);
}

static MateComponentUIToolbarControlDisplay
get_display_mode (MateComponentUIToolbarControlItem *control_item)
{
	GtkToolbar *toolbar = get_parent_toolbar (control_item);
	g_return_val_if_fail (toolbar != NULL, MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL);
	
	if (gtk_toolbar_get_orientation (toolbar) == GTK_ORIENTATION_HORIZONTAL)
		return control_item->hdisplay;
	else
		return control_item->vdisplay;
}

/*
 * We are assuming that there's room in horizontal orientation, but not
 * vertical. This can be made more sophisticated by looking at the control's
 * requested geometry.
 */
static void
impl_toolbar_reconfigured (GtkToolItem *item)
{
	GtkToolbar *toolbar;
	GtkOrientation orientation;
	MateComponentUIToolbarControlDisplay display;
	MateComponentUIToolbarControlItem *control_item = (MateComponentUIToolbarControlItem *) item;

	if (GTK_WIDGET (item)->parent == NULL)
		return;

	toolbar = get_parent_toolbar (control_item);
	g_return_if_fail (toolbar != NULL);

	orientation = gtk_toolbar_get_orientation (toolbar);

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		display = control_item->hdisplay;
	else
		display = control_item->vdisplay;
	
	switch (display) {

	case MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL:
		gtk_widget_hide (control_item->button);
		gtk_widget_show (control_item->widget);
		break;

	case MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_BUTTON:
		gtk_widget_hide (control_item->widget);
		gtk_widget_show (control_item->button);
		break;

	case MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_NONE:
		gtk_widget_hide (control_item->widget);
		gtk_widget_hide (control_item->button);
		break;

	default:
		g_assert_not_reached ();
	}

	set_control_property_bag_gint (control_item, "orientation", orientation);
	set_control_property_bag_gint (control_item, "style",
				       gtk_toolbar_get_style (toolbar));

	GTK_TOOL_ITEM_CLASS (matecomponent_ui_toolbar_control_item_parent_class)->toolbar_reconfigured (item);
}

static void
impl_dispose (GObject *object)
{
	MateComponentUIToolbarControlItem *control_item;

	control_item = (MateComponentUIToolbarControlItem *) object;
	
	if (control_item->widget) {
		gtk_widget_destroy (control_item->widget);
		control_item->control = NULL;
		control_item->widget = NULL;
	}

	G_OBJECT_CLASS (matecomponent_ui_toolbar_control_item_parent_class)->dispose (object);
}

static void
menu_item_map (GtkWidget *menu_item, MateComponentUIToolbarControlItem *control_item)
{
	if (GTK_BIN (menu_item)->child)
		return;

	g_object_ref (control_item->widget);
	gtk_container_remove (GTK_CONTAINER (control_item->box), 
			      control_item->widget);
	gtk_container_add (GTK_CONTAINER (menu_item), control_item->widget);
	g_object_unref (control_item->widget);
}

static void
menu_item_return_control (GtkWidget *menu_item, MateComponentUIToolbarControlItem *control_item)
{
	if (!GTK_BIN (menu_item)->child)
		return;

	if (GTK_BIN (menu_item)->child == control_item->widget) {
		g_object_ref (control_item->widget);
		gtk_container_remove (GTK_CONTAINER (menu_item), 
				      control_item->widget);
		gtk_container_add (GTK_CONTAINER (control_item->box), control_item->widget);
		g_object_unref (control_item->widget);
	}
}

static gboolean
impl_create_menu_proxy (GtkToolItem *tool_item)
{
	GtkWidget *menu_item;
	MateComponentUIToolbarControlItem *control_item = MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM (tool_item);

	if (get_display_mode (control_item) == MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_NONE)
		return FALSE;

	if (control_item->hdisplay != MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL ||
	    control_item->vdisplay != MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL)
		/* Can cope with just a button */
		return GTK_TOOL_ITEM_CLASS (matecomponent_ui_toolbar_control_item_parent_class)->create_menu_proxy (tool_item);

	menu_item = gtk_menu_item_new ();

	/* This sucks, but the best we can do */
	g_signal_connect (menu_item, "map", G_CALLBACK (menu_item_map), tool_item);
	g_signal_connect (menu_item, "destroy", G_CALLBACK (menu_item_return_control), tool_item);
	gtk_tool_item_set_proxy_menu_item (tool_item,
					   "matecomponent-control-button-menu-id",
					   menu_item);

	return TRUE;
}

static void
impl_notify (GObject    *object,
	     GParamSpec *pspec)
{
	MateComponentUIToolbarControlItem *control_item = MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM (object);

	if (control_item->control && !strcmp (pspec->name, "sensitive"))
		matecomponent_control_frame_control_set_state
			(matecomponent_widget_get_control_frame (control_item->control),
			 GTK_WIDGET_SENSITIVE (control_item) ? MateComponent_Gtk_StateNormal : MateComponent_Gtk_StateInsensitive);

	G_OBJECT_CLASS (matecomponent_ui_toolbar_control_item_parent_class)->notify (object, pspec);
}

static gboolean
impl_map_event (GtkWidget   *widget,
		GdkEventAny *event)
{
	MateComponentUIToolbarControlItem *control_item = MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM (widget);

	if (control_item->widget && control_item->widget->parent != control_item->box)
		menu_item_return_control (control_item->widget->parent, control_item);

	return GTK_WIDGET_CLASS (matecomponent_ui_toolbar_control_item_parent_class)->map_event (widget, event);
}

static void
matecomponent_ui_toolbar_control_item_class_init (MateComponentUIToolbarControlItemClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;
	GtkToolItemClass *tool_item_class = (GtkToolItemClass *) klass;
	
	object_class->dispose  = impl_dispose;
	object_class->notify = impl_notify;

	widget_class->map_event = impl_map_event;

	tool_item_class->create_menu_proxy = impl_create_menu_proxy;
	tool_item_class->toolbar_reconfigured = impl_toolbar_reconfigured;
}

static void
matecomponent_ui_toolbar_control_item_init (MateComponentUIToolbarControlItem *control_item)
{
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (control_item), FALSE);

	control_item->box = gtk_vbox_new (FALSE, 0);

	control_item->button = GTK_BIN (control_item)->child;
	g_object_ref_sink (control_item->button);
	gtk_container_remove (GTK_CONTAINER (control_item), control_item->button);
	gtk_container_add (GTK_CONTAINER (control_item->box), control_item->button);
	g_object_ref (control_item->button);

	gtk_container_add (GTK_CONTAINER (control_item), control_item->box);
	gtk_widget_show (control_item->box);
}

GtkWidget *
matecomponent_ui_toolbar_control_item_construct (
        MateComponentUIToolbarControlItem *control_item,
	GtkWidget                  *widget,
        MateComponent_Control              control_ref)
{
	if (!widget)
		widget = matecomponent_widget_new_control_from_objref (
			control_ref, CORBA_OBJECT_NIL);

        if (!widget)
                return NULL;

	control_item->widget  = widget;
	control_item->control = MATECOMPONENT_IS_WIDGET (widget) ? MATECOMPONENT_WIDGET (widget) : NULL;
	
	gtk_container_add (GTK_CONTAINER (control_item->box), control_item->widget);

        return GTK_WIDGET (control_item);
}

GtkWidget *
matecomponent_ui_toolbar_control_item_new (MateComponent_Control control_ref)
{
        MateComponentUIToolbarControlItem *control_item;
	GtkWidget *ret;

        control_item = g_object_new (
                matecomponent_ui_toolbar_control_item_get_type (), NULL);

        ret = matecomponent_ui_toolbar_control_item_construct (
		control_item, NULL, control_ref);

	if (!ret)
		g_object_unref (control_item);

	return ret;
}

GtkWidget *
matecomponent_ui_toolbar_control_item_new_widget (GtkWidget *custom_in_proc_widget)
{
	GtkWidget *ret;
        MateComponentUIToolbarControlItem *control_item;

        control_item = g_object_new (
                matecomponent_ui_toolbar_control_item_get_type (), NULL);

        ret = matecomponent_ui_toolbar_control_item_construct (
		control_item, custom_in_proc_widget, CORBA_OBJECT_NIL);

	if (!ret)
		g_object_unref (custom_in_proc_widget);

	return ret;
}

void
matecomponent_ui_toolbar_control_item_set_display (MateComponentUIToolbarControlItem    *item,
					    MateComponentUIToolbarControlDisplay  hdisplay,
					    MateComponentUIToolbarControlDisplay  vdisplay)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_CONTROL_ITEM (item));

	item->hdisplay = hdisplay;
	item->vdisplay = vdisplay;
}
