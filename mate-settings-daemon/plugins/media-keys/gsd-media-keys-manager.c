/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001-2003 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
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
#include <mateconf/mateconf-client.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mate-settings-profile.h"
#include "gsd-marshal.h"
#include "gsd-media-keys-manager.h"
#include "gsd-media-keys-manager-glue.h"

#include "eggaccelerators.h"
#include "acme.h"
#include "gsd-media-keys-window.h"

#ifdef HAVE_PULSE
#include <canberra-gtk.h>
#include "gvc-mixer-control.h"
#endif /* HAVE_PULSE */

#define GSD_DBUS_PATH "/org/mate/SettingsDaemon"
#define GSD_DBUS_NAME "org.mate.SettingsDaemon"
#define GSD_MEDIA_KEYS_DBUS_PATH GSD_DBUS_PATH "/MediaKeys"
#define GSD_MEDIA_KEYS_DBUS_NAME GSD_DBUS_NAME ".MediaKeys"

#define TOUCHPAD_ENABLED_KEY "/desktop/mate/peripherals/touchpad/touchpad_enabled"

#define VOLUME_STEP 6           /* percents for one volume button press */
#define MAX_VOLUME 65536.0

#define GSD_MEDIA_KEYS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MEDIA_KEYS_MANAGER, GsdMediaKeysManagerPrivate))

typedef struct {
        char   *application;
        guint32 time;
} MediaPlayer;

struct GsdMediaKeysManagerPrivate
{
#ifdef HAVE_PULSE
        /* Volume bits */
        GvcMixerControl *volume;
        GvcMixerStream  *stream;
#endif /* HAVE_PULSE */
        GtkWidget       *dialog;
        MateConfClient     *conf_client;
        GVolumeMonitor  *volume_monitor;

        /* Multihead stuff */
        GdkScreen       *current_screen;
        GSList          *screens;

        GList           *media_players;

        DBusGConnection *connection;
        guint            notify[HANDLED_KEYS];
};

enum {
        MEDIA_PLAYER_KEY_PRESSED,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     gsd_media_keys_manager_class_init  (GsdMediaKeysManagerClass *klass);
static void     gsd_media_keys_manager_init        (GsdMediaKeysManager      *media_keys_manager);
static void     gsd_media_keys_manager_finalize    (GObject                  *object);

G_DEFINE_TYPE (GsdMediaKeysManager, gsd_media_keys_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;


static void
init_screens (GsdMediaKeysManager *manager)
{
        GdkDisplay *display;
        int i;

        display = gdk_display_get_default ();
        for (i = 0; i < gdk_display_get_n_screens (display); i++) {
                GdkScreen *screen;

                screen = gdk_display_get_screen (display, i);
                if (screen == NULL) {
                        continue;
                }
                manager->priv->screens = g_slist_append (manager->priv->screens, screen);
        }

        manager->priv->current_screen = manager->priv->screens->data;
}


static void
acme_error (char * msg)
{
        GtkWidget *error_dialog;

        error_dialog = gtk_message_dialog_new (NULL,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               msg, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
                                         GTK_RESPONSE_OK);
        gtk_widget_show (error_dialog);
        g_signal_connect (error_dialog,
                          "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);
}

static char *
get_term_command (GsdMediaKeysManager *manager)
{
        char *cmd_term;
        char *cmd = NULL;

        cmd_term = mateconf_client_get_string (manager->priv->conf_client,
                                            "/desktop/mate/applications/terminal/exec", NULL);
        if ((cmd_term != NULL) && (strcmp (cmd_term, "") != 0)) {
                char *cmd_args;
                cmd_args = mateconf_client_get_string (manager->priv->conf_client,
                                                    "/desktop/mate/applications/terminal/exec_arg", NULL);
                if ((cmd_args != NULL) && (strcmp (cmd_term, "") != 0)) {
                        cmd = g_strdup_printf ("%s %s -e", cmd_term, cmd_args);
                } else {
                        cmd = g_strdup_printf ("%s -e", cmd_term);
                }

                g_free (cmd_args);
        }

        g_free (cmd_term);

        return cmd;
}

static void
execute (GsdMediaKeysManager *manager,
         char                *cmd,
         gboolean             sync,
         gboolean             need_term)
{
        gboolean retval;
        char   **argv;
        int      argc;
        char    *exec;
        char    *term = NULL;

        retval = FALSE;

        if (need_term) {
                term = get_term_command (manager);
                if (term == NULL) {
                        acme_error (_("Could not get default terminal. Verify that your default "
                                      "terminal command is set and points to a valid application."));
                        return;
                }
        }

        if (term) {
                exec = g_strdup_printf ("%s %s", term, cmd);
                g_free (term);
        } else {
                exec = g_strdup (cmd);
        }

        if (g_shell_parse_argv (exec, &argc, &argv, NULL)) {
                if (sync != FALSE) {
                        retval = g_spawn_sync (g_get_home_dir (),
                                               argv,
                                               NULL,
                                               G_SPAWN_SEARCH_PATH,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL);
                } else {
                        retval = g_spawn_async (g_get_home_dir (),
                                                argv,
                                                NULL,
                                                G_SPAWN_SEARCH_PATH,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL);
                }
                g_strfreev (argv);
        }

        if (retval == FALSE) {
                char *msg;
                msg = g_strdup_printf (_("Couldn't execute command: %s\n"
                                         "Verify that this is a valid command."),
                                       exec);

                acme_error (msg);
                g_free (msg);
        }
        g_free (exec);
}

static void
dialog_init (GsdMediaKeysManager *manager)
{
        if (manager->priv->dialog != NULL
            && !gsd_osd_window_is_valid (GSD_OSD_WINDOW (manager->priv->dialog))) {
                gtk_widget_destroy (manager->priv->dialog);
                manager->priv->dialog = NULL;
        }

        if (manager->priv->dialog == NULL) {
                manager->priv->dialog = gsd_media_keys_window_new ();
        }
}

static gboolean
is_valid_shortcut (const char *string)
{
        if (string == NULL || string[0] == '\0') {
                return FALSE;
        }
        if (strcmp (string, "disabled") == 0) {
                return FALSE;
        }

        return TRUE;
}

static void
update_kbd_cb (MateConfClient         *client,
               guint                id,
               MateConfEntry          *entry,
               GsdMediaKeysManager *manager)
{
        int      i;
        gboolean need_flush = TRUE;

        g_return_if_fail (entry->key != NULL);

        gdk_error_trap_push ();

        /* Find the key that was modified */
        for (i = 0; i < HANDLED_KEYS; i++) {
                if (strcmp (entry->key, keys[i].mateconf_key) == 0) {
                        char *tmp;
                        Key  *key;

                        if (keys[i].key != NULL) {
                                need_flush = TRUE;
                                grab_key_unsafe (keys[i].key, FALSE, manager->priv->screens);
                        }

                        g_free (keys[i].key);
                        keys[i].key = NULL;

                        tmp = mateconf_client_get_string (manager->priv->conf_client,
                                                       keys[i].mateconf_key, NULL);

                        if (is_valid_shortcut (tmp) == FALSE) {
                                g_free (tmp);
                                break;
                        }

                        key = g_new0 (Key, 1);
                        if (!egg_accelerator_parse_virtual (tmp, &key->keysym, &key->keycodes, &key->state)) {
                                g_free (tmp);
                                g_free (key);
                                break;
                        }

                        need_flush = TRUE;
                        grab_key_unsafe (key, TRUE, manager->priv->screens);
                        keys[i].key = key;

                        g_free (tmp);

                        break;
                }
        }

        if (need_flush)
                gdk_flush ();
        if (gdk_error_trap_pop ())
                g_warning ("Grab failed for some keys, another application may already have access the them.");
}

static void
init_kbd (GsdMediaKeysManager *manager)
{
        int i;
        gboolean need_flush = FALSE;

        mate_settings_profile_start (NULL);

        gdk_error_trap_push ();

        for (i = 0; i < HANDLED_KEYS; i++) {
                char *tmp;
                Key  *key;

                manager->priv->notify[i] =
                        mateconf_client_notify_add (manager->priv->conf_client,
                                                 keys[i].mateconf_key,
                                                 (MateConfClientNotifyFunc) update_kbd_cb,
                                                 manager,
                                                 NULL,
                                                 NULL);

                tmp = mateconf_client_get_string (manager->priv->conf_client,
                                               keys[i].mateconf_key,
                                               NULL);

                if (!is_valid_shortcut (tmp)) {
                        g_debug ("Not a valid shortcut: '%s'", tmp);
                        g_free (tmp);
                        continue;
                }

                key = g_new0 (Key, 1);
                if (!egg_accelerator_parse_virtual (tmp, &key->keysym, &key->keycodes, &key->state)) {
                        g_debug ("Unable to parse: '%s'", tmp);
                        g_free (tmp);
                        g_free (key);
                        continue;
                }

                g_free (tmp);

                keys[i].key = key;

                need_flush = TRUE;
                grab_key_unsafe (key, TRUE, manager->priv->screens);
        }

        if (need_flush)
                gdk_flush ();
        if (gdk_error_trap_pop ())
                g_warning ("Grab failed for some keys, another application may already have access the them.");

        mate_settings_profile_end (NULL);
}

static void
dialog_show (GsdMediaKeysManager *manager)
{
        int            orig_w;
        int            orig_h;
        int            screen_w;
        int            screen_h;
        int            x;
        int            y;
        int            pointer_x;
        int            pointer_y;
        GtkRequisition win_req;
        GdkScreen     *pointer_screen;
        GdkRectangle   geometry;
        int            monitor;

        gtk_window_set_screen (GTK_WINDOW (manager->priv->dialog),
                               manager->priv->current_screen);

        /*
         * get the window size
         * if the window hasn't been mapped, it doesn't necessarily
         * know its true size, yet, so we need to jump through hoops
         */
        gtk_window_get_default_size (GTK_WINDOW (manager->priv->dialog), &orig_w, &orig_h);
        gtk_widget_size_request (manager->priv->dialog, &win_req);

        if (win_req.width > orig_w) {
                orig_w = win_req.width;
        }
        if (win_req.height > orig_h) {
                orig_h = win_req.height;
        }

        pointer_screen = NULL;
        gdk_display_get_pointer (gdk_screen_get_display (manager->priv->current_screen),
                                 &pointer_screen,
                                 &pointer_x,
                                 &pointer_y,
                                 NULL);
        if (pointer_screen != manager->priv->current_screen) {
                /* The pointer isn't on the current screen, so just
                 * assume the default monitor
                 */
                monitor = 0;
        } else {
                monitor = gdk_screen_get_monitor_at_point (manager->priv->current_screen,
                                                           pointer_x,
                                                           pointer_y);
        }

        gdk_screen_get_monitor_geometry (manager->priv->current_screen,
                                         monitor,
                                         &geometry);

        screen_w = geometry.width;
        screen_h = geometry.height;

        x = ((screen_w - orig_w) / 2) + geometry.x;
        y = geometry.y + (screen_h / 2) + (screen_h / 2 - orig_h) / 2;

        gtk_window_move (GTK_WINDOW (manager->priv->dialog), x, y);

        gtk_widget_show (manager->priv->dialog);

        gdk_display_sync (gdk_screen_get_display (manager->priv->current_screen));
}

static void
do_unknown_action (GsdMediaKeysManager *manager,
                   const char          *url)
{
        char *string;

        g_return_if_fail (url != NULL);

        string = mateconf_client_get_string (manager->priv->conf_client,
                                          "/desktop/mate/url-handlers/unknown/command",
                                          NULL);

        if ((string != NULL) && (strcmp (string, "") != 0)) {
                char *cmd;
                cmd = g_strdup_printf (string, url);
                execute (manager, cmd, FALSE, FALSE);
                g_free (cmd);
        }
        g_free (string);
}

static void
do_help_action (GsdMediaKeysManager *manager)
{
        char *string;

        string = mateconf_client_get_string (manager->priv->conf_client,
                                          "/desktop/mate/url-handlers/ghelp/command",
                                          NULL);

        if ((string != NULL) && (strcmp (string, "") != 0)) {
                char *cmd;
                cmd = g_strdup_printf (string, "");
                execute (manager, cmd, FALSE, FALSE);
                g_free (cmd);
        } else {
                do_unknown_action (manager, "ghelp:");
        }

        g_free (string);
}

static void
do_mail_action (GsdMediaKeysManager *manager)
{
        char *string;

        string = mateconf_client_get_string (manager->priv->conf_client,
                                          "/desktop/mate/url-handlers/mailto/command",
                                          NULL);

        if ((string != NULL) && (strcmp (string, "") != 0)) {
                char *cmd;
                cmd = g_strdup_printf (string, "");
                execute (manager,
                         cmd,
                         FALSE,
                         mateconf_client_get_bool (manager->priv->conf_client,
                                                "/desktop/mate/url-handlers/mailto/needs_terminal", NULL));
                g_free (cmd);
        }
        g_free (string);
}

static void
do_media_action (GsdMediaKeysManager *manager)
{
        char *command;

        command = mateconf_client_get_string (manager->priv->conf_client,
                                           "/desktop/mate/applications/media/exec", NULL);
        if ((command != NULL) && (strcmp (command, "") != 0)) {
                execute (manager,
                         command,
                         FALSE,
                         mateconf_client_get_bool (manager->priv->conf_client,
                                                "/desktop/mate/applications/media/needs_term", NULL));
        }
        g_free (command);
}

static void
do_www_action (GsdMediaKeysManager *manager,
               const char          *url)
{
        char *string;

        string = mateconf_client_get_string (manager->priv->conf_client,
                                          "/desktop/mate/url-handlers/http/command",
                                          NULL);

        if ((string != NULL) && (strcmp (string, "") != 0)) {
                gchar *cmd;

                if (url == NULL) {
                        cmd = g_strdup_printf (string, "");
                } else {
                        cmd = g_strdup_printf (string, url);
                }

                execute (manager,
                         cmd,
                         FALSE,
                         mateconf_client_get_bool (manager->priv->conf_client,
                                                "/desktop/mate/url-handlers/http/needs_terminal", NULL));
                g_free (cmd);
        } else {
                do_unknown_action (manager, url ? url : "");
        }
        g_free (string);
}

static void
do_exit_action (GsdMediaKeysManager *manager)
{
        execute (manager, "mate-session-save --shutdown-dialog", FALSE, FALSE);
}

static void
do_eject_action_cb (GDrive              *drive,
                    GAsyncResult        *res,
                    GsdMediaKeysManager *manager)
{
        g_drive_eject_with_operation_finish (drive, res, NULL);
}

#define NO_SCORE 0
#define SCORE_CAN_EJECT 50
#define SCORE_HAS_MEDIA 100
static void
do_eject_action (GsdMediaKeysManager *manager)
{
        GList *drives, *l;
        GDrive *fav_drive;
        guint score;

        /* Find the best drive to eject */
        fav_drive = NULL;
        score = NO_SCORE;
        drives = g_volume_monitor_get_connected_drives (manager->priv->volume_monitor);
        for (l = drives; l != NULL; l = l->next) {
                GDrive *drive = l->data;

                if (g_drive_can_eject (drive) == FALSE)
                        continue;
                if (g_drive_is_media_removable (drive) == FALSE)
                        continue;
                if (score < SCORE_CAN_EJECT) {
                        fav_drive = drive;
                        score = SCORE_CAN_EJECT;
                }
                if (g_drive_has_media (drive) == FALSE)
                        continue;
                if (score < SCORE_HAS_MEDIA) {
                        fav_drive = drive;
                        score = SCORE_HAS_MEDIA;
                        break;
                }
        }

        /* Show the dialogue */
        dialog_init (manager);
        gsd_media_keys_window_set_action_custom (GSD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                 "media-eject",
                                                 FALSE);
        dialog_show (manager);

        /* Clean up the drive selection and exit if no suitable
         * drives are found */
        if (fav_drive != NULL)
                fav_drive = g_object_ref (fav_drive);

        g_list_foreach (drives, (GFunc) g_object_unref, NULL);
        if (fav_drive == NULL)
                return;

        /* Eject! */
        g_drive_eject_with_operation (fav_drive, G_MOUNT_UNMOUNT_FORCE,
                                      NULL, NULL,
                                      (GAsyncReadyCallback) do_eject_action_cb,
                                      manager);
        g_object_unref (fav_drive);
}

static void
do_touchpad_action (GsdMediaKeysManager *manager)
{
        MateConfClient *client = manager->priv->conf_client;
        gboolean state = mateconf_client_get_bool (client, TOUCHPAD_ENABLED_KEY, NULL);

        dialog_init (manager);
        gsd_media_keys_window_set_action_custom (GSD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                 (!state) ? "touchpad-enabled" : "touchpad-disabled",
                                                 FALSE);
        dialog_show (manager);

        mateconf_client_set_bool (client, TOUCHPAD_ENABLED_KEY, !state, NULL);
}

#ifdef HAVE_PULSE
static void
update_dialog (GsdMediaKeysManager *manager,
               guint vol,
               gboolean muted,
               gboolean sound_changed)
{
        vol = (int) (100 * (double) vol / PA_VOLUME_NORM);
        vol = CLAMP (vol, 0, 100);

        dialog_init (manager);
        gsd_media_keys_window_set_volume_muted (GSD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                muted);
        gsd_media_keys_window_set_volume_level (GSD_MEDIA_KEYS_WINDOW (manager->priv->dialog), vol);
        gsd_media_keys_window_set_action (GSD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                          GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME);
        dialog_show (manager);

        if (sound_changed != FALSE && muted == FALSE)
                ca_gtk_play_for_widget (manager->priv->dialog, 0,
                                        CA_PROP_EVENT_ID, "audio-volume-change",
                                        CA_PROP_EVENT_DESCRIPTION, "volume changed through key press",
                                        CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                                        NULL);
}

static void
do_sound_action (GsdMediaKeysManager *manager,
                 int                  type)
{
        gboolean muted;
        guint vol, norm_vol_step;
        int vol_step;
        gboolean sound_changed;

        if (manager->priv->stream == NULL)
                return;

        vol_step = mateconf_client_get_int (manager->priv->conf_client,
                                         MATECONF_MISC_DIR "/volume_step",
                                         NULL);

        if (vol_step <= 0 || vol_step > 100)
                vol_step = VOLUME_STEP;

        norm_vol_step = PA_VOLUME_NORM * vol_step / 100;

        /* FIXME: this is racy */
        vol = gvc_mixer_stream_get_volume (manager->priv->stream);
        muted = gvc_mixer_stream_get_is_muted (manager->priv->stream);
        sound_changed = FALSE;

        switch (type) {
        case MUTE_KEY:
                muted = !muted;
                gvc_mixer_stream_change_is_muted (manager->priv->stream, muted);
                sound_changed = TRUE;
                break;
        case VOLUME_DOWN_KEY:
                if (!muted && (vol <= norm_vol_step)) {
                        muted = !muted;
                        vol = 0;
                        gvc_mixer_stream_change_is_muted (manager->priv->stream, muted);
                        if (gvc_mixer_stream_set_volume (manager->priv->stream, vol) != FALSE) {
                                gvc_mixer_stream_push_volume (manager->priv->stream);
                                sound_changed = TRUE;
                        }
                } else if (!muted) {
                        vol = vol - norm_vol_step;
                        if (gvc_mixer_stream_set_volume (manager->priv->stream, vol) != FALSE) {
                                gvc_mixer_stream_push_volume (manager->priv->stream);
                                sound_changed = TRUE;
                        }
                }
                break;
        case VOLUME_UP_KEY:
                if (muted) {
                        muted = !muted;
                        if (vol == 0) {
                               vol = vol + norm_vol_step;
                               gvc_mixer_stream_change_is_muted (manager->priv->stream, muted);
                               if (gvc_mixer_stream_set_volume (manager->priv->stream, vol) != FALSE) {
                                        gvc_mixer_stream_push_volume (manager->priv->stream);
                                        sound_changed = TRUE;
                               }
                        } else {
                                gvc_mixer_stream_change_is_muted (manager->priv->stream, muted);
                                sound_changed = TRUE;
                        }
                } else {
                        if (vol < MAX_VOLUME) {
                                if (vol + norm_vol_step >= MAX_VOLUME) {
                                        vol = MAX_VOLUME;
                                } else {
                                        vol = vol + norm_vol_step;
                                }
                                if (gvc_mixer_stream_set_volume (manager->priv->stream, vol) != FALSE) {
                                        gvc_mixer_stream_push_volume (manager->priv->stream);
                                        sound_changed = TRUE;
                                }
                        }
                }
                break;
        }

        update_dialog (manager, vol, muted, sound_changed);
}

static void
update_default_sink (GsdMediaKeysManager *manager)
{
        GvcMixerStream *stream;

        stream = gvc_mixer_control_get_default_sink (manager->priv->volume);
        if (stream == manager->priv->stream)
                return;

        if (manager->priv->stream != NULL) {
                g_object_unref (manager->priv->stream);
                manager->priv->stream = NULL;
        }

        if (stream != NULL) {
                manager->priv->stream = g_object_ref (stream);
        } else {
                g_warning ("Unable to get default sink");
        }
}

static void
on_control_ready (GvcMixerControl     *control,
                  GsdMediaKeysManager *manager)
{
        update_default_sink (manager);
}

static void
on_control_default_sink_changed (GvcMixerControl     *control,
                                 guint                id,
                                 GsdMediaKeysManager *manager)
{
        update_default_sink (manager);
}

#endif /* HAVE_PULSE */

static gint
find_by_application (gconstpointer a,
                     gconstpointer b)
{
        return strcmp (((MediaPlayer *)a)->application, b);
}

static gint
find_by_time (gconstpointer a,
              gconstpointer b)
{
        return ((MediaPlayer *)a)->time < ((MediaPlayer *)b)->time;
}

/*
 * Register a new media player. Most applications will want to call
 * this with time = GDK_CURRENT_TIME. This way, the last registered
 * player will receive media events. In some cases, applications
 * may want to register with a lower priority (usually 1), to grab
 * events only nobody is interested.
 */
gboolean
gsd_media_keys_manager_grab_media_player_keys (GsdMediaKeysManager *manager,
                                               const char          *application,
                                               guint32              time,
                                               GError             **error)
{
        GList       *iter;
        MediaPlayer *media_player;

        if (time == GDK_CURRENT_TIME) {
                GTimeVal tv;

                g_get_current_time (&tv);
                time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        }

        iter = g_list_find_custom (manager->priv->media_players,
                                   application,
                                   find_by_application);

        if (iter != NULL) {
                if (((MediaPlayer *)iter->data)->time < time) {
                        g_free (((MediaPlayer *)iter->data)->application);
                        g_free (iter->data);
                        manager->priv->media_players = g_list_delete_link (manager->priv->media_players, iter);
                } else {
                        return TRUE;
                }
        }

        g_debug ("Registering %s at %u", application, time);
        media_player = g_new0 (MediaPlayer, 1);
        media_player->application = g_strdup (application);
        media_player->time = time;

        manager->priv->media_players = g_list_insert_sorted (manager->priv->media_players,
                                                             media_player,
                                                             find_by_time);

        return TRUE;
}

gboolean
gsd_media_keys_manager_release_media_player_keys (GsdMediaKeysManager *manager,
                                                  const char          *application,
                                                  GError             **error)
{
        GList *iter;

        iter = g_list_find_custom (manager->priv->media_players,
                                   application,
                                   find_by_application);

        if (iter != NULL) {
                g_debug ("Deregistering %s", application);
                g_free (((MediaPlayer *)iter->data)->application);
                g_free (iter->data);
                manager->priv->media_players = g_list_delete_link (manager->priv->media_players, iter);
        }

        return TRUE;
}

static gboolean
gsd_media_player_key_pressed (GsdMediaKeysManager *manager,
                              const char          *key)
{
        const char *application = NULL;
        gboolean    have_listeners;

        have_listeners = (manager->priv->media_players != NULL);

        if (have_listeners) {
                application = ((MediaPlayer *)manager->priv->media_players->data)->application;
        }

        g_signal_emit (manager, signals[MEDIA_PLAYER_KEY_PRESSED], 0, application, key);

        return !have_listeners;
}

static gboolean
do_multimedia_player_action (GsdMediaKeysManager *manager,
                             const char          *key)
{
        return gsd_media_player_key_pressed (manager, key);
}

static gboolean
do_action (GsdMediaKeysManager *manager,
           int                  type)
{
        char *cmd;
        char *path;

        switch (type) {
        case TOUCHPAD_KEY:
                do_touchpad_action (manager);
                break;
        case MUTE_KEY:
        case VOLUME_DOWN_KEY:
        case VOLUME_UP_KEY:
#ifdef HAVE_PULSE
                do_sound_action (manager, type);
#endif /* HAVE_PULSE */
                break;
        case POWER_KEY:
                do_exit_action (manager);
                break;
        case EJECT_KEY:
                do_eject_action (manager);
                break;
        case HOME_KEY:
                path = g_shell_quote (g_get_home_dir ());
                cmd = g_strconcat ("caja --no-desktop ", path, NULL);
                g_free (path);
                execute (manager, cmd, FALSE, FALSE);
                g_free (cmd);
                break;
        case SEARCH_KEY:
                cmd = NULL;
                if ((cmd = g_find_program_in_path ("beagle-search"))) {
                        execute (manager, "beagle-search", FALSE, FALSE);
                } else if ((cmd = g_find_program_in_path ("tracker-search-tool"))) {
                        execute (manager, "tracker-search-tool", FALSE, FALSE);
                } else {
                        execute (manager, "mate-search-tool", FALSE, FALSE);
                }
                g_free (cmd);
                break;
        case EMAIL_KEY:
                do_mail_action (manager);
                break;
        case SCREENSAVER_KEY:
                if ((cmd = g_find_program_in_path ("mate-screensaver-command"))) {
                        execute (manager, "mate-screensaver-command --lock", FALSE, FALSE);
                } else {
                        execute (manager, "xscreensaver-command -lock", FALSE, FALSE);
                }

                g_free (cmd);
                break;
        case HELP_KEY:
                do_help_action (manager);
                break;
        case WWW_KEY:
                do_www_action (manager, NULL);
                break;
        case MEDIA_KEY:
                do_media_action (manager);
                break;
        case CALCULATOR_KEY:
                execute (manager, "gcalctool", FALSE, FALSE);
                break;
        case PLAY_KEY:
                return do_multimedia_player_action (manager, "Play");
                break;
        case PAUSE_KEY:
                return do_multimedia_player_action (manager, "Pause");
                break;
        case STOP_KEY:
                return do_multimedia_player_action (manager, "Stop");
                break;
        case PREVIOUS_KEY:
                return do_multimedia_player_action (manager, "Previous");
                break;
        case NEXT_KEY:
                return do_multimedia_player_action (manager, "Next");
                break;
        default:
                g_assert_not_reached ();
        }

        return FALSE;
}

static GdkScreen *
acme_get_screen_from_event (GsdMediaKeysManager *manager,
                            XAnyEvent           *xanyev)
{
        GdkWindow *window;
        GdkScreen *screen;
        GSList    *l;

        /* Look for which screen we're receiving events */
        for (l = manager->priv->screens; l != NULL; l = l->next) {
                screen = (GdkScreen *) l->data;
                window = gdk_screen_get_root_window (screen);

                if (GDK_WINDOW_XID (window) == xanyev->window) {
                        return screen;
                }
        }

        return NULL;
}

static GdkFilterReturn
acme_filter_events (GdkXEvent           *xevent,
                    GdkEvent            *event,
                    GsdMediaKeysManager *manager)
{
        XEvent    *xev = (XEvent *) xevent;
        XAnyEvent *xany = (XAnyEvent *) xevent;
        int        i;

        /* verify we have a key event */
        if (xev->type != KeyPress && xev->type != KeyRelease) {
                return GDK_FILTER_CONTINUE;
        }

        for (i = 0; i < HANDLED_KEYS; i++) {
                if (match_key (keys[i].key, xev)) {
                        switch (keys[i].key_type) {
                        case VOLUME_DOWN_KEY:
                        case VOLUME_UP_KEY:
                                /* auto-repeatable keys */
                                if (xev->type != KeyPress) {
                                        return GDK_FILTER_CONTINUE;
                                }
                                break;
                        default:
                                if (xev->type != KeyRelease) {
                                        return GDK_FILTER_CONTINUE;
                                }
                        }

                        manager->priv->current_screen = acme_get_screen_from_event (manager, xany);

                        if (do_action (manager, keys[i].key_type) == FALSE) {
                                return GDK_FILTER_REMOVE;
                        } else {
                                return GDK_FILTER_CONTINUE;
                        }
                }
        }

        return GDK_FILTER_CONTINUE;
}

static gboolean
start_media_keys_idle_cb (GsdMediaKeysManager *manager)
{
        GSList *l;

        g_debug ("Starting media_keys manager");
        mate_settings_profile_start (NULL);
        manager->priv->volume_monitor = g_volume_monitor_get ();
        manager->priv->conf_client = mateconf_client_get_default ();

        mateconf_client_add_dir (manager->priv->conf_client,
                              MATECONF_BINDING_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        init_screens (manager);
        init_kbd (manager);

        /* Start filtering the events */
        for (l = manager->priv->screens; l != NULL; l = l->next) {
                mate_settings_profile_start ("gdk_window_add_filter");

                g_debug ("adding key filter for screen: %d",
                         gdk_screen_get_number (l->data));

                gdk_window_add_filter (gdk_screen_get_root_window (l->data),
                                       (GdkFilterFunc)acme_filter_events,
                                       manager);
                mate_settings_profile_end ("gdk_window_add_filter");
        }

        mate_settings_profile_end (NULL);

        return FALSE;
}

gboolean
gsd_media_keys_manager_start (GsdMediaKeysManager *manager,
                              GError             **error)
{
        mate_settings_profile_start (NULL);

#ifdef HAVE_PULSE
        /* initialise Volume handler
         *
         * We do this one here to force checking gstreamer cache, etc.
         * The rest (grabbing and setting the keys) can happen in an
         * idle.
         */
        mate_settings_profile_start ("gvc_mixer_control_new");

        manager->priv->volume = gvc_mixer_control_new ("MATE Volume Control Media Keys");

        g_signal_connect (manager->priv->volume,
                          "ready",
                          G_CALLBACK (on_control_ready),
                          manager);
        g_signal_connect (manager->priv->volume,
                          "default-sink-changed",
                          G_CALLBACK (on_control_default_sink_changed),
                          manager);

        gvc_mixer_control_open (manager->priv->volume);

        mate_settings_profile_end ("gvc_mixer_control_new");
#endif /* HAVE_PULSE */
        g_idle_add ((GSourceFunc) start_media_keys_idle_cb, manager);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_media_keys_manager_stop (GsdMediaKeysManager *manager)
{
        GsdMediaKeysManagerPrivate *priv = manager->priv;
        GSList *ls;
        GList *l;
        int i;
        gboolean need_flush;

        g_debug ("Stopping media_keys manager");

        for (ls = priv->screens; ls != NULL; ls = ls->next) {
                gdk_window_remove_filter (gdk_screen_get_root_window (ls->data),
                                          (GdkFilterFunc) acme_filter_events,
                                          manager);
        }

        if (priv->conf_client) {
                mateconf_client_remove_dir (priv->conf_client,
                                         MATECONF_BINDING_DIR,
                                         NULL);

                for (i = 0; i < HANDLED_KEYS; ++i) {
                        if (priv->notify[i] != 0) {
                                mateconf_client_notify_remove (priv->conf_client, priv->notify[i]);
                                priv->notify[i] = 0;
                        }
                }

                g_object_unref (priv->conf_client);
                priv->conf_client = NULL;
        }

        if (priv->volume_monitor != NULL) {
                g_object_unref (priv->volume_monitor);
                priv->volume_monitor = NULL;
        }

        if (priv->connection != NULL) {
                dbus_g_connection_unref (priv->connection);
                priv->connection = NULL;
        }

        need_flush = FALSE;
        gdk_error_trap_push ();

        for (i = 0; i < HANDLED_KEYS; ++i) {
                if (keys[i].key) {
                        need_flush = TRUE;
                        grab_key_unsafe (keys[i].key, FALSE, priv->screens);

                        g_free (keys[i].key->keycodes);
                        g_free (keys[i].key);
                        keys[i].key = NULL;
                }
        }

        if (need_flush)
                gdk_flush ();
        gdk_error_trap_pop ();

        g_slist_free (priv->screens);
        priv->screens = NULL;

#ifdef HAVE_PULSE
        if (priv->stream) {
                g_object_unref (priv->stream);
                priv->stream = NULL;
        }

        if (priv->volume) {
                g_object_unref (priv->volume);
                priv->volume = NULL;
        }
#endif /* HAVE_PULSE */

        if (priv->dialog != NULL) {
                gtk_widget_destroy (priv->dialog);
                priv->dialog = NULL;
        }

        for (l = priv->media_players; l; l = l->next) {
                MediaPlayer *mp = l->data;
                g_free (mp->application);
                g_free (mp);
        }
        g_list_free (priv->media_players);
        priv->media_players = NULL;
}

static void
gsd_media_keys_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdMediaKeysManager *self;

        self = GSD_MEDIA_KEYS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_media_keys_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdMediaKeysManager *self;

        self = GSD_MEDIA_KEYS_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_media_keys_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdMediaKeysManager      *media_keys_manager;
        GsdMediaKeysManagerClass *klass;

        klass = GSD_MEDIA_KEYS_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_MEDIA_KEYS_MANAGER));

        media_keys_manager = GSD_MEDIA_KEYS_MANAGER (G_OBJECT_CLASS (gsd_media_keys_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (media_keys_manager);
}

static void
gsd_media_keys_manager_dispose (GObject *object)
{
        GsdMediaKeysManager *media_keys_manager;

        media_keys_manager = GSD_MEDIA_KEYS_MANAGER (object);

        G_OBJECT_CLASS (gsd_media_keys_manager_parent_class)->dispose (object);
}

static void
gsd_media_keys_manager_class_init (GsdMediaKeysManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_media_keys_manager_get_property;
        object_class->set_property = gsd_media_keys_manager_set_property;
        object_class->constructor = gsd_media_keys_manager_constructor;
        object_class->dispose = gsd_media_keys_manager_dispose;
        object_class->finalize = gsd_media_keys_manager_finalize;

       signals[MEDIA_PLAYER_KEY_PRESSED] =
               g_signal_new ("media-player-key-pressed",
                             G_OBJECT_CLASS_TYPE (klass),
                             G_SIGNAL_RUN_LAST,
                             G_STRUCT_OFFSET (GsdMediaKeysManagerClass, media_player_key_pressed),
                             NULL,
                             NULL,
                             gsd_marshal_VOID__STRING_STRING,
                             G_TYPE_NONE,
                             2,
                             G_TYPE_STRING,
                             G_TYPE_STRING);

        dbus_g_object_type_install_info (GSD_TYPE_MEDIA_KEYS_MANAGER, &dbus_glib_gsd_media_keys_manager_object_info);

        g_type_class_add_private (klass, sizeof (GsdMediaKeysManagerPrivate));
}

static void
gsd_media_keys_manager_init (GsdMediaKeysManager *manager)
{
        manager->priv = GSD_MEDIA_KEYS_MANAGER_GET_PRIVATE (manager);

}

static void
gsd_media_keys_manager_finalize (GObject *object)
{
        GsdMediaKeysManager *media_keys_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_MEDIA_KEYS_MANAGER (object));

        media_keys_manager = GSD_MEDIA_KEYS_MANAGER (object);

        g_return_if_fail (media_keys_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_media_keys_manager_parent_class)->finalize (object);
}

static gboolean
register_manager (GsdMediaKeysManager *manager)
{
        GError *error = NULL;

        manager->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->connection == NULL) {
                if (error != NULL) {
                        g_error ("Error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        dbus_g_connection_register_g_object (manager->priv->connection, GSD_MEDIA_KEYS_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

GsdMediaKeysManager *
gsd_media_keys_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                gboolean res;

                manager_object = g_object_new (GSD_TYPE_MEDIA_KEYS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
                res = register_manager (manager_object);
                if (! res) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return GSD_MEDIA_KEYS_MANAGER (manager_object);
}
