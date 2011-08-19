/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2006-2007  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "bluetooth-filter-widget.h"
#include "bluetooth-client.h"
#include "mate-bluetooth-enum-types.h"

#define BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
						BLUETOOTH_TYPE_FILTER_WIDGET, BluetoothFilterWidgetPrivate))

typedef struct _BluetoothFilterWidgetPrivate BluetoothFilterWidgetPrivate;

struct _BluetoothFilterWidgetPrivate {
	GtkWidget *device_type_label, *device_type;
	GtkWidget *device_category_label, *device_category;
	GtkWidget *title;
	GtkWidget *chooser;
	GtkTreeModel *filter;

	/* Current filter */
	int device_type_filter;
	GtkTreeModel *device_type_filter_model;
	int device_category_filter;
	char *device_service_filter;

	guint show_device_type : 1;
	guint show_device_category : 1;
};

G_DEFINE_TYPE(BluetoothFilterWidget, bluetooth_filter_widget, GTK_TYPE_VBOX)

enum {
	DEVICE_TYPE_FILTER_COL_NAME = 0,
	DEVICE_TYPE_FILTER_COL_MASK,
	DEVICE_TYPE_FILTER_NUM_COLS
};

static const char *
bluetooth_device_category_to_string (int type)
{
	switch (type) {
	case BLUETOOTH_CATEGORY_ALL:
		return N_("All categories");
	case BLUETOOTH_CATEGORY_PAIRED:
		return N_("Paired");
	case BLUETOOTH_CATEGORY_TRUSTED:
		return N_("Trusted");
	case BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED:
		return N_("Not paired or trusted");
	case BLUETOOTH_CATEGORY_PAIRED_OR_TRUSTED:
		return N_("Paired or trusted");
	default:
		return N_("Unknown");
	}
}

static void
set_combobox_from_filter (BluetoothFilterWidget *self)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(self);
	GtkTreeIter iter;
	gboolean cont;

	/* Look for an exact matching filter first */
	cont = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->device_type_filter_model),
					      &iter);
	while (cont != FALSE) {
		int mask;

		gtk_tree_model_get (GTK_TREE_MODEL (priv->device_type_filter_model), &iter,
				    DEVICE_TYPE_FILTER_COL_MASK, &mask, -1);
		if (mask == priv->device_type_filter) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX(priv->device_type), &iter);
			return;
		}
		cont = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->device_type_filter_model), &iter);
	}

	/* Then a fuzzy match */
	cont = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->device_type_filter_model),
					      &iter);
	while (cont != FALSE) {
		int mask;

		gtk_tree_model_get (GTK_TREE_MODEL (priv->device_type_filter_model), &iter,
				    DEVICE_TYPE_FILTER_COL_MASK, &mask,
				    -1);
		if (mask & priv->device_type_filter) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX(priv->device_type), &iter);
			return;
		}
		cont = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->device_type_filter_model), &iter);
	}

	/* Then just set the any then */
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->device_type_filter_model), &iter);
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX(priv->device_type), &iter);
}

static void
filter_category_changed_cb (GtkComboBox *widget, gpointer data)
{
	BluetoothFilterWidget *self = BLUETOOTH_FILTER_WIDGET (data);
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(self);

	priv->device_category_filter = gtk_combo_box_get_active (GTK_COMBO_BOX(priv->device_category));
	if (priv->filter)
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
	g_object_notify (G_OBJECT(self), "device-category-filter");
}

static void
filter_type_changed_cb (GtkComboBox *widget, gpointer data)
{
	BluetoothFilterWidget *self = BLUETOOTH_FILTER_WIDGET (data);
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(self);
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter (widget, &iter) == FALSE)
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (priv->device_type_filter_model), &iter,
			    DEVICE_TYPE_FILTER_COL_MASK, &(priv->device_type_filter),
			    -1);

	if (priv->filter)
		gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
	g_object_notify (G_OBJECT(self), "device-type-filter");
}

/**
 * bluetooth_filter_widget_set_title:
 * @self: a #BluetoothFilterWidget.
 * @title: Title for the #BluetoothFilterWidget.
 *
 * Used to set a different title for the #BluetoothFilterWidget than the default.
 *
 **/
void
bluetooth_filter_widget_set_title (BluetoothFilterWidget *self, gchar *title)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(self);

	gtk_label_set_text (GTK_LABEL (priv->title), title);
	gtk_label_set_use_markup (GTK_LABEL (priv->title), TRUE);
}

static void
bluetooth_filter_widget_bind_chooser_single (BluetoothFilterWidget *self,
					     BluetoothChooser *chooser,
					     const char *property)
{
	/* NOTE: We are binding the chooser as the source so that all of it's
	 * properties are pushed to the filter.
	 * The bindings will be automatically removed when one of the
	 * objects go away */
	g_object_bind_property ((gpointer) chooser, property,
				(gpointer) self, property,
				G_BINDING_BIDIRECTIONAL);
}

/**
 * bluetooth_filter_widget_bind_filter:
 * @self: a #BluetoothFilterWidget.
 * @chooser: The #BluetoothChooser widget to bind the filter to.
 *
 * Binds a #BluetoothFilterWidget to a #BluetoothChooser such that changing the
 * #BluetoothFilterWidget results in filters being applied on the #BluetoothChooser.
 * Any properties set on a bound #BluetoothChooser will also be set on the
 * #BluetoothFilterWidget.
 *
 **/
void
bluetooth_filter_widget_bind_filter (BluetoothFilterWidget *self, BluetoothChooser *chooser)
{
	bluetooth_filter_widget_bind_chooser_single (self, chooser, "device-type-filter");
	bluetooth_filter_widget_bind_chooser_single (self, chooser, "device-category-filter");
	bluetooth_filter_widget_bind_chooser_single (self, chooser, "show-device-type");
	bluetooth_filter_widget_bind_chooser_single (self, chooser, "show-device-category");
}

static void
bluetooth_filter_widget_init(BluetoothFilterWidget *self)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(self);
	guint i;

	GtkWidget *label;
	GtkWidget *alignment;
	GtkWidget *table;
	GtkCellRenderer *renderer;

	gtk_widget_push_composite_child ();

	gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (self), 6);

	priv->title = gtk_label_new ("");
	bluetooth_filter_widget_set_title (self, _("<b>Show Only Bluetooth Devices With...</b>"));
	gtk_widget_show (priv->title);
	gtk_box_pack_start (GTK_BOX (self), priv->title, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC (priv->title), 0, 0.5);

	alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
	gtk_widget_show (alignment);
	gtk_box_pack_start (GTK_BOX (self), alignment, TRUE, TRUE, 0);

	table = gtk_table_new (2, 2, FALSE);
	gtk_widget_show (table);
	gtk_container_add (GTK_CONTAINER (alignment), table);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);

	/* The device category filter */
	label = gtk_label_new_with_mnemonic (_("Device _category:"));
	gtk_widget_set_no_show_all (label, TRUE);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	priv->device_category_label = label;

	priv->device_category = gtk_combo_box_new_text ();
	gtk_widget_set_no_show_all (priv->device_category, TRUE);
	gtk_widget_show (priv->device_category);
	gtk_table_attach (GTK_TABLE (table), priv->device_category, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_widget_set_tooltip_text (priv->device_category, _("Select the device category to filter"));
	for (i = 0; i < BLUETOOTH_CATEGORY_NUM_CATEGORIES; i++) {
		gtk_combo_box_append_text (GTK_COMBO_BOX (priv->device_category),
					   _(bluetooth_device_category_to_string (i)));
	}
	g_signal_connect (G_OBJECT (priv->device_category), "changed",
			  G_CALLBACK (filter_category_changed_cb), self);
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->device_category), priv->device_category_filter);
	if (priv->show_device_category) {
		gtk_widget_show (priv->device_category_label);
		gtk_widget_show (priv->device_category);
	}

	/* The device type filter */
	label = gtk_label_new_with_mnemonic (_("Device _type:"));
	gtk_widget_set_no_show_all (label, TRUE);
	gtk_widget_show (label);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	priv->device_type_label = label;

	priv->device_type_filter_model = GTK_TREE_MODEL (gtk_list_store_new (DEVICE_TYPE_FILTER_NUM_COLS,
									     G_TYPE_STRING, G_TYPE_INT));
	priv->device_type = gtk_combo_box_new_with_model (priv->device_type_filter_model);
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->device_type), renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (priv->device_type), renderer, "text", DEVICE_TYPE_FILTER_COL_NAME);

	gtk_widget_set_no_show_all (priv->device_type, TRUE);
	gtk_widget_show (priv->device_type);
	gtk_table_attach (GTK_TABLE (table), priv->device_type, 1, 2, 0, 1,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	gtk_widget_set_tooltip_text (priv->device_type, _("Select the device type to filter"));
	gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->device_type_filter_model), NULL, G_MAXUINT32,
					   DEVICE_TYPE_FILTER_COL_NAME, _(bluetooth_type_to_string (BLUETOOTH_TYPE_ANY)),
					   DEVICE_TYPE_FILTER_COL_MASK, BLUETOOTH_TYPE_ANY,
					   -1);
	gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->device_type_filter_model), NULL, G_MAXUINT32,
					   DEVICE_TYPE_FILTER_COL_NAME, _("Input devices (mice, keyboards, etc.)"),
					   DEVICE_TYPE_FILTER_COL_MASK, BLUETOOTH_TYPE_INPUT,
					   -1);
	gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->device_type_filter_model), NULL, G_MAXUINT32,
					   DEVICE_TYPE_FILTER_COL_NAME, _("Headphones, headsets and other audio devices"),
					   DEVICE_TYPE_FILTER_COL_MASK, BLUETOOTH_TYPE_AUDIO,
					   -1);
	/* The types match the types used in bluetooth-client.h */
	for (i = 1; i < _BLUETOOTH_TYPE_NUM_TYPES; i++) {
		int mask = 1 << i;
		if (mask & BLUETOOTH_TYPE_INPUT || mask & BLUETOOTH_TYPE_AUDIO)
			continue;
		gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->device_type_filter_model), NULL, G_MAXUINT32,
						   DEVICE_TYPE_FILTER_COL_NAME, _(bluetooth_type_to_string (mask)),
						   DEVICE_TYPE_FILTER_COL_MASK, mask,
						   -1);
	}
	g_signal_connect (G_OBJECT (priv->device_type), "changed",
			  G_CALLBACK(filter_type_changed_cb), self);
	set_combobox_from_filter (self);
	if (priv->show_device_type) {
		gtk_widget_show (priv->device_type_label);
		gtk_widget_show (priv->device_type);
	}

	/* The services filter */
	priv->device_service_filter = NULL;

	gtk_widget_pop_composite_child ();
}

static void
bluetooth_filter_widget_finalize (GObject *object)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(object);

	g_free (priv->device_service_filter);
	priv->device_service_filter = NULL;

	G_OBJECT_CLASS(bluetooth_filter_widget_parent_class)->finalize(object);
}

static void
bluetooth_filter_widget_dispose (GObject *object)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(object);

	if (priv->chooser) {
		g_object_unref (priv->chooser);
		priv->chooser = NULL;
	}

	G_OBJECT_CLASS(bluetooth_filter_widget_parent_class)->dispose(object);
}

enum {
	PROP_0,
	PROP_SHOW_DEVICE_TYPE,
	PROP_SHOW_DEVICE_CATEGORY,
	PROP_DEVICE_TYPE_FILTER,
	PROP_DEVICE_CATEGORY_FILTER,
	PROP_DEVICE_SERVICE_FILTER
};

static void
bluetooth_filter_widget_set_property (GObject *object, guint prop_id,
					 const GValue *value, GParamSpec *pspec)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_SHOW_DEVICE_TYPE:
		priv->show_device_type = g_value_get_boolean (value);
		g_object_set (G_OBJECT (priv->device_type_label), "visible", priv->show_device_type, NULL);
		g_object_set (G_OBJECT (priv->device_type), "visible", priv->show_device_type, NULL);
		break;
	case PROP_SHOW_DEVICE_CATEGORY:
		priv->show_device_category = g_value_get_boolean (value);
		g_object_set (G_OBJECT (priv->device_category_label), "visible", priv->show_device_category, NULL);
		g_object_set (G_OBJECT (priv->device_category), "visible", priv->show_device_category, NULL);
		break;
	case PROP_DEVICE_TYPE_FILTER:
		priv->device_type_filter = g_value_get_int (value);
		set_combobox_from_filter (BLUETOOTH_FILTER_WIDGET (object));
		break;
	case PROP_DEVICE_CATEGORY_FILTER:
		priv->device_category_filter = g_value_get_enum (value);
		gtk_combo_box_set_active (GTK_COMBO_BOX(priv->device_category), priv->device_category_filter);
		break;
	case PROP_DEVICE_SERVICE_FILTER:
		g_free (priv->device_service_filter);
		priv->device_service_filter = g_value_dup_string (value);
		if (priv->filter)
			gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (priv->filter));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
bluetooth_filter_widget_get_property (GObject *object, guint prop_id,
					 GValue *value, GParamSpec *pspec)
{
	BluetoothFilterWidgetPrivate *priv = BLUETOOTH_FILTER_WIDGET_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_SHOW_DEVICE_TYPE:
		g_value_set_boolean (value, priv->show_device_type);
		break;
	case PROP_SHOW_DEVICE_CATEGORY:
		g_value_set_boolean (value, priv->show_device_category);
		break;
	case PROP_DEVICE_TYPE_FILTER:
		g_value_set_int (value, priv->device_type_filter);
		break;
	case PROP_DEVICE_CATEGORY_FILTER:
		g_value_set_enum (value, priv->device_category_filter);
		break;
	case PROP_DEVICE_SERVICE_FILTER:
		g_value_set_string (value, priv->device_service_filter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
bluetooth_filter_widget_class_init (BluetoothFilterWidgetClass *klass)
{
	/* Use to calculate the maximum value for the
	 * device-type-filter value */
	guint i;
	int max_filter_val;

	g_type_class_add_private(klass, sizeof(BluetoothFilterWidgetPrivate));

	G_OBJECT_CLASS(klass)->dispose = bluetooth_filter_widget_dispose;
	G_OBJECT_CLASS(klass)->finalize = bluetooth_filter_widget_finalize;

	G_OBJECT_CLASS(klass)->set_property = bluetooth_filter_widget_set_property;
	G_OBJECT_CLASS(klass)->get_property = bluetooth_filter_widget_get_property;

	g_object_class_install_property (G_OBJECT_CLASS(klass),
					 PROP_SHOW_DEVICE_TYPE, g_param_spec_boolean ("show-device-type",
										      "show-device-type", "Whether to show the device type filter", TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (G_OBJECT_CLASS(klass),
					 PROP_SHOW_DEVICE_CATEGORY, g_param_spec_boolean ("show-device-category",
											  "show-device-category", "Whether to show the device category filter", TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	for (i = 0, max_filter_val = 0 ; i < _BLUETOOTH_TYPE_NUM_TYPES; i++)
		max_filter_val += 1 << i;
	g_object_class_install_property (G_OBJECT_CLASS(klass),
					 PROP_DEVICE_TYPE_FILTER, g_param_spec_int ("device-type-filter",
										    "device-type-filter", "A bitmask of #BluetoothType to show", 1, max_filter_val, 1, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (G_OBJECT_CLASS(klass),
					 PROP_DEVICE_CATEGORY_FILTER, g_param_spec_enum ("device-category-filter",
											 "device-category-filter", "The #BluetoothCategory to show", BLUETOOTH_TYPE_CATEGORY, BLUETOOTH_CATEGORY_ALL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (G_OBJECT_CLASS(klass),
					 PROP_DEVICE_SERVICE_FILTER, g_param_spec_string ("device-service-filter",
											  "device-service-filter", "A string representing the service to filter for", NULL, G_PARAM_WRITABLE));
}

/**
 * bluetooth_filter_widget_new:
 *
 * Creates a new #BluetoothFilterWidget which can be bound to a #BluetoothChooser to
 * control filtering of that #BluetoothChooser.
 * Usually used in conjunction with a #BluetoothChooser which has the "has-internal-filter"
 * property set to FALSE.
 *
 * Return value: A #BluetoothFilterWidget widget
 * 
 * Note: Must call bluetooth_filter_widget_bind_filter () to bind the #BluetoothFilterWidget
 * to a #BluetoothChooser.
 **/
GtkWidget *
bluetooth_filter_widget_new (void)
{
	return g_object_new(BLUETOOTH_TYPE_FILTER_WIDGET,
			    NULL);
}
