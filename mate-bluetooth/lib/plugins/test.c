/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
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

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <bluetooth-plugin.h>

static gboolean
has_config_widget (const char *bdaddr, const char **uuids)
{
	guint i;

	if (uuids == NULL)
		return FALSE;
	for (i = 0; uuids[i] != NULL; i++) {
		if (g_str_equal (uuids[i], "PANU"))
			return TRUE;
	}
	return FALSE;
}

static GtkWidget *
get_config_widgets (const char *bdaddr, const char **uuids)
{
	/* translators:
	 * This is in a test plugin, please make sure you add the "(test)" part,
	 * or leave untranslated */
	return gtk_check_button_new_with_label (_("Access the Internet using your cell phone (test)"));
}

static void
device_removed (const char *bdaddr)
{
	g_message ("Device '%s' got removed", bdaddr);
}

static GbtPluginInfo plugin_info = {
	"test-plugin",
	has_config_widget,
	get_config_widgets,
	device_removed
};

GBT_INIT_PLUGIN(plugin_info)

