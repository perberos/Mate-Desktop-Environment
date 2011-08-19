/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-toggle-button-item.h
 *
 * Author:
 *     Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <stdlib.h>
#include <matecomponent/matecomponent-ui-toolbar-toggle-button-item.h>

G_DEFINE_TYPE (MateComponentUIToolbarToggleButtonItem,
	       matecomponent_ui_toolbar_toggle_button_item,
	       MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM)

enum {
	TOGGLED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* GtkToggleButton callback.  */

static void
button_widget_toggled_cb (GtkToggleButton *toggle_button,
			  gpointer         user_data)
{
	g_signal_emit (user_data, signals[TOGGLED], 0);
}

static void
impl_set_state (MateComponentUIToolbarItem *item,
		const char          *state)
{
	GtkButton *button;
	gboolean   active = atoi (state);

	button = matecomponent_ui_toolbar_button_item_get_button_widget (
		MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (item));

	if (GTK_WIDGET_STATE (GTK_WIDGET (button)) != active)
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (button), active);
}		

/* GObject initialization.  */

static void
matecomponent_ui_toolbar_toggle_button_item_class_init (
	MateComponentUIToolbarToggleButtonItemClass *klass)
{
	MateComponentUIToolbarItemClass *item_class = (MateComponentUIToolbarItemClass *) klass;

	item_class->set_state = impl_set_state;

	signals[TOGGLED] = g_signal_new (
		"toggled", G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (MateComponentUIToolbarToggleButtonItemClass, toggled),
		NULL, NULL, g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}


static void
matecomponent_ui_toolbar_toggle_button_item_init (
	MateComponentUIToolbarToggleButtonItem *toolbar_toggle_button_item)
{
	/* Nothing to do here.  */
}

static void
proxy_toggle_click_cb (GtkWidget *button, GtkObject *item)
{
	gboolean active;
	char    *new_state;

	active = gtk_toggle_button_get_active (
		GTK_TOGGLE_BUTTON (button));

	new_state = g_strdup_printf ("%d", active);

	g_signal_emit_by_name (item, "state_altered", new_state);

	g_free (new_state);
}

void
matecomponent_ui_toolbar_toggle_button_item_construct (MateComponentUIToolbarToggleButtonItem *toggle_button_item,
					     GdkPixbuf *icon,
					     const char *label)
{
	GtkWidget *button_widget;

	button_widget = gtk_toggle_button_new ();

	g_signal_connect_object (
		button_widget, "toggled",
		G_CALLBACK (button_widget_toggled_cb),
		toggle_button_item, 0);

	g_signal_connect_object (
		button_widget, "clicked",
		G_CALLBACK (proxy_toggle_click_cb),
		toggle_button_item, 0);

	matecomponent_ui_toolbar_button_item_construct (
		MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (toggle_button_item),
		GTK_BUTTON (button_widget), icon, label);
}

GtkWidget *
matecomponent_ui_toolbar_toggle_button_item_new (GdkPixbuf *icon,
				       const char *label)
{
	MateComponentUIToolbarToggleButtonItem *toggle_button_item;

	toggle_button_item = g_object_new (
		matecomponent_ui_toolbar_toggle_button_item_get_type (), NULL);

	matecomponent_ui_toolbar_toggle_button_item_construct (toggle_button_item, icon, label);

	return GTK_WIDGET (toggle_button_item);
}


void
matecomponent_ui_toolbar_toggle_button_item_set_active (MateComponentUIToolbarToggleButtonItem *item,
					      gboolean active)
{
	GtkButton *button_widget;

	g_return_if_fail (item != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (item));

	button_widget = matecomponent_ui_toolbar_button_item_get_button_widget (MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (item));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button_widget), active);
}

gboolean
matecomponent_ui_toolbar_toggle_button_item_get_active (MateComponentUIToolbarToggleButtonItem *item)
{
	GtkButton *button_widget;

	g_return_val_if_fail (item != NULL, FALSE);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (item), FALSE);

	button_widget = matecomponent_ui_toolbar_button_item_get_button_widget (MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (item));

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button_widget));
}
