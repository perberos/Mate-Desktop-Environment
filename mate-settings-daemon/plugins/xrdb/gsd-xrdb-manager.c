/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2003 Ross Burton <ross@burtonini.com>
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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "mate-settings-profile.h"
#include "gsd-xrdb-manager.h"

#define GSD_XRDB_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_XRDB_MANAGER, GsdXrdbManagerPrivate))

#define SYSTEM_AD_DIR    DATADIR "/xrdb"
#define GENERAL_AD       SYSTEM_AD_DIR "/General.ad"
#define USER_AD_DIR      ".mate2/xrdb"
#define USER_X_RESOURCES ".Xresources"
#define USER_X_DEFAULTS  ".Xdefaults"

#define GTK_THEME_KEY "/desktop/mate/interface/gtk_theme"

struct GsdXrdbManagerPrivate
{
        GtkWidget *widget;
};

static void     gsd_xrdb_manager_class_init  (GsdXrdbManagerClass *klass);
static void     gsd_xrdb_manager_init        (GsdXrdbManager      *xrdb_manager);
static void     gsd_xrdb_manager_finalize    (GObject             *object);

G_DEFINE_TYPE (GsdXrdbManager, gsd_xrdb_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;
static void

append_color_define (GString        *string,
                     const char     *name,
                     const GdkColor *color)
{
        g_return_if_fail (string != NULL);
        g_return_if_fail (name != NULL);
        g_return_if_fail (color != NULL);

        g_string_append_printf (string,
                                "#define %s #%2.2hx%2.2hx%2.2hx\n",
                                name,
                                color->red>>8,
                                color->green>>8,
                                color->blue>>8);
}

static GdkColor*
color_shade (GdkColor *a,
             gdouble   shade,
             GdkColor *b)
{
        guint16 red, green, blue;

        red = CLAMP ((a->red) * shade, 0, 0xFFFF);
        green = CLAMP ((a->green) * shade, 0, 0xFFFF);
        blue = CLAMP ((a->blue) * shade, 0, 0xFFFF);

        b->red = red;
        b->green = green;
        b->blue = blue;

        return b;
}

static void
append_theme_colors (GtkStyle *style,
                     GString  *string)
{
        GdkColor tmp;

        g_return_if_fail (style != NULL);
        g_return_if_fail (string != NULL);

        append_color_define (string,
                             "BACKGROUND",
                             &style->bg[GTK_STATE_NORMAL]);
        append_color_define (string,
                             "FOREGROUND",
                             &style->fg[GTK_STATE_NORMAL]);
        append_color_define (string,
                             "SELECT_BACKGROUND",
                             &style->bg[GTK_STATE_SELECTED]);
        append_color_define (string,
                             "SELECT_FOREGROUND",
                             &style->text[GTK_STATE_SELECTED]);
        append_color_define (string,
                             "WINDOW_BACKGROUND",
                             &style->base[GTK_STATE_NORMAL]);
        append_color_define (string,
                             "WINDOW_FOREGROUND",
                             &style->text[GTK_STATE_NORMAL]);
        append_color_define (string,
                             "INACTIVE_BACKGROUND",
                             &style->bg[GTK_STATE_INSENSITIVE]);
        append_color_define (string,
                             "INACTIVE_FOREGROUND",
                             &style->text[GTK_STATE_INSENSITIVE]);
        append_color_define (string,
                             "ACTIVE_BACKGROUND",
                             &style->bg[GTK_STATE_SELECTED]);
        append_color_define (string,
                             "ACTIVE_FOREGROUND",
                             &style->text[GTK_STATE_SELECTED]);

        append_color_define (string,
                             "HIGHLIGHT",
                             color_shade (&style->bg[GTK_STATE_NORMAL], 1.2, &tmp));
        append_color_define (string,
                             "LOWLIGHT",
                             color_shade (&style->bg[GTK_STATE_NORMAL], 2.0/3.0, &tmp));
        return;
}

/**
 * Scan a single directory for .ad files, and return them all in a
 * GSList*
 */
static GSList*
scan_ad_directory (const char *path,
                   GError    **error)
{
        GSList     *list;
        GDir       *dir;
        const char *entry;
        GError     *local_error;

        list = NULL;

        g_return_val_if_fail (path != NULL, NULL);

        local_error = NULL;
        dir = g_dir_open (path, 0, &local_error);
        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return NULL;
        }

        while ((entry = g_dir_read_name (dir)) != NULL) {
                if (g_str_has_suffix (entry, ".ad")) {
                        list = g_slist_prepend (list, g_strdup_printf ("%s/%s", path, entry));
                }
        }

        g_dir_close (dir);

        /* TODO: sort still? */
        return g_slist_sort (list, (GCompareFunc)strcmp);
}

/**
 * Compare two file names on their base names.
 */
static gint
compare_basenames (gconstpointer a,
                   gconstpointer b)
{
        char *base_a;
        char *base_b;
        int   res;

        base_a = g_path_get_basename (a);
        base_b = g_path_get_basename (b);
        res = strcmp (base_a, base_b);
        g_free (base_a);
        g_free (base_b);

        return res;
}

/**
 * Scan the user and system paths, and return a list of strings in the
 * right order for processing.
 */
static GSList*
scan_for_files (GsdXrdbManager *manager,
                GError        **error)
{
        const char *home_dir;
        GSList     *user_list;
        GSList     *system_list;
        GSList     *list;
        GSList     *p;
        GError     *local_error;

        list = NULL;
        user_list = NULL;
        system_list = NULL;

        local_error = NULL;
        system_list = scan_ad_directory (SYSTEM_AD_DIR, &local_error);
        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                return NULL;
        }

        home_dir = g_get_home_dir ();
        if (home_dir != NULL) {
                char *user_ad;

                user_ad = g_build_filename (home_dir, USER_AD_DIR, NULL);

                if (g_file_test (user_ad, G_FILE_TEST_IS_DIR)) {

                        local_error = NULL;
                        user_list = scan_ad_directory (user_ad, &local_error);
                        if (local_error != NULL) {
                                g_propagate_error (error, local_error);

                                g_slist_foreach (system_list, (GFunc)g_free, NULL);
                                g_slist_free (system_list);
                                g_free (user_ad);
                                return NULL;
                        }
                }

                g_free (user_ad);

        } else {
                g_warning (_("Cannot determine user's home directory"));
        }

        /* An alternative approach would be to strdup() the strings
           and free the entire contents of these lists, but that is a
           little inefficient for my liking - RB */
        for (p = system_list; p != NULL; p = g_slist_next (p)) {
                if (strcmp (p->data, GENERAL_AD) == 0) {
                        /* We ignore this, free the data now */
                        g_free (p->data);
                        continue;
                }
                if (g_slist_find_custom (user_list, p->data, compare_basenames)) {
                        /* Ditto */
                        g_free (p->data);
                        continue;
                }
                list = g_slist_prepend (list, p->data);
        }
        g_slist_free (system_list);

        for (p = user_list; p != NULL; p = g_slist_next (p)) {
                list = g_slist_prepend (list, p->data);
        }
        g_slist_free (user_list);

        /* Reverse the order so it is the correct way */
        list = g_slist_reverse (list);

        /* Add the initial file */
        list = g_slist_prepend (list, g_strdup (GENERAL_AD));

        return list;
}

/**
 * Append the contents of a file onto the end of a GString
 */
static void
append_file (const char *file,
             GString    *string,
             GError    **error)
{
        char *contents;

        g_return_if_fail (string != NULL);
        g_return_if_fail (file != NULL);

        if (g_file_get_contents (file, &contents, NULL, error)) {
                g_string_append (string, contents);
                g_free (contents);
        }
}

/**
 * Append an X resources file, such as .Xresources, or .Xdefaults
 */
static void
append_xresource_file (const char *filename,
                       GString    *string,
                       GError    **error)
{
        const char *home_path;
        char       *xresources;

        g_return_if_fail (string != NULL);

        home_path = g_get_home_dir ();
        if (home_path == NULL) {
                g_warning (_("Cannot determine user's home directory"));
                return;
        }

        xresources = g_build_filename (home_path, filename, NULL);
        if (g_file_test (xresources, G_FILE_TEST_EXISTS)) {
                GError *local_error;

                local_error = NULL;

                append_file (xresources, string, &local_error);
                if (local_error != NULL) {
                        g_warning ("%s", local_error->message);
                        g_propagate_error (error, local_error);
                }
        }
        g_free (xresources);
}

static gboolean
write_all (int         fd,
           const char *buf,
           gsize       to_write)
{
        while (to_write > 0) {
                gssize count = write (fd, buf, to_write);
                if (count < 0) {
                        if (errno != EINTR)
                                return FALSE;
                } else {
                        to_write -= count;
                        buf += count;
                }
        }

        return TRUE;
}

static void
child_watch_cb (GPid     pid,
                int      status,
                gpointer user_data)
{
        char *command = user_data;

        if (!WIFEXITED (status) || WEXITSTATUS (status)) {
                g_warning ("Command %s failed", command);
        }
}

static void
spawn_with_input (const char *command,
                  const char *input)
{
        char   **argv;
        int      child_pid;
        int      inpipe;
        GError  *error;
        gboolean res;

        argv = NULL;
        res = g_shell_parse_argv (command, NULL, &argv, NULL);
        if (! res) {
                g_warning ("Unable to parse command: %s", command);
                return;
        }

        error = NULL;
        res = g_spawn_async_with_pipes (NULL,
                                        argv,
                                        NULL,
                                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                        NULL,
                                        NULL,
                                        &child_pid,
                                        &inpipe,
                                        NULL,
                                        NULL,
                                        &error);
        g_strfreev (argv);

        if (! res) {
                g_warning ("Could not execute %s: %s", command, error->message);
                g_error_free (error);

                return;
        }

        if (input != NULL) {
                if (! write_all (inpipe, input, strlen (input))) {
                        g_warning ("Could not write input to %s", command);
                }

                close (inpipe);
        }

        g_child_watch_add (child_pid, (GChildWatchFunc) child_watch_cb, (gpointer)command);
}

static void
apply_settings (GsdXrdbManager *manager,
                GtkStyle       *style)
{
        const char *command;
        GString    *string;
        GSList     *list;
        GSList     *p;
        GError     *error;

        mate_settings_profile_start (NULL);

        command = "xrdb -merge -quiet";

        string = g_string_sized_new (256);
        append_theme_colors (style, string);

        error = NULL;
        list = scan_for_files (manager, &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
        }

        for (p = list; p != NULL; p = p->next) {
                error = NULL;
                append_file (p->data, string, &error);
                if (error != NULL) {
                        g_warning ("%s", error->message);
                        g_error_free (error);
                }
        }

        g_slist_foreach (list, (GFunc)g_free, NULL);
        g_slist_free (list);

        error = NULL;
        append_xresource_file (USER_X_RESOURCES, string, &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
        }

        error = NULL;
        append_xresource_file (USER_X_DEFAULTS, string, &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
        }

        spawn_with_input (command, string->str);
        g_string_free (string, TRUE);

        mate_settings_profile_end (NULL);

        return;
}

static void
theme_changed (GtkSettings    *settings,
               GParamSpec     *pspec,
               GsdXrdbManager *manager)
{
        apply_settings (manager, gtk_widget_get_style (manager->priv->widget));
}

gboolean
gsd_xrdb_manager_start (GsdXrdbManager *manager,
                        GError        **error)
{
        mate_settings_profile_start (NULL);

        /* the initialization is done here otherwise
           mate_settings_xsettings_load would generate
           false hit as gtk-theme-name is set to Default in
           mate_settings_xsettings_init */
        g_signal_connect (gtk_settings_get_default (),
                          "notify::gtk-theme-name",
                          G_CALLBACK (theme_changed),
                          manager);

        manager->priv->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_widget_ensure_style (manager->priv->widget);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_xrdb_manager_stop (GsdXrdbManager *manager)
{
        GsdXrdbManagerPrivate *p = manager->priv;

        g_debug ("Stopping xrdb manager");

        g_signal_handlers_disconnect_by_func (gtk_settings_get_default (),
                                              theme_changed,
                                              manager);

        if (p->widget != NULL) {
                gtk_widget_destroy (p->widget);
                p->widget = NULL;
        }
}

static void
gsd_xrdb_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdXrdbManager *self;

        self = GSD_XRDB_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_xrdb_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdXrdbManager *self;

        self = GSD_XRDB_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_xrdb_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdXrdbManager      *xrdb_manager;
        GsdXrdbManagerClass *klass;

        klass = GSD_XRDB_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_XRDB_MANAGER));

        xrdb_manager = GSD_XRDB_MANAGER (G_OBJECT_CLASS (gsd_xrdb_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (xrdb_manager);
}

static void
gsd_xrdb_manager_dispose (GObject *object)
{
        GsdXrdbManager *xrdb_manager;

        xrdb_manager = GSD_XRDB_MANAGER (object);

        G_OBJECT_CLASS (gsd_xrdb_manager_parent_class)->dispose (object);
}

static void
gsd_xrdb_manager_class_init (GsdXrdbManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_xrdb_manager_get_property;
        object_class->set_property = gsd_xrdb_manager_set_property;
        object_class->constructor = gsd_xrdb_manager_constructor;
        object_class->dispose = gsd_xrdb_manager_dispose;
        object_class->finalize = gsd_xrdb_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdXrdbManagerPrivate));
}

static void
gsd_xrdb_manager_init (GsdXrdbManager *manager)
{
        manager->priv = GSD_XRDB_MANAGER_GET_PRIVATE (manager);

}

static void
gsd_xrdb_manager_finalize (GObject *object)
{
        GsdXrdbManager *xrdb_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_XRDB_MANAGER (object));

        xrdb_manager = GSD_XRDB_MANAGER (object);

        g_return_if_fail (xrdb_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_xrdb_manager_parent_class)->finalize (object);
}

GsdXrdbManager *
gsd_xrdb_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_XRDB_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_XRDB_MANAGER (manager_object);
}
