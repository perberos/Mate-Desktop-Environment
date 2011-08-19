/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 The MATE Foundation
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include <locale.h>

#include <X11/Xatom.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include "mate-settings-profile.h"
#include "gsd-font-manager.h"
#include "delayed-dialog.h"

static void     gsd_font_manager_class_init  (GsdFontManagerClass *klass);
static void     gsd_font_manager_init        (GsdFontManager      *font_manager);

G_DEFINE_TYPE (GsdFontManager, gsd_font_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static void
update_property (GString *props, const gchar* key, const gchar* value)
{
        gchar* needle;
        size_t needle_len;
        gchar* found = NULL;

        /* update an existing property */
        needle = g_strconcat (key, ":", NULL);
        needle_len = strlen (needle);
        if (g_str_has_prefix (props->str, needle))
                found = props->str;
        else 
            found = strstr (props->str, needle);

        if (found) {
                size_t value_index;
                gchar* end;

                end = strchr (found, '\n');
                value_index = (found - props->str) + needle_len + 1;
                g_string_erase (props, value_index, end ? (end - found - needle_len) : -1);
                g_string_insert (props, value_index, "\n");
                g_string_insert (props, value_index, value);
        } else {
                g_string_append_printf (props, "%s:\t%s\n", key, value);
        }
}

static void
load_xcursor_theme (MateConfClient *client)
{
        char       *cursor_theme;
        int         size;
        GString    *add_string;
        Display    *dpy;
        gchar      numbuf[20];

        mate_settings_profile_start (NULL);

        size = mateconf_client_get_int (client,
                                     "/desktop/mate/peripherals/mouse/cursor_size",
                                     NULL);
        if (size <= 0) {
                return;
        }

        cursor_theme = mateconf_client_get_string (client,
                                                "/desktop/mate/peripherals/mouse/cursor_theme",
                                                NULL);
        if (cursor_theme == NULL) {
                return;
        }

        /* get existing properties */
        dpy = XOpenDisplay (NULL);
        g_return_if_fail (dpy != NULL);
        add_string = g_string_new (XResourceManagerString (dpy));
        g_debug("load_xcursor_theme: existing res '%s'", add_string->str);

        update_property (add_string, "Xcursor.theme", cursor_theme);
        update_property (add_string, "Xcursor.theme_core", "true");
        g_snprintf (numbuf, sizeof (numbuf), "%i", size);
        update_property (add_string, "Xcursor.size", numbuf);

        g_debug("load_xcursor_theme: new res '%s'", add_string->str);

        /* Set the new X property */
        XChangeProperty(dpy, RootWindow (dpy, 0),
                XA_RESOURCE_MANAGER, XA_STRING, 8, PropModeReplace, add_string->str, add_string->len);
        XCloseDisplay (dpy);

        g_free (cursor_theme);
        g_string_free (add_string, TRUE);

        mate_settings_profile_end (NULL);
}

static char*
setup_dir (const char *font_dir_name, gboolean create)
{
        char *font_dir;

        font_dir = g_build_path (G_DIR_SEPARATOR_S, g_get_home_dir (), ".mate2", "share", font_dir_name, NULL);

        if (create) {
                if (g_mkdir_with_parents (font_dir, 0755) != 0) {
                        g_warning ("Cannot create needed directory \"%s\".", font_dir);
                        g_free (font_dir);
                        font_dir = NULL;
                }
        } else if (! g_file_test (font_dir, G_FILE_TEST_EXISTS)) {
                g_free (font_dir);
                font_dir = NULL;
        }

        return font_dir;
}

static char *
empty_check_dir (char *font_dir)
{
        char *file_name;

        if (!font_dir)
                return NULL;

        /* remove the fonts.dir and fonts.scale files that mkfontdir generates. */

        file_name = g_build_filename (G_DIR_SEPARATOR_S, font_dir, "fonts.dir", NULL);
        unlink (file_name);
        g_free (file_name);

        file_name = g_build_filename (G_DIR_SEPARATOR_S, font_dir, "fonts.scale", NULL);
        unlink (file_name);
        g_free (file_name);

        /* if it's empty, get rid of it. */
        if (0 == rmdir (font_dir)) {
                g_free (font_dir);
                font_dir = NULL;
        }

        return font_dir;
}

static char*
setup_font_dir (MateConfClient *client)
{
        return empty_check_dir (setup_dir ("fonts", FALSE));
}

static char*
setup_cursor_dir (MateConfClient *client)
{
        char          *cursor_dir;
        char          *cursor_font;
        DIR           *dir;
        struct dirent *file_dirent;

        cursor_font = mateconf_client_get_string (client,
                                               "/desktop/mate/peripherals/mouse/cursor_font",
                                               NULL);
        if (cursor_font != NULL) {
                if (!g_path_is_absolute (cursor_font) ||
                    !g_file_test (cursor_font, G_FILE_TEST_IS_REGULAR)) {
                        /* font file is not usable */
                        g_free (cursor_font);
                        cursor_font = NULL;
                }
        }

        cursor_dir = setup_dir ("cursor-fonts", cursor_font != NULL);

        /* remove previously made symlinks, if any */
        if (cursor_dir) {
                dir = opendir (cursor_dir);
                while ((file_dirent = readdir (dir)) != NULL) {
                        struct stat st;
                        char       *link_name;

                        link_name = g_build_filename (cursor_dir, file_dirent->d_name, NULL);
                        if (lstat (link_name, &st)) {
                                g_free (link_name);
                                continue;
                        }
                        g_free (link_name);

                        if (S_ISLNK (st.st_mode))
                                unlink (link_name);
                }
                closedir (dir);
        }

        if (cursor_font && cursor_dir) {
                char *newpath;
                char *font_name;

                font_name = strrchr (cursor_font, G_DIR_SEPARATOR);
                newpath = g_build_filename (cursor_dir, font_name, NULL);
                symlink (cursor_font, newpath);
                g_free (newpath);
                g_free (cursor_font);
                cursor_font = NULL;
        } else {
                cursor_dir = empty_check_dir (cursor_dir);
        }

        return cursor_dir;
}

static void
load_font_paths (MateConfClient *client)
{
        char          *font_dir_name;
        char          *cursor_dir_name;

        char         **font_path;
        char         **new_font_path;
        int            n_fonts;
        int            new_n_fonts;

        int            i;

        const char    *argv[4];
        int            argc = 0;

        mate_settings_profile_start (NULL);

        font_dir_name = setup_font_dir (client);
        cursor_dir_name = setup_cursor_dir (client);

        if (font_dir_name == NULL && cursor_dir_name == NULL)
                goto done;

        /* run mkfontdir */
        argv[argc++] = "mkfontdir";
        if (font_dir_name)
                argv[argc++] = font_dir_name;
        if (cursor_dir_name)
                argv[argc++] = cursor_dir_name;
        argv[argc] = NULL;
        g_spawn_sync (NULL, /* current dir */
                      (char **) (void *) argv, NULL /* envp */,
                      G_SPAWN_SEARCH_PATH,
                      NULL, NULL, /* child_setup */
                      NULL, NULL, NULL, NULL);

        /* Set the font path */
        font_path = XGetFontPath (gdk_x11_get_default_xdisplay (), &n_fonts);
        new_n_fonts = n_fonts;
        if (cursor_dir_name && (n_fonts == 0 || strcmp (font_path[0], cursor_dir_name)))
                new_n_fonts++;
        if (font_dir_name && (n_fonts == 0 || strcmp (font_path[n_fonts-1], font_dir_name)))
                new_n_fonts++;

        if (new_n_fonts == n_fonts)
                new_font_path = font_path;
        else {
                new_font_path = g_new0 (char *, new_n_fonts);

                if (cursor_dir_name && (n_fonts == 0 || strcmp (font_path[0], cursor_dir_name))) {
                        new_font_path[0] = cursor_dir_name;
                        for (i = 0; i < n_fonts; i++)
                                new_font_path [i+1] = font_path [i];
                } else {
                        for (i = 0; i < n_fonts; i++)
                                new_font_path [i] = font_path [i];
                }

                if (font_dir_name && (n_fonts == 0 || strcmp (font_path[n_fonts-1], font_dir_name))) {
                        new_font_path[new_n_fonts-1] = font_dir_name;
                }
        }

        /* We set font path even if it was not changed, to enforce dropping
         * caches in the server */
        gdk_error_trap_push ();
        XSetFontPath (gdk_display, new_font_path, new_n_fonts);
        gdk_flush ();

        /* if there was an error setting the new path, revert */
        if (gdk_error_trap_pop ()) {
                XSetFontPath (gdk_display, font_path, n_fonts);
        }

        g_free (font_dir_name);
        g_free (cursor_dir_name);

        if (new_font_path != font_path)
                g_free (new_font_path);

        XFreeFontPath (font_path);

done:
        mate_settings_profile_end (NULL);
}

gboolean
gsd_font_manager_start (GsdFontManager *manager,
                        GError        **error)
{
        MateConfClient *client;

        g_debug ("Starting font manager");
        mate_settings_profile_start (NULL);

        client = mateconf_client_get_default ();

        load_xcursor_theme (client);
        load_font_paths (client);

        g_object_unref (client);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_font_manager_stop (GsdFontManager *manager)
{
        g_debug ("Stopping font manager");
}

static void
gsd_font_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdFontManager *self;

        self = GSD_FONT_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_font_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdFontManager *self;

        self = GSD_FONT_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_font_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdFontManager      *font_manager;
        GsdFontManagerClass *klass;

        klass = GSD_FONT_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_FONT_MANAGER));

        font_manager = GSD_FONT_MANAGER (G_OBJECT_CLASS (gsd_font_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (font_manager);
}

static void
gsd_font_manager_class_init (GsdFontManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_font_manager_get_property;
        object_class->set_property = gsd_font_manager_set_property;
        object_class->constructor = gsd_font_manager_constructor;
}

static void
gsd_font_manager_init (GsdFontManager *manager)
{
}

GsdFontManager *
gsd_font_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_FONT_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_FONT_MANAGER (manager_object);
}
