/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 Jaap Haitsma <jaap@haitsma.org>
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2005-2009 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <math.h>
#include <string.h>
#include <mateconf/mateconf-client.h>
#include <libupower-glib/upower.h>

#include "egg-debug.h"
#include "egg-console-kit.h"

#include "gpm-tray-icon.h"
#include "gpm-common.h"
#include "gpm-prefs-core.h"
#include "gpm-stock-icons.h"
#include "gpm-prefs-server.h"

static void gpm_prefs_finalize (GObject *object);

#define GPM_PREFS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_PREFS, GpmPrefsPrivate))

struct GpmPrefsPrivate
{
	UpClient		*client;
	GtkBuilder		*builder;
	gboolean		 has_batteries;
	gboolean		 has_lcd;
	gboolean		 has_ups;
	gboolean		 has_button_lid;
	gboolean		 has_button_suspend;
	gboolean		 can_shutdown;
	gboolean		 can_suspend;
	gboolean		 can_hibernate;
	guint			 idle_delay;
	MateConfClient		*conf;
	EggConsoleKit		*console;
};

enum {
	ACTION_HELP,
	ACTION_CLOSE,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpmPrefs, gpm_prefs, G_TYPE_OBJECT)

/**
 * gpm_prefs_class_init:
 * @klass: This prefs class instance
 **/
static void
gpm_prefs_class_init (GpmPrefsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_prefs_finalize;
	g_type_class_add_private (klass, sizeof (GpmPrefsPrivate));

	signals [ACTION_HELP] =
		g_signal_new ("action-help",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmPrefsClass, action_help),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [ACTION_CLOSE] =
		g_signal_new ("action-close",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmPrefsClass, action_close),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * gpm_prefs_activate_window:
 * @prefs: This prefs class instance
 *
 * Activates (shows) the window.
 **/
void
gpm_prefs_activate_window (GpmPrefs *prefs)
{
	GtkWindow *window;
	window = GTK_WINDOW (gtk_builder_get_object (prefs->priv->builder, "dialog_preferences"));
	gtk_window_present (window);
}

/**
 * gpm_dbus_get_caps:
 * @method: The g-p-m DBUS method name, e.g. "AllowedSuspend"
 **/
static gint
gpm_dbus_get_caps (GpmPrefs *prefs)
{
	DBusGConnection *connection;
	DBusGProxy *proxy = NULL;
	GError *error = NULL;
	gboolean ret;
	gint value = 0;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		egg_warning ("Couldn't connect to g-p-m %s", error->message);
		g_error_free (error);
		goto out;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   GPM_DBUS_SERVICE,
					   GPM_DBUS_PATH,
					   GPM_DBUS_INTERFACE);
	ret = dbus_g_proxy_call (proxy, "GetPreferencesOptions", &error,
				 G_TYPE_INVALID,
				 G_TYPE_INT, &value,
				 G_TYPE_INVALID);
	if (!ret) {
		/* abort as the DBUS method failed */
		egg_warning ("GetPreferencesOptions failed: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	if (proxy != NULL)
		g_object_unref (proxy);
	return value;
}

/**
 * gpm_prefs_help_cb:
 * @widget: The GtkWidget object
 * @prefs: This prefs class instance
 **/
static void
gpm_prefs_help_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	egg_debug ("emitting action-help");
	g_signal_emit (prefs, signals [ACTION_HELP], 0);
}

/**
 * gpm_prefs_icon_radio_cb:
 * @widget: The GtkWidget object
 **/
static void
gpm_prefs_icon_radio_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	const gchar *str;
	gint policy;

	policy = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "policy"));
	str = gpm_icon_policy_to_string (policy);
	egg_debug ("Changing %s to %s", GPM_CONF_UI_ICON_POLICY, str);
	mateconf_client_set_string (prefs->priv->conf, GPM_CONF_UI_ICON_POLICY, str, NULL);
}

/**
 * gpm_prefs_format_percentage_cb:
 * @scale: The GtkScale object
 * @value: The value in %.
 **/
static gchar *
gpm_prefs_format_percentage_cb (GtkScale *scale, gdouble value)
{
	return g_strdup_printf ("%.0f%%", value);
}

/**
 * gpm_prefs_brightness_slider_changed_cb:
 * @range: The GtkRange object
 * @gpm_pref_key: The MateConf key for this preference setting.
 **/
static void
gpm_prefs_brightness_slider_changed_cb (GtkRange *range, GpmPrefs *prefs)
{
	gdouble value;
	gchar *gpm_pref_key;

	value = gtk_range_get_value (range);
	gpm_pref_key = (char *) g_object_get_data (G_OBJECT (range), "conf_key");

	g_object_set_data (G_OBJECT (range), "conf_key", (gpointer) gpm_pref_key);
	egg_debug ("Changing %s to %i", gpm_pref_key, (int) value);
	mateconf_client_set_int (prefs->priv->conf, gpm_pref_key, (gint) value, NULL);
}

/**
 * gpm_prefs_setup_brightness_slider:
 * @prefs: This prefs class instance
 * @widget_name: The GtkWidget name
 * @gpm_pref_key: The MateConf key for this preference setting.
 **/
static GtkWidget *
gpm_prefs_setup_brightness_slider (GpmPrefs *prefs, const gchar *widget_name, const gchar *gpm_pref_key)
{
	GtkWidget *widget;
	int value;
	gboolean is_writable;

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, widget_name));

	g_signal_connect (G_OBJECT (widget), "format-value",
			  G_CALLBACK (gpm_prefs_format_percentage_cb), NULL);

	value = mateconf_client_get_int (prefs->priv->conf, gpm_pref_key, NULL);
	is_writable = mateconf_client_key_is_writable (prefs->priv->conf, gpm_pref_key, NULL);

	gtk_widget_set_sensitive (widget, is_writable);

	gtk_range_set_value (GTK_RANGE (widget), value);

	g_object_set_data (G_OBJECT (widget), "conf_key", (gpointer) gpm_pref_key);

	g_signal_connect (G_OBJECT (widget), "value-changed",
			  G_CALLBACK (gpm_prefs_brightness_slider_changed_cb),
			  prefs);
	return widget;
}

/**
 * gpm_prefs_action_combo_changed_cb:
 **/
static void
gpm_prefs_action_combo_changed_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	GpmActionPolicy policy;
	const GpmActionPolicy *actions;
	const gchar *gpm_pref_key;
	const gchar *action;
	guint active;

	actions = (const GpmActionPolicy *) g_object_get_data (G_OBJECT (widget), "actions");
	gpm_pref_key = (const gchar *) g_object_get_data (G_OBJECT (widget), "conf_key");

	active = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	policy = actions[active];
	action = gpm_action_policy_to_string (policy);
	egg_debug ("Changing %s to %s", gpm_pref_key, action);
	mateconf_client_set_string (prefs->priv->conf, gpm_pref_key, action, NULL);
}

/**
 * gpm_prefs_action_time_changed_cb:
 **/
static void
gpm_prefs_action_time_changed_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	guint value;
	const gint *values;
	const gchar *gpm_pref_key;
	guint active;

	values = (const gint *) g_object_get_data (G_OBJECT (widget), "values");
	gpm_pref_key = (const gchar *) g_object_get_data (G_OBJECT (widget), "conf_key");

	active = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	value = values[active];

	egg_debug ("Changing %s to %i", gpm_pref_key, value);
	mateconf_client_set_int (prefs->priv->conf, gpm_pref_key, value, NULL);
}

/**
 * gpm_prefs_set_combo_simple_text:
 **/
static void
gpm_prefs_set_combo_simple_text (GtkWidget *combo_box)
{
	GtkCellRenderer *cell;
	GtkListStore *store;

	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
	g_object_unref (store);

	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
					"text", 0,
					NULL);
}

/**
 * gpm_prefs_actions_destroy_cb:
 **/
static void
gpm_prefs_actions_destroy_cb (GpmActionPolicy *array)
{
	g_free (array);
}

/**
 * gpm_prefs_setup_action_combo:
 * @prefs: This prefs class instance
 * @widget_name: The GtkWidget name
 * @gpm_pref_key: The MateConf key for this preference setting.
 * @actions: The actions to associate in an array.
 **/
static void
gpm_prefs_setup_action_combo (GpmPrefs *prefs, const gchar *widget_name,
			      const gchar *gpm_pref_key, const GpmActionPolicy *actions)
{
	gchar *value_txt;
	gint i;
	gboolean is_writable;
	GtkWidget *widget;
	GpmActionPolicy policy;
	GpmActionPolicy	value;
	GPtrArray *array;
	GpmActionPolicy *actions_added;

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, widget_name));
	gpm_prefs_set_combo_simple_text (widget);

	value_txt = mateconf_client_get_string (prefs->priv->conf, gpm_pref_key, NULL);
	is_writable = mateconf_client_key_is_writable (prefs->priv->conf, gpm_pref_key, NULL);
	value = gpm_action_policy_from_string (value_txt);

	gtk_widget_set_sensitive (widget, is_writable);

	array = g_ptr_array_new ();
	g_object_set_data (G_OBJECT (widget), "conf_key", (gpointer) gpm_pref_key);
	g_signal_connect (G_OBJECT (widget), "changed",
			  G_CALLBACK (gpm_prefs_action_combo_changed_cb), prefs);

	for (i=0; actions[i] != -1; i++) {
		policy = actions[i];
		if (policy == GPM_ACTION_POLICY_SHUTDOWN && !prefs->priv->can_shutdown) {
			egg_debug ("Cannot add option, as cannot shutdown.");
		} else if (policy == GPM_ACTION_POLICY_SHUTDOWN && prefs->priv->can_shutdown) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Shutdown"));
			g_ptr_array_add (array, GINT_TO_POINTER (policy));
		} else if (policy == GPM_ACTION_POLICY_SUSPEND && !prefs->priv->can_suspend) {
			egg_debug ("Cannot add option, as cannot suspend.");
		} else if (policy == GPM_ACTION_POLICY_HIBERNATE && !prefs->priv->can_hibernate) {
			egg_debug ("Cannot add option, as cannot hibernate.");
		} else if (policy == GPM_ACTION_POLICY_SUSPEND && prefs->priv->can_suspend) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Suspend"));
			g_ptr_array_add (array, GINT_TO_POINTER (policy));
		} else if (policy == GPM_ACTION_POLICY_HIBERNATE && prefs->priv->can_hibernate) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Hibernate"));
			g_ptr_array_add (array, GINT_TO_POINTER (policy));
		} else if (policy == GPM_ACTION_POLICY_BLANK) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Blank screen"));
			g_ptr_array_add (array, GINT_TO_POINTER (policy));
		} else if (policy == GPM_ACTION_POLICY_INTERACTIVE) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Ask me"));
			g_ptr_array_add (array, GINT_TO_POINTER (policy));
		} else if (policy == GPM_ACTION_POLICY_NOTHING) {
			/* we only add do nothing in the GUI if the user has explicitly specified this in MateConf */
			if (value == GPM_ACTION_POLICY_NOTHING) {
				gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Do nothing"));
				g_ptr_array_add (array, GINT_TO_POINTER (policy));
			}
		} else {
			egg_warning ("Unknown action read from conf: %i", policy);
		}
	}

	/* save as array _only_ the actions we could add */
	actions_added = g_new0 (GpmActionPolicy, array->len+1);
	for (i=0; i<array->len; i++)
		actions_added[i] = GPOINTER_TO_INT (g_ptr_array_index (array, i));
	actions_added[i] = -1;

	g_object_set_data_full (G_OBJECT (widget), "actions", (gpointer) actions_added, (GDestroyNotify) gpm_prefs_actions_destroy_cb);

	/* set what we have in MateConf */
	for (i=0; actions_added[i] != -1; i++) {
		policy = actions_added[i];
		egg_debug ("added: %s", gpm_action_policy_to_string (policy));
		if (value == policy)
			 gtk_combo_box_set_active (GTK_COMBO_BOX (widget), i);
	}

	g_ptr_array_unref (array);
	g_free (value_txt);
}

/**
 * gpm_prefs_setup_time_combo:
 * @prefs: This prefs class instance
 * @widget_name: The GtkWidget name
 * @gpm_pref_key: The MateConf key for this preference setting.
 * @actions: The actions to associate in an array.
 **/
static void
gpm_prefs_setup_time_combo (GpmPrefs *prefs, const gchar *widget_name,
			    const gchar *gpm_pref_key, const gint *values)
{
	guint value;
	gchar *text;
	guint i;
	gboolean is_writable;
	GtkWidget *widget;

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, widget_name));
	gpm_prefs_set_combo_simple_text (widget);

	value = mateconf_client_get_int (prefs->priv->conf, gpm_pref_key, NULL);
	is_writable = mateconf_client_key_is_writable (prefs->priv->conf, gpm_pref_key, NULL);
	gtk_widget_set_sensitive (widget, is_writable);

	g_object_set_data (G_OBJECT (widget), "conf_key", (gpointer) gpm_pref_key);
	g_object_set_data (G_OBJECT (widget), "values", (gpointer) values);

	/* add each time */
	for (i=0; values[i] != -1; i++) {

		/* get translation for number of seconds */
		if (values[i] != 0) {
			text = gpm_get_timestring (values[i]);
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), text);
			g_free (text);
		} else {
			gtk_combo_box_append_text (GTK_COMBO_BOX (widget), _("Never"));
		}

		/* matches, so set default */
		if (value == values[i])
			 gtk_combo_box_set_active (GTK_COMBO_BOX (widget), i);
	}

	/* connect after set */
	g_signal_connect (G_OBJECT (widget), "changed",
			  G_CALLBACK (gpm_prefs_action_time_changed_cb), prefs);
}

/**
 * gpm_prefs_checkbox_lock_cb:
 * @widget: The GtkWidget object
 * @gpm_pref_key: The MateConf key for this preference setting.
 **/
static void
gpm_prefs_checkbox_lock_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	gboolean checked;
	gchar *gpm_pref_key;

	checked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	gpm_pref_key = (char *) g_object_get_data (G_OBJECT (widget), "conf_key");
	egg_debug ("Changing %s to %i", gpm_pref_key, checked);
	mateconf_client_set_bool (prefs->priv->conf, gpm_pref_key, checked, NULL);
}

/**
 * gpm_prefs_setup_checkbox:
 * @prefs: This prefs class instance
 * @widget_name: The GtkWidget name
 * @gpm_pref_key: The MateConf key for this preference setting.
 **/
static GtkWidget *
gpm_prefs_setup_checkbox (GpmPrefs *prefs, const gchar *widget_name, const gchar *gpm_pref_key)
{
	gboolean checked;
	GtkWidget *widget;

	egg_debug ("Setting up %s", gpm_pref_key);

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, widget_name));

	checked = mateconf_client_get_bool (prefs->priv->conf, gpm_pref_key, NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), checked);

	g_object_set_data (G_OBJECT (widget), "conf_key", (gpointer) gpm_pref_key);

	/* manually do the callback in case we hide elements in the cb */
	gpm_prefs_checkbox_lock_cb (widget, prefs);

	/* setup after set */
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpm_prefs_checkbox_lock_cb), prefs);

	return widget;
}

/**
 * gpm_prefs_close_cb:
 * @widget: The GtkWidget object
 * @prefs: This prefs class instance
 **/
static void
gpm_prefs_close_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	egg_debug ("emitting action-close");
	g_signal_emit (prefs, signals [ACTION_CLOSE], 0);
}

/**
 * gpm_prefs_delete_event_cb:
 * @widget: The GtkWidget object
 * @event: The event type, unused.
 * @prefs: This prefs class instance
 **/
static gboolean
gpm_prefs_delete_event_cb (GtkWidget *widget, GdkEvent *event, GpmPrefs *prefs)
{
	gpm_prefs_close_cb (widget, prefs);
	return FALSE;
}

/**
 * gpm_conf_mateconf_key_changed_cb:
 *
 * We might have to do things when the mateconf keys change; do them here.
 **/
static void
gpm_conf_mateconf_key_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, GpmPrefs *prefs)
{
	MateConfValue *value;
	gint brightness;
	GtkWidget *widget;
	gboolean enabled;

	value = mateconf_entry_get_value (entry);
	if (value == NULL)
		return;

	if (g_strcmp0 (entry->key, GPM_CONF_BACKLIGHT_BRIGHTNESS_AC) == 0) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "hscale_ac_brightness"));
		brightness = mateconf_value_get_int (value);
		gtk_range_set_value (GTK_RANGE (widget), brightness);
	}

	if (g_strcmp0 (entry->key, GPM_CONF_DISKS_SPINDOWN_ENABLE_AC) == 0) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "checkbutton_ac_spindown"));
		enabled = mateconf_value_get_bool (value);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), enabled);

	} else if (g_strcmp0 (entry->key, GPM_CONF_DISKS_SPINDOWN_ENABLE_BATT) == 0) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "checkbutton_battery_spindown"));
		enabled = mateconf_value_get_bool (value);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), enabled);
	}
}

/** setup the notification page */
static void
prefs_setup_notification (GpmPrefs *prefs)
{
	gchar *icon_policy_str;
	gint icon_policy;
	GtkWidget *radiobutton_icon_always;
	GtkWidget *radiobutton_icon_present;
	GtkWidget *radiobutton_icon_charge;
	GtkWidget *radiobutton_icon_low;
	GtkWidget *radiobutton_icon_never;
	gboolean is_writable;

	icon_policy_str = mateconf_client_get_string (prefs->priv->conf, GPM_CONF_UI_ICON_POLICY, NULL);
	icon_policy = gpm_icon_policy_from_string (icon_policy_str);
	g_free (icon_policy_str);

	radiobutton_icon_always = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder,
					      "radiobutton_notification_always"));
	radiobutton_icon_present = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder,
					       "radiobutton_notification_present"));
	radiobutton_icon_charge = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder,
					      "radiobutton_notification_charge"));
	radiobutton_icon_low = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder,
					   "radiobutton_notification_low"));
	radiobutton_icon_never = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder,
					     "radiobutton_notification_never"));

	is_writable = mateconf_client_key_is_writable (prefs->priv->conf, GPM_CONF_UI_ICON_POLICY, NULL);
	gtk_widget_set_sensitive (radiobutton_icon_always, is_writable);
	gtk_widget_set_sensitive (radiobutton_icon_present, is_writable);
	gtk_widget_set_sensitive (radiobutton_icon_charge, is_writable);
	gtk_widget_set_sensitive (radiobutton_icon_low, is_writable);
	gtk_widget_set_sensitive (radiobutton_icon_never, is_writable);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_icon_always),
				      icon_policy == GPM_ICON_POLICY_ALWAYS);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_icon_present),
				      icon_policy == GPM_ICON_POLICY_PRESENT);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_icon_charge),
				      icon_policy == GPM_ICON_POLICY_CHARGE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_icon_low),
				      icon_policy == GPM_ICON_POLICY_LOW);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radiobutton_icon_never),
				      icon_policy == GPM_ICON_POLICY_NEVER);

	g_object_set_data (G_OBJECT (radiobutton_icon_always), "policy",
			   GINT_TO_POINTER (GPM_ICON_POLICY_ALWAYS));
	g_object_set_data (G_OBJECT (radiobutton_icon_present), "policy",
			   GINT_TO_POINTER (GPM_ICON_POLICY_PRESENT));
	g_object_set_data (G_OBJECT (radiobutton_icon_charge), "policy",
			   GINT_TO_POINTER (GPM_ICON_POLICY_CHARGE));
	g_object_set_data (G_OBJECT (radiobutton_icon_low), "policy",
			   GINT_TO_POINTER (GPM_ICON_POLICY_LOW));
	g_object_set_data (G_OBJECT (radiobutton_icon_never), "policy",
			   GINT_TO_POINTER (GPM_ICON_POLICY_NEVER));

	/* only connect the callbacks after we set the value, else the conf
	 * keys gets written to (for a split second), and the icon flickers. */
	g_signal_connect (radiobutton_icon_always, "clicked",
			  G_CALLBACK (gpm_prefs_icon_radio_cb), prefs);
	g_signal_connect (radiobutton_icon_present, "clicked",
			  G_CALLBACK (gpm_prefs_icon_radio_cb), prefs);
	g_signal_connect (radiobutton_icon_charge, "clicked",
			  G_CALLBACK (gpm_prefs_icon_radio_cb), prefs);
	g_signal_connect (radiobutton_icon_low, "clicked",
			  G_CALLBACK (gpm_prefs_icon_radio_cb), prefs);
	g_signal_connect (radiobutton_icon_never, "clicked",
			  G_CALLBACK (gpm_prefs_icon_radio_cb), prefs);
}

static void
prefs_setup_ac (GpmPrefs *prefs)
{
	GtkWidget *widget;
	const GpmActionPolicy button_lid_actions[] =
				{GPM_ACTION_POLICY_NOTHING,
				 GPM_ACTION_POLICY_BLANK,
				 GPM_ACTION_POLICY_SUSPEND,
				 GPM_ACTION_POLICY_HIBERNATE,
				 GPM_ACTION_POLICY_SHUTDOWN,
				 -1};

	static const gint computer_times[] =
		{10*60,
		 30*60,
		 1*60*60,
		 2*60*60,
		 0, /* never */
		 -1};
	static const gint display_times[] =
		{1*60,
		 5*60,
		 10*60,
		 30*60,
		 1*60*60,
		 0, /* never */
		 -1};

	gpm_prefs_setup_time_combo (prefs, "combobox_ac_computer",
				    GPM_CONF_TIMEOUT_SLEEP_COMPUTER_AC,
				    computer_times);
	gpm_prefs_setup_time_combo (prefs, "combobox_ac_display",
				    GPM_CONF_TIMEOUT_SLEEP_DISPLAY_AC,
				    display_times);

	gpm_prefs_setup_action_combo (prefs, "combobox_ac_lid",
				      GPM_CONF_BUTTON_LID_AC,
				      button_lid_actions);

	gpm_prefs_setup_brightness_slider (prefs, "hscale_ac_brightness",
					   GPM_CONF_BACKLIGHT_BRIGHTNESS_AC);

	gpm_prefs_setup_checkbox (prefs, "checkbutton_ac_display_dim",
				  GPM_CONF_BACKLIGHT_IDLE_DIM_AC);
	gpm_prefs_setup_checkbox (prefs, "checkbutton_ac_spindown",
				  GPM_CONF_DISKS_SPINDOWN_ENABLE_AC);

	if (prefs->priv->has_button_lid == FALSE) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "hbox_ac_lid"));
		gtk_widget_hide_all (widget);
	}
	if (prefs->priv->has_lcd == FALSE) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "hbox_ac_brightness"));
		gtk_widget_hide_all (widget);
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "checkbutton_ac_display_dim"));
		gtk_widget_hide_all (widget);
	}
}

static void
prefs_setup_battery (GpmPrefs *prefs)
{
	GtkWidget *widget;
	GtkNotebook *notebook;
	gint page;

	const GpmActionPolicy button_lid_actions[] =
				{GPM_ACTION_POLICY_NOTHING,
				 GPM_ACTION_POLICY_BLANK,
				 GPM_ACTION_POLICY_SUSPEND,
				 GPM_ACTION_POLICY_HIBERNATE,
				 GPM_ACTION_POLICY_SHUTDOWN,
				 -1};
	const GpmActionPolicy battery_critical_actions[] =
				{GPM_ACTION_POLICY_NOTHING,
				 GPM_ACTION_POLICY_SUSPEND,
				 GPM_ACTION_POLICY_HIBERNATE,
				 GPM_ACTION_POLICY_SHUTDOWN,
				 -1};

	static const gint computer_times[] =
		{10*60,
		 30*60,
		 1*60*60,
		 2*60*60,
		 0, /* never */
		 -1};
	static const gint display_times[] =
		{1*60,
		 5*60,
		 10*60,
		 30*60,
		 1*60*60,
		 0, /* never */
		 -1};

	gpm_prefs_setup_time_combo (prefs, "combobox_battery_computer",
				    GPM_CONF_TIMEOUT_SLEEP_COMPUTER_BATT,
				    computer_times);
	gpm_prefs_setup_time_combo (prefs, "combobox_battery_display",
				    GPM_CONF_TIMEOUT_SLEEP_DISPLAY_BATT,
				    display_times);

	if (prefs->priv->has_batteries == FALSE) {
		notebook = GTK_NOTEBOOK (gtk_builder_get_object (prefs->priv->builder, "notebook_preferences"));
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "vbox_battery"));
		page = gtk_notebook_page_num (notebook, GTK_WIDGET (widget));
		gtk_notebook_remove_page (notebook, page);
		return;
	}

	gpm_prefs_setup_action_combo (prefs, "combobox_battery_lid",
				      GPM_CONF_BUTTON_LID_BATT,
				      button_lid_actions);
	gpm_prefs_setup_action_combo (prefs, "combobox_battery_critical",
				      GPM_CONF_ACTIONS_CRITICAL_BATT,
				      battery_critical_actions);

	/* set up the battery reduce checkbox */
	gpm_prefs_setup_checkbox (prefs, "checkbutton_battery_display_reduce",
				  GPM_CONF_BACKLIGHT_BATTERY_REDUCE);
	gpm_prefs_setup_checkbox (prefs, "checkbutton_battery_display_dim",
				  GPM_CONF_BACKLIGHT_IDLE_DIM_BATT);
	gpm_prefs_setup_checkbox (prefs, "checkbutton_battery_spindown",
				  GPM_CONF_DISKS_SPINDOWN_ENABLE_BATT);

	if (prefs->priv->has_button_lid == FALSE) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "hbox_battery_lid"));
		gtk_widget_hide_all (widget);
	}
	if (prefs->priv->has_lcd == FALSE) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "checkbutton_battery_display_dim"));
		gtk_widget_hide_all (widget);
	}
}

static void
prefs_setup_ups (GpmPrefs *prefs)
{
	GtkWidget *widget;
	GtkNotebook *notebook;
	gint page;

	const GpmActionPolicy ups_low_actions[] =
				{GPM_ACTION_POLICY_NOTHING,
				 GPM_ACTION_POLICY_HIBERNATE,
				 GPM_ACTION_POLICY_SHUTDOWN,
				 -1};

	static const gint computer_times[] =
		{10*60,
		 30*60,
		 1*60*60,
		 2*60*60,
		 0, /* never */
		 -1};
	static const gint display_times[] =
		{1*60,
		 5*60,
		 10*60,
		 30*60,
		 1*60*60,
		 0, /* never */
		 -1};

	gpm_prefs_setup_time_combo (prefs, "combobox_ups_computer",
				    GPM_CONF_TIMEOUT_SLEEP_COMPUTER_UPS,
				    computer_times);
	gpm_prefs_setup_time_combo (prefs, "combobox_ups_display",
				    GPM_CONF_TIMEOUT_SLEEP_DISPLAY_UPS,
				    display_times);

	if (prefs->priv->has_ups == FALSE) {
		notebook = GTK_NOTEBOOK (gtk_builder_get_object (prefs->priv->builder, "notebook_preferences"));
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "vbox_ups"));
		page = gtk_notebook_page_num (notebook, GTK_WIDGET (widget));
		gtk_notebook_remove_page (notebook, page);
		return;
	}

	gpm_prefs_setup_action_combo (prefs, "combobox_ups_low",
				      GPM_CONF_ACTIONS_LOW_UPS,
				      ups_low_actions);
	gpm_prefs_setup_action_combo (prefs, "combobox_ups_critical",
				      GPM_CONF_ACTIONS_CRITICAL_UPS,
				      ups_low_actions);
}

static void
prefs_setup_general (GpmPrefs *prefs)
{
	GtkWidget *widget;
	const GpmActionPolicy power_button_actions[] =
				{GPM_ACTION_POLICY_INTERACTIVE,
				 GPM_ACTION_POLICY_SUSPEND,
				 GPM_ACTION_POLICY_HIBERNATE,
				 GPM_ACTION_POLICY_SHUTDOWN,
				 -1};
	const GpmActionPolicy suspend_button_actions[] =
				{GPM_ACTION_POLICY_NOTHING,
				 GPM_ACTION_POLICY_SUSPEND,
				 GPM_ACTION_POLICY_HIBERNATE,
				 -1};

	gpm_prefs_setup_action_combo (prefs, "combobox_general_power",
				      GPM_CONF_BUTTON_POWER,
				      power_button_actions);
	gpm_prefs_setup_action_combo (prefs, "combobox_general_suspend",
				      GPM_CONF_BUTTON_SUSPEND,
				      suspend_button_actions);

	if (prefs->priv->has_button_suspend == FALSE) {
		widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "hbox_general_suspend"));
		gtk_widget_hide_all (widget);
	}
}

/**
 * gpm_prefs_set_defaults_cb:
 **/
static void
gpm_prefs_set_defaults_cb (GtkWidget *widget, GpmPrefs *prefs)
{
	MateConfClient *client;
	DBusGProxy *proxy;
	DBusGConnection *connection;
	GError *error = NULL;
	const gchar *keys[5] = {
		"/apps/mate-power-manager/actions",
		"/apps/mate-power-manager/ui",
		"/apps/mate-power-manager/buttons",
		"/apps/mate-power-manager/backlight",
		"/apps/mate-power-manager/timeout"
	};

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_warning ("failed to get system bus connection: %s", error->message);
		g_error_free (error);
		return;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   "org.mate.MateConf.Defaults",
					   "/",
					   "org.mate.MateConf.Defaults");
	if (proxy == NULL) {
		g_warning ("Cannot connect to defaults mechanism");
		return;
	}

	client = mateconf_client_get_default ();
	mateconf_client_suggest_sync (client, NULL);
	g_object_unref (client);
	dbus_g_proxy_call (proxy, "SetSystem", &error,
			   G_TYPE_STRV, keys,
			   G_TYPE_STRV, NULL,
			   G_TYPE_INVALID, G_TYPE_INVALID);

	g_object_unref (proxy);
}

/**
 * gpm_prefs_init:
 * @prefs: This prefs class instance
 **/
static void
gpm_prefs_init (GpmPrefs *prefs)
{
	GtkWidget *main_window;
	GtkWidget *widget;
	gint caps;
	guint retval;
	GError *error = NULL;

	prefs->priv = GPM_PREFS_GET_PRIVATE (prefs);

	prefs->priv->client = up_client_new ();
	prefs->priv->console = egg_console_kit_new ();
	prefs->priv->conf = mateconf_client_get_default ();
	/* watch mate-power-manager keys */
	mateconf_client_add_dir (prefs->priv->conf, GPM_CONF_DIR,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	mateconf_client_notify_add (prefs->priv->conf, GPM_CONF_DIR,
				 (MateConfClientNotifyFunc) gpm_conf_mateconf_key_changed_cb,
				 prefs, NULL, NULL);

	/* get value of delay in mate-session */
	prefs->priv->idle_delay = mateconf_client_get_int (prefs->priv->conf, GPM_CONF_IDLE_DELAY, NULL);

	caps = gpm_dbus_get_caps (prefs);
	egg_debug ("caps=%i", caps);

	/* get properties from mate-power-manager */
	prefs->priv->has_batteries = ((caps & GPM_PREFS_SERVER_BATTERY) > 0);
	prefs->priv->has_ups = ((caps & GPM_PREFS_SERVER_UPS) > 0);
	prefs->priv->has_lcd = ((caps & GPM_PREFS_SERVER_BACKLIGHT) > 0);
	prefs->priv->has_button_lid = ((caps & GPM_PREFS_SERVER_LID) > 0);
	prefs->priv->has_button_suspend = TRUE;

	/* are we allowed to shutdown? */
	prefs->priv->can_shutdown = TRUE;
	egg_console_kit_can_stop (prefs->priv->console, &prefs->priv->can_shutdown, NULL);

	/* get values from UpClient */
	g_object_get (prefs->priv->client,
		      "can-suspend", &prefs->priv->can_suspend,
		      "can-hibernate", &prefs->priv->can_hibernate,
		      NULL);

	prefs->priv->builder = gtk_builder_new ();
	retval = gtk_builder_add_from_file (prefs->priv->builder, GPM_DATA "/gpm-prefs.ui", &error);
	if (retval == 0) {
		egg_warning ("failed to load ui: %s", error->message);
		g_error_free (error);
	}

	main_window = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "dialog_preferences"));

	/* Hide window first so that the dialogue resizes itself without redrawing */
	gtk_widget_hide (main_window);
	gtk_window_set_default_icon_name (GPM_STOCK_APP_ICON);

	/* Get the main window quit */
	g_signal_connect (main_window, "delete_event",
			  G_CALLBACK (gpm_prefs_delete_event_cb), prefs);

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "button_close"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpm_prefs_close_cb), prefs);

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "button_help"));
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpm_prefs_help_cb), prefs);

	widget = GTK_WIDGET (gtk_builder_get_object (prefs->priv->builder, "button_defaults"));
#ifdef HAVE_MATECONF_DEFAULTS
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (gpm_prefs_set_defaults_cb), prefs);
#else
	gtk_widget_hide (widget);
#endif

	prefs_setup_ac (prefs);
	prefs_setup_battery (prefs);
	prefs_setup_ups (prefs);
	prefs_setup_general (prefs);
	prefs_setup_notification (prefs);

	gtk_widget_show (main_window);
}

/**
 * gpm_prefs_finalize:
 * @object: This prefs class instance
 **/
static void
gpm_prefs_finalize (GObject *object)
{
	GpmPrefs *prefs;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_PREFS (object));

	prefs = GPM_PREFS (object);
	prefs->priv = GPM_PREFS_GET_PRIVATE (prefs);

	g_object_unref (prefs->priv->conf);
	g_object_unref (prefs->priv->client);
	g_object_unref (prefs->priv->console);

	G_OBJECT_CLASS (gpm_prefs_parent_class)->finalize (object);
}

/**
 * gpm_prefs_new:
 * Return value: new GpmPrefs instance.
 **/
GpmPrefs *
gpm_prefs_new (void)
{
	GpmPrefs *prefs;
	prefs = g_object_new (GPM_TYPE_PREFS, NULL);
	return GPM_PREFS (prefs);
}
