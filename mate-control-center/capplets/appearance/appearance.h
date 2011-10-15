/*
 * Copyright (C) 2007 The MATE Foundation
 * Written by Thomas Wood <thos@gnome.org>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <libmateui/mate-desktop-thumbnail.h>

#include "mate-theme-info.h"

#define APPEARANCE_KEY_DIR "/apps/control-center/appearance"
#define MORE_THEMES_URL_KEY APPEARANCE_KEY_DIR "/more_themes_url"
#define MORE_BACKGROUNDS_URL_KEY APPEARANCE_KEY_DIR "/more_backgrounds_url"

typedef struct {
	MateConfClient* client;
	GtkBuilder* ui;
	MateDesktopThumbnailFactory* thumb_factory;
	gulong screen_size_handler;
	gulong screen_monitors_handler;

	/* desktop */
	GHashTable* wp_hash;
	gboolean wp_update_mateconf;
	GtkIconView* wp_view;
	GtkTreeModel* wp_model;
	GtkWidget* wp_scpicker;
	GtkWidget* wp_pcpicker;
	GtkWidget* wp_style_menu;
	GtkWidget* wp_color_menu;
	GtkWidget* wp_rem_button;
	GtkFileChooser* wp_filesel;
	GtkWidget* wp_image;
	GSList* wp_uris;
	gint frame;
	gint thumb_width;
	gint thumb_height;

	/* font */
	GtkWidget* font_details;
	GSList* font_groups;

	/* themes */
	GtkListStore* theme_store;
	MateThemeMetaInfo* theme_custom;
	GdkPixbuf* theme_icon;
	GtkWidget* theme_save_dialog;
	GtkWidget* theme_message_area;
	GtkWidget* theme_message_label;
	GtkWidget* apply_background_button;
	GtkWidget* revert_font_button;
	GtkWidget* apply_font_button;
	GtkWidget* install_button;
	GtkWidget* theme_info_icon;
	GtkWidget* theme_error_icon;
	gchar* revert_application_font;
	gchar* revert_documents_font;
	gchar* revert_desktop_font;
	gchar* revert_windowtitle_font;
	gchar* revert_monospace_font;

	/* style */
	GdkPixbuf* gtk_theme_icon;
	GdkPixbuf* window_theme_icon;
	GdkPixbuf* icon_theme_icon;
	GtkWidget* style_message_area;
	GtkWidget* style_message_label;
	GtkWidget* style_install_button;
} AppearanceData;

#define appearance_capplet_get_widget(x, y) (GtkWidget*) gtk_builder_get_object(x->ui, y)
