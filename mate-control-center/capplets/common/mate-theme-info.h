/* mate-theme-info.h - MATE Theme information

   Copyright (C) 2002 Jonathan Blandford <jrb@gnome.org>
   All rights reserved.

   This file is part of the Mate Library.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef MATE_THEME_INFO_H
#define MATE_THEME_INFO_H

#include <glib.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

typedef enum {
	MATE_THEME_TYPE_METATHEME,
	MATE_THEME_TYPE_ICON,
	MATE_THEME_TYPE_CURSOR,
	MATE_THEME_TYPE_REGULAR
} MateThemeType;

typedef enum {
	MATE_THEME_CHANGE_CREATED,
	MATE_THEME_CHANGE_DELETED,
	MATE_THEME_CHANGE_CHANGED
} MateThemeChangeType;

typedef enum {
	MATE_THEME_MARCO = 1 << 0,
	MATE_THEME_GTK_2 = 1 << 1,
	MATE_THEME_GTK_2_KEYBINDING = 1 << 2
} MateThemeElement;

typedef struct _MateThemeCommonInfo MateThemeCommonInfo;
typedef struct _MateThemeCommonInfo MateThemeIconInfo;
struct _MateThemeCommonInfo
{
	MateThemeType type;
	gchar* path;
	gchar* name;
	gchar* readable_name;
	gint priority;
	gboolean hidden;
};

typedef struct _MateThemeInfo MateThemeInfo;
struct _MateThemeInfo
{
	MateThemeType type;
	gchar* path;
	gchar* name;
	gchar* readable_name;
	gint priority;
	gboolean hidden;

	guint has_gtk : 1;
	guint has_keybinding : 1;
	guint has_marco : 1;
};

typedef struct _MateThemeCursorInfo MateThemeCursorInfo;
struct _MateThemeCursorInfo {
	MateThemeType type;
	gchar* path;
	gchar* name;
	gchar* readable_name;
	gint priority;
	gboolean hidden;

	GArray* sizes;
	GdkPixbuf* thumbnail;
};

typedef struct _MateThemeMetaInfo MateThemeMetaInfo;
struct _MateThemeMetaInfo {
	MateThemeType type;
	gchar* path;
	gchar* name;
	gchar* readable_name;
	gint priority;
	gboolean hidden;

	gchar* comment;
	gchar* icon_file;

	gchar* gtk_theme_name;
	gchar* gtk_color_scheme;
	gchar* marco_theme_name;
	gchar* icon_theme_name;
	gchar* notification_theme_name;
	gchar* sound_theme_name;
	gchar* cursor_theme_name;
	guint cursor_size;

	gchar* application_font;
	gchar* documents_font;
	gchar* desktop_font;
	gchar* windowtitle_font;
	gchar* monospace_font;
	gchar* background_image;
};

enum {
	COLOR_FG,
	COLOR_BG,
	COLOR_TEXT,
	COLOR_BASE,
	COLOR_SELECTED_FG,
	COLOR_SELECTED_BG,
	COLOR_TOOLTIP_FG,
	COLOR_TOOLTIP_BG,
	NUM_SYMBOLIC_COLORS
};

typedef void (*ThemeChangedCallback) (MateThemeCommonInfo* theme, MateThemeChangeType change_type, MateThemeElement element_type, gpointer user_data);

#define MATE_THEME_ERROR mate_theme_info_error_quark()

enum {
	MATE_THEME_ERROR_GTK_THEME_NOT_AVAILABLE = 1,
	MATE_THEME_ERROR_WM_THEME_NOT_AVAILABLE,
	MATE_THEME_ERROR_ICON_THEME_NOT_AVAILABLE,
	MATE_THEME_ERROR_GTK_ENGINE_NOT_AVAILABLE,
	MATE_THEME_ERROR_UNKNOWN
};


/* GTK/Marco/keybinding Themes */
MateThemeInfo     *mate_theme_info_new                   (void);
void                mate_theme_info_free                  (MateThemeInfo     *theme_info);
MateThemeInfo     *mate_theme_info_find                  (const gchar        *theme_name);
GList              *mate_theme_info_find_by_type          (guint               elements);
GQuark              mate_theme_info_error_quark           (void);
gchar              *gtk_theme_info_missing_engine          (const gchar *gtk_theme,
                                                            gboolean nameOnly);

/* Icon Themes */
MateThemeIconInfo *mate_theme_icon_info_new              (void);
void                mate_theme_icon_info_free             (MateThemeIconInfo *icon_theme_info);
MateThemeIconInfo *mate_theme_icon_info_find             (const gchar        *icon_theme_name);
GList              *mate_theme_icon_info_find_all         (void);
gint                mate_theme_icon_info_compare          (MateThemeIconInfo *a,
							    MateThemeIconInfo *b);

/* Cursor Themes */
MateThemeCursorInfo *mate_theme_cursor_info_new	   (void);
void                  mate_theme_cursor_info_free	   (MateThemeCursorInfo *info);
MateThemeCursorInfo *mate_theme_cursor_info_find	   (const gchar          *name);
GList                *mate_theme_cursor_info_find_all	   (void);
gint                  mate_theme_cursor_info_compare      (MateThemeCursorInfo *a,
							    MateThemeCursorInfo *b);

/* Meta themes*/
MateThemeMetaInfo *mate_theme_meta_info_new              (void);
void                mate_theme_meta_info_free             (MateThemeMetaInfo *meta_theme_info);
MateThemeMetaInfo *mate_theme_meta_info_find             (const gchar        *meta_theme_name);
GList              *mate_theme_meta_info_find_all         (void);
gint                mate_theme_meta_info_compare          (MateThemeMetaInfo *a,
							    MateThemeMetaInfo *b);
gboolean            mate_theme_meta_info_validate         (const MateThemeMetaInfo *info,
                                                            GError            **error);
MateThemeMetaInfo *mate_theme_read_meta_theme            (GFile              *meta_theme_uri);

/* Other */
void                mate_theme_init                       (void);
void                mate_theme_info_register_theme_change (ThemeChangedCallback func,
							    gpointer             data);

gboolean            mate_theme_color_scheme_parse         (const gchar         *scheme,
							    GdkColor            *colors);
gboolean            mate_theme_color_scheme_equal         (const gchar         *s1,
							    const gchar         *s2);

#endif /* MATE_THEME_INFO_H */
