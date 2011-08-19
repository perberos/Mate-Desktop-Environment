/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Javier Goday <jgoday@gmail.com>
 * Based on the sidebar-test totem plugin example 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>

#include "totem-plugin.h"
#include "totem.h"
#include "totem-tracker-widget.h"

#define TOTEM_TYPE_TRACKER_PLUGIN		(totem_tracker_plugin_get_type ())
#define TOTEM_TRACKER_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_TRACKER_PLUGIN, TotemTrackerPlugin))
#define TOTEM_TRACKER_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_TRACKER_PLUGIN, TotemTrackerPluginClass))
#define TOTEM_IS_TRACKER_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_TRACKER_PLUGIN))
#define TOTEM_IS_TRACKER_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_TRACKER_PLUGIN))
#define TOTEM_TRACKER_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_TRACKER_PLUGIN, TotemTrackerPluginClass))

typedef struct
{
	TotemPlugin   parent;
} TotemTrackerPlugin;

typedef struct
{
	TotemPluginClass parent_class;
} TotemTrackerPluginClass;


G_MODULE_EXPORT GType register_totem_plugin		(GTypeModule *module);
GType	totem_tracker_plugin_get_type			(void) G_GNUC_CONST;

static gboolean impl_activate				(TotemPlugin *plugin, TotemObject *totem, GError **error);
static void impl_deactivate				(TotemPlugin *plugin, TotemObject *totem);

TOTEM_PLUGIN_REGISTER (TotemTrackerPlugin, totem_tracker_plugin)

static void
totem_tracker_plugin_class_init (TotemTrackerPluginClass *klass)
{
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
totem_tracker_plugin_init (TotemTrackerPlugin *plugin)
{
}

static gboolean
impl_activate (TotemPlugin *plugin,
	       TotemObject *totem,
	       GError **error)
{
	GtkWidget *widget;

	widget = totem_tracker_widget_new (totem);
	gtk_widget_show (widget);
	totem_add_sidebar_page (totem, "tracker", _("Local Search"), widget);

	return TRUE;
}

static void
impl_deactivate	(TotemPlugin *plugin,
		 TotemObject *totem)
{
	totem_remove_sidebar_page (totem, "tracker");
}

