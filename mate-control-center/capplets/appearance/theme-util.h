/*
 * Copyright (C) 2007 The MATE Foundation
 * Written by Jens Granseuer <jensgr@gmx.net>
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

#define GTK_THEME_KEY "/desktop/mate/interface/gtk_theme"
#define MARCO_THEME_KEY "/apps/marco/general/theme"
#define ICON_THEME_KEY "/desktop/mate/interface/icon_theme"
#define NOTIFICATION_THEME_KEY "/apps/notification-daemon/theme"
#define COLOR_SCHEME_KEY "/desktop/mate/interface/gtk_color_scheme"
#define LOCKDOWN_KEY "/desktop/mate/lockdown/disable_theme_settings"
#define BACKGROUND_KEY "/desktop/mate/background/picture_filename"
#define APPLICATION_FONT_KEY "/desktop/mate/interface/font_name"
#define DOCUMENTS_FONT_KEY "/desktop/mate/interface/document_font_name"
#define DESKTOP_FONT_KEY "/apps/caja/preferences/desktop_font"
#define WINDOWTITLE_FONT_KEY "/apps/marco/general/titlebar_font"
#define MONOSPACE_FONT_KEY "/desktop/mate/interface/monospace_font_name"

#ifdef HAVE_XCURSOR
	#define CURSOR_THEME_KEY "/desktop/mate/peripherals/mouse/cursor_theme"
	#define CURSOR_SIZE_KEY "/desktop/mate/peripherals/mouse/cursor_size"
#else
	#define CURSOR_THEME_KEY "/desktop/mate/peripherals/mouse/cursor_font"
#endif

enum {
	COL_THUMBNAIL,
	COL_LABEL,
	COL_NAME,
	NUM_COLS
};

typedef enum {
	THEME_TYPE_GTK,
	THEME_TYPE_WINDOW,
	THEME_TYPE_ICON,
	THEME_TYPE_META,
	THEME_TYPE_CURSOR
} ThemeType;

gboolean theme_is_writable(const gpointer theme);
gboolean theme_delete(const gchar* name, ThemeType type);

gboolean theme_model_iter_last(GtkTreeModel* model, GtkTreeIter* iter);
gboolean theme_find_in_model(GtkTreeModel* model, const gchar* name, GtkTreeIter* iter);

void theme_install_file(GtkWindow* parent, const gchar* path);
gboolean packagekit_available(void);
