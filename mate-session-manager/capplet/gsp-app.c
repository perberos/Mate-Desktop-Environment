/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 1999 Free Software Foundation, Inc.
 * Copyright (C) 2007, 2009 Vincent Untz.
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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

#include <string.h>
#include <sys/stat.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "gsm-app-dialog.h"
#include "gsm-properties-dialog.h"
#include "gsm-util.h"
#include "gsp-app-manager.h"
#include "gsp-keyfile.h"

#include "gsp-app.h"

#define GSP_APP_SAVE_DELAY 2

#define GSP_ASP_SAVE_MASK_HIDDEN   0x0001
#define GSP_ASP_SAVE_MASK_ENABLED  0x0002
#define GSP_ASP_SAVE_MASK_NAME     0x0004
#define GSP_ASP_SAVE_MASK_EXEC     0x0008
#define GSP_ASP_SAVE_MASK_COMMENT  0x0010
#define GSP_ASP_SAVE_MASK_ALL      0xffff

struct _GspAppPrivate {
        char         *basename;
        char         *path;

        gboolean      hidden;
        gboolean      enabled;

        char         *name;
        char         *exec;
        char         *comment;
        char         *icon;

        GIcon        *gicon;
        char         *description;

        /* position of the directory in the XDG environment variable */
        unsigned int  xdg_position;
        /* position of the first system directory in the XDG env var containing
         * this autostart app too (G_MAXUINT means none) */
        unsigned int  xdg_system_position;

        unsigned int  save_timeout;
        /* mask of what has changed */
        unsigned int  save_mask;
        /* path that contains the original file that needs to be saved */
        char         *old_system_path;
        /* after writing to file, we skip the next file monitor event of type
         * CHANGED */
        gboolean      skip_next_monitor_event;
};

#define GSP_APP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSP_TYPE_APP, GspAppPrivate))


enum {
        CHANGED,
        REMOVED,
        LAST_SIGNAL
};

static guint gsp_app_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GspApp, gsp_app, G_TYPE_OBJECT)

static void     gsp_app_dispose  (GObject *object);
static void     gsp_app_finalize (GObject *object);
static gboolean _gsp_app_save    (gpointer data);


static gboolean
_gsp_str_equal (const char *a,
                const char *b)
{
        if (g_strcmp0 (a, b) == 0) {
                return TRUE;
        }

        if (a && !b && a[0] == '\0') {
                return TRUE;
        }

        if (b && !a && b[0] == '\0') {
                return TRUE;
        }

        return FALSE;
}


static void
gsp_app_class_init (GspAppClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);

        gobject_class->dispose  = gsp_app_dispose;
        gobject_class->finalize = gsp_app_finalize;

        gsp_app_signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GspAppClass,
                                               changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

        gsp_app_signals[REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GspAppClass,
                                               removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

        g_type_class_add_private (class, sizeof (GspAppPrivate));
}

static void
gsp_app_init (GspApp *app)
{
        app->priv = GSP_APP_GET_PRIVATE (app);

        memset (app->priv, 0, sizeof (GspAppPrivate));
        app->priv->xdg_position        = G_MAXUINT;
        app->priv->xdg_system_position = G_MAXUINT;
}

static void
_gsp_app_free_reusable_data (GspApp *app)
{
        if (app->priv->path) {
                g_free (app->priv->path);
                app->priv->path = NULL;
        }

        if (app->priv->name) {
                g_free (app->priv->name);
                app->priv->name = NULL;
        }

        if (app->priv->exec) {
                g_free (app->priv->exec);
                app->priv->exec = NULL;
        }

        if (app->priv->comment) {
                g_free (app->priv->comment);
                app->priv->comment = NULL;
        }

        if (app->priv->icon) {
                g_free (app->priv->icon);
                app->priv->icon = NULL;
        }

        if (app->priv->gicon) {
                g_object_unref (app->priv->gicon);
                app->priv->gicon = NULL;
        }

        if (app->priv->description) {
                g_free (app->priv->description);
                app->priv->description = NULL;
        }

        if (app->priv->old_system_path) {
                g_free (app->priv->old_system_path);
                app->priv->old_system_path = NULL;
        }
}

static void
gsp_app_dispose (GObject *object)
{
        GspApp *app;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSP_IS_APP (object));

        app = GSP_APP (object);

        /* we save in dispose since we might need to reference GspAppManager */
        if (app->priv->save_timeout) {
                g_source_remove (app->priv->save_timeout);
                app->priv->save_timeout = 0;

                /* save now */
                _gsp_app_save (app);
        }

        G_OBJECT_CLASS (gsp_app_parent_class)->dispose (object);
}

static void
gsp_app_finalize (GObject *object)
{
        GspApp *app;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSP_IS_APP (object));

        app = GSP_APP (object);

        if (app->priv->basename) {
                g_free (app->priv->basename);
                app->priv->basename = NULL;
        }

        _gsp_app_free_reusable_data (app);

        G_OBJECT_CLASS (gsp_app_parent_class)->finalize (object);
}

static void
_gsp_app_emit_changed (GspApp *app)
{
        g_signal_emit (G_OBJECT (app), gsp_app_signals[CHANGED], 0);
}

static void
_gsp_app_emit_removed (GspApp *app)
{
        g_signal_emit (G_OBJECT (app), gsp_app_signals[REMOVED], 0);
}

static void
_gsp_app_update_description (GspApp *app)
{
        const char *primary;
        const char *secondary;

        if (!gsm_util_text_is_blank (app->priv->name)) {
                primary = app->priv->name;
        } else if (!gsm_util_text_is_blank (app->priv->exec)) {
                primary = app->priv->exec;
        } else {
                primary = _("No name");
        }

        if (!gsm_util_text_is_blank (app->priv->comment)) {
                secondary = app->priv->comment;
        } else {
                secondary = _("No description");
        }

        g_free (app->priv->description);
        app->priv->description = g_markup_printf_escaped ("<b>%s</b>\n%s",
                                                          primary,
                                                          secondary);
}

/*
 * Saving
 */

static void
_gsp_ensure_user_autostart_dir (void)
{
        char *dir;

        dir = g_build_filename (g_get_user_config_dir (), "autostart", NULL);
        g_mkdir_with_parents (dir, S_IRWXU);

        g_free (dir);
}

static gboolean
_gsp_app_user_equal_system (GspApp  *app,
                            char   **system_path)
{
        GspAppManager *manager;
        const char    *system_dir;
        char          *path;
        char          *str;
        GKeyFile      *keyfile;

        manager = gsp_app_manager_get ();
        system_dir = gsp_app_manager_get_dir (manager,
                                              app->priv->xdg_system_position);
        g_object_unref (manager);
        if (!system_dir) {
                return FALSE;
        }

        path = g_build_filename (system_dir, app->priv->basename, NULL);

        keyfile = g_key_file_new ();
        if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL)) {
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }

        if (gsp_key_file_get_boolean (keyfile,
                                      G_KEY_FILE_DESKTOP_KEY_HIDDEN,
                                      FALSE) != app->priv->hidden ||
            gsp_key_file_get_boolean (keyfile,
                                      GSP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLED,
                                      TRUE) != app->priv->enabled) {
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }

        str = gsp_key_file_get_locale_string (keyfile,
                                              G_KEY_FILE_DESKTOP_KEY_NAME);
        if (!_gsp_str_equal (str, app->priv->name)) {
                g_free (str);
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }
        g_free (str);

        str = gsp_key_file_get_locale_string (keyfile,
                                              G_KEY_FILE_DESKTOP_KEY_COMMENT);
        if (!_gsp_str_equal (str, app->priv->comment)) {
                g_free (str);
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }
        g_free (str);

        str = gsp_key_file_get_string (keyfile,
                                       G_KEY_FILE_DESKTOP_KEY_EXEC);
        if (!_gsp_str_equal (str, app->priv->exec)) {
                g_free (str);
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }
        g_free (str);

        str = gsp_key_file_get_locale_string (keyfile,
                                              G_KEY_FILE_DESKTOP_KEY_ICON);
        if (!_gsp_str_equal (str, app->priv->icon)) {
                g_free (str);
                g_free (path);
                g_key_file_free (keyfile);
                return FALSE;
        }
        g_free (str);

        g_key_file_free (keyfile);

        *system_path = path;

        return TRUE;
}

static inline void
_gsp_app_save_done_success (GspApp *app)
{
        app->priv->save_mask = 0;

        if (app->priv->old_system_path) {
                g_free (app->priv->old_system_path);
                app->priv->old_system_path = NULL;
        }
}

static gboolean
_gsp_app_save (gpointer data)
{
        GspApp   *app;
        char     *use_path;
        GKeyFile *keyfile;
        GError   *error;

        app = GSP_APP (data);

        /* first check if removing the data from the user dir and using the
         * data from the system dir is enough -- this helps us keep clean the
         * user config dir by removing unneeded files */
        if (_gsp_app_user_equal_system (app, &use_path)) {
                if (g_file_test (app->priv->path, G_FILE_TEST_EXISTS)) {
                        g_remove (app->priv->path);
                }

                g_free (app->priv->path);
                app->priv->path = use_path;

                app->priv->xdg_position = app->priv->xdg_system_position;

                _gsp_app_save_done_success (app);
                return FALSE;
        }

        if (app->priv->old_system_path)
                use_path = app->priv->old_system_path;
        else
                use_path = app->priv->path;

        keyfile = g_key_file_new ();

        error = NULL;
        g_key_file_load_from_file (keyfile, use_path,
                                   G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS,
                                   &error);

        if (error) {
                g_error_free (error);
                gsp_key_file_populate (keyfile);
        }

        if (app->priv->save_mask & GSP_ASP_SAVE_MASK_HIDDEN) {
                gsp_key_file_set_boolean (keyfile,
                                          G_KEY_FILE_DESKTOP_KEY_HIDDEN,
                                          app->priv->hidden);
        }

        if (app->priv->save_mask & GSP_ASP_SAVE_MASK_ENABLED) {
                gsp_key_file_set_boolean (keyfile,
                                          GSP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLED,
                                          app->priv->enabled);
        }

        if (app->priv->save_mask & GSP_ASP_SAVE_MASK_NAME) {
                gsp_key_file_set_locale_string (keyfile,
                                                G_KEY_FILE_DESKTOP_KEY_NAME,
                                                app->priv->name);
                gsp_key_file_ensure_C_key (keyfile, G_KEY_FILE_DESKTOP_KEY_NAME);
        }

        if (app->priv->save_mask & GSP_ASP_SAVE_MASK_COMMENT) {
                gsp_key_file_set_locale_string (keyfile,
                                                G_KEY_FILE_DESKTOP_KEY_COMMENT,
                                                app->priv->comment);
                gsp_key_file_ensure_C_key (keyfile, G_KEY_FILE_DESKTOP_KEY_COMMENT);
        }

        if (app->priv->save_mask & GSP_ASP_SAVE_MASK_EXEC) {
                gsp_key_file_set_string (keyfile,
                                         G_KEY_FILE_DESKTOP_KEY_EXEC,
                                         app->priv->exec);
        }

        _gsp_ensure_user_autostart_dir ();
        if (gsp_key_file_to_file (keyfile, app->priv->path, NULL)) {
                app->priv->skip_next_monitor_event = TRUE;
                _gsp_app_save_done_success (app);
        } else {
                g_warning ("Could not save %s file", app->priv->path);
        }

        g_key_file_free (keyfile);

        app->priv->save_timeout = 0;
        return FALSE;
}

static void
_gsp_app_queue_save (GspApp *app)
{
        if (app->priv->save_timeout) {
                g_source_remove (app->priv->save_timeout);
                app->priv->save_timeout = 0;
        }

        /* if the file was not in the user directory, then we'll create a copy
         * there */
        if (app->priv->xdg_position != 0) {
                app->priv->xdg_position = 0;

                if (app->priv->old_system_path == NULL) {
                        app->priv->old_system_path = app->priv->path;
                        /* if old_system_path was not NULL, then it means we
                         * tried to save and we failed; in that case, we want
                         * to try again and use the old file as a basis again */
                }

                app->priv->path = g_build_filename (g_get_user_config_dir (),
                                                    "autostart",
                                                    app->priv->basename, NULL);
        }

        app->priv->save_timeout = g_timeout_add_seconds (GSP_APP_SAVE_DELAY,
                                                         _gsp_app_save,
                                                         app);
}

/*
 * Accessors
 */

const char *
gsp_app_get_basename (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->basename;
}

const char *
gsp_app_get_path (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->path;
}

gboolean
gsp_app_get_hidden (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), FALSE);

        return app->priv->hidden;
}

gboolean
gsp_app_get_enabled (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), FALSE);

        return app->priv->enabled;
}

void
gsp_app_set_enabled (GspApp   *app,
                     gboolean  enabled)
{
        g_return_if_fail (GSP_IS_APP (app));

        if (enabled == app->priv->enabled) {
                return;
        }

        app->priv->enabled = enabled;
        app->priv->save_mask |= GSP_ASP_SAVE_MASK_ENABLED;

        _gsp_app_queue_save (app);
        _gsp_app_emit_changed (app);
}

const char *
gsp_app_get_name (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->name;
}

const char *
gsp_app_get_exec (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->exec;
}

const char *
gsp_app_get_comment (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->comment;
}

GIcon *
gsp_app_get_icon (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        if (app->priv->gicon) {
                return g_object_ref (app->priv->gicon);
        } else {
                return NULL;
        }
}

unsigned int
gsp_app_get_xdg_position (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), G_MAXUINT);

        return app->priv->xdg_position;
}

unsigned int
gsp_app_get_xdg_system_position (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), G_MAXUINT);

        return app->priv->xdg_system_position;
}

void
gsp_app_set_xdg_system_position (GspApp       *app,
                                 unsigned int  position)
{
        g_return_if_fail (GSP_IS_APP (app));

        app->priv->xdg_system_position = position;
}

const char *
gsp_app_get_description (GspApp *app)
{
        g_return_val_if_fail (GSP_IS_APP (app), NULL);

        return app->priv->description;
}

/*
 * High-level edition
 */

void
gsp_app_update (GspApp     *app,
                const char *name,
                const char *comment,
                const char *exec)
{
        gboolean    changed;

        g_return_if_fail (GSP_IS_APP (app));

        changed = FALSE;

        if (!_gsp_str_equal (name, app->priv->name)) {
                changed = TRUE;
                g_free (app->priv->name);
                app->priv->name = g_strdup (name);
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_NAME;
        }

        if (!_gsp_str_equal (comment, app->priv->comment)) {
                changed = TRUE;
                g_free (app->priv->comment);
                app->priv->comment = g_strdup (comment);
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_COMMENT;
        }

        if (changed) {
                _gsp_app_update_description (app);
        }

        if (!_gsp_str_equal (exec, app->priv->exec)) {
                changed = TRUE;
                g_free (app->priv->exec);
                app->priv->exec = g_strdup (exec);
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_EXEC;
        }

        if (changed) {
                _gsp_app_queue_save (app);
                _gsp_app_emit_changed (app);
        }
}

void
gsp_app_delete (GspApp *app)
{
        g_return_if_fail (GSP_IS_APP (app));

        if (app->priv->xdg_position == 0 &&
            app->priv->xdg_system_position == G_MAXUINT) {
                /* exists in user directory only */
                if (app->priv->save_timeout) {
                        g_source_remove (app->priv->save_timeout);
                        app->priv->save_timeout = 0;
                }

                if (g_file_test (app->priv->path, G_FILE_TEST_EXISTS)) {
                        g_remove (app->priv->path);
                }

                /* for extra safety */
                app->priv->hidden = TRUE;
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_HIDDEN;

                _gsp_app_emit_removed (app);
        } else {
                /* also exists in system directory, so we have to keep a file
                 * in the user directory */
                app->priv->hidden = TRUE;
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_HIDDEN;

                _gsp_app_queue_save (app);
                _gsp_app_emit_changed (app);
        }
}

/*
 * New autostart app
 */

void
gsp_app_reload_at (GspApp       *app,
                   const char   *path,
                   unsigned int  xdg_position)
{
        g_return_if_fail (GSP_IS_APP (app));

        app->priv->xdg_position = G_MAXUINT;
        gsp_app_new (path, xdg_position);
}

GspApp *
gsp_app_new (const char   *path,
             unsigned int  xdg_position)
{
        GspAppManager *manager;
        GspApp        *app;
        GKeyFile      *keyfile;
        char          *basename;
        gboolean       new;

        basename = g_path_get_basename (path);

        manager = gsp_app_manager_get ();
        app = gsp_app_manager_find_app_with_basename (manager, basename);
        g_object_unref (manager);

        new = (app == NULL);

        if (!new) {
                if (app->priv->xdg_position == xdg_position) {
                        if (app->priv->skip_next_monitor_event) {
                                app->priv->skip_next_monitor_event = FALSE;
                                return NULL;
                        }
                        /* else: the file got changed but not by us, we'll
                         * update our data from disk */
                }

                if (app->priv->xdg_position < xdg_position ||
                    app->priv->save_timeout != 0) {
                        /* we don't really care about this file, since we
                         * already have something with a higher priority, or
                         * we're going to write something in the user config
                         * anyway.
                         * Note: xdg_position >= 1 so it's a system dir */
                        app->priv->xdg_system_position = MIN (xdg_position,
                                                              app->priv->xdg_system_position);
                        return NULL;
                }
        }

        keyfile = g_key_file_new ();
        if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL)) {
                g_key_file_free (keyfile);
                g_free (basename);
                return NULL;
        }

        if (new) {
                app = g_object_new (GSP_TYPE_APP, NULL);
                app->priv->basename = basename;
        } else {
                g_free (basename);
                _gsp_app_free_reusable_data (app);
        }

        app->priv->path = g_strdup (path);

        app->priv->hidden = gsp_key_file_get_boolean (keyfile,
                                                      G_KEY_FILE_DESKTOP_KEY_HIDDEN,
                                                      FALSE);
        app->priv->enabled = gsp_key_file_get_boolean (keyfile,
                                                       GSP_KEY_FILE_DESKTOP_KEY_AUTOSTART_ENABLED,
                                                       TRUE);

        app->priv->name = gsp_key_file_get_locale_string (keyfile,
                                                          G_KEY_FILE_DESKTOP_KEY_NAME);
        app->priv->exec = gsp_key_file_get_string (keyfile,
                                                   G_KEY_FILE_DESKTOP_KEY_EXEC);
        app->priv->comment = gsp_key_file_get_locale_string (keyfile,
                                                             G_KEY_FILE_DESKTOP_KEY_COMMENT);

        if (gsm_util_text_is_blank (app->priv->name)) {
                g_free (app->priv->name);
                app->priv->name = g_strdup (app->priv->exec);
        }

        app->priv->icon = gsp_key_file_get_locale_string (keyfile,
                                                          G_KEY_FILE_DESKTOP_KEY_ICON);

        if (app->priv->icon) {
                /* look at icon and see if it's a themed icon or not */
                if (g_path_is_absolute (app->priv->icon)) {
                        GFile *iconfile;

                        iconfile = g_file_new_for_path (app->priv->icon);
                        app->priv->gicon = g_file_icon_new (iconfile);
                        g_object_unref (iconfile);
                } else {
                        app->priv->gicon = g_themed_icon_new (app->priv->icon);
                }
        } else {
                app->priv->gicon = NULL;
        }

        g_key_file_free (keyfile);

        _gsp_app_update_description (app);

        if (xdg_position > 0) {
                g_assert (xdg_position <= app->priv->xdg_system_position);
                app->priv->xdg_system_position = xdg_position;
        }
        /* else we keep the old value (which is G_MAXUINT if it wasn't set) */
        app->priv->xdg_position = xdg_position;

        g_assert (!new || app->priv->save_timeout == 0);
        app->priv->save_timeout = 0;
        app->priv->old_system_path = NULL;
        app->priv->skip_next_monitor_event = FALSE;

        if (!new) {
                _gsp_app_emit_changed (app);
        }

        return app;
}

static char *
_gsp_find_free_basename (const char *suggested_basename)
{
        GspAppManager *manager;
        char          *base_path;
        char          *filename;
        char          *basename;
        int            i;

        if (g_str_has_suffix (suggested_basename, ".desktop")) {
                char *basename_no_ext;

                basename_no_ext = g_strndup (suggested_basename,
                                             strlen (suggested_basename) - strlen (".desktop"));
                base_path = g_build_filename (g_get_user_config_dir (),
                                              "autostart",
                                              basename_no_ext, NULL);
                g_free (basename_no_ext);
        } else {
                base_path = g_build_filename (g_get_user_config_dir (),
                                              "autostart",
                                              suggested_basename, NULL);
        }

        filename = g_strdup_printf ("%s.desktop", base_path);
        basename = g_path_get_basename (filename);

        manager = gsp_app_manager_get ();

        i = 1;
#define _GSP_FIND_MAX_TRY 10000
        while (gsp_app_manager_find_app_with_basename (manager,
                                                       basename) != NULL &&
               g_file_test (filename, G_FILE_TEST_EXISTS) &&
               i < _GSP_FIND_MAX_TRY) {
                g_free (filename);
                g_free (basename);

                filename = g_strdup_printf ("%s-%d.desktop", base_path, i);
                basename = g_path_get_basename (filename);

                i++;
        }

        g_object_unref (manager);

        g_free (base_path);
        g_free (filename);

        if (i == _GSP_FIND_MAX_TRY) {
                g_free (basename);
                return NULL;
        }

        return basename;
}

void
gsp_app_create (const char *name,
                const char *comment,
                const char *exec)
{
        GspAppManager  *manager;
        GspApp         *app;
        char           *basename;
        char          **argv;
        int             argc;

        g_return_if_fail (!gsm_util_text_is_blank (exec));

        if (!g_shell_parse_argv (exec, &argc, &argv, NULL)) {
                return;
        }

        basename = _gsp_find_free_basename (argv[0]);
        g_strfreev (argv);
        if (basename == NULL) {
                return;
        }

        app = g_object_new (GSP_TYPE_APP, NULL);

        app->priv->basename = basename;
        app->priv->path = g_build_filename (g_get_user_config_dir (),
                                            "autostart",
                                            app->priv->basename, NULL);

        app->priv->hidden = FALSE;
        app->priv->enabled = TRUE;

        if (!gsm_util_text_is_blank (name)) {
                app->priv->name = g_strdup (name);
        } else {
                app->priv->name = g_strdup (exec);
        }
        app->priv->exec = g_strdup (exec);
        app->priv->comment = g_strdup (comment);
        app->priv->icon = NULL;

        app->priv->gicon = NULL;
        _gsp_app_update_description (app);

        /* by definition */
        app->priv->xdg_position = 0;
        app->priv->xdg_system_position = G_MAXUINT;

        app->priv->save_timeout = 0;
        app->priv->save_mask |= GSP_ASP_SAVE_MASK_ALL;
        app->priv->old_system_path = NULL;
        app->priv->skip_next_monitor_event = FALSE;

        _gsp_app_queue_save (app);

        manager = gsp_app_manager_get ();
        gsp_app_manager_add (manager, app);
        g_object_unref (app);
        g_object_unref (manager);
}

gboolean
gsp_app_copy_desktop_file (const char *uri)
{
        GspAppManager *manager;
        GspApp        *app;
        GFile         *src_file;
        char          *src_basename;
        char          *dst_basename;
        char          *dst_path;
        GFile         *dst_file;
        gboolean       changed;

        g_return_val_if_fail (uri != NULL, FALSE);

        src_file = g_file_new_for_uri (uri);
        src_basename = g_file_get_basename (src_file);

        if (src_basename == NULL) {
                g_object_unref (src_file);
                return FALSE;
        }

        dst_basename = _gsp_find_free_basename (src_basename);
        g_free (src_basename);

        if (dst_basename == NULL) {
                g_object_unref (src_file);
                return FALSE;
        }

        dst_path = g_build_filename (g_get_user_config_dir (),
                                     "autostart",
                                     dst_basename, NULL);
        g_free (dst_basename);

        dst_file = g_file_new_for_path (dst_path);

        _gsp_ensure_user_autostart_dir ();
        if (!g_file_copy (src_file, dst_file, G_FILE_COPY_NONE,
                          NULL, NULL, NULL, NULL)) {
                g_object_unref (src_file);
                g_object_unref (dst_file);
                g_free (dst_path);
                return FALSE;
        }

        g_object_unref (src_file);
        g_object_unref (dst_file);

        app = gsp_app_new (dst_path, 0);
        if (!app) {
                g_remove (dst_path);
                g_free (dst_path);
                return FALSE;
        }

        g_free (dst_path);

        changed = FALSE;
        if (app->priv->hidden) {
                changed = TRUE;
                app->priv->hidden = FALSE;
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_HIDDEN;
        }

        if (!app->priv->enabled) {
                changed = TRUE;
                app->priv->enabled = TRUE;
                app->priv->save_mask |= GSP_ASP_SAVE_MASK_ENABLED;
        }

        if (changed) {
                _gsp_app_queue_save (app);
        }

        manager = gsp_app_manager_get ();
        gsp_app_manager_add (manager, app);
        g_object_unref (app);
        g_object_unref (manager);

        return TRUE;
}
