/* -*- mode: c; style: linux -*- */

/* mate-keyboard-properties-xkb.c
 * Copyright (C) 2003-2007 Sergey V. Udaltsov
 *
 * Written by: Sergey V. Udaltsov <svu@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gdk/gdkx.h>
#include <mateconf/mateconf-client.h>
#include <glib/gi18n.h>

#include "capplet-util.h"
#include "mateconf-property-editor.h"

#include "mate-keyboard-properties-xkb.h"

#include <libmatekbd/matekbd-desktop-config.h>

XklEngine *engine;
XklConfigRegistry *config_registry;

MatekbdKeyboardConfig initial_config;
MatekbdDesktopConfig desktop_config;

MateConfClient *xkb_mateconf_client;

char *
xci_desc_to_utf8 (XklConfigItem * ci)
{
	char *sd = g_strstrip (ci->description);
	return sd[0] == 0 ? g_strdup (ci->name) : g_strdup (sd);
}

static void
set_model_text (GtkWidget * picker, MateConfValue * value)
{
	XklConfigItem *ci = xkl_config_item_new ();
	const char *model = NULL;

	if (value != NULL && value->type == MATECONF_VALUE_STRING) {
		model = mateconf_value_get_string (value);
		if (model != NULL && model[0] == '\0')
			model = NULL;
	}

	if (model == NULL) {
		model = initial_config.model;
		if (model == NULL)
			model = "";
	}

	g_snprintf (ci->name, sizeof (ci->name), "%s", model);

	if (xkl_config_registry_find_model (config_registry, ci)) {
		char *d;

		d = xci_desc_to_utf8 (ci);
		gtk_button_set_label (GTK_BUTTON (picker), d);
		g_free (d);
	} else {
		gtk_button_set_label (GTK_BUTTON (picker), _("Unknown"));
	}
	g_object_unref (G_OBJECT (ci));
}

static void
model_key_changed (MateConfClient * client,
		   guint cnxn_id, MateConfEntry * entry, GtkBuilder * dialog)
{
	set_model_text (WID ("xkb_model_pick"),
			mateconf_entry_get_value (entry));

	enable_disable_restoring (dialog);
}

static void
setup_model_entry (GtkBuilder * dialog)
{
	MateConfValue *value;

	value = mateconf_client_get (xkb_mateconf_client,
				  MATEKBD_KEYBOARD_CONFIG_KEY_MODEL, NULL);
	set_model_text (WID ("xkb_model_pick"), value);
	if (value != NULL)
		mateconf_value_free (value);

	mateconf_client_notify_add (xkb_mateconf_client,
				 MATEKBD_KEYBOARD_CONFIG_KEY_MODEL,
				 (MateConfClientNotifyFunc) model_key_changed,
				 dialog, NULL, NULL);
}

static void
cleanup_xkb_tabs (GtkBuilder * dialog)
{
	matekbd_desktop_config_term (&desktop_config);
	matekbd_keyboard_config_term (&initial_config);
	g_object_unref (G_OBJECT (config_registry));
	config_registry = NULL;
	g_object_unref (G_OBJECT (engine));
	engine = NULL;
	g_object_unref (G_OBJECT (xkb_mateconf_client));
	xkb_mateconf_client = NULL;
}

static void
reset_to_defaults (GtkWidget * button, GtkBuilder * dialog)
{
	MatekbdKeyboardConfig empty_kbd_config;

	matekbd_keyboard_config_init (&empty_kbd_config, xkb_mateconf_client,
				   engine);
	matekbd_keyboard_config_save_to_mateconf (&empty_kbd_config);
	matekbd_keyboard_config_term (&empty_kbd_config);

	mateconf_client_unset (xkb_mateconf_client,
			    MATEKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP, NULL);

	/* all the rest is g-s-d's business */
}

static void
chk_separate_group_per_window_toggled (MateConfPropertyEditor * peditor,
				       const gchar * key,
				       const MateConfValue * value,
				       GtkBuilder * dialog)
{
	gtk_widget_set_sensitive (WID ("chk_new_windows_inherit_layout"),
				  mateconf_value_get_bool (value));
}

static void
chk_new_windows_inherit_layout_toggled (GtkWidget *
					chk_new_windows_inherit_layout,
					GtkBuilder * dialog)
{
	xkb_save_default_group (gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON
				 (chk_new_windows_inherit_layout)) ? -1 :
				0);
}

void
setup_xkb_tabs (GtkBuilder * dialog, MateConfChangeSet * changeset)
{
	GObject *peditor;
	GtkWidget *chk_new_windows_inherit_layout =
	    WID ("chk_new_windows_inherit_layout");

	xkb_mateconf_client = mateconf_client_get_default ();

	engine = xkl_engine_get_instance (GDK_DISPLAY ());
	config_registry = xkl_config_registry_get_instance (engine);

	matekbd_desktop_config_init (&desktop_config, xkb_mateconf_client,
				  engine);
	matekbd_desktop_config_load_from_mateconf (&desktop_config);

	xkl_config_registry_load (config_registry,
				  desktop_config.load_extra_items);

	matekbd_keyboard_config_init (&initial_config, xkb_mateconf_client,
				   engine);
	matekbd_keyboard_config_load_from_x_initial (&initial_config, NULL);

	setup_model_entry (dialog);

	peditor = mateconf_peditor_new_boolean
	    (changeset, (gchar *) MATEKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW,
	     WID ("chk_separate_group_per_window"), NULL);

	g_signal_connect (peditor, "value-changed", (GCallback)
			  chk_separate_group_per_window_toggled, dialog);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
	if (strcmp (xkl_engine_get_backend_name (engine), "XKB"))
#endif
		gtk_widget_hide (WID ("xkb_layouts_print"));

	xkb_layouts_prepare_selected_tree (dialog, changeset);
	xkb_layouts_fill_selected_tree (dialog);

	gtk_widget_set_sensitive (chk_new_windows_inherit_layout,
				  gtk_toggle_button_get_active
				  (GTK_TOGGLE_BUTTON
				   (WID
				    ("chk_separate_group_per_window"))));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (chk_new_windows_inherit_layout),
				      xkb_get_default_group () < 0);

	xkb_layouts_register_buttons_handlers (dialog);
	g_signal_connect (G_OBJECT (WID ("xkb_reset_to_defaults")),
			  "clicked", G_CALLBACK (reset_to_defaults),
			  dialog);

	g_signal_connect (G_OBJECT (chk_new_windows_inherit_layout),
			  "toggled", (GCallback)
			  chk_new_windows_inherit_layout_toggled, dialog);

	g_signal_connect_swapped (G_OBJECT (WID ("xkb_layout_options")),
				  "clicked",
				  G_CALLBACK (xkb_options_popup_dialog),
				  dialog);

	g_signal_connect_swapped (G_OBJECT (WID ("xkb_model_pick")),
				  "clicked", G_CALLBACK (choose_model),
				  dialog);

	xkb_layouts_register_mateconf_listener (dialog);
	xkb_options_register_mateconf_listener (dialog);

	g_signal_connect (G_OBJECT (WID ("keyboard_dialog")),
			  "destroy", G_CALLBACK (cleanup_xkb_tabs),
			  dialog);

	enable_disable_restoring (dialog);
}

void
enable_disable_restoring (GtkBuilder * dialog)
{
	MatekbdKeyboardConfig gswic;
	gboolean enable;

	matekbd_keyboard_config_init (&gswic, xkb_mateconf_client, engine);
	matekbd_keyboard_config_load_from_mateconf (&gswic, NULL);

	enable = !matekbd_keyboard_config_equals (&gswic, &initial_config);

	matekbd_keyboard_config_term (&gswic);
	gtk_widget_set_sensitive (WID ("xkb_reset_to_defaults"), enable);
}
