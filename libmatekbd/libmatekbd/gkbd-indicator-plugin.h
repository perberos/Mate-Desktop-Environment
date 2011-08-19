/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
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

#ifndef __GKBD_INDICATOR_PLUGIN_H__
#define __GKBD_INDICATOR_PLUGIN_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <libmatekbd/gkbd-keyboard-config.h>

#define MAX_LOCAL_NAME_BUF_LENGTH 512

struct _GkbdIndicatorPlugin;

typedef struct _GkbdIndicatorPluginContainer {
	MateConfClient *conf_client;
} GkbdIndicatorPluginContainer;

typedef const struct _GkbdIndicatorPlugin
*(*GkbdIndicatorPluginGetPluginFunc) (void);

typedef
 gboolean (*GkbdIndicatorPluginInitFunc) (GkbdIndicatorPluginContainer *
					  pc);

typedef void (*GkbdIndicatorPluginGroupChangedFunc) (GtkWidget * notebook,
						     int new_group);

typedef void (*GkbdIndicatorPluginConfigChangedFunc) (const
						      GkbdKeyboardConfig *
						      from,
						      const
						      GkbdKeyboardConfig *
						      to);

typedef int (*GkbdIndicatorPluginWindowCreatedFunc) (const Window win,
						     const Window parent);

typedef void (*GkbdIndicatorPluginTermFunc) (void);

typedef GtkWidget *(*GkbdIndicatorPluginCreateWidget) (void);

typedef GtkWidget *(*GkbdIndicatorPluginDecorateWidget) (GtkWidget *
							 widget,
							 const gint group,
							 const char
							 *group_description,
							 GkbdKeyboardConfig
							 * config);

typedef
void (*GkbdIndicatorPluginConfigureProperties)
 (GkbdIndicatorPluginContainer * pc, GtkWindow * parent);

typedef struct _GkbdIndicatorPlugin {
	const char *name;

	const char *description;

/* implemented */
	GkbdIndicatorPluginInitFunc init_callback;

/* implemented */
	GkbdIndicatorPluginTermFunc term_callback;

/* implemented */
	 GkbdIndicatorPluginConfigureProperties
	    configure_properties_callback;

/* implemented */
	GkbdIndicatorPluginGroupChangedFunc group_changed_callback;

/* implemented */
	GkbdIndicatorPluginWindowCreatedFunc window_created_callback;

/* implemented */
	GkbdIndicatorPluginDecorateWidget decorate_widget_callback;

/* not implemented */
	GkbdIndicatorPluginConfigChangedFunc config_changed_callback;

/* not implemented */
	GkbdIndicatorPluginCreateWidget create_widget_callback;

} GkbdIndicatorPlugin;

/**
 * Functions accessible for plugins
 */

extern void
 gkbd_indicator_plugin_container_init (GkbdIndicatorPluginContainer * pc,
				       MateConfClient * conf_client);

extern void
 gkbd_indicator_plugin_container_term (GkbdIndicatorPluginContainer * pc);

extern void
 gkbd_indicator_plugin_container_reinit_ui (GkbdIndicatorPluginContainer * pc);

extern guint
gkbd_indicator_plugin_get_num_groups (GkbdIndicatorPluginContainer * pc);

extern gchar
    **
    gkbd_indicator_plugin_load_localized_group_names
    (GkbdIndicatorPluginContainer * pc);

#endif
