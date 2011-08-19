/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Lennart Poettering <lennart@poettering.net>
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
#include <signal.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>
#include <gtk/gtk.h>

#ifdef HAVE_PULSE
#include <pulse/pulseaudio.h>
#endif

#include "gsd-sound-manager.h"
#include "mate-settings-profile.h"

#define GSD_SOUND_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_SOUND_MANAGER, GsdSoundManagerPrivate))

struct GsdSoundManagerPrivate
{
        guint mateconf_notify;
        GList* monitors;
        guint timeout;
};

#define MATECONF_SOUND_DIR "/desktop/mate/sound"

static void gsd_sound_manager_class_init (GsdSoundManagerClass *klass);
static void gsd_sound_manager_init (GsdSoundManager *sound_manager);
static void gsd_sound_manager_finalize (GObject *object);

G_DEFINE_TYPE (GsdSoundManager, gsd_sound_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

#ifdef HAVE_PULSE

static void
sample_info_cb (pa_context *c, const pa_sample_info *i, int eol, void *userdata)
{
        pa_operation *o;

        if (!i)
                return;

        g_debug ("Found sample %s", i->name);

        /* We only flush those samples which have an XDG sound name
         * attached, because only those originate from themeing  */
        if (!(pa_proplist_gets (i->proplist, PA_PROP_EVENT_ID)))
                return;

        g_debug ("Dropping sample %s from cache", i->name);

        if (!(o = pa_context_remove_sample (c, i->name, NULL, NULL))) {
                g_debug ("pa_context_remove_sample (): %s", pa_strerror (pa_context_errno (c)));
                return;
        }

        pa_operation_unref (o);

        /* We won't wait until the operation is actually executed to
         * speed things up a bit.*/
}

static void
flush_cache (void)
{
        pa_mainloop *ml = NULL;
        pa_context *c = NULL;
        pa_proplist *pl = NULL;
        pa_operation *o = NULL;

        g_debug ("Flushing sample cache");

        if (!(ml = pa_mainloop_new ())) {
                g_debug ("Failed to allocate pa_mainloop");
                goto fail;
        }

        if (!(pl = pa_proplist_new ())) {
                g_debug ("Failed to allocate pa_proplist");
                goto fail;
        }

        pa_proplist_sets (pl, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
        pa_proplist_sets (pl, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
        pa_proplist_sets (pl, PA_PROP_APPLICATION_ID, "org.mate.SettingsDaemon");

        if (!(c = pa_context_new_with_proplist (pa_mainloop_get_api (ml), PACKAGE_NAME, pl))) {
                g_debug ("Failed to allocate pa_context");
                goto fail;
        }

        pa_proplist_free (pl);
        pl = NULL;

        if (pa_context_connect (c, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
                g_debug ("pa_context_connect(): %s", pa_strerror (pa_context_errno (c)));
                goto fail;
        }

        /* Wait until the connection is established */
        while (pa_context_get_state (c) != PA_CONTEXT_READY) {

                if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
                        g_debug ("Connection failed: %s", pa_strerror (pa_context_errno (c)));
                        goto fail;
                }

                if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
                        g_debug ("pa_mainloop_iterate() failed");
                        goto fail;
                }
        }

        /* Enumerate all cached samples */
        if (!(o = pa_context_get_sample_info_list (c, sample_info_cb, NULL))) {
                g_debug ("pa_context_get_sample_info_list(): %s", pa_strerror (pa_context_errno (c)));
                goto fail;
        }

        /* Wait until our operation is finished and there's nothing
         * more queued to send to the server */
        while (pa_operation_get_state (o) == PA_OPERATION_RUNNING || pa_context_is_pending (c)) {

                if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
                        g_debug ("Connection failed: %s", pa_strerror (pa_context_errno (c)));
                        goto fail;
                }

                if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
                        g_debug ("pa_mainloop_iterate() failed");
                        goto fail;
                }
        }

        g_debug ("Sample cache flushed");

fail:
        if (o) {
                pa_operation_cancel (o);
                pa_operation_unref (o);
        }

        if (c) {
                pa_context_disconnect (c);
                pa_context_unref (c);
        }

        if (pl)
                pa_proplist_free (pl);

        if (ml)
                pa_mainloop_free (ml);
}

static gboolean
flush_cb (GsdSoundManager *manager)
{
        flush_cache ();
        manager->priv->timeout = 0;
        return FALSE;
}

static void
trigger_flush (GsdSoundManager *manager)
{

        if (manager->priv->timeout)
                g_source_remove (manager->priv->timeout);

        /* We delay the flushing a bit so that we can coalesce
         * multiple changes into a single cache flush */
        manager->priv->timeout = g_timeout_add (500, (GSourceFunc) flush_cb, manager);
}

static void
mateconf_client_notify_cb (MateConfClient *client,
                        guint cnxn_id,
                        MateConfEntry *entry,
                        GsdSoundManager *manager)
{
        trigger_flush (manager);
}

static gboolean
register_config_callback (GsdSoundManager *manager, GError **error)
{
        MateConfClient *client;
        gboolean succ;

        client = mateconf_client_get_default ();

        mateconf_client_add_dir (client, MATECONF_SOUND_DIR, MATECONF_CLIENT_PRELOAD_NONE, error);
        succ = !error || !*error;

        if (!error) {
                manager->priv->mateconf_notify = mateconf_client_notify_add (client, MATECONF_SOUND_DIR, (MateConfClientNotifyFunc) mateconf_client_notify_cb, manager, NULL, error);
                succ = !error || !*error;
        }

        g_object_unref (client);

        return succ;
}

static void
file_monitor_changed_cb (GFileMonitor *monitor,
                         GFile *file,
                         GFile *other_file,
                         GFileMonitorEvent event,
                         GsdSoundManager *manager)
{
        g_debug ("Theme dir changed");
        trigger_flush (manager);
}

static gboolean
register_directory_callback (GsdSoundManager *manager,
                             const char *path,
                             GError **error)
{
        GFile *f;
        GFileMonitor *m;
        gboolean succ = FALSE;

        g_debug ("Registering directory monitor for %s", path);

        f = g_file_new_for_path (path);

        m = g_file_monitor_directory (f, 0, NULL, error);

        if (m != NULL) {
                g_signal_connect (m, "changed", G_CALLBACK (file_monitor_changed_cb), manager);

                manager->priv->monitors = g_list_prepend (manager->priv->monitors, m);

                succ = TRUE;
        }

        g_object_unref (f);

        return succ;
}

#endif

gboolean
gsd_sound_manager_start (GsdSoundManager *manager,
                         GError **error)
{

#ifdef HAVE_PULSE
        char *p, **ps, **k;
        const char *env, *dd;
#endif

        g_debug ("Starting sound manager");
        mate_settings_profile_start (NULL);

#ifdef HAVE_PULSE

        /* We listen for change of the selected theme ... */
        register_config_callback (manager, NULL);

        /* ... and we listen to changes of the theme base directories
         * in $HOME ...*/

        if ((env = g_getenv ("XDG_DATA_HOME")) && *env == '/')
                p = g_build_filename (env, "sounds", NULL);
        else if (((env = g_getenv ("HOME")) && *env == '/') || (env = g_get_home_dir ()))
                p = g_build_filename (env, ".local", "share", "sounds", NULL);
        else
                p = NULL;

        if (p) {
                register_directory_callback (manager, p, NULL);
                g_free (p);
        }

        /* ... and globally. */
        if (!(dd = g_getenv ("XDG_DATA_DIRS")) || *dd == 0)
                dd = "/usr/local/share:/usr/share";

        ps = g_strsplit (dd, ":", 0);

        for (k = ps; *k; ++k)
                register_directory_callback (manager, *k, NULL);

        g_strfreev (ps);
#endif

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_sound_manager_stop (GsdSoundManager *manager)
{
        g_debug ("Stopping sound manager");

#ifdef HAVE_PULSE
        if (manager->priv->mateconf_notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();

                mateconf_client_remove_dir (client, MATECONF_SOUND_DIR, NULL);

                mateconf_client_notify_remove (client, manager->priv->mateconf_notify);
                manager->priv->mateconf_notify = 0;

                g_object_unref (client);
        }

        if (manager->priv->timeout) {
                g_source_remove (manager->priv->timeout);
                manager->priv->timeout = 0;
        }

        while (manager->priv->monitors) {
                g_file_monitor_cancel (G_FILE_MONITOR (manager->priv->monitors->data));
                g_object_unref (manager->priv->monitors->data);
                manager->priv->monitors = g_list_delete_link (manager->priv->monitors, manager->priv->monitors);
        }
#endif
}

static GObject *
gsd_sound_manager_constructor (
                GType type,
                guint n_construct_properties,
                GObjectConstructParam *construct_properties)
{
        GsdSoundManager *m;
        GsdSoundManagerClass *klass;

        klass = GSD_SOUND_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_SOUND_MANAGER));

        m = GSD_SOUND_MANAGER (G_OBJECT_CLASS (gsd_sound_manager_parent_class)->constructor (
                                                           type,
                                                           n_construct_properties,
                                                           construct_properties));

        return G_OBJECT (m);
}

static void
gsd_sound_manager_dispose (GObject *object)
{
        GsdSoundManager *manager;

        manager = GSD_SOUND_MANAGER (object);

        gsd_sound_manager_stop (manager);

        G_OBJECT_CLASS (gsd_sound_manager_parent_class)->dispose (object);
}

static void
gsd_sound_manager_class_init (GsdSoundManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gsd_sound_manager_constructor;
        object_class->dispose = gsd_sound_manager_dispose;
        object_class->finalize = gsd_sound_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdSoundManagerPrivate));
}

static void
gsd_sound_manager_init (GsdSoundManager *manager)
{
        manager->priv = GSD_SOUND_MANAGER_GET_PRIVATE (manager);
}

static void
gsd_sound_manager_finalize (GObject *object)
{
        GsdSoundManager *sound_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_SOUND_MANAGER (object));

        sound_manager = GSD_SOUND_MANAGER (object);

        g_return_if_fail (sound_manager->priv);

        G_OBJECT_CLASS (gsd_sound_manager_parent_class)->finalize (object);
}

GsdSoundManager *
gsd_sound_manager_new (void)
{
        if (manager_object) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_SOUND_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object, (gpointer *) &manager_object);
        }

        return GSD_SOUND_MANAGER (manager_object);
}
