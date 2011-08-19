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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/keysym.h>

#include <pango/pango.h>

#include <glib/gi18n.h>
#include <gdk/gdkx.h>

#include <gkbd-keyboard-config.h>
#include <gkbd-indicator-config.h>

#include <gkbd-config-private.h>

/**
 * GkbdIndicatorConfig
 */
#define GKBD_INDICATOR_CONFIG_KEY_PREFIX  GKBD_CONFIG_KEY_PREFIX "/indicator"
#define GTK_STYLE_PATH "*PanelWidget*"

const gchar GKBD_INDICATOR_CONFIG_DIR[] = GKBD_INDICATOR_CONFIG_KEY_PREFIX;
const gchar GKBD_INDICATOR_CONFIG_KEY_SHOW_FLAGS[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/showFlags";
const gchar GKBD_INDICATOR_CONFIG_KEY_ENABLED_PLUGINS[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/enabledPlugins";
const gchar GKBD_INDICATOR_CONFIG_KEY_SECONDARIES[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/secondary";
const gchar GKBD_INDICATOR_CONFIG_KEY_FONT_FAMILY[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/fontFamily";
const gchar GKBD_INDICATOR_CONFIG_KEY_FONT_SIZE[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/fontSize";
const gchar GKBD_INDICATOR_CONFIG_KEY_FOREGROUND_COLOR[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/foregroundColor";
const gchar GKBD_INDICATOR_CONFIG_KEY_BACKGROUND_COLOR[] =
    GKBD_INDICATOR_CONFIG_KEY_PREFIX "/backgroundColor";

#define SYSTEM_FONT_MATECONF_ENTRY "/desktop/mate/interface/font_name"

/**
 * static applet config functions
 */
static void
gkbd_indicator_config_free_enabled_plugins (GkbdIndicatorConfig *
					    ind_config)
{
	GSList *plugin_node = ind_config->enabled_plugins;
	if (plugin_node != NULL) {
		do {
			if (plugin_node->data != NULL) {
				g_free (plugin_node->data);
				plugin_node->data = NULL;
			}
			plugin_node = g_slist_next (plugin_node);
		} while (plugin_node != NULL);
		g_slist_free (ind_config->enabled_plugins);
		ind_config->enabled_plugins = NULL;
	}
}

static void
gkbd_indicator_config_load_font (GkbdIndicatorConfig * ind_config)
{
	GError *gerror = NULL;

	ind_config->font_family =
	    mateconf_client_get_string (ind_config->conf_client,
				     GKBD_INDICATOR_CONFIG_KEY_FONT_FAMILY,
				     &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		ind_config->font_family = g_strdup ("Helvetica");
		g_error_free (gerror);
		gerror = NULL;
	}

	ind_config->font_size =
	    mateconf_client_get_int (ind_config->conf_client,
				  GKBD_INDICATOR_CONFIG_KEY_FONT_SIZE,
				  &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		ind_config->font_size = 10;
		g_error_free (gerror);
		gerror = NULL;
	}

	if (ind_config->font_family == NULL ||
	    ind_config->font_family[0] == '\0') {
		PangoFontDescription *fd = NULL;
		GtkStyle *style =
		    gtk_rc_get_style_by_paths (gtk_settings_get_default (),
					       GTK_STYLE_PATH,
					       GTK_STYLE_PATH,
					       GTK_TYPE_LABEL);
		if (style != NULL)
			fd = style->font_desc;
		if (fd != NULL) {
			ind_config->font_family =
			    g_strdup (pango_font_description_get_family
				      (fd));
			ind_config->font_size =
			    pango_font_description_get_size (fd) /
			    PANGO_SCALE;
		}
	}
	xkl_debug (150, "font: [%s], size %d\n", ind_config->font_family,
		   ind_config->font_size);

}

static void
gkbd_indicator_config_load_colors (GkbdIndicatorConfig * ind_config)
{
	GError *gerror = NULL;

	ind_config->foreground_color =
	    mateconf_client_get_string (ind_config->conf_client,
				     GKBD_INDICATOR_CONFIG_KEY_FOREGROUND_COLOR,
				     &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}

	if (ind_config->foreground_color == NULL ||
	    ind_config->foreground_color[0] == '\0') {
		GtkStyle *style =
		    gtk_rc_get_style_by_paths (gtk_settings_get_default (),
					       GTK_STYLE_PATH,
					       GTK_STYLE_PATH,
					       GTK_TYPE_LABEL);
		if (style != NULL) {
			ind_config->foreground_color =
			    g_strdup_printf ("%g %g %g", ((double)
							  style->
							  fg
							  [GTK_STATE_NORMAL].red)
					     / 0x10000, ((double)
							 style->
							 fg
							 [GTK_STATE_NORMAL].green)
					     / 0x10000, ((double)
							 style->
							 fg
							 [GTK_STATE_NORMAL].blue)
					     / 0x10000);
		}

	}

	ind_config->background_color =
	    mateconf_client_get_string (ind_config->conf_client,
				     GKBD_INDICATOR_CONFIG_KEY_BACKGROUND_COLOR,
				     &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
}

void
gkbd_indicator_config_refresh_style (GkbdIndicatorConfig * ind_config)
{
	g_free (ind_config->font_family);
	g_free (ind_config->foreground_color);
	g_free (ind_config->background_color);
	gkbd_indicator_config_load_font (ind_config);
	gkbd_indicator_config_load_colors (ind_config);
}

char *
gkbd_indicator_config_get_images_file (GkbdIndicatorConfig *
				       ind_config,
				       GkbdKeyboardConfig *
				       kbd_config, int group)
{
	char *image_file = NULL;
	GtkIconInfo *icon_info = NULL;

	if (!ind_config->show_flags)
		return NULL;

	if ((kbd_config->layouts_variants != NULL) &&
	    (g_slist_length (kbd_config->layouts_variants) > group)) {
		char *full_layout_name = (char *)
		    g_slist_nth_data (kbd_config->layouts_variants, group);

		if (full_layout_name != NULL) {
			char *l, *v;
			gkbd_keyboard_config_split_items (full_layout_name,
							  &l, &v);
			if (l != NULL) {
				/* probably there is something in theme? */
				icon_info = gtk_icon_theme_lookup_icon
				    (ind_config->icon_theme, l, 48, 0);
			}
		}
	}
	/* fallback to the default value */
	if (icon_info == NULL) {
		icon_info = gtk_icon_theme_lookup_icon
		    (ind_config->icon_theme, "stock_dialog-error", 48, 0);
	}
	if (icon_info != NULL) {
		image_file =
		    g_strdup (gtk_icon_info_get_filename (icon_info));
		gtk_icon_info_free (icon_info);
	}

	return image_file;
}

void
gkbd_indicator_config_load_image_filenames (GkbdIndicatorConfig *
					    ind_config,
					    GkbdKeyboardConfig *
					    kbd_config)
{
	int i;
	ind_config->image_filenames = NULL;

	if (!ind_config->show_flags)
		return;

	for (i = xkl_engine_get_max_num_groups (ind_config->engine);
	     --i >= 0;) {
		char *image_file =
		    gkbd_indicator_config_get_images_file (ind_config,
							   kbd_config,
							   i);
		ind_config->image_filenames =
		    g_slist_prepend (ind_config->image_filenames,
				     image_file);
	}
}

void
gkbd_indicator_config_free_image_filenames (GkbdIndicatorConfig *
					    ind_config)
{
	while (ind_config->image_filenames) {
		if (ind_config->image_filenames->data)
			g_free (ind_config->image_filenames->data);
		ind_config->image_filenames =
		    g_slist_delete_link (ind_config->image_filenames,
					 ind_config->image_filenames);
	}
}

void
gkbd_indicator_config_init (GkbdIndicatorConfig * ind_config,
			    MateConfClient * conf_client, XklEngine * engine)
{
	GError *gerror = NULL;
	gchar *sp;

	memset (ind_config, 0, sizeof (*ind_config));
	ind_config->conf_client = conf_client;
	ind_config->engine = engine;
	g_object_ref (ind_config->conf_client);

	mateconf_client_add_dir (ind_config->conf_client,
			      GKBD_INDICATOR_CONFIG_DIR,
			      MATECONF_CLIENT_PRELOAD_NONE, &gerror);
	if (gerror != NULL) {
		g_warning ("err1:%s\n", gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}

	ind_config->icon_theme = gtk_icon_theme_get_default ();

	gtk_icon_theme_append_search_path (ind_config->icon_theme, sp =
					   g_build_filename (g_get_home_dir
							     (),
							     ".icons/flags",
							     NULL));
	g_free (sp);

	gtk_icon_theme_append_search_path (ind_config->icon_theme,
					   sp =
					   g_build_filename (DATADIR,
							     "pixmaps/flags",
							     NULL));
	g_free (sp);

	gtk_icon_theme_append_search_path (ind_config->icon_theme,
					   sp =
					   g_build_filename (DATADIR,
							     "icons/flags",
							     NULL));
	g_free (sp);
}

void
gkbd_indicator_config_term (GkbdIndicatorConfig * ind_config)
{
	g_free (ind_config->font_family);
	ind_config->font_family = NULL;

	g_free (ind_config->foreground_color);
	ind_config->foreground_color = NULL;

	g_free (ind_config->background_color);
	ind_config->background_color = NULL;

	ind_config->icon_theme = NULL;

	gkbd_indicator_config_free_image_filenames (ind_config);

	gkbd_indicator_config_free_enabled_plugins (ind_config);
	g_object_unref (ind_config->conf_client);
	ind_config->conf_client = NULL;
}

void
gkbd_indicator_config_load_from_mateconf (GkbdIndicatorConfig * ind_config)
{
	GError *gerror = NULL;

	ind_config->secondary_groups_mask =
	    mateconf_client_get_int (ind_config->conf_client,
				  GKBD_INDICATOR_CONFIG_KEY_SECONDARIES,
				  &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading configuration:%s\n",
			   gerror->message);
		ind_config->secondary_groups_mask = 0;
		g_error_free (gerror);
		gerror = NULL;
	}

	ind_config->show_flags =
	    mateconf_client_get_bool (ind_config->conf_client,
				   GKBD_INDICATOR_CONFIG_KEY_SHOW_FLAGS,
				   &gerror);
	if (gerror != NULL) {
		g_warning ("Error reading kbdConfiguration:%s\n",
			   gerror->message);
		ind_config->show_flags = FALSE;
		g_error_free (gerror);
		gerror = NULL;
	}

	gkbd_indicator_config_load_font (ind_config);
	gkbd_indicator_config_load_colors (ind_config);

	gkbd_indicator_config_free_enabled_plugins (ind_config);
	ind_config->enabled_plugins =
	    mateconf_client_get_list (ind_config->conf_client,
				   GKBD_INDICATOR_CONFIG_KEY_ENABLED_PLUGINS,
				   MATECONF_VALUE_STRING, &gerror);

	if (gerror != NULL) {
		g_warning ("Error reading kbd_configuration:%s\n",
			   gerror->message);
		ind_config->enabled_plugins = NULL;
		g_error_free (gerror);
		gerror = NULL;
	}
}

void
gkbd_indicator_config_save_to_mateconf (GkbdIndicatorConfig * ind_config)
{
	MateConfChangeSet *cs;
	GError *gerror = NULL;

	cs = mateconf_change_set_new ();

	mateconf_change_set_set_int (cs,
				  GKBD_INDICATOR_CONFIG_KEY_SECONDARIES,
				  ind_config->secondary_groups_mask);
	mateconf_change_set_set_bool (cs,
				   GKBD_INDICATOR_CONFIG_KEY_SHOW_FLAGS,
				   ind_config->show_flags);
	mateconf_change_set_set_list (cs,
				   GKBD_INDICATOR_CONFIG_KEY_ENABLED_PLUGINS,
				   MATECONF_VALUE_STRING,
				   ind_config->enabled_plugins);

	mateconf_client_commit_change_set (ind_config->conf_client, cs,
					TRUE, &gerror);
	if (gerror != NULL) {
		g_warning ("Error saving configuration: %s\n",
			   gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
	mateconf_change_set_unref (cs);
}

void
gkbd_indicator_config_activate (GkbdIndicatorConfig * ind_config)
{
	xkl_engine_set_secondary_groups_mask (ind_config->engine,
					      ind_config->secondary_groups_mask);
}

void
gkbd_indicator_config_start_listen (GkbdIndicatorConfig *
				    ind_config,
				    MateConfClientNotifyFunc func,
				    gpointer user_data)
{
	gkbd_desktop_config_add_listener (ind_config->conf_client,
					  GKBD_INDICATOR_CONFIG_DIR, func,
					  user_data,
					  &ind_config->config_listener_id);
}

void
gkbd_indicator_config_stop_listen (GkbdIndicatorConfig * ind_config)
{
	gkbd_desktop_config_remove_listener (ind_config->conf_client,
					     &ind_config->config_listener_id);
}
