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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bluetooth-chooser-combo.h"
#include "bluetooth-client.h"
#include "bluetooth-chooser.h"
#include "bluetooth-chooser-private.h"
#include "marshal.h"

struct _BluetoothChooserCombo {
	GtkVBox            parent;

	GtkWidget         *chooser;
	GtkWidget         *drop_box;
	GtkWidget         *drop;
	GtkTreeModel      *model;
	guint              model_notify_id;
	GtkTreeSelection  *selection;

	char              *bdaddr;
};

enum {
	PROP_0,
	PROP_CHOOSER,
	PROP_DEVICE,
};

enum {
	CHOOSER_CREATED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

static void	bluetooth_chooser_combo_class_init	(BluetoothChooserComboClass * klass);
static void	bluetooth_chooser_combo_init		(BluetoothChooserCombo      * combo);

static GtkVBoxClass *parent_class;

G_DEFINE_TYPE(BluetoothChooserCombo, bluetooth_chooser_combo, GTK_TYPE_VBOX);

static void
bluetooth_chooser_combo_set_device (BluetoothChooserCombo *combo,
				    const char *bdaddr)
{
	if (bdaddr == NULL || combo->model == NULL) {
		g_free (combo->bdaddr);
		gtk_widget_set_sensitive (combo->drop_box, FALSE);
	} else {
		GtkTreeIter iter;
		gboolean cont = FALSE;

		gtk_widget_set_sensitive (combo->drop_box, TRUE);

		g_free (combo->bdaddr);
		if (g_strcmp0 (BLUETOOTH_CHOOSER_COMBO_FIRST_DEVICE, bdaddr) != 0)
			combo->bdaddr = g_strdup (bdaddr);
		else
			combo->bdaddr = NULL;

		cont = gtk_tree_model_iter_children (combo->model, &iter, NULL);
		while (cont == TRUE) {
			char *value;

			gtk_tree_model_get (GTK_TREE_MODEL (combo->model), &iter,
					    BLUETOOTH_COLUMN_ADDRESS, &value, -1);

			if (combo->bdaddr == NULL) {
				gtk_tree_selection_select_iter (combo->selection, &iter);
				combo->bdaddr = value;
				break;
			}

			if (g_ascii_strcasecmp(bdaddr, value) == 0) {
				gtk_tree_selection_select_iter (combo->selection, &iter);
				g_free (value);
				break;
			}
			g_free (value);
			cont = gtk_tree_model_iter_next (GTK_TREE_MODEL (combo->model), &iter);
		}
	}
	g_object_notify (G_OBJECT (combo), "device");
}

static void
bluetooth_chooser_combo_dispose (GObject *object)
{
	BluetoothChooserCombo *combo = BLUETOOTH_CHOOSER_COMBO (object);

	if (combo->model_notify_id != 0) {
		GtkWidget *treeview;

		treeview = bluetooth_chooser_get_treeview (BLUETOOTH_CHOOSER (combo->chooser));
		g_signal_handler_disconnect (treeview, combo->model_notify_id);
		combo->model_notify_id = 0;
	}
	if (combo->model != NULL) {
		g_object_unref (combo->model);
		combo->model = NULL;
	}
	if (combo->chooser != NULL) {
		g_object_unref (combo->chooser);
		combo->chooser = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
bluetooth_chooser_combo_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	BluetoothChooserCombo *combo;

	g_return_if_fail (BLUETOOTH_IS_CHOOSER_COMBO (object));
	combo = BLUETOOTH_CHOOSER_COMBO (object);

	switch (property_id)
	case PROP_DEVICE: {
		g_return_if_fail (bluetooth_verify_address (g_value_get_string (value)) || g_value_get_string (value) == NULL);
		bluetooth_chooser_combo_set_device (combo, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
bluetooth_chooser_combo_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	BluetoothChooserCombo *combo;

	g_return_if_fail (BLUETOOTH_IS_CHOOSER_COMBO (object));
	combo = BLUETOOTH_CHOOSER_COMBO (object);

	switch (property_id) {
	case PROP_CHOOSER:
		g_value_set_object (value, combo->chooser);
		break;
	case PROP_DEVICE:
		g_value_set_string (value, combo->bdaddr);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
bluetooth_chooser_combo_class_init (BluetoothChooserComboClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = bluetooth_chooser_combo_dispose;
	object_class->set_property = bluetooth_chooser_combo_set_property;
	object_class->get_property = bluetooth_chooser_combo_get_property;

	/**
	 * BluetoothChooserCombo::chooser-created:
	 * @self: a #BluetoothChooserCombo widget
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
			      G_STRUCT_OFFSET (BluetoothChooserComboClass, chooser_created),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, G_TYPE_OBJECT);

	/**
	 * BluetoothChooserCombo:chooser:
	 *
	 * The #BluetoothChooser used in the widget
	 **/
	g_object_class_install_property (object_class, PROP_CHOOSER,
					 g_param_spec_object ("chooser", "Chooser", "The #BluetoothChooser used in the widget",
							      BLUETOOTH_TYPE_CHOOSER, G_PARAM_READABLE));
	/**
	 * BluetoothChooserCombo:device:
	 *
	 * The Bluetooth address of the selected device or %NULL
	 **/
	g_object_class_install_property (object_class, PROP_DEVICE,
					 g_param_spec_string ("device", "Device", "The Bluetooth address of the selected device.",
							      NULL, G_PARAM_READWRITE));
}

static void
treeview_model_notify_cb (GObject    *gobject,
			  GParamSpec *pspec,
			  gpointer    user_data)
{
	BluetoothChooserCombo *combo = BLUETOOTH_CHOOSER_COMBO (user_data);
	GtkTreeModel *model;

	g_object_get (gobject, "model", &model, NULL);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo->drop), model);
	if (combo->model != NULL) {
		g_object_unref (combo->model);
		combo->model = NULL;
	}
	combo->model = model;
}

static void
treeview_selection_changed_cb (GtkTreeSelection *treeselection,
			       gpointer          user_data)
{
	BluetoothChooserCombo *combo = BLUETOOTH_CHOOSER_COMBO (user_data);
	GtkTreeIter iter;
	char *value = NULL;

	if (gtk_tree_selection_get_selected (combo->selection, NULL, &iter)) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo->drop), &iter);
		if (combo->model != NULL)
			gtk_tree_model_get (GTK_TREE_MODEL (combo->model), &iter,
					    BLUETOOTH_COLUMN_ADDRESS, &value, -1);
	} else {
		if (combo->model != NULL)
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo->drop), -1);
	}

	if (g_strcmp0 (combo->bdaddr, value) != 0) {
		g_free (combo->bdaddr);
		combo->bdaddr = value;
		g_object_notify (G_OBJECT (combo), "device");
	} else {
		g_free (value);
	}
}

static void
drop_changed_cb (GtkComboBox *widget,
		 gpointer     user_data)
{
	BluetoothChooserCombo *combo = BLUETOOTH_CHOOSER_COMBO (user_data);
	GtkTreeIter iter;
	char *value = NULL;

	if (gtk_combo_box_get_active_iter (widget, &iter)) {
		gtk_tree_selection_select_iter (combo->selection, &iter);
		if (combo->model != NULL)
			gtk_tree_model_get (GTK_TREE_MODEL (combo->model), &iter,
					    BLUETOOTH_COLUMN_ADDRESS, &value, -1);
	} else {
		if (combo->model != NULL)
			gtk_tree_selection_unselect_all (combo->selection);
	}

	if (g_strcmp0 (combo->bdaddr, value) != 0) {
		g_free (combo->bdaddr);
		combo->bdaddr = value;
		g_object_notify (G_OBJECT (combo), "device");
	} else {
		g_free (value);
	}
}

static void
bluetooth_chooser_combo_init (BluetoothChooserCombo *combo)
{
	GtkWidget *treeview;
	GtkCellRenderer *renderer;

	combo->drop_box = gtk_hbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (combo), combo->drop_box,
			    TRUE, FALSE, 0);
	/* Setup the combo itself */
	combo->drop = gtk_combo_box_new ();
	gtk_box_pack_start (GTK_BOX (combo->drop_box), combo->drop,
			    TRUE, TRUE, 0);
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo->drop),
				    renderer,
				    FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo->drop),
					renderer,
					"icon-name", BLUETOOTH_COLUMN_ICON,
					NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo->drop),
				    renderer,
				    TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo->drop),
					renderer,
					"text", BLUETOOTH_COLUMN_ALIAS,
					NULL);

	combo->chooser = bluetooth_chooser_new ("");

	treeview = bluetooth_chooser_get_treeview (BLUETOOTH_CHOOSER (combo->chooser));
	combo->model_notify_id = g_signal_connect (G_OBJECT (treeview), "notify::model",
						   G_CALLBACK (treeview_model_notify_cb), combo);
	treeview_model_notify_cb (G_OBJECT (treeview), NULL, combo);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo->drop), 0);

	combo->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (combo->selection), "changed",
			  G_CALLBACK (treeview_selection_changed_cb), combo);
	g_signal_connect (G_OBJECT (combo->drop), "changed",
			  G_CALLBACK (drop_changed_cb), combo);

	gtk_widget_show_all (GTK_WIDGET (combo));
}

/**
 * bluetooth_chooser_combo_new:
 *
 * Returns a new #BluetoothChooserCombo widget.
 *
 * Return value: a #BluetoothChooserCombo widget.
 **/
GtkWidget *
bluetooth_chooser_combo_new (void)
{
	return g_object_new (BLUETOOTH_TYPE_CHOOSER_COMBO, NULL);
}

