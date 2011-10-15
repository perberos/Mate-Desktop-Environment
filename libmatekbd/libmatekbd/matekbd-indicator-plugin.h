/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MATEKBD_INDICATOR_PLUGIN_H__
#define __MATEKBD_INDICATOR_PLUGIN_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <libmatekbd/matekbd-keyboard-config.h>

#define MAX_LOCAL_NAME_BUF_LENGTH 512

struct _MatekbdIndicatorPlugin;

typedef struct _MatekbdIndicatorPluginContainer {
	MateConfClient *conf_client;
} MatekbdIndicatorPluginContainer;

typedef const struct _MatekbdIndicatorPlugin
*(*MatekbdIndicatorPluginGetPluginFunc) (void);

typedef
 gboolean (*MatekbdIndicatorPluginInitFunc) (MatekbdIndicatorPluginContainer *
					  pc);

typedef void (*MatekbdIndicatorPluginGroupChangedFunc) (GtkWidget * notebook,
						     int new_group);

typedef void (*MatekbdIndicatorPluginConfigChangedFunc) (const
						      MatekbdKeyboardConfig *
						      from,
						      const
						      MatekbdKeyboardConfig *
						      to);

typedef int (*MatekbdIndicatorPluginWindowCreatedFunc) (const Window win,
						     const Window parent);

typedef void (*MatekbdIndicatorPluginTermFunc) (void);

typedef GtkWidget *(*MatekbdIndicatorPluginCreateWidget) (void);

typedef GtkWidget *(*MatekbdIndicatorPluginDecorateWidget) (GtkWidget *
							 widget,
							 const gint group,
							 const char
							 *group_description,
							 MatekbdKeyboardConfig
							 * config);

typedef
void (*MatekbdIndicatorPluginConfigureProperties)
 (MatekbdIndicatorPluginContainer * pc, GtkWindow * parent);

typedef struct _MatekbdIndicatorPlugin {
	const char *name;

	const char *description;

/* implemented */
	MatekbdIndicatorPluginInitFunc init_callback;

/* implemented */
	MatekbdIndicatorPluginTermFunc term_callback;

/* implemented */
	 MatekbdIndicatorPluginConfigureProperties
	    configure_properties_callback;

/* implemented */
	MatekbdIndicatorPluginGroupChangedFunc group_changed_callback;

/* implemented */
	MatekbdIndicatorPluginWindowCreatedFunc window_created_callback;

/* implemented */
	MatekbdIndicatorPluginDecorateWidget decorate_widget_callback;

/* not implemented */
	MatekbdIndicatorPluginConfigChangedFunc config_changed_callback;

/* not implemented */
	MatekbdIndicatorPluginCreateWidget create_widget_callback;

} MatekbdIndicatorPlugin;

/**
 * Functions accessible for plugins
 */

extern void
 matekbd_indicator_plugin_container_init (MatekbdIndicatorPluginContainer * pc,
				       MateConfClient * conf_client);

extern void
 matekbd_indicator_plugin_container_term (MatekbdIndicatorPluginContainer * pc);

extern void
 matekbd_indicator_plugin_container_reinit_ui (MatekbdIndicatorPluginContainer * pc);

extern guint
matekbd_indicator_plugin_get_num_groups (MatekbdIndicatorPluginContainer * pc);

extern gchar
    **
    matekbd_indicator_plugin_load_localized_group_names
    (MatekbdIndicatorPluginContainer * pc);

#endif
