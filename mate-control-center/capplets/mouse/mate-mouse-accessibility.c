/*
 * Copyright (C) 2007 Gerd Kohlberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"

#define MT_MATECONF_HOME "/desktop/mate/accessibility/mouse"

/* 5th entry in combo box */
#define DIRECTION_DISABLE 4

enum {
	CLICK_TYPE_SINGLE,
	CLICK_TYPE_DOUBLE,
	CLICK_TYPE_DRAG,
	CLICK_TYPE_SECONDARY,
	N_CLICK_TYPES
};

static void
update_mode_sensitivity (GtkBuilder *dialog, gint mode)
{
	gtk_widget_set_sensitive (WID ("box_ctw"), !mode);
	gtk_widget_set_sensitive (WID ("box_gesture"), mode);
}

/* check if a direction (gesture mode) is already in use */
static gboolean
verify_setting (MateConfClient *client, gint value, gint type)
{
	gint i, ct[N_CLICK_TYPES];

	ct[CLICK_TYPE_SINGLE] =
		mateconf_client_get_int (client,
				      MT_MATECONF_HOME "/dwell_gesture_single",
				      NULL);
	ct[CLICK_TYPE_DOUBLE] =
		mateconf_client_get_int (client,
				      MT_MATECONF_HOME "/dwell_gesture_double",
				      NULL);
	ct[CLICK_TYPE_DRAG] =
		mateconf_client_get_int (client,
				      MT_MATECONF_HOME "/dwell_gesture_drag",
				      NULL);
	ct[CLICK_TYPE_SECONDARY] =
		mateconf_client_get_int (client,
				      MT_MATECONF_HOME "/dwell_gesture_secondary",
				      NULL);

	for (i = 0; i < N_CLICK_TYPES; ++i) {
		if (i == type)
			continue;
		if (ct[i] == value)
			return FALSE;
	}

	return TRUE;
}

static void
populate_gesture_combo (GtkWidget *combo)
{
	GtkListStore *model;
	GtkTreeIter iter;
	GtkCellRenderer *cr;

	model = gtk_list_store_new (1, G_TYPE_STRING);

	gtk_list_store_append (model, &iter);
	/* Translators: this is the gesture to trigger/choose the click type.
	   Don't include the prefix "gesture|" in the translation. */
	gtk_list_store_set (model, &iter, 0, Q_("gesture|Move left"), -1);

	gtk_list_store_append (model, &iter);
	/* Translators: this is the gesture to trigger/choose the click type.
	   Don't include the prefix "gesture|" in the translation. */
	gtk_list_store_set (model, &iter, 0, Q_("gesture|Move right"), -1);

	gtk_list_store_append (model, &iter);
	/* Translators: this is the gesture to trigger/choose the click type.
	   Don't include the prefix "gesture|" in the translation. */
	gtk_list_store_set (model, &iter, 0, Q_("gesture|Move up"), -1);

	gtk_list_store_append (model, &iter);
	/* Translators: this is the gesture to trigger/choose the click type.
	   Don't include the prefix "gesture|" in the translation. */
	gtk_list_store_set (model, &iter, 0, Q_("gesture|Move down"), -1);

	gtk_list_store_append (model, &iter);
	/* Translators: this is the gesture to trigger/choose the click type.
	   Don't include the prefix "gesture|" in the translation. */
	gtk_list_store_set (model, &iter, 0, Q_("gesture|Disabled"), -1);

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (model));

	cr = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cr, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cr,
					"text", 0,
					NULL);
}

static void
delay_enable_toggled_cb (GtkWidget *checkbox, GtkBuilder *dialog)
{
	gtk_widget_set_sensitive (WID ("delay_box"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)));
}

static void
dwell_enable_toggled_cb (GtkWidget *checkbox, GtkBuilder *dialog)
{
	gtk_widget_set_sensitive (WID ("dwell_box"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)));
}

static void
gesture_single (GtkComboBox *combo, gpointer data)
{
	if (!verify_setting (data, gtk_combo_box_get_active (combo), CLICK_TYPE_SINGLE))
		gtk_combo_box_set_active (combo, DIRECTION_DISABLE);
}

static void
gesture_double (GtkComboBox *combo, gpointer data)
{
	if (!verify_setting (data, gtk_combo_box_get_active (combo), CLICK_TYPE_DOUBLE))
		gtk_combo_box_set_active (combo, DIRECTION_DISABLE);
}

static void
gesture_drag (GtkComboBox *combo, gpointer data)
{
	if (!verify_setting (data, gtk_combo_box_get_active (combo), CLICK_TYPE_DRAG))
		gtk_combo_box_set_active (combo, DIRECTION_DISABLE);
}

static void
gesture_secondary (GtkComboBox *combo, gpointer data)
{
	if (!verify_setting (data, gtk_combo_box_get_active (combo), CLICK_TYPE_SECONDARY))
		gtk_combo_box_set_active (combo, DIRECTION_DISABLE);
}

static void
mateconf_value_changed (MateConfClient *client,
		     const gchar *key,
		     MateConfValue  *value,
		     gpointer     dialog)
{
	if (g_str_equal (key, MT_MATECONF_HOME "/dwell_mode"))
		update_mode_sensitivity (dialog, mateconf_value_get_int (value));
}

void
setup_accessibility (GtkBuilder *dialog, MateConfClient *client)
{
	mateconf_client_add_dir (client, MT_MATECONF_HOME,
			      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	g_signal_connect (client, "value_changed",
			  G_CALLBACK (mateconf_value_changed), dialog);

	mateconf_peditor_new_boolean (NULL, MT_MATECONF_HOME "/dwell_enable",
				   WID ("dwell_enable"), NULL);
	mateconf_peditor_new_boolean (NULL, MT_MATECONF_HOME "/delay_enable",
				   WID ("delay_enable"), NULL);
	mateconf_peditor_new_boolean (NULL, MT_MATECONF_HOME "/dwell_show_ctw",
				   WID ("dwell_show_ctw"), NULL);

	mateconf_peditor_new_numeric_range (NULL, MT_MATECONF_HOME "/delay_time",
					 WID ("delay_time"), NULL);
	mateconf_peditor_new_numeric_range (NULL, MT_MATECONF_HOME "/dwell_time",
					 WID ("dwell_time"), NULL);
	mateconf_peditor_new_numeric_range (NULL, MT_MATECONF_HOME "/threshold",
					 WID ("threshold"), NULL);

	mateconf_peditor_new_select_radio (NULL, MT_MATECONF_HOME "/dwell_mode",
					gtk_radio_button_get_group (GTK_RADIO_BUTTON (WID ("dwell_mode_ctw"))),
										      NULL);
	update_mode_sensitivity (dialog,
				 mateconf_client_get_int (client,
						       MT_MATECONF_HOME "/dwell_mode",
						       NULL));

	populate_gesture_combo (WID ("dwell_gest_single"));
	mateconf_peditor_new_combo_box (NULL, MT_MATECONF_HOME "/dwell_gesture_single",
				     WID ("dwell_gest_single"), NULL);
	g_signal_connect (WID ("dwell_gest_single"), "changed",
			  G_CALLBACK (gesture_single), client);

	populate_gesture_combo (WID ("dwell_gest_double"));
	mateconf_peditor_new_combo_box (NULL, MT_MATECONF_HOME "/dwell_gesture_double",
				     WID ("dwell_gest_double"), NULL);
	g_signal_connect (WID ("dwell_gest_double"), "changed",
			  G_CALLBACK (gesture_double), client);

	populate_gesture_combo (WID ("dwell_gest_drag"));
	mateconf_peditor_new_combo_box (NULL, MT_MATECONF_HOME "/dwell_gesture_drag",
				     WID ("dwell_gest_drag"), NULL);
	g_signal_connect (WID ("dwell_gest_drag"), "changed",
			  G_CALLBACK (gesture_drag), client);

	populate_gesture_combo (WID ("dwell_gest_secondary"));
	mateconf_peditor_new_combo_box (NULL, MT_MATECONF_HOME "/dwell_gesture_secondary",
				     WID ("dwell_gest_secondary"), NULL);
	g_signal_connect (WID ("dwell_gest_secondary"), "changed",
			  G_CALLBACK (gesture_secondary), client);

	g_signal_connect (WID ("delay_enable"), "toggled",
			  G_CALLBACK (delay_enable_toggled_cb), dialog);
	delay_enable_toggled_cb (WID ("delay_enable"), dialog);
	g_signal_connect (WID ("dwell_enable"), "toggled",
			  G_CALLBACK (dwell_enable_toggled_cb), dialog);
	dwell_enable_toggled_cb (WID ("dwell_enable"), dialog);
}
