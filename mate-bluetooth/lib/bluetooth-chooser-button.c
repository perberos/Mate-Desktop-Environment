/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * (C) Copyright 2007-2009 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:bluetooth-chooser-button
 * @short_description: a Bluetooth chooser button
 * @stability: Stable
 * @include: bluetooth-plugin.h
 *
 * A button used to select Bluetooth devices which will pop-up a
 * #BluetoothChooser widget inside a dialogue when clicked.
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bluetooth-chooser-button.h"
#include "bluetooth-client.h"
#include "bluetooth-chooser.h"
#include "marshal.h"

struct _BluetoothChooserButton {
	GtkButton          parent;

	BluetoothClient   *client;
	GtkWidget         *image;
	GtkWidget         *dialog;
	GtkWidget         *chooser;
	char              *bdaddr;
	guint              is_available : 1;
	guint              has_selection : 1;
};

enum {
	PROP_0,
	PROP_DEVICE,
	PROP_IS_AVAILABLE,
};

enum {
	CHOOSER_CREATED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

static void	bluetooth_chooser_button_class_init	(BluetoothChooserButtonClass * klass);
static void	bluetooth_chooser_button_init		(BluetoothChooserButton      * button);

static GtkButtonClass *parent_class;

G_DEFINE_TYPE(BluetoothChooserButton, bluetooth_chooser_button, GTK_TYPE_BUTTON);

#define DEFAULT_STR N_("Click to select device...")

static void
set_btdevname (BluetoothChooserButton *button, const char *bdaddr, const char *name, const char *icon)
{
	char *found_name, *found_icon;

	found_name = NULL;
	found_icon = NULL;

	if (bdaddr != NULL && (name == NULL || icon == NULL)) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		gboolean cont = FALSE;

		model = bluetooth_client_get_device_model (button->client, NULL);
		if (model != NULL) {
			cont = gtk_tree_model_iter_children (GTK_TREE_MODEL(model),
							     &iter, NULL);
		}

		while (cont == TRUE) {
			char *value;

			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
					   BLUETOOTH_COLUMN_ADDRESS, &value, -1);
			if (g_ascii_strcasecmp(bdaddr, value) == 0) {
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
						   BLUETOOTH_COLUMN_ALIAS, &found_name,
						   BLUETOOTH_COLUMN_ICON, &found_icon,
						   -1);
				g_free (value);
				break;
			}
			g_free (value);
			cont = gtk_tree_model_iter_next (GTK_TREE_MODEL(model), &iter);
		}

		if (model != NULL)
			g_object_unref (model);

		if (found_name == NULL) {
			found_name = g_strdup (bdaddr);
			g_strdelimit (found_name, ":", '-');
		}
		if (found_icon == NULL)
			found_icon = g_strdup ("bluetooth");
	}

	if (bdaddr != NULL) {
		/* Update the name */
		if (name == NULL)
			gtk_button_set_label (GTK_BUTTON (button), found_name);
		else
			gtk_button_set_label (GTK_BUTTON (button), name);
		/* And the icon */
		if (icon == NULL)
			gtk_image_set_from_icon_name (GTK_IMAGE (button->image), found_icon, GTK_ICON_SIZE_MENU);
		else
			gtk_image_set_from_icon_name (GTK_IMAGE (button->image), icon, GTK_ICON_SIZE_MENU);

		/* And our copy of the address, and notify if it's actually changed */
		if (button->bdaddr == NULL || strcmp (bdaddr, button->bdaddr) != 0) {
			g_free (button->bdaddr);
			button->bdaddr = g_strdup (bdaddr);
			g_object_notify (G_OBJECT (button), "device");
		}
	} else {
		gtk_button_set_label (GTK_BUTTON (button), _(DEFAULT_STR));
		if (button->bdaddr != NULL) {
			g_free (button->bdaddr);
			button->bdaddr = NULL;
			gtk_image_clear (GTK_IMAGE (button->image));
			g_object_notify (G_OBJECT (button), "device");
		}
	}

	g_free (found_name);
	g_free (found_icon);
}

static void select_device_changed(BluetoothChooser *self, gchar *address, gpointer data)
{
	BluetoothChooserButton *button = BLUETOOTH_CHOOSER_BUTTON (data);

	button->has_selection = (address != NULL);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (button->dialog), GTK_RESPONSE_ACCEPT,
					  button->has_selection && button->is_available);
}

static void
dialog_response_cb (GtkDialog *dialog, int response_id, gpointer data)
{
	BluetoothChooserButton *button = BLUETOOTH_CHOOSER_BUTTON (data);
	char *bdaddr, *icon, *name;

	if (response_id == GTK_RESPONSE_ACCEPT) {
		BluetoothChooser *chooser = BLUETOOTH_CHOOSER (button->chooser);
		bdaddr = bluetooth_chooser_get_selected_device (chooser);
		name = bluetooth_chooser_get_selected_device_name (chooser);
		icon = bluetooth_chooser_get_selected_device_icon (chooser);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
	button->dialog = NULL;

	if (response_id != GTK_RESPONSE_ACCEPT)
		return;

	set_btdevname (button, bdaddr, name, icon);
	g_free (bdaddr);
	g_free (name);
	g_free (icon);
}

static void
bluetooth_chooser_button_clicked (GtkButton *widget)
{
	BluetoothChooserButton *button = BLUETOOTH_CHOOSER_BUTTON (widget);
	GtkWidget *parent;

	if (button->dialog != NULL) {
		gtk_window_present (GTK_WINDOW (button->dialog));
		return;
	}

	parent = gtk_widget_get_toplevel (GTK_WIDGET (button));
	//FIXME title
	button->dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW (parent),
						     GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
						     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect (button->dialog, "response",
			  G_CALLBACK (dialog_response_cb), button);
	gtk_dialog_set_response_sensitive (GTK_DIALOG(button->dialog),
					   GTK_RESPONSE_ACCEPT, FALSE);
	gtk_window_set_default_size (GTK_WINDOW(button->dialog), 480, 400);

	gtk_container_set_border_width (GTK_CONTAINER (button->dialog), 5);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (button->dialog))), 2);

	/* Create the button->chooser */
	button->chooser = bluetooth_chooser_new (NULL);
	g_signal_connect(button->chooser, "selected-device-changed",
			 G_CALLBACK(select_device_changed), button);
	g_signal_emit (G_OBJECT (button),
		       signals[CHOOSER_CREATED],
		       0, button->chooser);
	g_object_set (G_OBJECT (button->chooser), "device-selected", button->bdaddr, NULL);
	gtk_container_set_border_width (GTK_CONTAINER(button->chooser), 5);
	gtk_widget_show (button->chooser);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (button->dialog))), button->chooser);

	gtk_widget_show (button->dialog);
}

static void
default_adapter_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
	BluetoothChooserButton *button = BLUETOOTH_CHOOSER_BUTTON (data);
	char *adapter;
	gboolean powered;

	g_object_get (G_OBJECT (button->client),
		      "default-adapter", &adapter,
		      "default-adapter-powered", &powered,
		      NULL);
	if (adapter != NULL)
		button->is_available = powered;
	else
		button->is_available = FALSE;

	if (adapter != NULL)
		set_btdevname (button, button->bdaddr, NULL, NULL);
	g_free (adapter);

	if (button->dialog != NULL)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (button->dialog), GTK_RESPONSE_ACCEPT,
						   button->has_selection && button->is_available);

	g_object_notify (G_OBJECT (button), "is-available");
}

static void
bluetooth_chooser_button_finalize (GObject *object)
{
	BluetoothChooserButton *button = BLUETOOTH_CHOOSER_BUTTON (object);

	if (button->client != NULL) {
		g_object_unref (button->client);
		button->client = NULL;
	}
	if (button->dialog != NULL) {
		gtk_widget_destroy (button->dialog);
		button->dialog = NULL;
		button->chooser = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bluetooth_chooser_button_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	BluetoothChooserButton *button;

	g_return_if_fail (BLUETOOTH_IS_CHOOSER_BUTTON (object));
	button = BLUETOOTH_CHOOSER_BUTTON (object);

	switch (property_id)
	case PROP_DEVICE: {
		g_return_if_fail (bluetooth_verify_address (g_value_get_string (value)) || g_value_get_string (value) == NULL);
		set_btdevname (button, g_value_get_string (value), NULL, NULL);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
bluetooth_chooser_button_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	BluetoothChooserButton *button;

	g_return_if_fail (BLUETOOTH_IS_CHOOSER_BUTTON (object));
	button = BLUETOOTH_CHOOSER_BUTTON (object);

	switch (property_id) {
	case PROP_DEVICE:
		g_value_set_string (value, button->bdaddr);
		break;
	case PROP_IS_AVAILABLE:
		g_value_set_boolean (value, button->is_available);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
bluetooth_chooser_button_class_init (BluetoothChooserButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = bluetooth_chooser_button_finalize;
	object_class->set_property = bluetooth_chooser_button_set_property;
	object_class->get_property = bluetooth_chooser_button_get_property;

	button_class->clicked = bluetooth_chooser_button_clicked;

	/**
	 * BluetoothChooserButton::chooser-created:
	 * @self: a #BluetoothChooserButton widget
	 * @chooser: a #BluetoothChooser widget
	 *
	 * The signal is sent when a popup dialogue is created for the user to select
	 * a device. This signal allows you to change the configuration and filtering
	 * of the tree from its defaults.
	 **/
	signals[CHOOSER_CREATED] =
		g_signal_new ("chooser-created",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (BluetoothChooserButtonClass, chooser_created),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);

	/**
	 * BluetoothChooserButton:device:
	 *
	 * The Bluetooth address of the selected device or %NULL
	 **/
	g_object_class_install_property (object_class, PROP_DEVICE,
					 g_param_spec_string ("device", "Device", "The Bluetooth address of the selected device.",
							      NULL, G_PARAM_READWRITE));
	/**
	 * BluetoothChooserButton:is-available:
	 *
	 * %TRUE if there is a powered Bluetooth adapter available.
	 *
	 * See also: bluetooth_chooser_button_available()
	 **/
	g_object_class_install_property (object_class, PROP_IS_AVAILABLE,
					 g_param_spec_boolean ("is-available", "Bluetooth is available", "Whether Bluetooth is available.",
							       TRUE, G_PARAM_READABLE));
}

static void
bluetooth_chooser_button_init (BluetoothChooserButton *button)
{
	gtk_button_set_label (GTK_BUTTON (button), _("Click to select device..."));

	button->image = gtk_image_new ();
	gtk_button_set_image (GTK_BUTTON (button), button->image);

	button->bdaddr = NULL;
	button->dialog = NULL;

	button->client = bluetooth_client_new ();
	g_signal_connect (G_OBJECT (button->client), "notify::default-adapter",
			  G_CALLBACK (default_adapter_changed), button);
	g_signal_connect (G_OBJECT (button->client), "notify::default-adapter-powered",
			  G_CALLBACK (default_adapter_changed), button);

	/* And set the default value already */
	default_adapter_changed (NULL, NULL, button);

	set_btdevname (button, NULL, NULL, NULL);
}

/**
 * bluetooth_chooser_button_new:
 *
 * Returns a new #BluetoothChooserButton widget.
 *
 * Return value: a #BluetoothChooserButton widget.
 **/
GtkWidget *
bluetooth_chooser_button_new (void)
{
	return g_object_new (BLUETOOTH_TYPE_CHOOSER_BUTTON,
			     "label", _(DEFAULT_STR),
			     NULL);
}

/**
 * bluetooth_chooser_button_available:
 * @button: a #BluetoothChooserButton
 *
 * Returns whether there is a powered Bluetooth adapter.
 *
 * Return value: %TRUE if there is a powered Bluetooth adapter available, and the button should be sensitive.
 **/
gboolean
bluetooth_chooser_button_available (BluetoothChooserButton *button)
{
	g_return_val_if_fail (BLUETOOTH_IS_CHOOSER_BUTTON (button), FALSE);

	return button->is_available;
}

