/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2007, 2008 Red Hat, Inc
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
#include <sys/stat.h>
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

#define MATE_DESKTOP_USE_UNSTABLE_API

#include <libmateui/mate-rr-config.h>
#include <libmateui/mate-rr.h>
#include <libmateui/mate-rr-labeler.h>

#ifdef HAVE_LIBMATENOTIFY
#include <libmatenotify/notify.h>
#endif

#include "mate-settings-profile.h"
#include "gsd-xrandr-manager.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX   255
#endif

#define GSD_XRANDR_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_XRANDR_MANAGER, GsdXrandrManagerPrivate))

#define CONF_DIR "/apps/mate_settings_daemon/xrandr"
#define CONF_KEY_SHOW_NOTIFICATION_ICON (CONF_DIR "/show_notification_icon")
#define CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP	(CONF_DIR "/turn_on_external_monitors_at_startup")
#define CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP	(CONF_DIR "/turn_on_laptop_monitor_at_startup")
#define CONF_KEY_DEFAULT_CONFIGURATION_FILE             (CONF_DIR "/default_configuration_file")

#define VIDEO_KEYSYM    "XF86Display"
#define ROTATE_KEYSYM   "XF86RotateWindows"

/* Number of seconds that the confirmation dialog will last before it resets the
 * RANDR configuration to its old state.
 */
#define CONFIRMATION_DIALOG_SECONDS 30

/* name of the icon files (gsd-xrandr.svg, etc.) */
#define GSD_XRANDR_ICON_NAME "gsd-xrandr"

/* executable of the control center's display configuration capplet */
#define GSD_XRANDR_DISPLAY_CAPPLET "mate-control-center display"

#define GSD_DBUS_PATH "/org/mate/SettingsDaemon"
#define GSD_DBUS_NAME "org.mate.SettingsDaemon"
#define GSD_XRANDR_DBUS_PATH GSD_DBUS_PATH "/XRANDR"
#define GSD_XRANDR_DBUS_NAME GSD_DBUS_NAME ".XRANDR"

struct GsdXrandrManagerPrivate
{
        DBusGConnection *dbus_connection;

        /* Key code of the XF86Display key (Fn-F7 on Thinkpads, Fn-F4 on HP machines, etc.) */
        guint switch_video_mode_keycode;

        /* Key code of the XF86RotateWindows key (present on some tablets) */
        guint rotate_windows_keycode;

        MateRRScreen *rw_screen;
        gboolean running;

        GtkStatusIcon *status_icon;
        GtkWidget *popup_menu;
        MateRRConfig *configuration;
        MateRRLabeler *labeler;
        MateConfClient *client;
        int notify_id;

        /* fn-F7 status */
        int             current_fn_f7_config;             /* -1 if no configs */
        MateRRConfig **fn_f7_configs;  /* NULL terminated, NULL if there are no configs */

        /* Last time at which we got a "screen got reconfigured" event; see on_randr_event() */
        guint32 last_config_timestamp;
};

static const MateRRRotation possible_rotations[] = {
        MATE_RR_ROTATION_0,
        MATE_RR_ROTATION_90,
        MATE_RR_ROTATION_180,
        MATE_RR_ROTATION_270
        /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
};

static void     gsd_xrandr_manager_class_init  (GsdXrandrManagerClass *klass);
static void     gsd_xrandr_manager_init        (GsdXrandrManager      *xrandr_manager);
static void     gsd_xrandr_manager_finalize    (GObject             *object);

static void error_message (GsdXrandrManager *mgr, const char *primary_text, GError *error_to_display, const char *secondary_text);

static void status_icon_popup_menu (GsdXrandrManager *manager, guint button, guint32 timestamp);
static void run_display_capplet (GtkWidget *widget);
static void get_allowed_rotations_for_output (MateRRConfig *config,
                                              MateRRScreen *rr_screen,
                                              MateOutputInfo *output,
                                              int *out_num_rotations,
                                              MateRRRotation *out_rotations);

G_DEFINE_TYPE (GsdXrandrManager, gsd_xrandr_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static FILE *log_file;

static void
log_open (void)
{
        char *toggle_filename;
        char *log_filename;
        struct stat st;

        if (log_file)
                return;

        toggle_filename = g_build_filename (g_get_home_dir (), "gsd-debug-randr", NULL);
        log_filename = g_build_filename (g_get_home_dir (), "gsd-debug-randr.log", NULL);

        if (stat (toggle_filename, &st) != 0)
                goto out;

        log_file = fopen (log_filename, "a");

        if (log_file && ftell (log_file) == 0)
                fprintf (log_file, "To keep this log from being created, please rm ~/gsd-debug-randr\n");

out:
        g_free (toggle_filename);
        g_free (log_filename);
}

static void
log_close (void)
{
        if (log_file) {
                fclose (log_file);
                log_file = NULL;
        }
}

static void
log_msg (const char *format, ...)
{
        if (log_file) {
                va_list args;

                va_start (args, format);
                vfprintf (log_file, format, args);
                va_end (args);
        }
}

static void
log_output (MateOutputInfo *output)
{
        log_msg ("        %s: ", output->name ? output->name : "unknown");

        if (output->connected) {
                if (output->on) {
                        log_msg ("%dx%d@%d +%d+%d",
                                 output->width,
                                 output->height,
                                 output->rate,
                                 output->x,
                                 output->y);
                } else
                        log_msg ("off");
        } else
                log_msg ("disconnected");

        if (output->display_name)
                log_msg (" (%s)", output->display_name);

        if (output->primary)
                log_msg (" (primary output)");

        log_msg ("\n");
}

static void
log_configuration (MateRRConfig *config)
{
        int i;

        log_msg ("        cloned: %s\n", config->clone ? "yes" : "no");

        for (i = 0; config->outputs[i] != NULL; i++)
                log_output (config->outputs[i]);

        if (i == 0)
                log_msg ("        no outputs!\n");
}

static char
timestamp_relationship (guint32 a, guint32 b)
{
        if (a < b)
                return '<';
        else if (a > b)
                return '>';
        else
                return '=';
}

static void
log_screen (MateRRScreen *screen)
{
        MateRRConfig *config;
        int min_w, min_h, max_w, max_h;
        guint32 change_timestamp, config_timestamp;

        if (!log_file)
                return;

        config = mate_rr_config_new_current (screen);

        mate_rr_screen_get_ranges (screen, &min_w, &max_w, &min_h, &max_h);
        mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

        log_msg ("        Screen min(%d, %d), max(%d, %d), change=%u %c config=%u\n",
                 min_w, min_h,
                 max_w, max_h,
                 change_timestamp,
                 timestamp_relationship (change_timestamp, config_timestamp),
                 config_timestamp);

        log_configuration (config);
        mate_rr_config_free (config);
}

static void
log_configurations (MateRRConfig **configs)
{
        int i;

        if (!configs) {
                log_msg ("    No configurations\n");
                return;
        }

        for (i = 0; configs[i]; i++) {
                log_msg ("    Configuration %d\n", i);
                log_configuration (configs[i]);
        }
}

static void
show_timestamps_dialog (GsdXrandrManager *manager, const char *msg)
{
#if 1
        return;
#else
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *dialog;
        guint32 change_timestamp, config_timestamp;
        static int serial;

        mate_rr_screen_get_timestamps (priv->rw_screen, &change_timestamp, &config_timestamp);

        dialog = gtk_message_dialog_new (NULL,
                                         0,
                                         GTK_MESSAGE_INFO,
                                         GTK_BUTTONS_CLOSE,
                                         "RANDR timestamps (%d):\n%s\nchange: %u\nconfig: %u",
                                         serial++,
                                         msg,
                                         change_timestamp,
                                         config_timestamp);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
#endif
}

/* This function centralizes the use of mate_rr_config_apply_from_filename_with_time().
 *
 * Optionally filters out MATE_RR_ERROR_NO_MATCHING_CONFIG from
 * mate_rr_config_apply_from_filename_with_time(), since that is not usually an error.
 */
static gboolean
apply_configuration_from_filename (GsdXrandrManager *manager,
                                   const char       *filename,
                                   gboolean          no_matching_config_is_an_error,
                                   guint32           timestamp,
                                   GError          **error)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        GError *my_error;
        gboolean success;
        char *str;

        str = g_strdup_printf ("Applying %s with timestamp %d", filename, timestamp);
        show_timestamps_dialog (manager, str);
        g_free (str);

        my_error = NULL;
        success = mate_rr_config_apply_from_filename_with_time (priv->rw_screen, filename, timestamp, &my_error);
        if (success)
                return TRUE;

        if (g_error_matches (my_error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG)) {
                if (no_matching_config_is_an_error)
                        goto fail;

                /* This is not an error; the user probably changed his monitors
                 * and so they don't match any of the stored configurations.
                 */
                g_error_free (my_error);
                return TRUE;
        }

fail:
        g_propagate_error (error, my_error);
        return FALSE;
}

/* This function centralizes the use of mate_rr_config_apply_with_time().
 *
 * Applies a configuration and displays an error message if an error happens.
 * We just return whether setting the configuration succeeded.
 */
static gboolean
apply_configuration_and_display_error (GsdXrandrManager *manager, MateRRConfig *config, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = manager->priv;
        GError *error;
        gboolean success;

        error = NULL;
        success = mate_rr_config_apply_with_time (config, priv->rw_screen, timestamp, &error);
        if (!success) {
                log_msg ("Could not switch to the following configuration (timestamp %u): %s\n", timestamp, error->message);
                log_configuration (config);
                error_message (manager, _("Could not switch the monitor configuration"), error, NULL);
                g_error_free (error);
        }

        return success;
}

static void
restore_backup_configuration_without_messages (const char *backup_filename, const char *intended_filename)
{
        backup_filename = mate_rr_config_get_backup_filename ();
        rename (backup_filename, intended_filename);
}

static void
restore_backup_configuration (GsdXrandrManager *manager, const char *backup_filename, const char *intended_filename, guint32 timestamp)
{
        int saved_errno;

        if (rename (backup_filename, intended_filename) == 0) {
                GError *error;

                error = NULL;
                if (!apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, &error)) {
                        error_message (manager, _("Could not restore the display's configuration"), error, NULL);

                        if (error)
                                g_error_free (error);
                }

                return;
        }

        saved_errno = errno;

        /* ENOENT means the original file didn't exist.  That is *not* an error;
         * the backup was not created because there wasn't even an original
         * monitors.xml (such as on a first-time login).  Note that *here* there
         * is a "didn't work" monitors.xml, so we must delete that one.
         */
        if (saved_errno == ENOENT)
                unlink (intended_filename);
        else {
                char *msg;

                msg = g_strdup_printf ("Could not rename %s to %s: %s",
                                       backup_filename, intended_filename,
                                       g_strerror (saved_errno));
                error_message (manager,
                               _("Could not restore the display's configuration from a backup"),
                               NULL,
                               msg);
                g_free (msg);
        }

        unlink (backup_filename);
}

typedef struct {
        GsdXrandrManager *manager;
        GtkWidget *dialog;

        int countdown;
        int response_id;
} TimeoutDialog;

static void
print_countdown_text (TimeoutDialog *timeout)
{
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (timeout->dialog),
                                                  ngettext ("The display will be reset to its previous configuration in %d second",
                                                            "The display will be reset to its previous configuration in %d seconds",
                                                            timeout->countdown),
                                                  timeout->countdown);
}

static gboolean
timeout_cb (gpointer data)
{
        TimeoutDialog *timeout = data;

        timeout->countdown--;

        if (timeout->countdown == 0) {
                timeout->response_id = GTK_RESPONSE_CANCEL;
                gtk_main_quit ();
        } else {
                print_countdown_text (timeout);
        }

        return TRUE;
}

static void
timeout_response_cb (GtkDialog *dialog, int response_id, gpointer data)
{
        TimeoutDialog *timeout = data;

        if (response_id == GTK_RESPONSE_DELETE_EVENT) {
                /* The user closed the dialog or pressed ESC, revert */
                timeout->response_id = GTK_RESPONSE_CANCEL;
        } else
                timeout->response_id = response_id;

        gtk_main_quit ();
}

static gboolean
user_says_things_are_ok (GsdXrandrManager *manager, GdkWindow *parent_window)
{
        TimeoutDialog timeout;
        guint timeout_id;

        timeout.manager = manager;

        timeout.dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_NONE,
                                                 _("Does the display look OK?"));

        timeout.countdown = CONFIRMATION_DIALOG_SECONDS;

        print_countdown_text (&timeout);

        gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), _("_Restore Previous Configuration"), GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), _("_Keep This Configuration"), GTK_RESPONSE_ACCEPT);
        gtk_dialog_set_default_response (GTK_DIALOG (timeout.dialog), GTK_RESPONSE_ACCEPT); /* ah, the optimism */

        g_signal_connect (timeout.dialog, "response",
                          G_CALLBACK (timeout_response_cb),
                          &timeout);

        gtk_widget_realize (timeout.dialog);

        if (parent_window)
                gdk_window_set_transient_for (gtk_widget_get_window (timeout.dialog), parent_window);

        gtk_widget_show_all (timeout.dialog);
        /* We don't use g_timeout_add_seconds() since we actually care that the user sees "real" second ticks in the dialog */
        timeout_id = g_timeout_add (1000,
                                    timeout_cb,
                                    &timeout);
        gtk_main ();

        gtk_widget_destroy (timeout.dialog);
        g_source_remove (timeout_id);

        if (timeout.response_id == GTK_RESPONSE_ACCEPT)
                return TRUE;
        else
                return FALSE;
}

struct confirmation {
        GsdXrandrManager *manager;
        GdkWindow *parent_window;
        guint32 timestamp;
};

static gboolean
confirm_with_user_idle_cb (gpointer data)
{
        struct confirmation *confirmation = data;
        char *backup_filename;
        char *intended_filename;

        backup_filename = mate_rr_config_get_backup_filename ();
        intended_filename = mate_rr_config_get_intended_filename ();

        if (user_says_things_are_ok (confirmation->manager, confirmation->parent_window))
                unlink (backup_filename);
        else
                restore_backup_configuration (confirmation->manager, backup_filename, intended_filename, confirmation->timestamp);

        g_free (confirmation);

        return FALSE;
}

static void
queue_confirmation_by_user (GsdXrandrManager *manager, GdkWindow *parent_window, guint32 timestamp)
{
        struct confirmation *confirmation;

        confirmation = g_new (struct confirmation, 1);
        confirmation->manager = manager;
        confirmation->parent_window = parent_window;
        confirmation->timestamp = timestamp;

        g_idle_add (confirm_with_user_idle_cb, confirmation);
}

static gboolean
try_to_apply_intended_configuration (GsdXrandrManager *manager, GdkWindow *parent_window, guint32 timestamp, GError **error)
{
        char *backup_filename;
        char *intended_filename;
        gboolean result;

        /* Try to apply the intended configuration */

        backup_filename = mate_rr_config_get_backup_filename ();
        intended_filename = mate_rr_config_get_intended_filename ();

        result = apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, error);
        if (!result) {
                error_message (manager, _("The selected configuration for displays could not be applied"), error ? *error : NULL, NULL);
                restore_backup_configuration_without_messages (backup_filename, intended_filename);
                goto out;
        } else {
                /* We need to return as quickly as possible, so instead of
                 * confirming with the user right here, we do it in an idle
                 * handler.  The caller only expects a status for "could you
                 * change the RANDR configuration?", not "is the user OK with it
                 * as well?".
                 */
                queue_confirmation_by_user (manager, parent_window, timestamp);
        }

out:
        g_free (backup_filename);
        g_free (intended_filename);

        return result;
}

/* DBus method for org.mate.SettingsDaemon.XRANDR ApplyConfiguration; see gsd-xrandr-manager.xml for the interface definition */
static gboolean
gsd_xrandr_manager_apply_configuration (GsdXrandrManager *manager,
                                        GError          **error)
{
        return try_to_apply_intended_configuration (manager, NULL, GDK_CURRENT_TIME, error);
}

/* DBus method for org.mate.SettingsDaemon.XRANDR_2 ApplyConfiguration; see gsd-xrandr-manager.xml for the interface definition */
static gboolean
gsd_xrandr_manager_2_apply_configuration (GsdXrandrManager *manager,
                                          gint64            parent_window_id,
                                          gint64            timestamp,
                                          GError          **error)
{
        GdkWindow *parent_window;
        gboolean result;

        if (parent_window_id != 0)
                parent_window = gdk_window_foreign_new_for_display (gdk_display_get_default (), (GdkNativeWindow) parent_window_id);
        else
                parent_window = NULL;

        result = try_to_apply_intended_configuration (manager, parent_window, (guint32) timestamp, error);

        if (parent_window)
                g_object_unref (parent_window);

        return result;
}

/* We include this after the definition of gsd_xrandr_manager_apply_configuration() so the prototype will already exist */
#include "gsd-xrandr-manager-glue.h"

static gboolean
is_laptop (MateRRScreen *screen, MateOutputInfo *output)
{
        MateRROutput *rr_output;

        rr_output = mate_rr_screen_get_output_by_name (screen, output->name);
        return mate_rr_output_is_laptop (rr_output);
}

static gboolean
get_clone_size (MateRRScreen *screen, int *width, int *height)
{
        MateRRMode **modes = mate_rr_screen_list_clone_modes (screen);
        int best_w, best_h;
        int i;

        best_w = 0;
        best_h = 0;

        for (i = 0; modes[i] != NULL; ++i) {
                MateRRMode *mode = modes[i];
                int w, h;

                w = mate_rr_mode_get_width (mode);
                h = mate_rr_mode_get_height (mode);

                if (w * h > best_w * best_h) {
                        best_w = w;
                        best_h = h;
                }
        }

        if (best_w > 0 && best_h > 0) {
                if (width)
                        *width = best_w;
                if (height)
                        *height = best_h;

                return TRUE;
        }

        return FALSE;
}

static void
print_output (MateOutputInfo *info)
{
        g_print ("  Output: %s attached to %s\n", info->display_name, info->name);
        g_print ("     status: %s\n", info->on ? "on" : "off");
        g_print ("     width: %d\n", info->width);
        g_print ("     height: %d\n", info->height);
        g_print ("     rate: %d\n", info->rate);
        g_print ("     position: %d %d\n", info->x, info->y);
}

static void
print_configuration (MateRRConfig *config, const char *header)
{
        int i;

        g_print ("=== %s Configuration ===\n", header);
        if (!config) {
                g_print ("  none\n");
                return;
        }

        for (i = 0; config->outputs[i] != NULL; ++i)
                print_output (config->outputs[i]);
}

static gboolean
config_is_all_off (MateRRConfig *config)
{
        int j;

        for (j = 0; config->outputs[j] != NULL; ++j) {
                if (config->outputs[j]->on) {
                        return FALSE;
                }
        }

        return TRUE;
}

static MateRRConfig *
make_clone_setup (MateRRScreen *screen)
{
        MateRRConfig *result;
        int width, height;
        int i;

        if (!get_clone_size (screen, &width, &height))
                return NULL;

        result = mate_rr_config_new_current (screen);

        for (i = 0; result->outputs[i] != NULL; ++i) {
                MateOutputInfo *info = result->outputs[i];

                info->on = FALSE;
                if (info->connected) {
                        MateRROutput *output =
                                mate_rr_screen_get_output_by_name (screen, info->name);
                        MateRRMode **modes = mate_rr_output_list_modes (output);
                        int j;
                        int best_rate = 0;

                        for (j = 0; modes[j] != NULL; ++j) {
                                MateRRMode *mode = modes[j];
                                int w, h;

                                w = mate_rr_mode_get_width (mode);
                                h = mate_rr_mode_get_height (mode);

                                if (w == width && h == height) {
                                        int r = mate_rr_mode_get_freq (mode);
                                        if (r > best_rate)
                                                best_rate = r;
                                }
                        }

                        if (best_rate > 0) {
                                info->on = TRUE;
                                info->width = width;
                                info->height = height;
                                info->rate = best_rate;
                                info->rotation = MATE_RR_ROTATION_0;
                                info->x = 0;
                                info->y = 0;
                        }
                }
        }

        if (config_is_all_off (result)) {
                mate_rr_config_free (result);
                result = NULL;
        }

        print_configuration (result, "clone setup");

        return result;
}

static MateRRMode *
find_best_mode (MateRROutput *output)
{
        MateRRMode *preferred;
        MateRRMode **modes;
        int best_size;
        int best_width, best_height, best_rate;
        int i;
        MateRRMode *best_mode;

        preferred = mate_rr_output_get_preferred_mode (output);
        if (preferred)
                return preferred;

        modes = mate_rr_output_list_modes (output);
        if (!modes)
                return NULL;

        best_size = best_width = best_height = best_rate = 0;
        best_mode = NULL;

        for (i = 0; modes[i] != NULL; i++) {
                int w, h, r;
                int size;

                w = mate_rr_mode_get_width (modes[i]);
                h = mate_rr_mode_get_height (modes[i]);
                r = mate_rr_mode_get_freq  (modes[i]);

                size = w * h;

                if (size > best_size) {
                        best_size   = size;
                        best_width  = w;
                        best_height = h;
                        best_rate   = r;
                        best_mode   = modes[i];
                } else if (size == best_size) {
                        if (r > best_rate) {
                                best_rate = r;
                                best_mode = modes[i];
                        }
                }
        }

        return best_mode;
}

static gboolean
turn_on (MateRRScreen *screen,
         MateOutputInfo *info,
         int x, int y)
{
        MateRROutput *output = mate_rr_screen_get_output_by_name (screen, info->name);
        MateRRMode *mode = find_best_mode (output);

        if (mode) {
                info->on = TRUE;
                info->x = x;
                info->y = y;
                info->width = mate_rr_mode_get_width (mode);
                info->height = mate_rr_mode_get_height (mode);
                info->rotation = MATE_RR_ROTATION_0;
                info->rate = mate_rr_mode_get_freq (mode);

                return TRUE;
        }

        return FALSE;
}

static MateRRConfig *
make_laptop_setup (MateRRScreen *screen)
{
        /* Turn on the laptop, disable everything else */
        MateRRConfig *result = mate_rr_config_new_current (screen);
        int i;

        for (i = 0; result->outputs[i] != NULL; ++i) {
                MateOutputInfo *info = result->outputs[i];

                if (is_laptop (screen, info)) {
                        if (!turn_on (screen, info, 0, 0)) {
                                mate_rr_config_free (result);
                                result = NULL;
                                break;
                        }
                }
                else {
                        info->on = FALSE;
                }
        }

        if (config_is_all_off (result)) {
                mate_rr_config_free (result);
                result = NULL;
        }

        print_configuration (result, "Laptop setup");

        /* FIXME - Maybe we should return NULL if there is more than
         * one connected "laptop" screen?
         */
        return result;

}

static int
turn_on_and_get_rightmost_offset (MateRRScreen *screen, MateOutputInfo *info, int x)
{
        if (turn_on (screen, info, x, 0))
                x += info->width;

        return x;
}

static MateRRConfig *
make_xinerama_setup (MateRRScreen *screen)
{
        /* Turn on everything that has a preferred mode, and
         * position it from left to right
         */
        MateRRConfig *result = mate_rr_config_new_current (screen);
        int i;
        int x;

        x = 0;
        for (i = 0; result->outputs[i] != NULL; ++i) {
                MateOutputInfo *info = result->outputs[i];

                if (is_laptop (screen, info))
                        x = turn_on_and_get_rightmost_offset (screen, info, x);
        }

        for (i = 0; result->outputs[i] != NULL; ++i) {
                MateOutputInfo *info = result->outputs[i];

                if (info->connected && !is_laptop (screen, info))
                        x = turn_on_and_get_rightmost_offset (screen, info, x);
        }

        if (config_is_all_off (result)) {
                mate_rr_config_free (result);
                result = NULL;
        }

        print_configuration (result, "xinerama setup");

        return result;
}

static MateRRConfig *
make_other_setup (MateRRScreen *screen)
{
        /* Turn off all laptops, and make all external monitors clone
         * from (0, 0)
         */

        MateRRConfig *result = mate_rr_config_new_current (screen);
        int i;

        for (i = 0; result->outputs[i] != NULL; ++i) {
                MateOutputInfo *info = result->outputs[i];

                if (is_laptop (screen, info)) {
                        info->on = FALSE;
                }
                else {
                        if (info->connected)
                                turn_on (screen, info, 0, 0);
               }
        }

        if (config_is_all_off (result)) {
                mate_rr_config_free (result);
                result = NULL;
        }

        print_configuration (result, "other setup");

        return result;
}

static GPtrArray *
sanitize (GsdXrandrManager *manager, GPtrArray *array)
{
        int i;
        GPtrArray *new;

        g_debug ("before sanitizing");

        for (i = 0; i < array->len; ++i) {
                if (array->pdata[i]) {
                        print_configuration (array->pdata[i], "before");
                }
        }


        /* Remove configurations that are duplicates of
         * configurations earlier in the cycle
         */
        for (i = 0; i < array->len; i++) {
                int j;

                for (j = i + 1; j < array->len; j++) {
                        MateRRConfig *this = array->pdata[j];
                        MateRRConfig *other = array->pdata[i];

                        if (this && other && mate_rr_config_equal (this, other)) {
                                g_debug ("removing duplicate configuration");
                                mate_rr_config_free (this);
                                array->pdata[j] = NULL;
                                break;
                        }
                }
        }

        for (i = 0; i < array->len; ++i) {
                MateRRConfig *config = array->pdata[i];

                if (config && config_is_all_off (config)) {
                        g_debug ("removing configuration as all outputs are off");
                        mate_rr_config_free (array->pdata[i]);
                        array->pdata[i] = NULL;
                }
        }

        /* Do a final sanitization pass.  This will remove configurations that
         * don't fit in the framebuffer's Virtual size.
         */

        for (i = 0; i < array->len; i++) {
                MateRRConfig *config = array->pdata[i];

                if (config) {
                        GError *error;

                        error = NULL;
                        if (!mate_rr_config_applicable (config, manager->priv->rw_screen, &error)) { /* NULL-GError */
                                g_debug ("removing configuration which is not applicable because %s", error->message);
                                g_error_free (error);

                                mate_rr_config_free (config);
                                array->pdata[i] = NULL;
                        }
                }
        }

        /* Remove NULL configurations */
        new = g_ptr_array_new ();

        for (i = 0; i < array->len; ++i) {
                if (array->pdata[i]) {
                        g_ptr_array_add (new, array->pdata[i]);
                        print_configuration (array->pdata[i], "Final");
                }
        }

        if (new->len > 0) {
                g_ptr_array_add (new, NULL);
        } else {
                g_ptr_array_free (new, TRUE);
                new = NULL;
        }

        g_ptr_array_free (array, TRUE);

        return new;
}

static void
generate_fn_f7_configs (GsdXrandrManager *mgr)
{
        GPtrArray *array = g_ptr_array_new ();
        MateRRScreen *screen = mgr->priv->rw_screen;

        g_debug ("Generating configurations");

        /* Free any existing list of configurations */
        if (mgr->priv->fn_f7_configs) {
                int i;

                for (i = 0; mgr->priv->fn_f7_configs[i] != NULL; ++i)
                        mate_rr_config_free (mgr->priv->fn_f7_configs[i]);
                g_free (mgr->priv->fn_f7_configs);

                mgr->priv->fn_f7_configs = NULL;
                mgr->priv->current_fn_f7_config = -1;
        }

        g_ptr_array_add (array, mate_rr_config_new_current (screen));
        g_ptr_array_add (array, make_clone_setup (screen));
        g_ptr_array_add (array, make_xinerama_setup (screen));
        g_ptr_array_add (array, make_laptop_setup (screen));
        g_ptr_array_add (array, make_other_setup (screen));

        array = sanitize (mgr, array);

        if (array) {
                mgr->priv->fn_f7_configs = (MateRRConfig **)g_ptr_array_free (array, FALSE);
                mgr->priv->current_fn_f7_config = 0;
        }
}

static void
error_message (GsdXrandrManager *mgr, const char *primary_text, GError *error_to_display, const char *secondary_text)
{
#ifdef HAVE_LIBMATENOTIFY
        GsdXrandrManagerPrivate *priv = mgr->priv;
        NotifyNotification *notification;

        g_assert (error_to_display == NULL || secondary_text == NULL);

        if (priv->status_icon)
                notification = notify_notification_new_with_status_icon (primary_text,
                                                                         error_to_display ? error_to_display->message : secondary_text,
                                                                         GSD_XRANDR_ICON_NAME,
                                                                         priv->status_icon);
        else
                notification = notify_notification_new (primary_text,
                                                        error_to_display ? error_to_display->message : secondary_text,
                                                        GSD_XRANDR_ICON_NAME,
                                                        NULL);

        notify_notification_show (notification, NULL); /* NULL-GError */
#else
        GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                         "%s", primary_text);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
                                                  error_to_display ? error_to_display->message : secondary_text);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
#endif /* HAVE_LIBMATENOTIFY */
}

static void
handle_fn_f7 (GsdXrandrManager *mgr, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = mgr->priv;
        MateRRScreen *screen = priv->rw_screen;
        MateRRConfig *current;
        GError *error;

        /* Theory of fn-F7 operation
         *
         * We maintain a datastructure "fn_f7_status", that contains
         * a list of MateRRConfig's. Each of the MateRRConfigs has a
         * mode (or "off") for each connected output.
         *
         * When the user hits fn-F7, we cycle to the next MateRRConfig
         * in the data structure. If the data structure does not exist, it
         * is generated. If the configs in the data structure do not match
         * the current hardware reality, it is regenerated.
         *
         */
        g_debug ("Handling fn-f7");

        log_open ();
        log_msg ("Handling XF86Display hotkey - timestamp %u\n", timestamp);

        error = NULL;
        if (!mate_rr_screen_refresh (screen, &error) && error) {
                char *str;

                str = g_strdup_printf (_("Could not refresh the screen information: %s"), error->message);
                g_error_free (error);

                log_msg ("%s\n", str);
                error_message (mgr, str, NULL, _("Trying to switch the monitor configuration anyway."));
                g_free (str);
        }

        if (!priv->fn_f7_configs) {
                log_msg ("Generating stock configurations:\n");
                generate_fn_f7_configs (mgr);
                log_configurations (priv->fn_f7_configs);
        }

        current = mate_rr_config_new_current (screen);

        if (priv->fn_f7_configs &&
            (!mate_rr_config_match (current, priv->fn_f7_configs[0]) ||
             !mate_rr_config_equal (current, priv->fn_f7_configs[mgr->priv->current_fn_f7_config]))) {
                    /* Our view of the world is incorrect, so regenerate the
                     * configurations
                     */
                    generate_fn_f7_configs (mgr);
                    log_msg ("Regenerated stock configurations:\n");
                    log_configurations (priv->fn_f7_configs);
            }

        mate_rr_config_free (current);

        if (priv->fn_f7_configs) {
                guint32 server_timestamp;
                gboolean success;

                mgr->priv->current_fn_f7_config++;

                if (priv->fn_f7_configs[mgr->priv->current_fn_f7_config] == NULL)
                        mgr->priv->current_fn_f7_config = 0;

                g_debug ("cycling to next configuration (%d)", mgr->priv->current_fn_f7_config);

                print_configuration (priv->fn_f7_configs[mgr->priv->current_fn_f7_config], "new config");

                g_debug ("applying");

                /* See https://bugzilla.gnome.org/show_bug.cgi?id=610482
                 *
                 * Sometimes we'll get two rapid XF86Display keypress events,
                 * but their timestamps will be out of order with respect to the
                 * RANDR timestamps.  This *may* be due to stupid BIOSes sending
                 * out display-switch keystrokes "to make Windows work".
                 *
                 * The X server will error out if the timestamp provided is
                 * older than a previous change configuration timestamp. We
                 * assume here that we do want this event to go through still,
                 * since kernel timestamps may be skewed wrt the X server.
                 */
                mate_rr_screen_get_timestamps (screen, NULL, &server_timestamp);
                if (timestamp < server_timestamp)
                        timestamp = server_timestamp;

                success = apply_configuration_and_display_error (mgr, priv->fn_f7_configs[mgr->priv->current_fn_f7_config], timestamp);

                if (success) {
                        log_msg ("Successfully switched to configuration (timestamp %u):\n", timestamp);
                        log_configuration (priv->fn_f7_configs[mgr->priv->current_fn_f7_config]);
                }
        }
        else {
                g_debug ("no configurations generated");
        }

        log_close ();

        g_debug ("done handling fn-f7");
}

static MateOutputInfo *
get_laptop_output_info (MateRRScreen *screen, MateRRConfig *config)
{
        int i;

        for (i = 0; config->outputs[i] != NULL; i++) {
                MateOutputInfo *info;

                info = config->outputs[i];
                if (is_laptop (screen, info))
                        return info;
        }

        return NULL;

}

static MateRRRotation
get_next_rotation (MateRRRotation allowed_rotations, MateRRRotation current_rotation)
{
        int i;
        int current_index;

        /* First, find the index of the current rotation */

        current_index = -1;

        for (i = 0; i < G_N_ELEMENTS (possible_rotations); i++) {
                MateRRRotation r;

                r = possible_rotations[i];
                if (r == current_rotation) {
                        current_index = i;
                        break;
                }
        }

        if (current_index == -1) {
                /* Huh, the current_rotation was not one of the supported rotations.  Bail out. */
                return current_rotation;
        }

        /* Then, find the next rotation that is allowed */

        i = (current_index + 1) % G_N_ELEMENTS (possible_rotations);

        while (1) {
                MateRRRotation r;

                r = possible_rotations[i];
                if (r == current_rotation) {
                        /* We wrapped around and no other rotation is suported.  Bummer. */
                        return current_rotation;
                } else if (r & allowed_rotations)
                        return r;

                i = (i + 1) % G_N_ELEMENTS (possible_rotations);
        }
}

/* We use this when the XF86RotateWindows key is pressed.  That key is present
 * on some tablet PCs; they use it so that the user can rotate the tablet
 * easily.
 */
static void
handle_rotate_windows (GsdXrandrManager *mgr, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = mgr->priv;
        MateRRScreen *screen = priv->rw_screen;
        MateRRConfig *current;
        MateOutputInfo *rotatable_output_info;
        int num_allowed_rotations;
        MateRRRotation allowed_rotations;
        MateRRRotation next_rotation;

        g_debug ("Handling XF86RotateWindows");

        /* Which output? */

        current = mate_rr_config_new_current (screen);

        rotatable_output_info = get_laptop_output_info (screen, current);
        if (rotatable_output_info == NULL) {
                g_debug ("No laptop outputs found to rotate; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Which rotation? */

        get_allowed_rotations_for_output (current, priv->rw_screen, rotatable_output_info, &num_allowed_rotations, &allowed_rotations);
        next_rotation = get_next_rotation (allowed_rotations, rotatable_output_info->rotation);

        if (next_rotation == rotatable_output_info->rotation) {
                g_debug ("No rotations are supported other than the current one; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Rotate */

        rotatable_output_info->rotation = next_rotation;

        apply_configuration_and_display_error (mgr, current, timestamp);

out:
        mate_rr_config_free (current);
}

static GdkFilterReturn
event_filter (GdkXEvent           *xevent,
              GdkEvent            *event,
              gpointer             data)
{
        GsdXrandrManager *manager = data;
        XEvent *xev = (XEvent *) xevent;

        if (!manager->priv->running)
                return GDK_FILTER_CONTINUE;

        /* verify we have a key event */
        if (xev->xany.type != KeyPress && xev->xany.type != KeyRelease)
                return GDK_FILTER_CONTINUE;

        if (xev->xany.type == KeyPress) {
                if (xev->xkey.keycode == manager->priv->switch_video_mode_keycode)
                        handle_fn_f7 (manager, xev->xkey.time);
                else if (xev->xkey.keycode == manager->priv->rotate_windows_keycode)
                        handle_rotate_windows (manager, xev->xkey.time);

                return GDK_FILTER_CONTINUE;
        }

        return GDK_FILTER_CONTINUE;
}

static void
refresh_tray_icon_menu_if_active (GsdXrandrManager *manager, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = manager->priv;

        if (priv->popup_menu) {
                gtk_menu_shell_cancel (GTK_MENU_SHELL (priv->popup_menu)); /* status_icon_popup_menu_selection_done_cb() will free everything */
                status_icon_popup_menu (manager, 0, timestamp);
        }
}

static void
auto_configure_outputs (GsdXrandrManager *manager, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = manager->priv;
        MateRRConfig *config;
        int i;
        GList *just_turned_on;
        GList *l;
        int x;
        GError *error;
        gboolean applicable;

        config = mate_rr_config_new_current (priv->rw_screen);

        /* For outputs that are connected and on (i.e. they have a CRTC assigned
         * to them, so they are getting a signal), we leave them as they are
         * with their current modes.
         *
         * For other outputs, we will turn on connected-but-off outputs and turn
         * off disconnected-but-on outputs.
         *
         * FIXME: If an output remained connected+on, it would be nice to ensure
         * that the output's CRTCs still has a reasonable mode (think of
         * changing one monitor for another with different capabilities).
         */

        just_turned_on = NULL;

        for (i = 0; config->outputs[i] != NULL; i++) {
                MateOutputInfo *output = config->outputs[i];

                if (output->connected && !output->on) {
                        output->on = TRUE;
                        output->rotation = MATE_RR_ROTATION_0;
                        just_turned_on = g_list_prepend (just_turned_on, GINT_TO_POINTER (i));
                } else if (!output->connected && output->on)
                        output->on = FALSE;
        }

        /* Now, lay out the outputs from left to right.  Put first the outputs
         * which remained on; put last the outputs that were newly turned on.
         */

        x = 0;

        /* First, outputs that remained on */

        for (i = 0; config->outputs[i] != NULL; i++) {
                MateOutputInfo *output = config->outputs[i];

                if (g_list_find (just_turned_on, GINT_TO_POINTER (i)))
                        continue;

                if (output->on) {
                        g_assert (output->connected);

                        output->x = x;
                        output->y = 0;

                        x += output->width;
                }
        }

        /* Second, outputs that were newly-turned on */

        for (l = just_turned_on; l; l = l->next) {
                MateOutputInfo *output;

                i = GPOINTER_TO_INT (l->data);
                output = config->outputs[i];

                g_assert (output->on && output->connected);

                output->x = x;
                output->y = 0;

                /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
                output->width = output->pref_width;
                output->height = output->pref_height;

                x += output->width;
        }

        /* Check if we have a large enough framebuffer size.  If not, turn off
         * outputs from right to left until we reach a usable size.
         */

        just_turned_on = g_list_reverse (just_turned_on); /* now the outputs here are from right to left */

        l = just_turned_on;
        while (1) {
                MateOutputInfo *output;
                gboolean is_bounds_error;

                error = NULL;
                applicable = mate_rr_config_applicable (config, priv->rw_screen, &error);

                if (applicable)
                        break;

                is_bounds_error = g_error_matches (error, MATE_RR_ERROR, MATE_RR_ERROR_BOUNDS_ERROR);
                g_error_free (error);

                if (!is_bounds_error)
                        break;

                if (l) {
                        i = GPOINTER_TO_INT (l->data);
                        l = l->next;

                        output = config->outputs[i];
                        output->on = FALSE;
                } else
                        break;
        }

        /* Apply the configuration! */

        if (applicable)
                apply_configuration_and_display_error (manager, config, timestamp);

        g_list_free (just_turned_on);
        mate_rr_config_free (config);

        /* Finally, even though we did a best-effort job in sanitizing the
         * outputs, we don't know the physical layout of the monitors.  We'll
         * start the display capplet so that the user can tweak things to his
         * liking.
         */

#if 0
        /* FIXME: This is disabled for now.  The capplet is not a single-instance application.
         * If you do this:
         *
         *   1. Start the display capplet
         *
         *   2. Plug an extra monitor
         *
         *   3. Hit the "Detect displays" button
         *
         * Then we will get a RANDR event because X re-probes the outputs.  We don't want to
         * start up a second display capplet right there!
         */

        run_display_capplet (NULL);
#endif
}

static void
apply_color_profiles (void)
{
        gboolean ret;
        GError *error = NULL;

        /* run the mate-color-manager apply program */
        ret = g_spawn_command_line_async (BINDIR "/gcm-apply", &error);
        if (!ret) {
                /* only print the warning if the binary is installed */
                if (error->code != G_SPAWN_ERROR_NOENT) {
                        g_warning ("failed to apply color profiles: %s", error->message);
                }
                g_error_free (error);
        }
}

static void
on_randr_event (MateRRScreen *screen, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);
        GsdXrandrManagerPrivate *priv = manager->priv;
        guint32 change_timestamp, config_timestamp;

        if (!priv->running)
                return;

        mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

        log_open ();
        log_msg ("Got RANDR event with timestamps change=%u %c config=%u\n",
                 change_timestamp,
                 timestamp_relationship (change_timestamp, config_timestamp),
                 config_timestamp);

        if (change_timestamp >= config_timestamp) {
                /* The event is due to an explicit configuration change.
                 *
                 * If the change was performed by us, then we need to do nothing.
                 *
                 * If the change was done by some other X client, we don't need
                 * to do anything, either; the screen is already configured.
                 */
                show_timestamps_dialog (manager, "ignoring since change > config");
                log_msg ("  Ignoring event since change >= config\n");
        } else {
                /* Here, config_timestamp > change_timestamp.  This means that
                 * the screen got reconfigured because of hotplug/unplug; the X
                 * server is just notifying us, and we need to configure the
                 * outputs in a sane way.
                 */

                char *intended_filename;
                GError *error;
                gboolean success;

                show_timestamps_dialog (manager, "need to deal with reconfiguration, as config > change");

                intended_filename = mate_rr_config_get_intended_filename ();

                error = NULL;
                success = apply_configuration_from_filename (manager, intended_filename, TRUE, config_timestamp, &error);
                g_free (intended_filename);

                if (!success) {
                        /* We don't bother checking the error type.
                         *
                         * Both G_FILE_ERROR_NOENT and
                         * MATE_RR_ERROR_NO_MATCHING_CONFIG would mean, "there
                         * was no configuration to apply, or none that matched
                         * the current outputs", and in that case we need to run
                         * our fallback.
                         *
                         * Any other error means "we couldn't do the smart thing
                         * of using a previously- saved configuration, anyway,
                         * for some other reason.  In that case, we also need to
                         * run our fallback to avoid leaving the user with a
                         * bogus configuration.
                         */

                        if (error)
                                g_error_free (error);

                        if (config_timestamp != priv->last_config_timestamp) {
                                priv->last_config_timestamp = config_timestamp;
                                auto_configure_outputs (manager, config_timestamp);
                                log_msg ("  Automatically configured outputs to deal with event\n");
                        } else
                                log_msg ("  Ignored event as old and new config timestamps are the same\n");
                } else
                        log_msg ("Applied stored configuration to deal with event\n");
        }

        /* poke mate-color-manager */
        apply_color_profiles ();

        refresh_tray_icon_menu_if_active (manager, MAX (change_timestamp, config_timestamp));

        log_close ();
}

static void
run_display_capplet (GtkWidget *widget)
{
        GdkScreen *screen;
        GError *error;

        if (widget)
                screen = gtk_widget_get_screen (widget);
        else
                screen = gdk_screen_get_default ();

        error = NULL;
        if (!gdk_spawn_command_line_on_screen (screen, GSD_XRANDR_DISPLAY_CAPPLET, &error)) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                             "<span weight=\"bold\" size=\"larger\">"
                                                             "Display configuration could not be run"
                                                             "</span>\n\n"
                                                             "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_error_free (error);
        }
}

static void
popup_menu_configure_display_cb (GtkMenuItem *item, gpointer data)
{
        run_display_capplet (GTK_WIDGET (item));
}

static void
status_icon_popup_menu_selection_done_cb (GtkMenuShell *menu_shell, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);
        struct GsdXrandrManagerPrivate *priv = manager->priv;

        gtk_widget_destroy (priv->popup_menu);
        priv->popup_menu = NULL;

        mate_rr_labeler_hide (priv->labeler);
        g_object_unref (priv->labeler);
        priv->labeler = NULL;

        mate_rr_config_free (priv->configuration);
        priv->configuration = NULL;
}

#define OUTPUT_TITLE_ITEM_BORDER 2
#define OUTPUT_TITLE_ITEM_PADDING 4

/* This is an expose-event hander for the title label for each MateRROutput.
 * We want each title to have a colored background, so we paint that background, then
 * return FALSE to let GtkLabel expose itself (i.e. paint the label's text), and then
 * we have a signal_connect_after handler as well.  See the comments below
 * to see why that "after" handler is needed.
 */
static gboolean
output_title_label_expose_event_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        MateOutputInfo *output;
        GdkColor color;
        cairo_t *cr;
        GtkAllocation allocation;

        g_assert (GTK_IS_LABEL (widget));

        output = g_object_get_data (G_OBJECT (widget), "output");
        g_assert (output != NULL);

        g_assert (priv->labeler != NULL);

        /* Draw a black rectangular border, filled with the color that corresponds to this output */

        mate_rr_labeler_get_color_for_output (priv->labeler, output, &color);

        cr = gdk_cairo_create (gtk_widget_get_window (widget));

        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_set_line_width (cr, OUTPUT_TITLE_ITEM_BORDER);
        gtk_widget_get_allocation (widget, &allocation);
        cairo_rectangle (cr,
                         allocation.x + OUTPUT_TITLE_ITEM_BORDER / 2.0,
                         allocation.y + OUTPUT_TITLE_ITEM_BORDER / 2.0,
                         allocation.width - OUTPUT_TITLE_ITEM_BORDER,
                         allocation.height - OUTPUT_TITLE_ITEM_BORDER);
        cairo_stroke (cr);

        gdk_cairo_set_source_color (cr, &color);
        cairo_rectangle (cr,
                         allocation.x + OUTPUT_TITLE_ITEM_BORDER,
                         allocation.y + OUTPUT_TITLE_ITEM_BORDER,
                         allocation.width - 2 * OUTPUT_TITLE_ITEM_BORDER,
                         allocation.height - 2 * OUTPUT_TITLE_ITEM_BORDER);

        cairo_fill (cr);

        /* We want the label to always show up as if it were sensitive
         * ("style->fg[GTK_STATE_NORMAL]"), even though the label is insensitive
         * due to being inside an insensitive menu item.  So, here we have a
         * HACK in which we frob the label's state directly.  GtkLabel's expose
         * handler will be run after this function, so it will think that the
         * label is in GTK_STATE_NORMAL.  We reset the label's state back to
         * insensitive in output_title_label_after_expose_event_cb().
         *
         * Yay for fucking with GTK+'s internals.
         */

        gtk_widget_set_state (widget, GTK_STATE_NORMAL);

        return FALSE;
}

/* See the comment in output_title_event_box_expose_event_cb() about this funny label widget */
static gboolean
output_title_label_after_expose_event_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
        g_assert (GTK_IS_LABEL (widget));
        gtk_widget_set_state (widget, GTK_STATE_INSENSITIVE);

        return FALSE;
}

static void
title_item_size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
        /* When GtkMenu does size_request on its items, it asks them for their "toggle size",
         * which will be non-zero when there are check/radio items.  GtkMenu remembers
         * the largest of those sizes.  During the size_allocate pass, GtkMenu calls
         * gtk_menu_item_toggle_size_allocate() with that value, to tell the menu item
         * that it should later paint its child a bit to the right of its edge.
         *
         * However, we want the "title" menu items for each RANDR output to span the *whole*
         * allocation of the menu item, not just the "allocation minus toggle" area.
         *
         * So, we let the menu item size_allocate itself as usual, but this
         * callback gets run afterward.  Here we hack a toggle size of 0 into
         * the menu item, and size_allocate it by hand *again*.  We also need to
         * avoid recursing into this function.
         */

        g_assert (GTK_IS_MENU_ITEM (widget));

        gtk_menu_item_toggle_size_allocate (GTK_MENU_ITEM (widget), 0);

        g_signal_handlers_block_by_func (widget, title_item_size_allocate_cb, NULL);

        /* Sigh. There is no way to turn on GTK_ALLOC_NEEDED outside of GTK+
         * itself; also, since calling size_allocate on a widget with the same
         * allcation is a no-op, we need to allocate with a "different" size
         * first.
         */

        allocation->width++;
        gtk_widget_size_allocate (widget, allocation);

        allocation->width--;
        gtk_widget_size_allocate (widget, allocation);

        g_signal_handlers_unblock_by_func (widget, title_item_size_allocate_cb, NULL);
}

static GtkWidget *
make_menu_item_for_output_title (GsdXrandrManager *manager, MateOutputInfo *output)
{
        GtkWidget *item;
        GtkWidget *label;
        char *str;
	GdkColor black = { 0, 0, 0, 0 };

        item = gtk_menu_item_new ();

        g_signal_connect (item, "size-allocate",
                          G_CALLBACK (title_item_size_allocate_cb), NULL);

        str = g_markup_printf_escaped ("<b>%s</b>", output->display_name);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), str);
        g_free (str);

	/* Make the label explicitly black.  We don't want it to follow the
	 * theme's colors, since the label is always shown against a light
	 * pastel background.  See bgo#556050
	 */
	gtk_widget_modify_fg (label, gtk_widget_get_state (label), &black);

        /* Add padding around the label to fit the box that we'll draw for color-coding */
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_misc_set_padding (GTK_MISC (label),
                              OUTPUT_TITLE_ITEM_BORDER + OUTPUT_TITLE_ITEM_PADDING,
                              OUTPUT_TITLE_ITEM_BORDER + OUTPUT_TITLE_ITEM_PADDING);

        gtk_container_add (GTK_CONTAINER (item), label);

        /* We want to paint a colored box as the background of the label, so we connect
         * to its expose-event signal.  See the comment in *** to see why need to connect
         * to the label both 'before' and 'after'.
         */
        g_signal_connect (label, "expose-event",
                          G_CALLBACK (output_title_label_expose_event_cb), manager);
        g_signal_connect_after (label, "expose-event",
                                G_CALLBACK (output_title_label_after_expose_event_cb), manager);

        g_object_set_data (G_OBJECT (label), "output", output);

        gtk_widget_set_sensitive (item, FALSE); /* the title is not selectable */
        gtk_widget_show_all (item);

        return item;
}

static void
get_allowed_rotations_for_output (MateRRConfig *config,
                                  MateRRScreen *rr_screen,
                                  MateOutputInfo *output,
                                  int *out_num_rotations,
                                  MateRRRotation *out_rotations)
{
        MateRRRotation current_rotation;
        int i;

        *out_num_rotations = 0;
        *out_rotations = 0;

        current_rotation = output->rotation;

        /* Yay for brute force */

        for (i = 0; i < G_N_ELEMENTS (possible_rotations); i++) {
                MateRRRotation rotation_to_test;

                rotation_to_test = possible_rotations[i];

                output->rotation = rotation_to_test;

                if (mate_rr_config_applicable (config, rr_screen, NULL)) { /* NULL-GError */
                        (*out_num_rotations)++;
                        (*out_rotations) |= rotation_to_test;
                }
        }

        output->rotation = current_rotation;

        if (*out_num_rotations == 0 || *out_rotations == 0) {
                g_warning ("Huh, output %p says it doesn't support any rotations, and yet it has a current rotation?", output);
                *out_num_rotations = 1;
                *out_rotations = output->rotation;
        }
}

static void
add_unsupported_rotation_item (GsdXrandrManager *manager)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;
        GtkWidget *label;
        gchar *markup;

        item = gtk_menu_item_new ();

        label = gtk_label_new (NULL);
        markup = g_strdup_printf ("<i>%s</i>", _("Rotation not supported"));
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_container_add (GTK_CONTAINER (item), label);

        gtk_widget_show_all (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);
}

static void
ensure_current_configuration_is_saved (void)
{
        MateRRScreen *rr_screen;
        MateRRConfig *rr_config;

        /* Normally, mate_rr_config_save() creates a backup file based on the
         * old monitors.xml.  However, if *that* file didn't exist, there is
         * nothing from which to create a backup.  So, here we'll save the
         * current/unchanged configuration and then let our caller call
         * mate_rr_config_save() again with the new/changed configuration, so
         * that there *will* be a backup file in the end.
         */

        rr_screen = mate_rr_screen_new (gdk_screen_get_default (), NULL, NULL, NULL); /* NULL-GError */
        if (!rr_screen)
                return;

        rr_config = mate_rr_config_new_current (rr_screen);
        mate_rr_config_save (rr_config, NULL); /* NULL-GError */

        mate_rr_config_free (rr_config);
        mate_rr_screen_destroy (rr_screen);
}

static void
output_rotation_item_activate_cb (GtkCheckMenuItem *item, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        MateOutputInfo *output;
        MateRRRotation rotation;
        GError *error;

	/* Not interested in deselected items */
	if (!gtk_check_menu_item_get_active (item))
		return;

        ensure_current_configuration_is_saved ();

        output = g_object_get_data (G_OBJECT (item), "output");
        rotation = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "rotation"));

        output->rotation = rotation;

        error = NULL;
        if (!mate_rr_config_save (priv->configuration, &error)) {
                error_message (manager, _("Could not save monitor configuration"), error, NULL);
                if (error)
                        g_error_free (error);

                return;
        }

        try_to_apply_intended_configuration (manager, NULL, gtk_get_current_event_time (), NULL); /* NULL-GError */
}

static void
add_items_for_rotations (GsdXrandrManager *manager, MateOutputInfo *output, MateRRRotation allowed_rotations)
{
        typedef struct {
                MateRRRotation	rotation;
                const char *	name;
        } RotationInfo;
        static const RotationInfo rotations[] = {
                { MATE_RR_ROTATION_0, N_("Normal") },
                { MATE_RR_ROTATION_90, N_("Left") },
                { MATE_RR_ROTATION_270, N_("Right") },
                { MATE_RR_ROTATION_180, N_("Upside Down") },
                /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
        };

        struct GsdXrandrManagerPrivate *priv = manager->priv;
        int i;
        GSList *group;
        GtkWidget *active_item;
        gulong active_item_activate_id;

        group = NULL;
        active_item = NULL;
        active_item_activate_id = 0;

        for (i = 0; i < G_N_ELEMENTS (rotations); i++) {
                MateRRRotation rot;
                GtkWidget *item;
                gulong activate_id;

                rot = rotations[i].rotation;

                if ((allowed_rotations & rot) == 0) {
                        /* don't display items for rotations which are
                         * unavailable.  Their availability is not under the
                         * user's control, anyway.
                         */
                        continue;
                }

                item = gtk_radio_menu_item_new_with_label (group, _(rotations[i].name));
                gtk_widget_show_all (item);
                gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

                g_object_set_data (G_OBJECT (item), "output", output);
                g_object_set_data (G_OBJECT (item), "rotation", GINT_TO_POINTER (rot));

                activate_id = g_signal_connect (item, "activate",
                                                G_CALLBACK (output_rotation_item_activate_cb), manager);

                group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

                if (rot == output->rotation) {
                        active_item = item;
                        active_item_activate_id = activate_id;
                }
        }

        if (active_item) {
                /* Block the signal temporarily so our callback won't be called;
                 * we are just setting up the UI.
                 */
                g_signal_handler_block (active_item, active_item_activate_id);

                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (active_item), TRUE);

                g_signal_handler_unblock (active_item, active_item_activate_id);
        }

}

static void
add_rotation_items_for_output (GsdXrandrManager *manager, MateOutputInfo *output)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        int num_rotations;
        MateRRRotation rotations;

        get_allowed_rotations_for_output (priv->configuration, priv->rw_screen, output, &num_rotations, &rotations);

        if (num_rotations == 1)
                add_unsupported_rotation_item (manager);
        else
                add_items_for_rotations (manager, output, rotations);
}

static void
add_menu_items_for_output (GsdXrandrManager *manager, MateOutputInfo *output)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;

        item = make_menu_item_for_output_title (manager, output);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        add_rotation_items_for_output (manager, output);
}

static void
add_menu_items_for_outputs (GsdXrandrManager *manager)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        int i;

        for (i = 0; priv->configuration->outputs[i] != NULL; i++) {
                if (priv->configuration->outputs[i]->connected)
                        add_menu_items_for_output (manager, priv->configuration->outputs[i]);
        }
}

static void
status_icon_popup_menu (GsdXrandrManager *manager, guint button, guint32 timestamp)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;

        g_assert (priv->configuration == NULL);
        priv->configuration = mate_rr_config_new_current (priv->rw_screen);

        g_assert (priv->labeler == NULL);
        priv->labeler = mate_rr_labeler_new (priv->configuration);

        g_assert (priv->popup_menu == NULL);
        priv->popup_menu = gtk_menu_new ();

        add_menu_items_for_outputs (manager);

        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        item = gtk_menu_item_new_with_mnemonic (_("_Configure Display Settings…"));
        g_signal_connect (item, "activate",
                          G_CALLBACK (popup_menu_configure_display_cb), manager);
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        g_signal_connect (priv->popup_menu, "selection-done",
                          G_CALLBACK (status_icon_popup_menu_selection_done_cb), manager);

        gtk_menu_popup (GTK_MENU (priv->popup_menu), NULL, NULL,
                        gtk_status_icon_position_menu,
                        priv->status_icon, button, timestamp);
}

static void
status_icon_activate_cb (GtkStatusIcon *status_icon, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);

        /* Suck; we don't get a proper button/timestamp */
        status_icon_popup_menu (manager, 0, gtk_get_current_event_time ());
}

static void
status_icon_popup_menu_cb (GtkStatusIcon *status_icon, guint button, guint32 timestamp, gpointer data)
{
        GsdXrandrManager *manager = GSD_XRANDR_MANAGER (data);

        status_icon_popup_menu (manager, button, timestamp);
}

static void
status_icon_start (GsdXrandrManager *manager)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;

        /* Ideally, we should detect if we are on a tablet and only display
         * the icon in that case.
         */
        if (!priv->status_icon) {
                priv->status_icon = gtk_status_icon_new_from_icon_name (GSD_XRANDR_ICON_NAME);
                gtk_status_icon_set_tooltip_text (priv->status_icon, _("Configure display settings"));

                g_signal_connect (priv->status_icon, "activate",
                                  G_CALLBACK (status_icon_activate_cb), manager);
                g_signal_connect (priv->status_icon, "popup-menu",
                                  G_CALLBACK (status_icon_popup_menu_cb), manager);
        }
}

static void
status_icon_stop (GsdXrandrManager *manager)
{
        struct GsdXrandrManagerPrivate *priv = manager->priv;

        if (priv->status_icon) {
                g_signal_handlers_disconnect_by_func (
                        priv->status_icon, G_CALLBACK (status_icon_activate_cb), manager);
                g_signal_handlers_disconnect_by_func (
                        priv->status_icon, G_CALLBACK (status_icon_popup_menu_cb), manager);

                /* hide the icon before unreffing it; otherwise we will leak
                   whitespace in the notification area due to a bug in there */
                gtk_status_icon_set_visible (priv->status_icon, FALSE);
                g_object_unref (priv->status_icon);
                priv->status_icon = NULL;
        }
}

static void
start_or_stop_icon (GsdXrandrManager *manager)
{
        if (mateconf_client_get_bool (manager->priv->client, CONF_KEY_SHOW_NOTIFICATION_ICON, NULL)) {
                status_icon_start (manager);
        }
        else {
                status_icon_stop (manager);
        }
}

static void
on_config_changed (MateConfClient          *client,
                   guint                 cnxn_id,
                   MateConfEntry           *entry,
                   GsdXrandrManager *manager)
{
        if (strcmp (entry->key, CONF_KEY_SHOW_NOTIFICATION_ICON) == 0)
                start_or_stop_icon (manager);
}

static gboolean
apply_intended_configuration (GsdXrandrManager *manager, const char *intended_filename, guint32 timestamp)
{
        GError *my_error;
	gboolean result;

        my_error = NULL;
        result = apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, &my_error);
        if (!result) {
                if (my_error) {
                        if (!g_error_matches (my_error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
                                error_message (manager, _("Could not apply the stored configuration for monitors"), my_error, NULL);

                        g_error_free (my_error);
                }
        }

        return result;
}

static void
apply_default_boot_configuration (GsdXrandrManager *mgr, guint32 timestamp)
{
	GsdXrandrManagerPrivate *priv = mgr->priv;
	MateRRScreen *screen = priv->rw_screen;
   	MateRRConfig *config;
   	gboolean turn_on_external, turn_on_laptop;

        turn_on_external =
                mateconf_client_get_bool (mgr->priv->client, CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP, NULL);
        turn_on_laptop =
                mateconf_client_get_bool (mgr->priv->client, CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP, NULL);

	if (turn_on_external && turn_on_laptop)
		config = make_clone_setup (screen);
	else if (!turn_on_external && turn_on_laptop)
		config = make_laptop_setup (screen);
	else if (turn_on_external && !turn_on_laptop)
		config = make_other_setup (screen);
	else
		config = make_laptop_setup (screen);

        if (config) {
                apply_configuration_and_display_error (mgr, config, timestamp);
                mate_rr_config_free (config);
        }
}

static gboolean
apply_stored_configuration_at_startup (GsdXrandrManager *manager, guint32 timestamp)
{
        GError *my_error;
        gboolean success;
        char *backup_filename;
        char *intended_filename;

        backup_filename = mate_rr_config_get_backup_filename ();
        intended_filename = mate_rr_config_get_intended_filename ();

        /* 1. See if there was a "saved" configuration.  If there is one, it means
         * that the user had selected to change the display configuration, but the
         * machine crashed.  In that case, we'll apply *that* configuration and save it on top of the
         * "intended" one.
         */

        my_error = NULL;

        success = apply_configuration_from_filename (manager, backup_filename, FALSE, timestamp, &my_error);
        if (success) {
                /* The backup configuration existed, and could be applied
                 * successfully, so we must restore it on top of the
                 * failed/intended one.
                 */
                restore_backup_configuration (manager, backup_filename, intended_filename, timestamp);
                goto out;
        }

        if (!g_error_matches (my_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
                /* Epic fail:  there (probably) was a backup configuration, but
                 * we could not apply it.  The only thing we can do is delete
                 * the backup configuration.  Let's hope that the user doesn't
                 * get left with an unusable display...
                 */

                unlink (backup_filename);
                goto out;
        }

        /* 2. There was no backup configuration!  This means we are
         * good.  Apply the intended configuration instead.
         */

        success = apply_intended_configuration (manager, intended_filename, timestamp);

out:

        if (my_error)
                g_error_free (my_error);

        g_free (backup_filename);
        g_free (intended_filename);

        return success;
}

static gboolean
apply_default_configuration_from_file (GsdXrandrManager *manager, guint32 timestamp)
{
        GsdXrandrManagerPrivate *priv = manager->priv;
        char *default_config_filename;
        gboolean result;

        default_config_filename = mateconf_client_get_string (priv->client, CONF_KEY_DEFAULT_CONFIGURATION_FILE, NULL);
        if (!default_config_filename)
                return FALSE;

        result = apply_configuration_from_filename (manager, default_config_filename, TRUE, timestamp, NULL);

        g_free (default_config_filename);
        return result;
}

gboolean
gsd_xrandr_manager_start (GsdXrandrManager *manager,
                          GError          **error)
{
        g_debug ("Starting xrandr manager");
        mate_settings_profile_start (NULL);

        log_open ();
        log_msg ("------------------------------------------------------------\nSTARTING XRANDR PLUGIN\n");

        manager->priv->rw_screen = mate_rr_screen_new (
                gdk_screen_get_default (), on_randr_event, manager, error);

        if (manager->priv->rw_screen == NULL) {
                log_msg ("Could not initialize the RANDR plugin%s%s\n",
                         (error && *error) ? ": " : "",
                         (error && *error) ? (*error)->message : "");
                log_close ();
                return FALSE;
        }

        log_msg ("State of screen at startup:\n");
        log_screen (manager->priv->rw_screen);

        manager->priv->running = TRUE;
        manager->priv->client = mateconf_client_get_default ();

        g_assert (manager->priv->notify_id == 0);

        mateconf_client_add_dir (manager->priv->client, CONF_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);

        manager->priv->notify_id =
                mateconf_client_notify_add (
                        manager->priv->client, CONF_DIR,
                        (MateConfClientNotifyFunc)on_config_changed,
                        manager, NULL, NULL);

        if (manager->priv->switch_video_mode_keycode) {
                gdk_error_trap_push ();

                XGrabKey (gdk_x11_get_default_xdisplay(),
                          manager->priv->switch_video_mode_keycode, AnyModifier,
                          gdk_x11_get_default_root_xwindow(),
                          True, GrabModeAsync, GrabModeAsync);

                gdk_flush ();
                gdk_error_trap_pop ();
        }

        if (manager->priv->rotate_windows_keycode) {
                gdk_error_trap_push ();

                XGrabKey (gdk_x11_get_default_xdisplay(),
                          manager->priv->rotate_windows_keycode, AnyModifier,
                          gdk_x11_get_default_root_xwindow(),
                          True, GrabModeAsync, GrabModeAsync);

                gdk_flush ();
                gdk_error_trap_pop ();
        }

        show_timestamps_dialog (manager, "Startup");
        if (!apply_stored_configuration_at_startup (manager, GDK_CURRENT_TIME)) /* we don't have a real timestamp at startup anyway */
                if (!apply_default_configuration_from_file (manager, GDK_CURRENT_TIME))
                        apply_default_boot_configuration (manager, GDK_CURRENT_TIME);

        log_msg ("State of screen after initial configuration:\n");
        log_screen (manager->priv->rw_screen);

        gdk_window_add_filter (gdk_get_default_root_window(),
                               (GdkFilterFunc)event_filter,
                               manager);

        start_or_stop_icon (manager);

        log_close ();

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_xrandr_manager_stop (GsdXrandrManager *manager)
{
        g_debug ("Stopping xrandr manager");

        manager->priv->running = FALSE;

        if (manager->priv->switch_video_mode_keycode) {
                gdk_error_trap_push ();

                XUngrabKey (gdk_x11_get_default_xdisplay(),
                            manager->priv->switch_video_mode_keycode, AnyModifier,
                            gdk_x11_get_default_root_xwindow());

                gdk_error_trap_pop ();
        }

        if (manager->priv->rotate_windows_keycode) {
                gdk_error_trap_push ();

                XUngrabKey (gdk_x11_get_default_xdisplay(),
                            manager->priv->rotate_windows_keycode, AnyModifier,
                            gdk_x11_get_default_root_xwindow());

                gdk_error_trap_pop ();
        }

        gdk_window_remove_filter (gdk_get_default_root_window (),
                                  (GdkFilterFunc) event_filter,
                                  manager);

        if (manager->priv->notify_id != 0) {
                mateconf_client_remove_dir (manager->priv->client,
                                         CONF_DIR, NULL);
                mateconf_client_notify_remove (manager->priv->client,
                                            manager->priv->notify_id);
                manager->priv->notify_id = 0;
        }

        if (manager->priv->client != NULL) {
                g_object_unref (manager->priv->client);
                manager->priv->client = NULL;
        }

        if (manager->priv->rw_screen != NULL) {
                mate_rr_screen_destroy (manager->priv->rw_screen);
                manager->priv->rw_screen = NULL;
        }

        if (manager->priv->dbus_connection != NULL) {
                dbus_g_connection_unref (manager->priv->dbus_connection);
                manager->priv->dbus_connection = NULL;
        }

        status_icon_stop (manager);

        log_open ();
        log_msg ("STOPPING XRANDR PLUGIN\n------------------------------------------------------------\n");
        log_close ();
}

static void
gsd_xrandr_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdXrandrManager *self;

        self = GSD_XRANDR_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_xrandr_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdXrandrManager *self;

        self = GSD_XRANDR_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_xrandr_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdXrandrManager      *xrandr_manager;
        GsdXrandrManagerClass *klass;

        klass = GSD_XRANDR_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_XRANDR_MANAGER));

        xrandr_manager = GSD_XRANDR_MANAGER (G_OBJECT_CLASS (gsd_xrandr_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (xrandr_manager);
}

static void
gsd_xrandr_manager_dispose (GObject *object)
{
        GsdXrandrManager *xrandr_manager;

        xrandr_manager = GSD_XRANDR_MANAGER (object);

        G_OBJECT_CLASS (gsd_xrandr_manager_parent_class)->dispose (object);
}

static void
gsd_xrandr_manager_class_init (GsdXrandrManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_xrandr_manager_get_property;
        object_class->set_property = gsd_xrandr_manager_set_property;
        object_class->constructor = gsd_xrandr_manager_constructor;
        object_class->dispose = gsd_xrandr_manager_dispose;
        object_class->finalize = gsd_xrandr_manager_finalize;

        dbus_g_object_type_install_info (GSD_TYPE_XRANDR_MANAGER, &dbus_glib_gsd_xrandr_manager_object_info);

        g_type_class_add_private (klass, sizeof (GsdXrandrManagerPrivate));
}

static guint
get_keycode_for_keysym_name (const char *name)
{
        Display *dpy;
        guint keyval;

        dpy = gdk_x11_get_default_xdisplay ();

        keyval = gdk_keyval_from_name (name);
        return XKeysymToKeycode (dpy, keyval);
}

static void
gsd_xrandr_manager_init (GsdXrandrManager *manager)
{
        manager->priv = GSD_XRANDR_MANAGER_GET_PRIVATE (manager);

        manager->priv->switch_video_mode_keycode = get_keycode_for_keysym_name (VIDEO_KEYSYM);
        manager->priv->rotate_windows_keycode = get_keycode_for_keysym_name (ROTATE_KEYSYM);

        manager->priv->current_fn_f7_config = -1;
        manager->priv->fn_f7_configs = NULL;
}

static void
gsd_xrandr_manager_finalize (GObject *object)
{
        GsdXrandrManager *xrandr_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_XRANDR_MANAGER (object));

        xrandr_manager = GSD_XRANDR_MANAGER (object);

        g_return_if_fail (xrandr_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_xrandr_manager_parent_class)->finalize (object);
}

static gboolean
register_manager_dbus (GsdXrandrManager *manager)
{
        GError *error = NULL;

        manager->priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->dbus_connection == NULL) {
                if (error != NULL) {
                        g_warning ("Error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        /* Hmm, should we do this in gsd_xrandr_manager_start()? */
        dbus_g_connection_register_g_object (manager->priv->dbus_connection, GSD_XRANDR_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

GsdXrandrManager *
gsd_xrandr_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_XRANDR_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);

                if (!register_manager_dbus (manager_object)) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return GSD_XRANDR_MANAGER (manager_object);
}
