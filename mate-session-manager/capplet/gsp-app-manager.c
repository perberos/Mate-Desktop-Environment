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

#include "gsm-util.h"
#include "gsp-app.h"

#include "gsp-app-manager.h"

static GspAppManager *manager = NULL;

typedef struct {
        char         *dir;
        int           index;
        GFileMonitor *monitor;
} GspXdgDir;

struct _GspAppManagerPrivate {
        GSList *apps;
        GSList *dirs;
};

#define GSP_APP_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSP_TYPE_APP_MANAGER, GspAppManagerPrivate))


enum {
        ADDED,
        REMOVED,
        LAST_SIGNAL
};

static guint gsp_app_manager_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GspAppManager, gsp_app_manager, G_TYPE_OBJECT)

static void     gsp_app_manager_dispose      (GObject       *object);
static void     gsp_app_manager_finalize     (GObject       *object);
static void     _gsp_app_manager_app_unref   (GspApp        *app,
                                              GspAppManager *manager);
static void     _gsp_app_manager_app_removed (GspAppManager *manager,
                                              GspApp        *app);

static GspXdgDir *
_gsp_xdg_dir_new (const char *dir,
                  int         index)
{
        GspXdgDir *xdgdir;

        xdgdir = g_slice_new (GspXdgDir);

        xdgdir->dir = g_strdup (dir);
        xdgdir->index = index;
        xdgdir->monitor = NULL;

        return xdgdir;
}

static void
_gsp_xdg_dir_free (GspXdgDir *xdgdir)
{
        if (xdgdir->dir) {
                g_free (xdgdir->dir);
                xdgdir->dir = NULL;
        }

        if (xdgdir->monitor) {
                g_file_monitor_cancel (xdgdir->monitor);
                g_object_unref (xdgdir->monitor);
                xdgdir->monitor = NULL;
        }

        g_slice_free (GspXdgDir, xdgdir);
}

static void
gsp_app_manager_class_init (GspAppManagerClass *class)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (class);

        gobject_class->dispose  = gsp_app_manager_dispose;
        gobject_class->finalize = gsp_app_manager_finalize;

        gsp_app_manager_signals[ADDED] =
                g_signal_new ("added",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GspAppManagerClass,
                                               added),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1, G_TYPE_OBJECT);

        gsp_app_manager_signals[REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GspAppManagerClass,
                                               removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1, G_TYPE_OBJECT);

        g_type_class_add_private (class, sizeof (GspAppManagerPrivate));
}

static void
gsp_app_manager_init (GspAppManager *manager)
{
        manager->priv = GSP_APP_MANAGER_GET_PRIVATE (manager);

        memset (manager->priv, 0, sizeof (GspAppManagerPrivate));
}

static void
gsp_app_manager_dispose (GObject *object)
{
        GspAppManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSP_IS_APP_MANAGER (object));

        manager = GSP_APP_MANAGER (object);

        /* we unref GspApp objects in dispose since they might need to
         * reference us during their dispose/finalize */
        g_slist_foreach (manager->priv->apps,
                         (GFunc) _gsp_app_manager_app_unref, manager);
        g_slist_free (manager->priv->apps);
        manager->priv->apps = NULL;

        G_OBJECT_CLASS (gsp_app_manager_parent_class)->dispose (object);
}

static void
gsp_app_manager_finalize (GObject *object)
{
        GspAppManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSP_IS_APP_MANAGER (object));

        manager = GSP_APP_MANAGER (object);

        g_slist_foreach (manager->priv->dirs,
                         (GFunc) _gsp_xdg_dir_free, NULL);
        g_slist_free (manager->priv->dirs);
        manager->priv->dirs = NULL;

        G_OBJECT_CLASS (gsp_app_manager_parent_class)->finalize (object);

        manager = NULL;
}

static void
_gsp_app_manager_emit_added (GspAppManager *manager,
                             GspApp        *app)
{
        g_signal_emit (G_OBJECT (manager), gsp_app_manager_signals[ADDED],
                       0, app);
}

static void
_gsp_app_manager_emit_removed (GspAppManager *manager,
                               GspApp        *app)
{
        g_signal_emit (G_OBJECT (manager), gsp_app_manager_signals[REMOVED],
                       0, app);
}

/*
 * Directories
 */

static int
gsp_app_manager_get_dir_index (GspAppManager *manager,
                               const char    *dir)
{
        GSList    *l;
        GspXdgDir *xdgdir;

        g_return_val_if_fail (GSP_IS_APP_MANAGER (manager), -1);
        g_return_val_if_fail (dir != NULL, -1);

        for (l = manager->priv->dirs; l != NULL; l = l->next) {
                xdgdir = l->data;
                if (strcmp (dir, xdgdir->dir) == 0) {
                        return xdgdir->index;
                }
        }

        return -1;
}

const char *
gsp_app_manager_get_dir (GspAppManager *manager,
                         unsigned int   index)
{
        GSList    *l;
        GspXdgDir *xdgdir;

        g_return_val_if_fail (GSP_IS_APP_MANAGER (manager), NULL);

        for (l = manager->priv->dirs; l != NULL; l = l->next) {
                xdgdir = l->data;
                if (index == xdgdir->index) {
                        return xdgdir->dir;
                }
        }

        return NULL;
}

static int
_gsp_app_manager_find_dir_with_basename (GspAppManager *manager,
                                         const char    *basename,
                                         int            minimum_index)
{
        GSList    *l;
        GspXdgDir *xdgdir;
        char      *path;
        GKeyFile  *keyfile;
        int        result = -1;

        path = NULL;
        keyfile = g_key_file_new ();

        for (l = manager->priv->dirs; l != NULL; l = l->next) {
                xdgdir = l->data;

                if (xdgdir->index <= minimum_index) {
                        continue;
                }

                g_free (path);
                path = g_build_filename (xdgdir->dir, basename, NULL);
                if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
                        continue;
                }

                if (!g_key_file_load_from_file (keyfile, path,
                                                G_KEY_FILE_NONE, NULL)) {
                        continue;
                }

                /* the file exists and is readable */
                if (result == -1) {
                        result = xdgdir->index;
                } else {
                        result = MIN (result, xdgdir->index);
                }
        }

        g_key_file_free (keyfile);
        g_free (path);

        return result;
}

static void
_gsp_app_manager_handle_delete (GspAppManager *manager,
                                GspApp        *app,
                                const char    *basename,
                                int            index)
{
        unsigned int position;
        unsigned int system_position;

        position = gsp_app_get_xdg_position (app);
        system_position = gsp_app_get_xdg_system_position (app);

        if (system_position < index) {
                /* it got deleted, but we don't even care about it */
                return;
        }

        if (index < position) {
                /* it got deleted, but in a position earlier than the current
                 * one. This happens when the user file was changed and became
                 * identical to the system file; in this case, the user file is
                 * simply removed. */
                 g_assert (index == 0);
                 return;
        }

        if (position == index &&
            (system_position == index || system_position == G_MAXUINT)) {
                /* the file used by the user was deleted, and there's no other
                 * file in system directories. So it really got deleted. */
                _gsp_app_manager_app_removed (manager, app);
                return;
        }

        if (system_position == index) {
                /* then we know that position != index; we just hae to tell
                 * GspApp if there's still a system directory containing this
                 * basename */
                int new_system;

                new_system = _gsp_app_manager_find_dir_with_basename (manager,
                                                                      basename,
                                                                      index);
                if (new_system < 0) {
                        gsp_app_set_xdg_system_position (app, G_MAXUINT);
                } else {
                        gsp_app_set_xdg_system_position (app, new_system);
                }

                return;
        }

        if (position == index) {
                /* then we know that system_position != G_MAXUINT; we need to
                 * tell GspApp to change position to system_position */
                const char *dir;

                dir = gsp_app_manager_get_dir (manager, system_position);
                if (dir) {
                        char *path;

                        path = g_build_filename (dir, basename, NULL);
                        gsp_app_reload_at (app, path,
                                           (unsigned int) system_position);
                        g_free (path);
                } else {
                        _gsp_app_manager_app_removed (manager, app);
                }

                return;
        }

        g_assert_not_reached ();
}

static gboolean
gsp_app_manager_xdg_dir_monitor (GFileMonitor      *monitor,
                                 GFile             *child,
                                 GFile             *other_file,
                                 GFileMonitorEvent  flags,
                                 gpointer           data)
{
        GspAppManager *manager;
        GspApp        *old_app;
        GspApp        *app;
        GFile         *parent;
        char          *basename;
        char          *dir;
        char          *path;
        int            index;

        manager = GSP_APP_MANAGER (data);

        basename = g_file_get_basename (child);
        if (!g_str_has_suffix (basename, ".desktop")) {
                /* not a desktop file, we can ignore */
                g_free (basename);
                return TRUE;
        }
        old_app = gsp_app_manager_find_app_with_basename (manager, basename);

        parent = g_file_get_parent (child);
        dir = g_file_get_path (parent);
        g_object_unref (parent);

        index = gsp_app_manager_get_dir_index (manager, dir);
        if (index < 0) {
                /* not a directory we know; should never happen, though */
                g_free (dir);
                return TRUE;
        }

        path = g_file_get_path (child);

        switch (flags) {
        case G_FILE_MONITOR_EVENT_CHANGED:
        case G_FILE_MONITOR_EVENT_CREATED:
                /* we just do as if it was a new file: GspApp is clever enough
                 * to do the right thing */
                app = gsp_app_new (path, (unsigned int) index);

                /* we didn't have this app before, so add it */
                if (old_app == NULL && app != NULL) {
                        gsp_app_manager_add (manager, app);
                        g_object_unref (app);
                }
                /* else: it was just updated, GspApp took care of
                 * sending the event */
                break;
        case G_FILE_MONITOR_EVENT_DELETED:
                if (!old_app) {
                        /* it got deleted, but we don't know about it, so
                         * nothing to do */
                        break;
                }

                _gsp_app_manager_handle_delete (manager, old_app,
                                                basename, index);
                break;
        default:
                break;
        }

        g_free (path);
        g_free (dir);
        g_free (basename);

        return TRUE;
}

/*
 * Initialization
 */

static void
_gsp_app_manager_fill_from_dir (GspAppManager *manager,
                                GspXdgDir     *xdgdir)
{
        GFile      *file;
        GDir       *dir;
        const char *name;

        file = g_file_new_for_path (xdgdir->dir);
        xdgdir->monitor = g_file_monitor_directory (file, G_FILE_MONITOR_NONE,
                                                    NULL, NULL);
        g_object_unref (file);

        if (xdgdir->monitor) {
                g_signal_connect (xdgdir->monitor, "changed",
                                  G_CALLBACK (gsp_app_manager_xdg_dir_monitor),
                                  manager);
        }

        dir = g_dir_open (xdgdir->dir, 0, NULL);
        if (!dir) {
                return;
        }

        while ((name = g_dir_read_name (dir))) {
                GspApp *app;
                char   *desktop_file_path;

                if (!g_str_has_suffix (name, ".desktop")) {
                        continue;
                }

                desktop_file_path = g_build_filename (xdgdir->dir, name, NULL);
                app = gsp_app_new (desktop_file_path, xdgdir->index);

                if (app != NULL) {
                        gsp_app_manager_add (manager, app);
                        g_object_unref (app);
                }

                g_free (desktop_file_path);
        }

        g_dir_close (dir);
}

void
gsp_app_manager_fill (GspAppManager *manager)
{
        char **autostart_dirs;
        int    i;

        if (manager->priv->apps != NULL)
                return;

        autostart_dirs = gsm_util_get_autostart_dirs ();
        /* we always assume that the first directory is the user one */
        g_assert (g_str_has_prefix (autostart_dirs[0],
                                    g_get_user_config_dir ()));

        for (i = 0; autostart_dirs[i] != NULL; i++) {
                GspXdgDir *xdgdir;

                if (gsp_app_manager_get_dir_index (manager,
                                                   autostart_dirs[i]) >= 0) {
                        continue;
                }

                xdgdir = _gsp_xdg_dir_new (autostart_dirs[i], i);
                manager->priv->dirs = g_slist_prepend (manager->priv->dirs,
                                                       xdgdir);

                _gsp_app_manager_fill_from_dir (manager, xdgdir);
        }

        g_strfreev (autostart_dirs);
}

/*
 * App handling
 */

static void
_gsp_app_manager_app_unref (GspApp        *app,
                            GspAppManager *manager)
{
        g_signal_handlers_disconnect_by_func (app,
                                              _gsp_app_manager_app_removed,
                                              manager);
        g_object_unref (app);
}

static void
_gsp_app_manager_app_removed (GspAppManager *manager,
                              GspApp        *app)
{
        _gsp_app_manager_emit_removed (manager, app);
        manager->priv->apps = g_slist_remove (manager->priv->apps, app);
        _gsp_app_manager_app_unref (app, manager);
}

void
gsp_app_manager_add (GspAppManager *manager,
                     GspApp        *app)
{
        g_return_if_fail (GSP_IS_APP_MANAGER (manager));
        g_return_if_fail (GSP_IS_APP (app));

        manager->priv->apps = g_slist_prepend (manager->priv->apps,
                                               g_object_ref (app));
        g_signal_connect_swapped (app, "removed",
                                  G_CALLBACK (_gsp_app_manager_app_removed),
                                  manager);
        _gsp_app_manager_emit_added (manager, app);
}

GspApp *
gsp_app_manager_find_app_with_basename (GspAppManager *manager,
                                        const char    *basename)
{
        GSList *l;
        GspApp *app;

        g_return_val_if_fail (GSP_IS_APP_MANAGER (manager), NULL);
        g_return_val_if_fail (basename != NULL, NULL);

        for (l = manager->priv->apps; l != NULL; l = l->next) {
                app = GSP_APP (l->data);
                if (strcmp (basename, gsp_app_get_basename (app)) == 0)
                        return app;
        }

        return NULL;
}

/*
 * Singleton
 */

GspAppManager *
gsp_app_manager_get (void)
{
        if (manager == NULL) {
                manager = g_object_new (GSP_TYPE_APP_MANAGER, NULL);
                return manager;
        } else {
                return g_object_ref (manager);
        }
}

GSList *
gsp_app_manager_get_apps (GspAppManager *manager)
{
        g_return_val_if_fail (GSP_IS_APP_MANAGER (manager), NULL);

        return g_slist_copy (manager->priv->apps);
}
