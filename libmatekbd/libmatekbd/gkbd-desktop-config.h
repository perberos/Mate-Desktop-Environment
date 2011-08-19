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

#ifndef __GKBD_DESKTOP_CONFIG_H__
#define __GKBD_DESKTOP_CONFIG_H__

#include <X11/Xlib.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <libxklavier/xklavier.h>

extern const gchar GKBD_DESKTOP_CONFIG_DIR[];
extern const gchar GKBD_DESKTOP_CONFIG_KEY_DEFAULT_GROUP[];
extern const gchar GKBD_DESKTOP_CONFIG_KEY_GROUP_PER_WINDOW[];
extern const gchar GKBD_DESKTOP_CONFIG_KEY_HANDLE_INDICATORS[];
extern const gchar GKBD_DESKTOP_CONFIG_KEY_LAYOUT_NAMES_AS_GROUP_NAMES[];

/*
 * General configuration
 */
typedef struct _GkbdDesktopConfig {
	gint default_group;
	gboolean group_per_app;
	gboolean handle_indicators;
	gboolean layout_names_as_group_names;
	gboolean load_extra_items;

	/* private, transient */
	MateConfClient *conf_client;
	int config_listener_id;
	XklEngine *engine;
} GkbdDesktopConfig;

/**
 * GkbdDesktopConfig functions
 */
extern void gkbd_desktop_config_init (GkbdDesktopConfig * config,
				      MateConfClient * conf_client,
				      XklEngine * engine);
extern void gkbd_desktop_config_term (GkbdDesktopConfig * config);

extern void gkbd_desktop_config_load_from_mateconf (GkbdDesktopConfig *
						 config);

extern void gkbd_desktop_config_save_to_mateconf (GkbdDesktopConfig * config);

extern gboolean gkbd_desktop_config_activate (GkbdDesktopConfig * config);

extern gboolean
gkbd_desktop_config_load_group_descriptions (GkbdDesktopConfig
					     * config,
					     XklConfigRegistry *
					     registry,
					     const gchar **
					     layout_ids,
					     const gchar **
					     variant_ids,
					     gchar ***
					     short_group_names,
					     gchar *** full_group_names);

extern void gkbd_desktop_config_lock_next_group (GkbdDesktopConfig *
						 config);

extern void gkbd_desktop_config_lock_prev_group (GkbdDesktopConfig *
						 config);

extern void gkbd_desktop_config_restore_group (GkbdDesktopConfig * config);

extern void gkbd_desktop_config_start_listen (GkbdDesktopConfig * config,
					      MateConfClientNotifyFunc func,
					      gpointer user_data);

extern void gkbd_desktop_config_stop_listen (GkbdDesktopConfig * config);

#endif
