/* Eye Of Mate - EOG Plugin
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-plugin.h"

G_DEFINE_TYPE (EogPlugin, eog_plugin, G_TYPE_OBJECT)

static void
dummy (EogPlugin *plugin, EogWindow *window)
{
}

static GtkWidget *
create_configure_dialog	(EogPlugin *plugin)
{
	return NULL;
}

static gboolean
is_configurable (EogPlugin *plugin)
{
	return (EOG_PLUGIN_GET_CLASS (plugin)->create_configure_dialog !=
		create_configure_dialog);
}

static void
eog_plugin_class_init (EogPluginClass *klass)
{
	klass->activate = dummy;
	klass->deactivate = dummy;
	klass->update_ui = dummy;

	klass->create_configure_dialog = create_configure_dialog;
	klass->is_configurable = is_configurable;
}

static void
eog_plugin_init (EogPlugin *plugin)
{
}

void
eog_plugin_activate (EogPlugin *plugin, EogWindow *window)
{
	g_return_if_fail (EOG_IS_PLUGIN (plugin));
	g_return_if_fail (EOG_IS_WINDOW (window));

	EOG_PLUGIN_GET_CLASS (plugin)->activate (plugin, window);
}

void
eog_plugin_deactivate (EogPlugin *plugin, EogWindow *window)
{
	g_return_if_fail (EOG_IS_PLUGIN (plugin));
	g_return_if_fail (EOG_IS_WINDOW (window));

	EOG_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, window);
}

void
eog_plugin_update_ui (EogPlugin *plugin, EogWindow *window)
{
	g_return_if_fail (EOG_IS_PLUGIN (plugin));
	g_return_if_fail (EOG_IS_WINDOW (window));

	EOG_PLUGIN_GET_CLASS (plugin)->update_ui (plugin, window);
}

gboolean
eog_plugin_is_configurable (EogPlugin *plugin)
{
	g_return_val_if_fail (EOG_IS_PLUGIN (plugin), FALSE);

	return EOG_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

GtkWidget *
eog_plugin_create_configure_dialog (EogPlugin *plugin)
{
	g_return_val_if_fail (EOG_IS_PLUGIN (plugin), NULL);

	return EOG_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
}
