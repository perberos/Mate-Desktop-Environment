/* mate-theme-info.c - MATE Theme information
 *
 * Copyright (C) 2002 Jonathan Blandford <jrb@gnome.org>
 * Copyright (C) 2011 Perberos
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <string.h>
#include <libmate/mate-desktop-item.h>
#include "mate-theme-info.h"
#include "gtkrc-utils.h"

#ifdef HAVE_XCURSOR
	#include <X11/Xcursor/Xcursor.h>
#endif

#define THEME_NAME "X-GNOME-Metatheme/Name"
#define THEME_COMMENT "X-GNOME-Metatheme/Comment"
#define GTK_THEME_KEY "X-GNOME-Metatheme/GtkTheme"
#define GTK_COLOR_SCHEME_KEY "X-GNOME-Metatheme/GtkColorScheme"
#define MARCO_THEME_KEY "X-GNOME-Metatheme/MetacityTheme"
#define ICON_THEME_KEY "X-GNOME-Metatheme/IconTheme"
#define CURSOR_THEME_KEY "X-GNOME-Metatheme/CursorTheme"
#define NOTIFICATION_THEME_KEY "X-GNOME-Metatheme/NotificationTheme"
#define CURSOR_SIZE_KEY "X-GNOME-Metatheme/CursorSize"
#define SOUND_THEME_KEY "X-GNOME-Metatheme/SoundTheme"
#define APPLICATION_FONT_KEY "X-GNOME-Metatheme/ApplicationFont"
#define DOCUMENTS_FONT_KEY "X-GNOME-Metatheme/DocumentsFont"
#define DESKTOP_FONT_KEY "X-GNOME-Metatheme/DesktopFont"
#define WINDOWTITLE_FONT_KEY "X-GNOME-Metatheme/WindowTitleFont"
#define MONOSPACE_FONT_KEY "X-GNOME-Metatheme/MonospaceFont"
#define BACKGROUND_IMAGE_KEY "X-GNOME-Metatheme/BackgroundImage"
#define HIDDEN_KEY "X-GNOME-Metatheme/Hidden"

/* Terminology used in this lib:
 *
 * /usr/share/themes, ~/.themes   -- top_theme_dir
 * top_theme_dir/theme_name/      -- common_theme_dir
 * /usr/share/icons, ~/.icons     -- top_icon_theme_dir
 * top_icon_theme_dir/theme_name/ -- icon_common_theme_dir
 *
 */

typedef struct _ThemeCallbackData {
	ThemeChangedCallback func;
	gpointer data;
} ThemeCallbackData;

typedef struct {
	GFileMonitor* common_theme_dir_handle;
	GFileMonitor* gtk2_dir_handle;
	GFileMonitor* keybinding_dir_handle;
	GFileMonitor* marco_dir_handle;
	gint priority;
} CommonThemeDirMonitorData;

typedef struct {
	GFileMonitor* common_icon_theme_dir_handle;
	gint priority;
} CommonIconThemeDirMonitorData;

typedef struct {
	GHashTable* handle_hash;
	gint priority;
} CallbackTuple;


/* Hash tables */

/* The hashes_by_dir are indexed by an escaped uri of the common_theme_dir that
 * that particular theme is part of.  The data pointed to by them is a
 * MateTheme{Meta,Icon,}Info struct.  Note that the uri is of the form
 * "file:///home/username/.themes/foo", and not "/home/username/.themes/foo"
 */

/* The hashes_by_name are hashed by the index of the theme.  The data pointed to
 * by them is a GList whose data elements are MateTheme{Meta,Icon,}Info
 * structs.  This is because a theme can be found both in the users ~/.theme as
 * well as globally in $prefix.  All access to them must be done via helper
 * functions.
 */
static GList* callbacks = NULL;

static GHashTable* meta_theme_hash_by_uri;
static GHashTable* meta_theme_hash_by_name;
static GHashTable* icon_theme_hash_by_uri;
static GHashTable* icon_theme_hash_by_name;
static GHashTable* cursor_theme_hash_by_uri;
static GHashTable* cursor_theme_hash_by_name;
static GHashTable* theme_hash_by_uri;
static GHashTable* theme_hash_by_name;
static gboolean initting = FALSE;

/* private functions */
static gint safe_strcmp(const gchar* a_str, const gchar* b_str)
{
	if (a_str && b_str)
	{
		return strcmp(a_str, b_str);
	}
	else
	{
		return a_str - b_str;
	}
}

static GFileType
get_file_type (GFile *file)
{
  GFileType file_type = G_FILE_TYPE_UNKNOWN;
  GFileInfo *file_info;

  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, NULL);
  if (file_info != NULL) {
    file_type = g_file_info_get_file_type (file_info);
    g_object_unref (file_info);
  }

  return file_type;
}

static void
add_theme_to_hash_by_name (GHashTable *hash_table,
                           gpointer    data)
{
  MateThemeCommonInfo *info = data;
  GList *list;

  list = g_hash_table_lookup (hash_table, info->name);
  if (list == NULL) {
    list = g_list_append (list, info);
  } else {
    GList *list_ptr = list;
    gboolean added = FALSE;

    while (list_ptr) {
      gint theme_priority;

      theme_priority = ((MateThemeCommonInfo *) list_ptr->data)->priority;

      if (theme_priority == info->priority) {
        /* Swap it in */
        list_ptr->data = info;
        added = TRUE;
        break;
      } else if (theme_priority > info->priority) {
        list = g_list_insert_before (list, list_ptr, info);
        added = TRUE;
        break;
      }
      list_ptr = list_ptr->next;
    }
    if (!added)
      list = g_list_append (list, info);
  }
  g_hash_table_insert (hash_table, g_strdup (info->name), list);
}

static void
remove_theme_from_hash_by_name (GHashTable *hash_table,
                                gpointer    data)
{
  MateThemeCommonInfo *info = data;
  GList *list;

  list = g_hash_table_lookup (hash_table, info->name);

  list = g_list_remove (list, info);
  if (list == NULL)
    g_hash_table_remove (hash_table, info->name);
  else
    g_hash_table_insert (hash_table, g_strdup (info->name), list);
}

static MateThemeCommonInfo *
get_theme_from_hash_by_name (GHashTable  *hash_table,
                             const gchar *name,
                             gint         priority)
{
  GList *list;

  list = g_hash_table_lookup (hash_table, name);

  /* -1 implies return the first one */
  if (priority == -1) {
    return list ? list->data : NULL;
  }

  while (list) {
    MateThemeCommonInfo *info = (MateThemeCommonInfo *) list->data;

    if (info->priority == priority)
      return info;

    list = list->next;
  }
  return NULL;
}

static gint
theme_compare (MateThemeCommonInfo *a,
               MateThemeCommonInfo *b)
{
  gint cmp;

  g_return_val_if_fail (a->type == b->type, a->type - b->type);

  switch (a->type) {
  case MATE_THEME_TYPE_METATHEME:
    cmp = mate_theme_meta_info_compare (
                    (MateThemeMetaInfo *) a, (MateThemeMetaInfo *) b);
    break;
  case MATE_THEME_TYPE_ICON:
    cmp = mate_theme_icon_info_compare (
                    (MateThemeIconInfo *) a, (MateThemeIconInfo *) b);
    break;
  case MATE_THEME_TYPE_CURSOR:
    cmp = mate_theme_cursor_info_compare (
                    (MateThemeCursorInfo *) a, (MateThemeCursorInfo *) b);
    break;
  default:
    /* not supported at this time */
    g_assert_not_reached ();
  }

  return cmp;
}

static void
theme_free (MateThemeCommonInfo *info)
{
  switch (info->type) {
  case MATE_THEME_TYPE_METATHEME:
    mate_theme_meta_info_free ((MateThemeMetaInfo *) info);
    break;
  case MATE_THEME_TYPE_ICON:
    mate_theme_icon_info_free ((MateThemeIconInfo *) info);
    break;
  case MATE_THEME_TYPE_REGULAR:
    mate_theme_info_free ((MateThemeInfo *) info);
    break;
  case MATE_THEME_TYPE_CURSOR:
    mate_theme_cursor_info_free ((MateThemeCursorInfo *) info);
    break;
  default:
    g_assert_not_reached ();
  }
}

GQuark mate_theme_info_error_quark(void)
{
	return g_quark_from_static_string("mate-theme-info-error-quark");
}

MateThemeMetaInfo* mate_theme_read_meta_theme(GFile* meta_theme_uri)
{
	MateThemeMetaInfo* meta_theme_info;
	GFile* common_theme_dir_uri;
	MateDesktopItem* meta_theme_ditem;
	gchar* meta_theme_file;
	const gchar* str;
	gchar* scheme;

	meta_theme_file = g_file_get_uri(meta_theme_uri);
	meta_theme_ditem = mate_desktop_item_new_from_uri(meta_theme_file, 0, NULL);
	g_free(meta_theme_file);

	if (meta_theme_ditem == NULL)
		return NULL;

	common_theme_dir_uri = g_file_get_parent(meta_theme_uri);
	meta_theme_info = mate_theme_meta_info_new();
	meta_theme_info->path = g_file_get_path(meta_theme_uri);
	meta_theme_info->name = g_file_get_basename(common_theme_dir_uri);
	g_object_unref(common_theme_dir_uri);

	str = mate_desktop_item_get_localestring(meta_theme_ditem, THEME_NAME);

	if (!str)
	{
		str = mate_desktop_item_get_localestring(meta_theme_ditem, MATE_DESKTOP_ITEM_NAME);
		if (!str)
		{ /* shouldn't reach */
			mate_theme_meta_info_free(meta_theme_info);
			return NULL;
		}
	}

	meta_theme_info->readable_name = g_strdup(str);

	str = mate_desktop_item_get_localestring(meta_theme_ditem, THEME_COMMENT);

	if (str == NULL)
		str = mate_desktop_item_get_localestring(meta_theme_ditem, MATE_DESKTOP_ITEM_COMMENT);

	if (str != NULL)
		meta_theme_info->comment = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, MATE_DESKTOP_ITEM_ICON);

	if (str != NULL)
		meta_theme_info->icon_file = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, GTK_THEME_KEY);

	if (str == NULL)
	{
		mate_theme_meta_info_free(meta_theme_info);
		return NULL;
	}
	meta_theme_info->gtk_theme_name = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, GTK_COLOR_SCHEME_KEY);

	if (str == NULL || str[0] == '\0')
		scheme = gtkrc_get_color_scheme_for_theme(meta_theme_info->gtk_theme_name);
	else
		scheme = g_strdup(str);

	if (scheme != NULL)
	{
		meta_theme_info->gtk_color_scheme = scheme;

		for (; *scheme != '\0'; scheme++)
			if (*scheme == ',')
				*scheme = '\n';
	}

	str = mate_desktop_item_get_string (meta_theme_ditem, MARCO_THEME_KEY);

	if (str == NULL)
	{
		mate_theme_meta_info_free (meta_theme_info);
		return NULL;
	}

	meta_theme_info->marco_theme_name = g_strdup (str);

	str = mate_desktop_item_get_string(meta_theme_ditem, ICON_THEME_KEY);

	if (str == NULL)
	{
		mate_theme_meta_info_free(meta_theme_info);
		return NULL;
	}

	meta_theme_info->icon_theme_name = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, NOTIFICATION_THEME_KEY);

	if (str != NULL)
		meta_theme_info->notification_theme_name = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, CURSOR_THEME_KEY);

	if (str != NULL)
	{
		meta_theme_info->cursor_theme_name = g_strdup(str);

		str = mate_desktop_item_get_string(meta_theme_ditem, CURSOR_SIZE_KEY);

		if (str)
			meta_theme_info->cursor_size = (int) g_ascii_strtoll(str, NULL, 10);
		else
			meta_theme_info->cursor_size = 18;
	}
	else
	{
		meta_theme_info->cursor_theme_name = g_strdup("default");
		meta_theme_info->cursor_size = 18;
	}

	str = mate_desktop_item_get_string(meta_theme_ditem, APPLICATION_FONT_KEY);

	if (str != NULL)
		meta_theme_info->application_font = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, DOCUMENTS_FONT_KEY);

	if (str != NULL)
		meta_theme_info->documents_font = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, DESKTOP_FONT_KEY);

	if (str != NULL)
		meta_theme_info->desktop_font = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, WINDOWTITLE_FONT_KEY);

	if (str != NULL)
		meta_theme_info->windowtitle_font = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, MONOSPACE_FONT_KEY);

	if (str != NULL)
		meta_theme_info->monospace_font = g_strdup(str);

	str = mate_desktop_item_get_string(meta_theme_ditem, BACKGROUND_IMAGE_KEY);

	if (str != NULL)
		meta_theme_info->background_image = g_strdup(str);

	meta_theme_info->hidden = mate_desktop_item_get_boolean(meta_theme_ditem, HIDDEN_KEY);

	mate_desktop_item_unref(meta_theme_ditem);

	return meta_theme_info;
}

static MateThemeIconInfo *
read_icon_theme (GFile *icon_theme_uri)
{
  MateThemeIconInfo *icon_theme_info;
  MateDesktopItem *icon_theme_ditem;
  gchar *icon_theme_file;
  gchar *dir_name;
  const gchar *name;
  const gchar *directories;

  icon_theme_file = g_file_get_uri (icon_theme_uri);
  icon_theme_ditem = mate_desktop_item_new_from_uri (icon_theme_file, 0, NULL);
  g_free (icon_theme_file);

  if (icon_theme_ditem == NULL)
    return NULL;

  name = mate_desktop_item_get_localestring (icon_theme_ditem, "Icon Theme/Name");
  if (!name) {
    name = mate_desktop_item_get_localestring (icon_theme_ditem, MATE_DESKTOP_ITEM_NAME);
    if (!name) {
      mate_desktop_item_unref (icon_theme_ditem);
      return NULL;
    }
  }

  /* If index.theme has no Directories entry, it is only a cursor theme */
  directories = mate_desktop_item_get_string (icon_theme_ditem, "Icon Theme/Directories");
  if (directories == NULL) {
    mate_desktop_item_unref (icon_theme_ditem);
    return NULL;
  }

  icon_theme_info = mate_theme_icon_info_new ();
  icon_theme_info->readable_name = g_strdup (name);
  icon_theme_info->path = g_file_get_path (icon_theme_uri);
  icon_theme_info->hidden = mate_desktop_item_get_boolean (icon_theme_ditem, "Icon Theme/Hidden");
  dir_name = g_path_get_dirname (icon_theme_info->path);
  icon_theme_info->name = g_path_get_basename (dir_name);
  g_free (dir_name);

  mate_desktop_item_unref (icon_theme_ditem);

  return icon_theme_info;
}

#ifdef HAVE_XCURSOR
static void
add_default_cursor_theme ()
{
  MateThemeCursorInfo *theme_info;

  theme_info = mate_theme_cursor_info_new ();
  theme_info->path = g_strdup ("builtin");
  theme_info->name = g_strdup ("default");
  theme_info->readable_name = g_strdup (_("Default Pointer"));
  theme_info->sizes = g_array_sized_new (FALSE, FALSE, sizeof (gint), 0);

  g_hash_table_insert (cursor_theme_hash_by_uri, theme_info->path, theme_info);
  add_theme_to_hash_by_name (cursor_theme_hash_by_name, theme_info);
}

static GdkPixbuf *
gdk_pixbuf_from_xcursor_image (XcursorImage *cursor)
{
  GdkPixbuf *pixbuf;
#define BUF_SIZE sizeof(guint32) * cursor->width * cursor->height
  guchar *buf = g_malloc0 (BUF_SIZE);
  guchar *it;

  for (it = buf; it < (buf + BUF_SIZE); it += 4) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    /* on little endianess it's BGRA to RGBA */
    it[0] = ((guchar *) (cursor->pixels))[it - buf + 2];
    it[1] = ((guchar *) (cursor->pixels))[it - buf + 1];
    it[2] = ((guchar *) (cursor->pixels))[it - buf + 0];
    it[3] = ((guchar *) (cursor->pixels))[it - buf + 3];
#else
    /* on big endianess it's ARGB to RGBA */
    it[0] = ((guchar *) cursor->pixels)[it - buf + 1];
    it[1] = ((guchar *) cursor->pixels)[it - buf + 2];
    it[2] = ((guchar *) cursor->pixels)[it - buf + 3];
    it[3] = ((guchar *) cursor->pixels)[it - buf + 0];
#endif
  }

  pixbuf = gdk_pixbuf_new_from_data ((const guchar *) buf,
                        GDK_COLORSPACE_RGB, TRUE, 8,
                        cursor->width, cursor->height,
                        cursor->width * 4,
                        (GdkPixbufDestroyNotify) g_free,
                        NULL);

  if (!pixbuf)
    g_free (buf);

  return pixbuf;
}

static MateThemeCursorInfo *
read_cursor_theme (GFile *cursor_theme_uri)
{
  MateThemeCursorInfo *cursor_theme_info = NULL;
  GFile *parent_uri, *cursors_uri;

  const gint filter_sizes[] = { 12, 16, 24, 32, 36, 40, 48, 64 };
  const gint num_sizes = G_N_ELEMENTS (filter_sizes);

  parent_uri = g_file_get_parent (cursor_theme_uri);
  cursors_uri = g_file_get_child (parent_uri, "cursors");

  if (get_file_type (cursors_uri) == G_FILE_TYPE_DIRECTORY) {
    GArray *sizes;
    XcursorImage *cursor;
    GdkPixbuf *thumbnail = NULL;
    gchar *name;
    gint i;

    name = g_file_get_basename (parent_uri);

    sizes = g_array_sized_new (FALSE, FALSE, sizeof (gint), num_sizes);

    for (i = 0; i < num_sizes; ++i) {
      cursor = XcursorLibraryLoadImage ("left_ptr", name, filter_sizes[i]);

      if (cursor) {
        if (cursor->size == filter_sizes[i]) {
          g_array_append_val (sizes, filter_sizes[i]);

          if (thumbnail == NULL && i >= 1)
            thumbnail = gdk_pixbuf_from_xcursor_image (cursor);
        }

        XcursorImageDestroy (cursor);
      }
    }

    if (sizes->len == 0) {
      g_array_free (sizes, TRUE);
      g_free (name);
    } else {
      MateDesktopItem *cursor_theme_ditem;
      gchar *cursor_theme_file;

      if (!thumbnail) {
        cursor = XcursorLibraryLoadImage ("left_ptr", name,
                                          g_array_index (sizes, gint, 0));
        if (cursor) {
          thumbnail = gdk_pixbuf_from_xcursor_image (cursor);
          XcursorImageDestroy (cursor);
        }
      }

      cursor_theme_info = mate_theme_cursor_info_new ();
      cursor_theme_info->path = g_file_get_path (parent_uri);
      cursor_theme_info->name = name;
      cursor_theme_info->sizes = sizes;
      cursor_theme_info->thumbnail = thumbnail;

      cursor_theme_file = g_file_get_path (cursor_theme_uri);
      cursor_theme_ditem = mate_desktop_item_new_from_file (cursor_theme_file, 0, NULL);
      g_free (cursor_theme_file);

      if (cursor_theme_ditem != NULL) {
        const gchar *readable_name;

        readable_name = mate_desktop_item_get_string (cursor_theme_ditem,
                                                       "Icon Theme/Name");
        if (readable_name)
          cursor_theme_info->readable_name = g_strdup (readable_name);
        else
          cursor_theme_info->readable_name = g_strdup (name);

        cursor_theme_info->hidden = mate_desktop_item_get_boolean (cursor_theme_ditem,
                                                                    "Icon Theme/Hidden");

        mate_desktop_item_unref (cursor_theme_ditem);
      } else {
        cursor_theme_info->readable_name = g_strdup (name);
      }
    }
  }

  g_object_unref (cursors_uri);
  g_object_unref (parent_uri);

  return cursor_theme_info;
}

#else /* !HAVE_XCURSOR */

static gchar *
read_current_cursor_font (void)
{
  DIR *dir;
  gchar *dir_name;
  struct dirent *file_dirent;

  dir_name = g_build_filename (g_get_home_dir (), ".mate2/share/cursor-fonts", NULL);
  if (! g_file_test (dir_name, G_FILE_TEST_EXISTS)) {
    g_free (dir_name);
    return NULL;
  }

  dir = opendir (dir_name);

  while ((file_dirent = readdir (dir)) != NULL) {
    struct stat st;
    gchar *link_name;

    link_name = g_build_filename (dir_name, file_dirent->d_name, NULL);
    if (lstat (link_name, &st)) {
      g_free (link_name);
      continue;
    }

    if (S_ISLNK (st.st_mode)) {
      gint length;
      gchar target[256];

      length = readlink (link_name, target, 255);
      if (length > 0) {
        gchar *retval;
        target[length] = '\0';
        retval = g_strdup (target);
        g_free (link_name);
        closedir (dir);
        return retval;
      }

    }
    g_free (link_name);
  }
  g_free (dir_name);
  closedir (dir);
  return NULL;
}

static void
read_cursor_fonts (void)
{
  gchar *cursor_font;
  gint i;

  const gchar *builtins[][4] = {
    {
      "mate/cursor-fonts/cursor-normal.pcf",
      N_("Default Pointer"),
      N_("Default Pointer - Current"),
      "mouse-cursor-normal.png"
    }, {
      "mate/cursor-fonts/cursor-white.pcf",
      N_("White Pointer"),
      N_("White Pointer - Current"),
      "mouse-cursor-white.png"
    }, {
      "mate/cursor-fonts/cursor-large.pcf",
      N_("Large Pointer"),
      N_("Large Pointer - Current"),
      "mouse-cursor-normal-large.png"
    }, {
      "mate/cursor-fonts/cursor-large-white.pcf",
      N_("Large White Pointer - Current"),
      N_("Large White Pointer"),
      "mouse-cursor-white-large.png"
    }
  };

  cursor_font = read_current_cursor_font();

  if (!cursor_font)
    cursor_font = g_strdup (builtins[0][0]);

  for (i = 0; i < G_N_ELEMENTS (builtins); i++) {
    MateThemeCursorInfo *theme_info;
    gchar *filename;

    theme_info = mate_theme_cursor_info_new ();

    filename = g_build_filename (MATECC_DATA_DIR, "pixmaps", builtins[i][3], NULL);
    theme_info->thumbnail = gdk_pixbuf_new_from_file (filename, NULL);
    g_free (filename);

    theme_info->path = g_build_filename (MATECC_DATA_DIR, builtins[i][0], NULL);
    theme_info->name = g_strdup (theme_info->path);

    if (!strcmp (theme_info->path, cursor_font))
      theme_info->readable_name = g_strdup (_(builtins[i][2]));
    else
      theme_info->readable_name = g_strdup (_(builtins[i][1]));

    g_hash_table_insert (cursor_theme_hash_by_uri, theme_info->path, theme_info);
    add_theme_to_hash_by_name (cursor_theme_hash_by_name, theme_info);
  }

  g_free (cursor_font);
}
#endif /* HAVE_XCURSOR */

static void
handle_change_signal (gpointer             data,
                      MateThemeChangeType change_type,
                      MateThemeElement    element_type)
{
#ifdef DEBUG
  gchar *type_str = NULL;
  gchar *change_str = NULL;
  gchar *element_str = NULL;
#endif
  MateThemeCommonInfo *theme = data;
  GList *list;

  if (initting)
    return;

  for (list = callbacks; list; list = list->next) {
    ThemeCallbackData *callback_data = list->data;
    (* callback_data->func) (theme, change_type, element_type, callback_data->data);
  }

#ifdef DEBUG
  if (theme->type == MATE_THEME_TYPE_METATHEME)
    type_str = "meta";
  else if (theme->type == MATE_THEME_TYPE_ICON)
    type_str = "icon";
  else if (theme->type == MATE_THEME_TYPE_CURSOR)
    type_str = "cursor";
  else if (theme->type == MATE_THEME_TYPE_REGULAR) {
    if (element_type & MATE_THEME_GTK_2)
      element_str = "gtk-2";
    else if (element_type & MATE_THEME_GTK_2_KEYBINDING)
      element_str = "keybinding";
    else if (element_type & MATE_THEME_MARCO)
      element_str = "marco";
  }

  if (change_type == MATE_THEME_CHANGE_CREATED)
    change_str = "created";
  else if (change_type == MATE_THEME_CHANGE_CHANGED)
    change_str = "changed";
  else if (change_type == MATE_THEME_CHANGE_DELETED)
    change_str = "deleted";

  if (type == MATE_THEME_TYPE_REGULAR) {
    g_print ("theme \"%s\" has a theme of type %s (priority %d) has been %s\n",
             theme->name,
             element_str,
             theme->priority,
             type_str);
  } else if (type_str != NULL) {
    g_print ("%s theme \"%s\" (priority %d) has been %s\n",
             type_str,
             theme->name,
             theme->priority,
             type_str);
    }
#endif
}

/* index_uri should point to the gtkrc file that was modified */
static void
update_theme_index (GFile            *index_uri,
                    MateThemeElement key_element,
                    gint              priority)
{
  gboolean theme_exists;
  MateThemeInfo *theme_info;
  GFile *parent;
  GFile *common_theme_dir_uri;
  gchar *common_theme_dir;

  /* First, we determine the new state of the file.  We do no more
   * sophisticated a test than "files exists and is a file" */
  theme_exists = (get_file_type (index_uri) == G_FILE_TYPE_REGULAR);

  /* Next, we see what currently exists */
  parent = g_file_get_parent (index_uri);
  common_theme_dir_uri = g_file_get_parent (parent);
  common_theme_dir = g_file_get_path (common_theme_dir_uri);

  theme_info = g_hash_table_lookup (theme_hash_by_uri, common_theme_dir);
  if (theme_info == NULL) {
    if (theme_exists) {
      theme_info = mate_theme_info_new ();
      theme_info->path = g_strdup (common_theme_dir);
      theme_info->name = g_file_get_basename (common_theme_dir_uri);
      theme_info->readable_name = g_strdup (theme_info->name);
      theme_info->priority = priority;
      if (key_element & MATE_THEME_GTK_2)
        theme_info->has_gtk = TRUE;
      else if (key_element & MATE_THEME_GTK_2_KEYBINDING)
        theme_info->has_keybinding = TRUE;
      else if (key_element & MATE_THEME_MARCO)
        theme_info->has_marco = TRUE;

      g_hash_table_insert (theme_hash_by_uri, g_strdup (common_theme_dir), theme_info);
      add_theme_to_hash_by_name (theme_hash_by_name, theme_info);
      handle_change_signal (theme_info, MATE_THEME_CHANGE_CREATED, key_element);
    }
  } else {
    gboolean theme_used_to_exist = FALSE;

    if (key_element & MATE_THEME_GTK_2) {
      theme_used_to_exist = theme_info->has_gtk;
      theme_info->has_gtk = theme_exists;
    } else if (key_element & MATE_THEME_GTK_2_KEYBINDING) {
      theme_used_to_exist = theme_info->has_keybinding;
      theme_info->has_keybinding = theme_exists;
    } else if (key_element & MATE_THEME_MARCO) {
      theme_used_to_exist = theme_info->has_marco;
      theme_info->has_marco = theme_exists;
    }

    if (!theme_info->has_marco && !theme_info->has_keybinding && !theme_info->has_gtk) {
      g_hash_table_remove (theme_hash_by_uri, common_theme_dir);
      remove_theme_from_hash_by_name (theme_hash_by_name, theme_info);
    }

    if (theme_exists && theme_used_to_exist) {
      handle_change_signal (theme_info, MATE_THEME_CHANGE_CHANGED, key_element);
    } else if (theme_exists && !theme_used_to_exist) {
      handle_change_signal (theme_info, MATE_THEME_CHANGE_CREATED, key_element);
    } else if (!theme_exists && theme_used_to_exist) {
      handle_change_signal (theme_info, MATE_THEME_CHANGE_DELETED, key_element);
    }

    if (!theme_info->has_marco && !theme_info->has_keybinding && !theme_info->has_gtk) {
      mate_theme_info_free (theme_info);
    }
  }

  g_free (common_theme_dir);
  g_object_unref (parent);
  g_object_unref (common_theme_dir_uri);
}

static void
update_gtk2_index (GFile *gtk2_index_uri,
                   gint   priority)
{
  update_theme_index (gtk2_index_uri, MATE_THEME_GTK_2, priority);
}

static void
update_keybinding_index (GFile *keybinding_index_uri,
                         gint   priority)
{
  update_theme_index (keybinding_index_uri, MATE_THEME_GTK_2_KEYBINDING, priority);
}

static void
update_marco_index (GFile *marco_index_uri,
                       gint   priority)
{
  update_theme_index (marco_index_uri, MATE_THEME_MARCO, priority);
}

static void
update_common_theme_dir_index (GFile         *theme_index_uri,
                               MateThemeType type,
                               gint           priority)
{
  gboolean theme_exists;
  MateThemeCommonInfo *theme_info = NULL;
  MateThemeCommonInfo *old_theme_info;
  GFile *common_theme_dir_uri;
  gchar *common_theme_dir;
  GHashTable *hash_by_uri;
  GHashTable *hash_by_name;

  if (type == MATE_THEME_TYPE_ICON) {
    hash_by_uri = icon_theme_hash_by_uri;
    hash_by_name = icon_theme_hash_by_name;
  } else if (type == MATE_THEME_TYPE_CURSOR) {
    hash_by_uri = cursor_theme_hash_by_uri;
    hash_by_name = cursor_theme_hash_by_name;
  } else {
    hash_by_uri = meta_theme_hash_by_uri;
    hash_by_name = meta_theme_hash_by_name;
  }

  if (type != MATE_THEME_TYPE_CURSOR) {
    /* First, we determine the new state of the file. */
    if (get_file_type (theme_index_uri) == G_FILE_TYPE_REGULAR) {
      /* It's an interesting file. Let's try to load it. */
      if (type == MATE_THEME_TYPE_ICON)
        theme_info = (MateThemeCommonInfo *) read_icon_theme (theme_index_uri);
      else
        theme_info = (MateThemeCommonInfo *) mate_theme_read_meta_theme (theme_index_uri);
    } else {
      theme_info = NULL;
    }

  }
#ifdef HAVE_XCURSOR
  /* cursor themes don't necessarily have an index file, so try those in any case */
  else {
    theme_info = (MateThemeCommonInfo *) read_cursor_theme (theme_index_uri);
  }
#endif

  if (theme_info) {
    theme_info->priority = priority;
    theme_exists = TRUE;
  } else {
    theme_exists = FALSE;
  }

  /* Next, we see what currently exists */
  common_theme_dir_uri = g_file_get_parent (theme_index_uri);
  common_theme_dir = g_file_get_path (common_theme_dir_uri);
  g_object_unref (common_theme_dir_uri);

  old_theme_info = (MateThemeCommonInfo *) g_hash_table_lookup (hash_by_uri, common_theme_dir);

  if (old_theme_info == NULL) {
    if (theme_exists) {
      g_hash_table_insert (hash_by_uri, g_strdup (common_theme_dir), theme_info);
      add_theme_to_hash_by_name (hash_by_name, theme_info);
      handle_change_signal (theme_info, MATE_THEME_CHANGE_CREATED, 0);
    }
  } else {
    if (theme_exists) {
      if (theme_compare (theme_info, old_theme_info) != 0) {
        /* Remove old theme */
        g_hash_table_remove (hash_by_uri, common_theme_dir);
        remove_theme_from_hash_by_name (hash_by_name, old_theme_info);
        g_hash_table_insert (hash_by_uri, g_strdup (common_theme_dir), theme_info);
        add_theme_to_hash_by_name (hash_by_name, theme_info);
        handle_change_signal (theme_info, MATE_THEME_CHANGE_CHANGED, 0);
        theme_free (old_theme_info);
      } else {
        theme_free (theme_info);
      }
    } else {
      g_hash_table_remove (hash_by_uri, common_theme_dir);
      remove_theme_from_hash_by_name (hash_by_name, old_theme_info);

      handle_change_signal (old_theme_info, MATE_THEME_CHANGE_DELETED, 0);
      theme_free (old_theme_info);
    }
  }

  g_free (common_theme_dir);
}

static void
update_meta_theme_index (GFile *meta_theme_index_uri,
                         gint   priority)
{
  update_common_theme_dir_index (meta_theme_index_uri, MATE_THEME_TYPE_METATHEME, priority);
}

static void
update_icon_theme_index (GFile *icon_theme_index_uri,
                         gint   priority)
{
  update_common_theme_dir_index (icon_theme_index_uri, MATE_THEME_TYPE_ICON, priority);
}

static void
update_cursor_theme_index (GFile *cursor_theme_index_uri,
                           gint   priority)
{
#ifdef HAVE_XCURSOR
  update_common_theme_dir_index (cursor_theme_index_uri, MATE_THEME_TYPE_CURSOR, priority);
#endif
}

static void
gtk2_dir_changed (GFileMonitor              *monitor,
                  GFile                     *file,
                  GFile                     *other_file,
                  GFileMonitorEvent          event_type,
                  CommonThemeDirMonitorData *monitor_data)
{
  gchar *affected_file;

  affected_file = g_file_get_basename (file);

  /* The only file we care about is gtkrc */
  if (!strcmp (affected_file, "gtkrc")) {
    update_gtk2_index (file, monitor_data->priority);
  }

  g_free (affected_file);
}

static void
keybinding_dir_changed (GFileMonitor              *monitor,
                        GFile                     *file,
                        GFile                     *other_file,
                        GFileMonitorEvent          event_type,
                        CommonThemeDirMonitorData *monitor_data)
{
  gchar *affected_file;

  affected_file = g_file_get_basename (file);

  /* The only file we care about is gtkrc */
  if (!strcmp (affected_file, "gtkrc")) {
    update_keybinding_index (file, monitor_data->priority);
  }

  g_free (affected_file);
}

static void
marco_dir_changed (GFileMonitor              *monitor,
                      GFile                     *file,
                      GFile                     *other_file,
                      GFileMonitorEvent          event_type,
                      CommonThemeDirMonitorData *monitor_data)
{
  gchar *affected_file;

  affected_file = g_file_get_basename (file);

  /* The only file we care about is marco-theme-1.xml */
  if (!strcmp (affected_file, "metacity-theme-1.xml")) {
    update_marco_index (file, monitor_data->priority);
  }

  g_free (affected_file);
}

static void
common_theme_dir_changed (GFileMonitor              *monitor,
                          GFile                     *file,
                          GFile                     *other_file,
                          GFileMonitorEvent          event_type,
                          CommonThemeDirMonitorData *monitor_data)
{
  gchar *affected_file;

  affected_file = g_file_get_basename (file);

  /* The only file we care about is index.theme */
  if (!strcmp (affected_file, "index.theme")) {
    update_meta_theme_index (file, monitor_data->priority);
  }

  g_free (affected_file);
}

static void
common_icon_theme_dir_changed (GFileMonitor                  *monitor,
                               GFile                         *file,
                               GFile                         *other_file,
                               GFileMonitorEvent              event_type,
                               CommonIconThemeDirMonitorData *monitor_data)
{
  gchar *affected_file;

  affected_file = g_file_get_basename (file);

  /* The only file we care about is index.theme */
  if (!strcmp (affected_file, "index.theme")) {
    update_icon_theme_index (file, monitor_data->priority);
    update_cursor_theme_index (file, monitor_data->priority);
  }
  /* and the cursors subdir for cursor themes */
  else if (!strcmp (affected_file, "cursors")) {
    /* always call update_cursor_theme_index with the index.theme URI */
    GFile *parent, *index;

    parent = g_file_get_parent (file);
    index = g_file_get_child (parent, "index.theme");
    g_object_unref (parent);

    update_cursor_theme_index (index, monitor_data->priority);

    g_object_unref (index);
  }

  g_free (affected_file);
}

/* Add a monitor to a common_theme_dir. */
static gboolean
add_common_theme_dir_monitor (GFile                      *theme_dir_uri,
                              CommonThemeDirMonitorData  *monitor_data,
                              GError                    **error)
{
  GFile *uri, *subdir;
  GFileMonitor *monitor;

  uri = g_file_get_child (theme_dir_uri, "index.theme");
  update_meta_theme_index (uri, monitor_data->priority);
  g_object_unref (uri);

  /* Add the handle for this directory */
  monitor = g_file_monitor_file (theme_dir_uri, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor == NULL)
    return FALSE;

  g_signal_connect (monitor, "changed",
                    (GCallback) common_theme_dir_changed, monitor_data);

  monitor_data->common_theme_dir_handle = monitor;


  /* gtk-2 theme subdir */
  subdir = g_file_get_child (theme_dir_uri, "gtk-2.0");
  uri = g_file_get_child (subdir, "gtkrc");
  if (g_file_query_exists (uri, NULL)) {
    update_gtk2_index (uri, monitor_data->priority);
  }
  g_object_unref (uri);

  monitor = g_file_monitor_directory (subdir, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor != NULL) {
    g_signal_connect (monitor, "changed",
                      (GCallback) gtk2_dir_changed, monitor_data);
  }
  monitor_data->gtk2_dir_handle = monitor;
  g_object_unref (subdir);

  /* keybinding theme subdir */
  subdir = g_file_get_child (theme_dir_uri, "gtk-2.0-key");
  uri = g_file_get_child (subdir, "gtkrc");
  if (g_file_query_exists (uri, NULL)) {
    update_keybinding_index (uri, monitor_data->priority);
  }
  g_object_unref (uri);

  monitor = g_file_monitor_directory (subdir, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor != NULL) {
    g_signal_connect (monitor, "changed",
                      (GCallback) keybinding_dir_changed, monitor_data);
  }
  monitor_data->keybinding_dir_handle = monitor;
  g_object_unref (subdir);

  /* marco theme subdir */
  subdir = g_file_get_child (theme_dir_uri, "metacity-1");
  uri = g_file_get_child (subdir, "metacity-theme-1.xml");
  if (g_file_query_exists (uri, NULL)) {
    update_marco_index (uri, monitor_data->priority);
  }
  g_object_unref (uri);

  monitor = g_file_monitor_directory (subdir, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor != NULL) {
    g_signal_connect (monitor, "changed",
                      (GCallback) marco_dir_changed, monitor_data);
  }
  monitor_data->marco_dir_handle = monitor;
  g_object_unref (subdir);

  return TRUE;
}

static gboolean
add_common_icon_theme_dir_monitor (GFile                          *theme_dir_uri,
                                   CommonIconThemeDirMonitorData  *monitor_data,
                                   GError                        **error)
{
  GFile *index_uri;
  GFileMonitor *monitor;

  /* Add the handle for this directory */
  index_uri = g_file_get_child (theme_dir_uri, "index.theme");
  update_icon_theme_index (index_uri, monitor_data->priority);
  update_cursor_theme_index (index_uri, monitor_data->priority);
  g_object_unref (index_uri);

  monitor = g_file_monitor_file (theme_dir_uri, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor == NULL)
    return FALSE;

  g_signal_connect (monitor, "changed",
                    (GCallback) common_icon_theme_dir_changed, monitor_data);

  monitor_data->common_icon_theme_dir_handle = monitor;
  return TRUE;
}

static void
remove_common_theme_dir_monitor (CommonThemeDirMonitorData *monitor_data)
{
  g_file_monitor_cancel (monitor_data->common_theme_dir_handle);
  g_file_monitor_cancel (monitor_data->gtk2_dir_handle);
  g_file_monitor_cancel (monitor_data->keybinding_dir_handle);
  g_file_monitor_cancel (monitor_data->marco_dir_handle);
}

static void
remove_common_icon_theme_dir_monitor (CommonIconThemeDirMonitorData *monitor_data)
{
  g_file_monitor_cancel (monitor_data->common_icon_theme_dir_handle);
}

static void
top_theme_dir_changed (GFileMonitor     *monitor,
                       GFile            *file,
                       GFile            *other_file,
                       GFileMonitorEvent event_type,
                       CallbackTuple    *tuple)
{
  GHashTable *handle_hash;
  CommonThemeDirMonitorData *monitor_data;
  gint priority;

  handle_hash = tuple->handle_hash;
  priority = tuple->priority;

  if (event_type == G_FILE_MONITOR_EVENT_CREATED) {
    if (get_file_type (file) == G_FILE_TYPE_DIRECTORY) {
      monitor_data = g_new0 (CommonThemeDirMonitorData, 1);
      monitor_data->priority = priority;
      add_common_theme_dir_monitor (file, monitor_data, NULL);
      g_hash_table_insert (handle_hash, g_file_get_basename (file), monitor_data);
    }

  } else if (event_type == G_FILE_MONITOR_EVENT_DELETED) {
    gchar *name;

    name = g_file_get_basename (file);
    monitor_data = g_hash_table_lookup (handle_hash, name);
    if (monitor_data != NULL) {
      remove_common_theme_dir_monitor (monitor_data);
      g_hash_table_remove (handle_hash, name);
    }
    g_free (name);
  }
}

static void
top_icon_theme_dir_changed (GFileMonitor     *monitor,
                            GFile            *file,
                            GFile            *other_file,
                            GFileMonitorEvent event_type,
                            CallbackTuple    *tuple)
{
  GHashTable *handle_hash;
  CommonIconThemeDirMonitorData *monitor_data;
  gint priority;

  handle_hash = tuple->handle_hash;
  priority = tuple->priority;

  if (event_type == G_FILE_MONITOR_EVENT_CREATED) {
    if (get_file_type (file) == G_FILE_TYPE_DIRECTORY) {
      monitor_data = g_new0 (CommonIconThemeDirMonitorData, 1);
      monitor_data->priority = priority;
      add_common_icon_theme_dir_monitor (file, monitor_data, NULL);
      g_hash_table_insert (handle_hash, g_file_get_basename (file), monitor_data);
    }

  } else if (event_type == G_FILE_MONITOR_EVENT_DELETED) {
    gchar *name;

    name = g_file_get_basename (file);
    monitor_data = g_hash_table_lookup (handle_hash, name);
    if (monitor_data != NULL) {
      remove_common_icon_theme_dir_monitor (monitor_data);
      g_hash_table_remove (handle_hash, name);
    }
    g_free (name);
  }
}

/* Add a monitor to a top dir.  These monitors persist for the duration of the
 * lib.
 */
static gboolean
real_add_top_theme_dir_monitor (GFile    *uri,
                                gint      priority,
                                gboolean  icon_theme,
                                GError  **error)
{
  GFileInfo *file_info;
  GFileMonitor *monitor;
  GFileEnumerator *enumerator;
  CallbackTuple *tuple;

  /* Check the URI */
  if (get_file_type (uri) != G_FILE_TYPE_DIRECTORY)
    return FALSE;

  /* handle_hash is a hash of common_theme_dir names to their monitor_data.  We
   * use it to remove the monitor handles when a dir is removed.
   */
  tuple = g_new (CallbackTuple, 1);
  tuple->handle_hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, (GDestroyNotify) g_free);
  tuple->priority = priority;

  /* Monitor the top directory */
  monitor = g_file_monitor_directory (uri, G_FILE_MONITOR_NONE, NULL, NULL);
  if (monitor != NULL) {
    g_signal_connect (monitor, "changed",
                      (GCallback) (icon_theme ? top_icon_theme_dir_changed : top_theme_dir_changed),
                      tuple);
  }

  /* Go through the directory to add monitoring */
  enumerator = g_file_enumerate_children (uri,
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_STANDARD_NAME,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator == NULL)
    return FALSE;

  while ((file_info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {
    GFileType type = g_file_info_get_file_type (file_info);

    if (type == G_FILE_TYPE_DIRECTORY || type == G_FILE_TYPE_SYMBOLIC_LINK) {
      GFile *child;
      const gchar *name;
      gpointer data;

      /* Add the directory */
      name = g_file_info_get_name (file_info);
      child = g_file_get_child (uri, name);

      if (icon_theme) {
        CommonIconThemeDirMonitorData *monitor_data;
        monitor_data = g_new0 (CommonIconThemeDirMonitorData, 1);
        monitor_data->priority = priority;
        add_common_icon_theme_dir_monitor (child, monitor_data, error);
        data = monitor_data;
      } else {
        CommonThemeDirMonitorData *monitor_data;
        monitor_data = g_new0 (CommonThemeDirMonitorData, 1);
        monitor_data->priority = priority;
        add_common_theme_dir_monitor (child, monitor_data, error);
        data = monitor_data;
      }
      g_object_unref (child);

      g_hash_table_insert (tuple->handle_hash, g_strdup (name), data);
    }
    g_object_unref (file_info);
  }
  g_file_enumerator_close (enumerator, NULL, NULL);

  return TRUE;
}

static gboolean
add_top_theme_dir_monitor (GFile   *uri,
                           gint     priority,
                           GError **error)
{
  return real_add_top_theme_dir_monitor (uri, priority, FALSE, error);
}

static gboolean
add_top_icon_theme_dir_monitor (GFile   *uri,
                                gint     priority,
                                GError **error)
{
  return real_add_top_theme_dir_monitor (uri, priority, TRUE, error);
}

/* Public functions */

/* GTK/Marco/keybinding Themes */
MateThemeInfo *
mate_theme_info_new (void)
{
  MateThemeInfo *theme_info;

  theme_info = g_new0 (MateThemeInfo, 1);
  theme_info->type = MATE_THEME_TYPE_REGULAR;

  return theme_info;
}

void
mate_theme_info_free (MateThemeInfo *theme_info)
{
  g_free (theme_info->path);
  g_free (theme_info->name);
  g_free (theme_info->readable_name);
  g_free (theme_info);
}

MateThemeInfo *
mate_theme_info_find (const gchar *theme_name)
{
  return (MateThemeInfo *)
         get_theme_from_hash_by_name (theme_hash_by_name, theme_name, -1);
}

struct MateThemeInfoHashData
{
  gconstpointer user_data;
  GList *list;
};

static void
mate_theme_info_find_by_type_helper (gpointer key,
                                      GList *list,
                                      struct MateThemeInfoHashData *hash_data)
{
  guint elements = GPOINTER_TO_INT (hash_data->user_data);

  do {
    MateThemeInfo *theme_info = list->data;

    if ((elements & MATE_THEME_MARCO && theme_info->has_marco) ||
        (elements & MATE_THEME_GTK_2 && theme_info->has_gtk) ||
        (elements & MATE_THEME_GTK_2_KEYBINDING && theme_info->has_keybinding)) {
      hash_data->list = g_list_prepend (hash_data->list, theme_info);
      return;
    }

    list = list->next;
  } while (list);
}

GList *
mate_theme_info_find_by_type (guint elements)
{
  struct MateThemeInfoHashData data;
  data.user_data = GINT_TO_POINTER (elements);
  data.list = NULL;

  g_hash_table_foreach (theme_hash_by_name,
                        (GHFunc) mate_theme_info_find_by_type_helper,
                        &data);

  return data.list;
}

static void mate_theme_info_find_all_helper(const gchar* key, GList* list, GList** themes)
{
	/* only return visible themes */
	if (!((MateThemeCommonInfo*) list->data)->hidden)
	{
		*themes = g_list_prepend(*themes, list->data);
	}
}

gchar* gtk_theme_info_missing_engine(const gchar* gtk_theme, gboolean nameOnly)
{
	gchar* engine = NULL;
	gchar* gtkrc;

	gtkrc = gtkrc_find_named(gtk_theme);

	if (gtkrc)
	{
		GSList* engines = NULL;
		GSList* l;

    	gtkrc_get_details(gtkrc, &engines, NULL);

   		g_free(gtkrc);

		for (l = engines; l; l = l->next)
		{
			#if 1 // set to 0 if you can not compile with the follow code
			GtkThemeEngine* a = gtk_theme_engine_get((const gchar*) l->data);

			if (!a)
			{
				if (nameOnly)
				{
         			engine = g_strdup(l->data);
        		}
        		else
        		{
					// esto necesita más trabajo, pero creo que debian no se
					// salva ni con el anterior fix.
					// GTK_ENGINE_DIR aún sigue conteniendo un path erroneo.
         			engine = g_module_build_path(GTK_ENGINE_DIR, l->data);
        		}

        		break;
			}

			#else

			/* This code do not work on distros with more of one gtk theme
			 * engine path. Like debian. But yes on others like Archlinux.
			 * Example, debian use:
			 * /usr/lib/i386-linux-gnu/2.10.0/engines/
			 * and /usr/lib/2.10.0/engines/
			 *
			 * some links
			 * http://forums.linuxmint.com/viewtopic.php?f=190&t=85015
			 */
			gchar* full = g_module_build_path(GTK_ENGINE_DIR, l->data);

			gboolean found = g_file_test(full, G_FILE_TEST_EXISTS);

			if (!found)
			{
				if (nameOnly)
				{
         			engine = g_strdup(l->data);
         			g_free(full);
        		}
        		else
        		{
        			engine = full;
        		}

        		break;
        	}

        	g_free(full);
			#endif
        }

		g_slist_foreach(engines, (GFunc) g_free, NULL);
		g_slist_free(engines);
	}

	return engine;
}

/* Icon themes */
MateThemeIconInfo *
mate_theme_icon_info_new (void)
{
  MateThemeIconInfo *icon_theme_info;

  icon_theme_info = g_new0 (MateThemeIconInfo, 1);
  icon_theme_info->type = MATE_THEME_TYPE_ICON;

  return icon_theme_info;
}

void
mate_theme_icon_info_free (MateThemeIconInfo *icon_theme_info)
{
  g_free (icon_theme_info->name);
  g_free (icon_theme_info->readable_name);
  g_free (icon_theme_info->path);
  g_free (icon_theme_info);
}

MateThemeIconInfo *
mate_theme_icon_info_find (const gchar *icon_theme_name)
{
  g_return_val_if_fail (icon_theme_name != NULL, NULL);

  return (MateThemeIconInfo *)
         get_theme_from_hash_by_name (icon_theme_hash_by_name, icon_theme_name, -1);
}

GList *
mate_theme_icon_info_find_all (void)
{
  GList *list = NULL;

  g_hash_table_foreach (icon_theme_hash_by_name,
                        (GHFunc) mate_theme_info_find_all_helper,
                        &list);

  return list;
}

gint
mate_theme_icon_info_compare (MateThemeIconInfo *a,
                               MateThemeIconInfo *b)
{
  gint cmp;

  cmp = safe_strcmp (a->path, b->path);
  if (cmp != 0) return cmp;

  return safe_strcmp (a->name, b->name);
}

/* Cursor themes */
MateThemeCursorInfo *
mate_theme_cursor_info_new (void)
{
  MateThemeCursorInfo *theme_info;

  theme_info = g_new0 (MateThemeCursorInfo, 1);
  theme_info->type = MATE_THEME_TYPE_CURSOR;

  return theme_info;
}

void
mate_theme_cursor_info_free (MateThemeCursorInfo *cursor_theme_info)
{
  g_free (cursor_theme_info->name);
  g_free (cursor_theme_info->readable_name);
  g_free (cursor_theme_info->path);
  g_array_free (cursor_theme_info->sizes, TRUE);
  if (cursor_theme_info->thumbnail != NULL)
    g_object_unref (cursor_theme_info->thumbnail);
  g_free (cursor_theme_info);
}

MateThemeCursorInfo *
mate_theme_cursor_info_find (const gchar *cursor_theme_name)
{
  g_return_val_if_fail (cursor_theme_name != NULL, NULL);

  return (MateThemeCursorInfo *)
         get_theme_from_hash_by_name (cursor_theme_hash_by_name, cursor_theme_name, -1);
}

GList *
mate_theme_cursor_info_find_all (void)
{
  GList *list = NULL;

  g_hash_table_foreach (cursor_theme_hash_by_name,
                        (GHFunc) mate_theme_info_find_all_helper,
                        &list);

  return list;
}

gint
mate_theme_cursor_info_compare (MateThemeCursorInfo *a,
                                 MateThemeCursorInfo *b)
{
  gint cmp;

  cmp = safe_strcmp (a->path, b->path);
  if (cmp != 0) return cmp;

  return safe_strcmp (a->name, b->name);
}

/* Meta themes */
MateThemeMetaInfo* mate_theme_meta_info_new(void)
{
	MateThemeMetaInfo* theme_info;

	theme_info = g_new0(MateThemeMetaInfo, 1);
	theme_info->type = MATE_THEME_TYPE_METATHEME;

	return theme_info;
}

void mate_theme_meta_info_free(MateThemeMetaInfo* meta_theme_info)
{
	g_free(meta_theme_info->path);
	g_free(meta_theme_info->readable_name);
	g_free(meta_theme_info->name);
	g_free(meta_theme_info->comment);
	g_free(meta_theme_info->application_font);
	g_free(meta_theme_info->documents_font);
	g_free(meta_theme_info->desktop_font);
	g_free(meta_theme_info->windowtitle_font);
	g_free(meta_theme_info->monospace_font);
	g_free(meta_theme_info->background_image);
	g_free(meta_theme_info->gtk_theme_name);
	g_free(meta_theme_info->gtk_color_scheme);
	g_free(meta_theme_info->icon_theme_name);
	g_free(meta_theme_info->marco_theme_name);
	g_free(meta_theme_info->notification_theme_name);
	g_free(meta_theme_info);
}

gboolean mate_theme_meta_info_validate(const MateThemeMetaInfo* info, GError** error)
{
	MateThemeInfo* theme;
	gchar* engine;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	theme = mate_theme_info_find (info->gtk_theme_name);

	if (!theme || !theme->has_gtk)
	{
		g_set_error (error, MATE_THEME_ERROR, MATE_THEME_ERROR_GTK_THEME_NOT_AVAILABLE,
			_("This theme will not look as intended because the required GTK+ theme '%s' is not installed."),
			info->gtk_theme_name);
		return FALSE;
	}

	theme = mate_theme_info_find (info->marco_theme_name);

	if (!theme || !theme->has_marco)
	{
		g_set_error (error, MATE_THEME_ERROR, MATE_THEME_ERROR_WM_THEME_NOT_AVAILABLE,
			_("This theme will not look as intended because the required window manager theme '%s' is not installed."),
			info->marco_theme_name);
		return FALSE;
	}

	if (!mate_theme_icon_info_find (info->icon_theme_name))
	{
		g_set_error (error, MATE_THEME_ERROR, MATE_THEME_ERROR_ICON_THEME_NOT_AVAILABLE,
			_("This theme will not look as intended because the required icon theme '%s' is not installed."),
			info->icon_theme_name);
		return FALSE;
	}

	/* check for gtk theme engines */
	engine = gtk_theme_info_missing_engine(info->gtk_theme_name, TRUE);

	if (engine != NULL)
	{
		g_set_error (error, MATE_THEME_ERROR, MATE_THEME_ERROR_GTK_ENGINE_NOT_AVAILABLE,
			_("This theme will not look as intended because the required GTK+ theme engine '%s' is not installed."),
			engine);
		g_free (engine);
		return FALSE;
	}

	return TRUE;
}

MateThemeMetaInfo* mate_theme_meta_info_find(const char* meta_theme_name)
{
	g_return_val_if_fail(meta_theme_name != NULL, NULL);

	return (MateThemeMetaInfo*) get_theme_from_hash_by_name (meta_theme_hash_by_name, meta_theme_name, -1);
}

GList* mate_theme_meta_info_find_all(void)
{
  GList* list = NULL;

  g_hash_table_foreach (meta_theme_hash_by_name, (GHFunc) mate_theme_info_find_all_helper, &list);

  return list;
}

gint
mate_theme_meta_info_compare (MateThemeMetaInfo *a,
                               MateThemeMetaInfo *b)
{
  gint cmp;

  cmp = safe_strcmp (a->path, b->path);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->readable_name, b->readable_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->name, b->name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->comment, b->comment);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->icon_file, b->icon_file);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->gtk_theme_name, b->gtk_theme_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->gtk_color_scheme, b->gtk_color_scheme);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->marco_theme_name, b->marco_theme_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->icon_theme_name, b->icon_theme_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->notification_theme_name, b->notification_theme_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->sound_theme_name, b->sound_theme_name);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->application_font, b->application_font);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->documents_font, b->documents_font);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->desktop_font, b->desktop_font);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->windowtitle_font, b->windowtitle_font);
  if (cmp != 0) return cmp;

  cmp = safe_strcmp (a->monospace_font, b->monospace_font);
  if (cmp != 0) return cmp;

  return safe_strcmp (a->background_image, b->background_image);
}

void
mate_theme_info_register_theme_change (ThemeChangedCallback func,
                                        gpointer data)
{
  ThemeCallbackData *callback_data;

  g_return_if_fail (func != NULL);

  callback_data = g_new (ThemeCallbackData, 1);
  callback_data->func = func;
  callback_data->data = data;

  callbacks = g_list_prepend (callbacks, callback_data);
}

gboolean
mate_theme_color_scheme_parse (const gchar *scheme, GdkColor *colors)
{
  gchar **color_scheme_strings, **color_scheme_pair, *current_string;
  gint i;

  if (!scheme || !strcmp (scheme, ""))
    return FALSE;

  /* initialise the array */
  for (i = 0; i < NUM_SYMBOLIC_COLORS; i++)
    colors[i].red = colors[i].green = colors[i].blue = 0;

  /* The color scheme string consists of name:color pairs, separated by
   * newlines, so first we split the string up by new line */

  color_scheme_strings = g_strsplit (scheme, "\n", 0);

  /* loop through the name:color pairs, and save the color if we recognise the name */
  i = 0;
  while ((current_string = color_scheme_strings[i++])) {
    color_scheme_pair = g_strsplit (current_string, ":", 0);

    if (color_scheme_pair[0] != NULL && color_scheme_pair[1] != NULL) {
      g_strstrip (color_scheme_pair[0]);
      g_strstrip (color_scheme_pair[1]);

      if (!strcmp ("fg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_FG]);
      else if (!strcmp ("bg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_BG]);
      else if (!strcmp ("text_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_TEXT]);
      else if (!strcmp ("base_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_BASE]);
      else if (!strcmp ("selected_fg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_SELECTED_FG]);
      else if (!strcmp ("selected_bg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_SELECTED_BG]);
      else if (!strcmp ("tooltip_fg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_TOOLTIP_FG]);
      else if (!strcmp ("tooltip_bg_color", color_scheme_pair[0]))
        gdk_color_parse (color_scheme_pair[1], &colors[COLOR_TOOLTIP_BG]);
    }

    g_strfreev (color_scheme_pair);
  }

  g_strfreev (color_scheme_strings);

  return TRUE;
}

gboolean
mate_theme_color_scheme_equal (const gchar *s1, const gchar *s2)
{
  GdkColor c1[NUM_SYMBOLIC_COLORS], c2[NUM_SYMBOLIC_COLORS];
  int i;

  if (!mate_theme_color_scheme_parse (s1, c1) ||
      !mate_theme_color_scheme_parse (s2, c2))
    return FALSE;

  for (i = 0; i < NUM_SYMBOLIC_COLORS; ++i) {
    if (!gdk_color_equal (&c1[i], &c2[i]))
      return FALSE;
  }

  return TRUE;
}

void
mate_theme_init ()
{
  GFile *top_theme_dir;
  gchar *top_theme_dir_string;
  static gboolean initted = FALSE;
  gchar **search_path;
  gint i, n;

  if (initted)
    return;

  initting = TRUE;

  meta_theme_hash_by_uri = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  meta_theme_hash_by_name = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  icon_theme_hash_by_uri = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  icon_theme_hash_by_name = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  cursor_theme_hash_by_uri = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  cursor_theme_hash_by_name = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  theme_hash_by_uri = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  theme_hash_by_name = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Add all the toplevel theme dirs. */
  /* $datadir/themes */
  top_theme_dir_string = gtk_rc_get_theme_dir ();
  top_theme_dir = g_file_new_for_path (top_theme_dir_string);
  g_free (top_theme_dir_string);
  add_top_theme_dir_monitor (top_theme_dir, 1, NULL);
  g_object_unref (top_theme_dir);

  /* ~/.themes */
  top_theme_dir_string  = g_build_filename (g_get_home_dir (), ".themes", NULL);
  top_theme_dir = g_file_new_for_path (top_theme_dir_string);
  g_free (top_theme_dir_string);
  if (!g_file_query_exists (top_theme_dir, NULL))
    g_file_make_directory (top_theme_dir, NULL, NULL);
  add_top_theme_dir_monitor (top_theme_dir, 0, NULL);
  g_object_unref (top_theme_dir);

  /* ~/.icons */
  top_theme_dir_string = g_build_filename (g_get_home_dir (), ".icons", NULL);
  top_theme_dir = g_file_new_for_path (top_theme_dir_string);
  g_free (top_theme_dir_string);
  if (!g_file_query_exists (top_theme_dir, NULL))
    g_file_make_directory (top_theme_dir, NULL, NULL);
  g_object_unref (top_theme_dir);

  /* icon theme search path */
  gtk_icon_theme_get_search_path (gtk_icon_theme_get_default (), &search_path, &n);
  for (i = 0; i < n; ++i) {
    top_theme_dir = g_file_new_for_path (search_path[i]);
    add_top_icon_theme_dir_monitor (top_theme_dir, i, NULL);
    g_object_unref (top_theme_dir);
  }
  g_strfreev (search_path);

#ifdef XCURSOR_ICONDIR
  /* if there's a separate xcursors dir, add that as well */
  if (strcmp (XCURSOR_ICONDIR, top_theme_dir_string) &&
      strcmp (XCURSOR_ICONDIR, "/usr/share/icons")) {
    top_theme_dir = g_file_new_for_path (XCURSOR_ICONDIR);
    add_top_icon_theme_dir_monitor (top_theme_dir, 1, NULL);
    g_object_unref (top_theme_dir);
  }
#endif

#ifdef HAVE_XCURSOR
  /* make sure we have the default theme */
  if (!mate_theme_cursor_info_find ("default"))
    add_default_cursor_theme ();
#else
  /* If we don't have Xcursor, use the built-in cursor fonts instead */
  read_cursor_fonts ();
#endif

  /* done */
  initted = TRUE;
  initting = FALSE;
}
