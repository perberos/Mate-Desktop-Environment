/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009, Intel Corporation.
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
 *  Written by: Joshua Lock <josh@linux.intel.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <mx/mx-gtk.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-gtk.h>
#include <bluetooth-enums.h>

#include "moblin-panel.h"

static void
bluetooth_status_changed (MoblinPanel *panel, gboolean connecting, gpointer user_data)
{
	MplPanelClient *client = MPL_PANEL_CLIENT (user_data);
	gchar *style = NULL;

	if (connecting) {
		style = g_strdup ("state-connecting");
	} else {
		style = g_strdup ("state-idle");
	}

	mpl_panel_client_request_button_style (client, style);
	g_free (style);
}

static void
panel_request_focus (MoblinPanel *panel, gpointer user_data)
{
	MplPanelClient *client = MPL_PANEL_CLIENT (user_data);

	mpl_panel_client_request_focus (client);
}

/*
 * When the panel is hidden we should re-set the MoblinPanel to its default state.
 * i.e. stop any discovery and show the defaults devices view
 */
static void
_reset_view_cb (MplPanelClient *client, gpointer user_data)
{
	MoblinPanel *panel = MOBLIN_PANEL (user_data);

	moblin_panel_reset_view (panel);
}

int
main (int argc, char *argv[])
{
	MplPanelClient *panel;
	GtkWidget      *window, *content;
	GtkRequisition  req;
	gboolean        standalone = FALSE;
	GtkSettings    *settings;
	GError         *error = NULL;
	GOptionEntry    entries[] = {
		{ "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
		_("Run in standalone mode"), NULL },
		{ NULL }
	};

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_set_application_name (_("Moblin Bluetooth Panel"));
	gtk_init_with_args (&argc, &argv, _("- Moblin Bluetooth Panel"),
				entries, GETTEXT_PACKAGE, &error);

	if (error) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		return 1;
	}

	gtk_window_set_default_icon_name ("bluetooth");

	/* Force to correct theme */
	settings = gtk_settings_get_default ();
	gtk_settings_set_string_property (settings, "gtk-theme-name",
					"Moblin-Netbook", NULL);

	if (standalone) {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		g_signal_connect (window, "delete-event", (GCallback) gtk_main_quit,
				NULL);
		gtk_widget_set_size_request (window, 1000, -1);
		content = moblin_panel_new ();
		gtk_widget_show (content);

		gtk_container_add (GTK_CONTAINER (window), content);
		gtk_widget_show (window);
	}  else {
		panel = mpl_panel_gtk_new (MPL_PANEL_BLUETOOTH, _("bluetooth"),
					THEME_DIR "/bluetooth-panel.css",
					"state-idle", TRUE);
		window  = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel));

		content = moblin_panel_new ();
		g_signal_connect (panel, "hide-end", (GCallback) _reset_view_cb, content);
		g_signal_connect (content, "state-changed",
				G_CALLBACK (bluetooth_status_changed), panel);
		g_signal_connect (content, "request-focus",
				  G_CALLBACK (panel_request_focus), panel);
		gtk_widget_show (content);

		gtk_container_add (GTK_CONTAINER (window), content);
		gtk_widget_size_request (window, &req);
		mpl_panel_client_set_height_request (panel, req.height);
	}

	gtk_main ();

	return 0;
}
