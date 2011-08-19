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

#ifndef __GKBD_INDICATOR_CONFIG_H__
#define __GKBD_INDICATOR_CONFIG_H__

#include <gtk/gtk.h>

#include "libmatekbd/gkbd-keyboard-config.h"

/*
 * Indicator configuration
 */
typedef struct _GkbdIndicatorConfig {
	int secondary_groups_mask;
	gboolean show_flags;

	gchar *font_family;
	int font_size;
	gchar *foreground_color;
	gchar *background_color;

	GSList *enabled_plugins;

	/* private, transient */
	MateConfClient *conf_client;
	GSList *image_filenames;
	GtkIconTheme *icon_theme;
	int config_listener_id;
	XklEngine *engine;
} GkbdIndicatorConfig;

/**
 * GkbdIndicatorConfig functions - 
 * some of them require GkbdKeyboardConfig as well - 
 * for loading approptiate images
 */
extern void gkbd_indicator_config_init (GkbdIndicatorConfig *
					applet_config,
					MateConfClient * conf_client,
					XklEngine * engine);
extern void gkbd_indicator_config_term (GkbdIndicatorConfig *
					applet_config);

extern void gkbd_indicator_config_load_from_mateconf (GkbdIndicatorConfig
						   * applet_config);
extern void gkbd_indicator_config_save_to_mateconf (GkbdIndicatorConfig *
						 applet_config);

extern void gkbd_indicator_config_refresh_style (GkbdIndicatorConfig *
						 applet_config);

extern gchar
    * gkbd_indicator_config_get_images_file (GkbdIndicatorConfig *
					     applet_config,
					     GkbdKeyboardConfig *
					     kbd_config, int group);

extern void gkbd_indicator_config_load_image_filenames (GkbdIndicatorConfig
							* applet_config,
							GkbdKeyboardConfig
							* kbd_config);
extern void gkbd_indicator_config_free_image_filenames (GkbdIndicatorConfig
							* applet_config);

/* Should be updated on Indicator/MateConf configuration change */
extern void gkbd_indicator_config_activate (GkbdIndicatorConfig *
					    applet_config);

extern void gkbd_indicator_config_start_listen (GkbdIndicatorConfig *
						applet_config,
						MateConfClientNotifyFunc
						func, gpointer user_data);

extern void gkbd_indicator_config_stop_listen (GkbdIndicatorConfig *
					       applet_config);

#endif
