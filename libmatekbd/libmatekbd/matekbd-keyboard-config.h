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

#ifndef __MATEKBD_KEYBOARD_CONFIG_H__
#define __MATEKBD_KEYBOARD_CONFIG_H__

#include <X11/Xlib.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <libxklavier/xklavier.h>

extern const gchar MATEKBD_KEYBOARD_CONFIG_DIR[];
extern const gchar MATEKBD_KEYBOARD_CONFIG_KEY_MODEL[];
extern const gchar MATEKBD_KEYBOARD_CONFIG_KEY_LAYOUTS[];
extern const gchar MATEKBD_KEYBOARD_CONFIG_KEY_OPTIONS[];

/*
 * Keyboard Configuration
 */
typedef struct _MatekbdKeyboardConfig {
	gchar *model;
	GSList *layouts_variants;
	GSList *options;

	/* private, transient */
	MateConfClient *conf_client;
	int config_listener_id;
	XklEngine *engine;
} MatekbdKeyboardConfig;

/**
 * MatekbdKeyboardConfig functions
 */
extern void matekbd_keyboard_config_init (MatekbdKeyboardConfig * kbd_config,
				       MateConfClient * conf_client,
				       XklEngine * engine);
extern void matekbd_keyboard_config_term (MatekbdKeyboardConfig * kbd_config);

extern void matekbd_keyboard_config_load_from_mateconf (MatekbdKeyboardConfig *
						  kbd_config,
						  MatekbdKeyboardConfig *
						  kbd_config_default);

extern void matekbd_keyboard_config_save_to_mateconf (MatekbdKeyboardConfig *
						kbd_config);

extern void matekbd_keyboard_config_load_from_x_initial (MatekbdKeyboardConfig *
						      kbd_config,
						      XklConfigRec * buf);

extern void matekbd_keyboard_config_load_from_x_current (MatekbdKeyboardConfig *
						      kbd_config,
						      XklConfigRec * buf);

extern void matekbd_keyboard_config_start_listen (MatekbdKeyboardConfig *
					       kbd_config,
					       MateConfClientNotifyFunc func,
					       gpointer user_data);

extern void matekbd_keyboard_config_stop_listen (MatekbdKeyboardConfig *
					      kbd_config);

extern gboolean matekbd_keyboard_config_equals (MatekbdKeyboardConfig *
					     kbd_config1,
					     MatekbdKeyboardConfig *
					     kbd_config2);

extern gboolean matekbd_keyboard_config_activate (MatekbdKeyboardConfig *
					       kbd_config);

extern const gchar *matekbd_keyboard_config_merge_items (const gchar * parent,
						      const gchar * child);

extern gboolean matekbd_keyboard_config_split_items (const gchar * merged,
						  gchar ** parent,
						  gchar ** child);

extern gboolean matekbd_keyboard_config_get_descriptions (XklConfigRegistry *
						       config_registry,
						       const gchar * name,
						       gchar **
						       layout_short_descr,
						       gchar **
						       layout_descr,
						       gchar **
						       variant_short_descr,
						       gchar **
						       variant_descr);

extern const gchar *matekbd_keyboard_config_format_full_layout (const gchar
							     *
							     layout_descr,
							     const gchar *
							     variant_descr);

extern gchar *matekbd_keyboard_config_to_string (const MatekbdKeyboardConfig *
					      config);

extern GSList
    *matekbd_keyboard_config_add_default_switch_option_if_necessary (GSList *
								  layouts_list,
								  GSList *
								  options_list,
								  gboolean
								  *
								  was_appended);

#endif
