/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2005-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <mateconf/mateconf-client.h>
#include <canberra-gtk.h>
#include <libupower-glib/upower.h>
#include <libmatenotify/notify.h>

#include "egg-debug.h"
#include "egg-console-kit.h"

#include "gpm-button.h"
#include "gpm-control.h"
#include "gpm-common.h"
#include "gpm-dpms.h"
#include "gpm-idle.h"
#include "gpm-manager.h"
#include "gpm-screensaver.h"
#include "gpm-backlight.h"
#include "gpm-session.h"
#include "gpm-stock-icons.h"
#include "gpm-prefs-server.h"
#include "gpm-tray-icon.h"
#include "gpm-engine.h"
#include "gpm-upower.h"
#include "gpm-disks.h"

#include "org.mate.PowerManager.Backlight.h"

static void     gpm_manager_finalize	(GObject	 *object);

#define GPM_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_MANAGER, GpmManagerPrivate))
#define GPM_MANAGER_RECALL_DELAY		30 /* seconds */
#define GPM_MANAGER_NOTIFY_TIMEOUT_NEVER	0 /* ms */
#define GPM_MANAGER_NOTIFY_TIMEOUT_SHORT	10 * 1000 /* ms */
#define GPM_MANAGER_NOTIFY_TIMEOUT_LONG		30 * 1000 /* ms */

#define GPM_MANAGER_CRITICAL_ALERT_TIMEOUT      5 /* seconds */

struct GpmManagerPrivate
{
	GpmButton		*button;
	MateConfClient		*conf;
	GpmDisks		*disks;
	GpmDpms			*dpms;
	GpmIdle			*idle;
	GpmPrefsServer		*prefs_server;
	GpmControl		*control;
	GpmScreensaver		*screensaver;
	GpmTrayIcon		*tray_icon;
	GpmEngine		*engine;
	GpmBacklight		*backlight;
	EggConsoleKit		*console;
	guint32			 screensaver_ac_throttle_id;
	guint32			 screensaver_dpms_throttle_id;
	guint32			 screensaver_lid_throttle_id;
	guint32                  critical_alert_timeout_id;
	ca_proplist             *critical_alert_loop_props;
	UpClient		*client;
	gboolean		 on_battery;
	gboolean		 just_resumed;
	GtkStatusIcon		*status_icon;
	NotifyNotification	*notification_general;
	NotifyNotification	*notification_warning_low;
	NotifyNotification	*notification_discharging;
	NotifyNotification	*notification_fully_charged;
};

typedef enum {
	GPM_MANAGER_SOUND_POWER_PLUG,
	GPM_MANAGER_SOUND_POWER_UNPLUG,
	GPM_MANAGER_SOUND_LID_OPEN,
	GPM_MANAGER_SOUND_LID_CLOSE,
	GPM_MANAGER_SOUND_BATTERY_CAUTION,
	GPM_MANAGER_SOUND_BATTERY_LOW,
	GPM_MANAGER_SOUND_BATTERY_FULL,
	GPM_MANAGER_SOUND_SUSPEND_START,
	GPM_MANAGER_SOUND_SUSPEND_RESUME,
	GPM_MANAGER_SOUND_SUSPEND_ERROR,
	GPM_MANAGER_SOUND_LAST
} GpmManagerSound;

G_DEFINE_TYPE (GpmManager, gpm_manager, G_TYPE_OBJECT)

/**
 * gpm_manager_error_quark:
 * Return value: Our personal error quark.
 **/
GQuark
gpm_manager_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("gpm_manager_error");
	return quark;
}

/**
 * gpm_manager_error_get_type:
 **/
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }
GType
gpm_manager_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] =
		{
			ENUM_ENTRY (GPM_MANAGER_ERROR_DENIED, "PermissionDenied"),
			ENUM_ENTRY (GPM_MANAGER_ERROR_NO_HW, "NoHardwareSupport"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("GpmManagerError", values);
	}
	return etype;
}

/**
 * gpm_manager_play_loop_timeout_cb:
 **/
static gboolean
gpm_manager_play_loop_timeout_cb (GpmManager *manager)
{
	ca_context *context;
	context = ca_gtk_context_get_for_screen (gdk_screen_get_default ());
	ca_context_play_full (context, 0,
			      manager->priv->critical_alert_loop_props,
			      NULL,
			      NULL);
	return TRUE;
}

/**
 * gpm_manager_play_loop_stop:
 **/
static gboolean
gpm_manager_play_loop_stop (GpmManager *manager)
{
	if (manager->priv->critical_alert_timeout_id == 0) {
		egg_warning ("no sound loop present to stop");
		return FALSE;
	}

	g_source_remove (manager->priv->critical_alert_timeout_id);
	ca_proplist_destroy (manager->priv->critical_alert_loop_props);

	manager->priv->critical_alert_loop_props = NULL;
	manager->priv->critical_alert_timeout_id = 0;

	return TRUE;
}

/**
 * gpm_manager_play_loop_start:
 **/
static gboolean
gpm_manager_play_loop_start (GpmManager *manager, GpmManagerSound action, gboolean force, guint timeout)
{
	const gchar *id = NULL;
	const gchar *desc = NULL;
	gboolean ret;
	gint retval;
	ca_context *context;

	ret = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_UI_ENABLE_SOUND, NULL);
	if (!ret && !force) {
		egg_debug ("ignoring sound due to policy");
		return FALSE;
	}

	if (timeout == 0) {
		egg_warning ("received invalid timeout");
		return FALSE;
	}

	/* if a sound loop is already running, stop the existing loop */
	if (manager->priv->critical_alert_timeout_id != 0) {
		egg_warning ("was instructed to play a sound loop with one already playing");
		gpm_manager_play_loop_stop (manager);
	}

	if (action == GPM_MANAGER_SOUND_BATTERY_LOW) {
		id = "battery-low";
		/* TRANSLATORS: this is the sound description */
		desc = _("Battery is very low");
	}

	/* no match */
	if (id == NULL) {
		egg_warning ("no sound match for %i", action);
		return FALSE;
	}

	ca_proplist_create (&(manager->priv->critical_alert_loop_props));
	ca_proplist_sets (manager->priv->critical_alert_loop_props,
			  CA_PROP_EVENT_ID, id);
	ca_proplist_sets (manager->priv->critical_alert_loop_props,
			  CA_PROP_EVENT_DESCRIPTION, desc);

	manager->priv->critical_alert_timeout_id = g_timeout_add_seconds (timeout,
									  (GSourceFunc) gpm_manager_play_loop_timeout_cb,
									  manager);

	/* play the sound, using sounds from the naming spec */
	context = ca_gtk_context_get_for_screen (gdk_screen_get_default ());
	retval = ca_context_play (context, 0,
				  CA_PROP_EVENT_ID, id,
				  CA_PROP_EVENT_DESCRIPTION, desc, NULL);
	if (retval < 0)
		egg_warning ("failed to play %s: %s", id, ca_strerror (retval));
	return TRUE;
}

/**
 * gpm_manager_play:
 **/
static gboolean
gpm_manager_play (GpmManager *manager, GpmManagerSound action, gboolean force)
{
	const gchar *id = NULL;
	const gchar *desc = NULL;
	gboolean ret;
	gint retval;
	ca_context *context;

	ret = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_UI_ENABLE_SOUND, NULL);
	if (!ret && !force) {
		egg_debug ("ignoring sound due to policy");
		return FALSE;
	}

	if (action == GPM_MANAGER_SOUND_POWER_PLUG) {
		id = "power-plug";
		/* TRANSLATORS: this is the sound description */
		desc = _("Power plugged in");
	} else if (action == GPM_MANAGER_SOUND_POWER_UNPLUG) {
		id = "power-unplug";
		/* TRANSLATORS: this is the sound description */
		desc = _("Power unplugged");
	} else if (action == GPM_MANAGER_SOUND_LID_OPEN) {
		id = "lid-open";
		/* TRANSLATORS: this is the sound description */
		desc = _("Lid has opened");
	} else if (action == GPM_MANAGER_SOUND_LID_CLOSE) {
		id = "lid-close";
		/* TRANSLATORS: this is the sound description */
		desc = _("Lid has closed");
	} else if (action == GPM_MANAGER_SOUND_BATTERY_CAUTION) {
		id = "battery-caution";
		/* TRANSLATORS: this is the sound description */
		desc = _("Battery is low");
	} else if (action == GPM_MANAGER_SOUND_BATTERY_LOW) {
		id = "battery-low";
		/* TRANSLATORS: this is the sound description */
		desc = _("Battery is very low");
	} else if (action == GPM_MANAGER_SOUND_BATTERY_FULL) {
		id = "battery-full";
		/* TRANSLATORS: this is the sound description */
		desc = _("Battery is full");
	} else if (action == GPM_MANAGER_SOUND_SUSPEND_START) {
		id = "suspend-start";
		/* TRANSLATORS: this is the sound description */
		desc = _("Suspend started");
	} else if (action == GPM_MANAGER_SOUND_SUSPEND_RESUME) {
		id = "suspend-resume";
		/* TRANSLATORS: this is the sound description */
		desc = _("Resumed");
	} else if (action == GPM_MANAGER_SOUND_SUSPEND_ERROR) {
		id = "suspend-error";
		/* TRANSLATORS: this is the sound description */
		desc = _("Suspend failed");
	}

	/* no match */
	if (id == NULL) {
		egg_warning ("no match");
		return FALSE;
	}

	/* play the sound, using sounds from the naming spec */
	context = ca_gtk_context_get_for_screen (gdk_screen_get_default ());
	retval = ca_context_play (context, 0,
				  CA_PROP_EVENT_ID, id,
				  CA_PROP_EVENT_DESCRIPTION, desc, NULL);
	if (retval < 0)
		egg_warning ("failed to play %s: %s", id, ca_strerror (retval));
	return TRUE;
}

/**
 * gpm_manager_is_inhibit_valid:
 * @manager: This class instance
 * @action: The action we want to do, e.g. "suspend"
 *
 * Checks to see if the specific action has been inhibited by a program.
 *
 * Return value: TRUE if we can perform the action.
 **/
static gboolean
gpm_manager_is_inhibit_valid (GpmManager *manager, gboolean user_action, const char *action)
{
	return TRUE;
}

/**
 * gpm_manager_sync_policy_sleep:
 * @manager: This class instance
 *
 * Changes the policy if required, setting brightness, display and computer
 * timeouts.
 * We have to make sure mate-screensaver disables screensaving, and enables
 * monitor DPMS instead when on batteries to save power.
 **/
static void
gpm_manager_sync_policy_sleep (GpmManager *manager)
{
	guint sleep_display;
	guint sleep_computer;

	if (!manager->priv->on_battery) {
		sleep_computer = mateconf_client_get_int (manager->priv->conf, GPM_CONF_TIMEOUT_SLEEP_COMPUTER_AC, NULL);
		sleep_display = mateconf_client_get_int (manager->priv->conf, GPM_CONF_TIMEOUT_SLEEP_DISPLAY_AC, NULL);
	} else {
		sleep_computer = mateconf_client_get_int (manager->priv->conf, GPM_CONF_TIMEOUT_SLEEP_COMPUTER_BATT, NULL);
		sleep_display = mateconf_client_get_int (manager->priv->conf, GPM_CONF_TIMEOUT_SLEEP_DISPLAY_BATT, NULL);
	}

	/* set the new sleep (inactivity) value */
	gpm_idle_set_timeout_blank (manager->priv->idle, sleep_display);
	gpm_idle_set_timeout_sleep (manager->priv->idle, sleep_computer);
}

/**
 * gpm_manager_blank_screen:
 * @manager: This class instance
 *
 * Turn off the backlight of the LCD when we shut the lid, and lock
 * if required. This is required because some laptops do not turn off the
 * LCD backlight when the lid is closed.
 * See http://bugzilla.mate.org/show_bug.cgi?id=321313
 *
 * Return value: Success.
 **/
static gboolean
gpm_manager_blank_screen (GpmManager *manager, GError **noerror)
{
	gboolean do_lock;
	gboolean ret = TRUE;
	GError *error = NULL;

	do_lock = gpm_control_get_lock_policy (manager->priv->control,
					       GPM_CONF_LOCK_ON_BLANK_SCREEN);
	if (do_lock) {
		if (!gpm_screensaver_lock (manager->priv->screensaver))
			egg_debug ("Could not lock screen via mate-screensaver");
	}
	gpm_dpms_set_mode (manager->priv->dpms, GPM_DPMS_MODE_OFF, &error);
	if (error) {
		egg_debug ("Unable to set DPMS mode: %s", error->message);
		g_error_free (error);
		ret = FALSE;
	}
	return ret;
}

/**
 * gpm_manager_unblank_screen:
 * @manager: This class instance
 *
 * Unblank the screen after we have opened the lid of the laptop
 *
 * Return value: Success.
 **/
static gboolean
gpm_manager_unblank_screen (GpmManager *manager, GError **noerror)
{
	gboolean do_lock;
	gboolean ret = TRUE;
	GError *error = NULL;

	gpm_dpms_set_mode (manager->priv->dpms, GPM_DPMS_MODE_ON, &error);
	if (error) {
		egg_debug ("Unable to set DPMS mode: %s", error->message);
		g_error_free (error);
		ret = FALSE;
	}

	do_lock = gpm_control_get_lock_policy (manager->priv->control, GPM_CONF_LOCK_ON_BLANK_SCREEN);
	if (do_lock)
		gpm_screensaver_poke (manager->priv->screensaver);
	return ret;
}

/**
 * gpm_manager_notify_close:
 **/
static gboolean
gpm_manager_notify_close (GpmManager *manager, NotifyNotification *notification)
{
	gboolean ret = FALSE;
	GError *error = NULL;

	/* exists? */
	if (notification == NULL)
		goto out;

	/* try to close */
	ret = notify_notification_close (notification, &error);
	if (!ret) {
		egg_warning ("failed to close notification: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	return ret;
}

/**
 * gpm_manager_notification_closed_cb:
 **/
static void
gpm_manager_notification_closed_cb (NotifyNotification *notification, NotifyNotification **notification_class)
{
	egg_debug ("caught notification closed signal %p", notification);
	/* the object is already unreffed in _close_signal_handler */
	*notification_class = NULL;
}

/**
 * gpm_manager_notify:
 **/
static gboolean
gpm_manager_notify (GpmManager *manager, NotifyNotification **notification_class,
		    const gchar *title, const gchar *message,
		    guint timeout, const gchar *icon, NotifyUrgency urgency)
{
	gboolean ret;
	GError *error = NULL;
	NotifyNotification *notification;
	GtkWidget *dialog;

	/* close any existing notification of this class */
	gpm_manager_notify_close (manager, *notification_class);

	/* if the status icon is hidden, don't point at it */
	if (manager->priv->status_icon != NULL &&
	    gtk_status_icon_is_embedded (manager->priv->status_icon))
		notification = notify_notification_new_with_status_icon (title, message, icon, manager->priv->status_icon);
	else
		notification = notify_notification_new (title, message, icon, NULL);
	notify_notification_set_timeout (notification, timeout);
	notify_notification_set_urgency (notification, urgency);
	g_signal_connect (notification, "closed", G_CALLBACK (gpm_manager_notification_closed_cb), notification_class);

	egg_debug ("notification %p: %s : %s", notification, title, message);

	/* try to show */
	ret = notify_notification_show (notification, &error);
	if (!ret) {
		egg_warning ("failed to show notification: %s", error->message);
		g_error_free (error);

		/* show modal dialog as libmatenotify failed */
		dialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
							     "<span size='larger'><b>%s</b></span>", title);
		gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", message);

		/* wait async for close */
		gtk_widget_show (dialog);
		g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);

		g_object_unref (notification);
		goto out;
	}

	/* save this local instance as the class instance */
	g_object_add_weak_pointer (G_OBJECT (notification), (gpointer) &notification);
	*notification_class = notification;
out:
	return ret;
}


/**
 * gpm_manager_sleep_failure_response_cb:
 **/
static void
gpm_manager_sleep_failure_response_cb (GtkDialog *dialog, gint response_id, GpmManager *manager)
{
	GdkScreen *screen;
	GtkWidget *dialog_error;
	GError *error = NULL;
	gboolean ret;
	gchar *uri = NULL;

	/* user clicked the help button */
	if (response_id == GTK_RESPONSE_HELP) {
		uri = mateconf_client_get_string (manager->priv->conf, GPM_CONF_NOTIFY_SLEEP_FAILED_URI, NULL);
		screen = gdk_screen_get_default();
		ret = gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error);
		if (!ret) {
			dialog_error = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
							       "Failed to show uri %s", error->message);
			gtk_dialog_run (GTK_DIALOG (dialog_error));
			g_error_free (error);
		}
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
	g_free (uri);
}

/**
 * gpm_manager_sleep_failure:
 **/
static void
gpm_manager_sleep_failure (GpmManager *manager, gboolean is_suspend, const gchar *detail)
{
	gboolean show_sleep_failed;
	GString *string = NULL;
	const gchar *title;
	gchar *uri = NULL;
	const gchar *icon;
	GtkWidget *dialog;

	/* only show this if specified in mateconf */
	show_sleep_failed = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_NOTIFY_SLEEP_FAILED, NULL);

	egg_debug ("sleep failed");
	gpm_manager_play (manager, GPM_MANAGER_SOUND_SUSPEND_ERROR, TRUE);

	/* only emit if in MateConf */
	if (!show_sleep_failed)
		goto out;

	/* TRANSLATORS: window title: there was a problem putting the machine to sleep */
	string = g_string_new ("");
	if (is_suspend) {
		/* TRANSLATORS: message text */
		g_string_append (string, _("Computer failed to suspend."));
		/* TRANSLATORS: title text */
		title = _("Failed to suspend");
		icon = GPM_STOCK_SUSPEND;
	} else {
		/* TRANSLATORS: message text */
		g_string_append (string, _("Computer failed to hibernate."));
		/* TRANSLATORS: title text */
		title = _("Failed to hibernate");
		icon = GPM_STOCK_HIBERNATE;
	}

	/* TRANSLATORS: message text */
	g_string_append_printf (string, "\n\n%s %s", _("Failure was reported as:"), detail);

	/* show modal dialog */
	dialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
						     "<span size='larger'><b>%s</b></span>", title);
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", string->str);
	gtk_window_set_icon_name (GTK_WINDOW(dialog), icon);

	/* show a button? */
	uri = mateconf_client_get_string (manager->priv->conf, GPM_CONF_NOTIFY_SLEEP_FAILED_URI, NULL);
	if (uri != NULL && uri[0] != '\0') {
		/* TRANSLATORS: button text, visit the suspend help website */
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("Visit help page"), GTK_RESPONSE_HELP);
	}

	/* wait async for close */
	gtk_widget_show (dialog);
	g_signal_connect (dialog, "response", G_CALLBACK (gpm_manager_sleep_failure_response_cb), manager);
out:
	g_free (uri);
	g_string_free (string, TRUE);
}

/**
 * gpm_manager_action_suspend:
 **/
static gboolean
gpm_manager_action_suspend (GpmManager *manager, const gchar *reason)
{
	gboolean ret;
	GError *error = NULL;

	/* check to see if we are inhibited */
	if (gpm_manager_is_inhibit_valid (manager, FALSE, "suspend") == FALSE)
		return FALSE;

	egg_debug ("suspending, reason: %s", reason);
	ret = gpm_control_suspend (manager->priv->control, &error);
	if (!ret) {
		gpm_manager_sleep_failure (manager, TRUE, error->message);
		g_error_free (error);
	}
	gpm_button_reset_time (manager->priv->button);
	return TRUE;
}

/**
 * gpm_manager_action_hibernate:
 **/
static gboolean
gpm_manager_action_hibernate (GpmManager *manager, const gchar *reason)
{
	gboolean ret;
	GError *error = NULL;

	/* check to see if we are inhibited */
	if (gpm_manager_is_inhibit_valid (manager, FALSE, "hibernate") == FALSE)
		return FALSE;

	egg_debug ("hibernating, reason: %s", reason);
	ret = gpm_control_hibernate (manager->priv->control, &error);
	if (!ret) {
		gpm_manager_sleep_failure (manager, TRUE, error->message);
		g_error_free (error);
	}
	gpm_button_reset_time (manager->priv->button);
	return TRUE;
}

/**
 * gpm_manager_perform_policy:
 * @manager: This class instance
 * @policy: The policy that we should do, e.g. "suspend"
 * @reason: The reason we are performing the policy action, e.g. "battery critical"
 *
 * Does one of the policy actions specified in mateconf.
 **/
static gboolean
gpm_manager_perform_policy (GpmManager  *manager, const gchar *policy_key, const gchar *reason)
{
	gchar *action = NULL;
	GpmActionPolicy policy;

	/* are we inhibited? */
	if (gpm_manager_is_inhibit_valid (manager, FALSE, "policy action") == FALSE)
		return FALSE;

	action = mateconf_client_get_string (manager->priv->conf, policy_key, NULL);
	egg_debug ("action: %s set to %s (%s)", policy_key, action, reason);
	policy = gpm_action_policy_from_string (action);

	if (policy == GPM_ACTION_POLICY_NOTHING) {
		egg_debug ("doing nothing, reason: %s", reason);
	} else if (policy == GPM_ACTION_POLICY_SUSPEND) {
		gpm_manager_action_suspend (manager, reason);

	} else if (policy == GPM_ACTION_POLICY_HIBERNATE) {
		gpm_manager_action_hibernate (manager, reason);

	} else if (policy == GPM_ACTION_POLICY_BLANK) {
		gpm_manager_blank_screen (manager, NULL);

	} else if (policy == GPM_ACTION_POLICY_SHUTDOWN) {
		egg_debug ("shutting down, reason: %s", reason);
		gpm_control_shutdown (manager->priv->control, NULL);

	} else if (policy == GPM_ACTION_POLICY_INTERACTIVE) {
		GpmSession *session;
		egg_debug ("logout, reason: %s", reason);
		session = gpm_session_new ();
		gpm_session_logout (session);
		g_object_unref (session);
	} else {
		egg_warning ("unknown action %s", action);
	}

	g_free (action);
	return TRUE;
}

/**
 * gpm_manager_get_preferences_options:
 **/
gboolean
gpm_manager_get_preferences_options (GpmManager *manager, gint *capability, GError **error)
{
	g_return_val_if_fail (manager != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_MANAGER (manager), FALSE);
	return gpm_prefs_server_get_capability (manager->priv->prefs_server, capability);
}

/**
 * gpm_manager_idle_do_sleep:
 * @manager: This class instance
 *
 * This callback is called when we want to sleep. Use the users
 * preference from mateconf, but change it if we can't do the action.
 **/
static void
gpm_manager_idle_do_sleep (GpmManager *manager)
{
	gchar *action = NULL;
	gboolean ret;
	GError *error = NULL;
	GpmActionPolicy policy;

	if (!manager->priv->on_battery)
		action = mateconf_client_get_string (manager->priv->conf, GPM_CONF_ACTIONS_SLEEP_TYPE_AC, NULL);
	else
		action = mateconf_client_get_string (manager->priv->conf, GPM_CONF_ACTIONS_SLEEP_TYPE_BATT, NULL);
	policy = gpm_action_policy_from_string (action);

	if (policy == GPM_ACTION_POLICY_NOTHING) {
		egg_debug ("doing nothing as system idle action");

	} else if (policy == GPM_ACTION_POLICY_SUSPEND) {
		egg_debug ("suspending, reason: System idle");
		ret = gpm_control_suspend (manager->priv->control, &error);
		if (!ret) {
			egg_warning ("cannot suspend (error: %s), so trying hibernate", error->message);
			g_error_free (error);
			error = NULL;
			ret = gpm_control_hibernate (manager->priv->control, &error);
			if (!ret) {
				egg_warning ("cannot suspend or hibernate: %s", error->message);
				g_error_free (error);
			}
		}

	} else if (policy == GPM_ACTION_POLICY_HIBERNATE) {
		egg_debug ("hibernating, reason: System idle");
		ret = gpm_control_hibernate (manager->priv->control, &error);
		if (!ret) {
			egg_warning ("cannot hibernate (error: %s), so trying suspend", error->message);
			g_error_free (error);
			error = NULL;
			ret = gpm_control_suspend (manager->priv->control, &error);
			if (!ret) {
				egg_warning ("cannot suspend or hibernate: %s", error->message);
				g_error_free (error);
			}
		}
	}
	g_free (action);
}

/**
 * gpm_manager_idle_changed_cb:
 * @idle: The idle class instance
 * @mode: The idle mode, e.g. GPM_IDLE_MODE_BLANK
 * @manager: This class instance
 *
 * This callback is called when the idle class detects that the idle state
 * has changed. GPM_IDLE_MODE_BLANK is when the session has become inactive,
 * and GPM_IDLE_MODE_SLEEP is where the session has become inactive, AND the
 * session timeout has elapsed for the idle action.
 **/
static void
gpm_manager_idle_changed_cb (GpmIdle *idle, GpmIdleMode mode, GpmManager *manager)
{
	/* ConsoleKit says we are not on active console */
	if (!egg_console_kit_is_active (manager->priv->console)) {
		egg_debug ("ignoring as not on active console");
		return;
	}

	/* Ignore back-to-NORMAL events when the lid is closed, as the DPMS is
	 * already off, and we don't want to re-enable the screen when the user
	 * moves the mouse on systems that do not support hardware blanking. */
	if (gpm_button_is_lid_closed (manager->priv->button) &&
	    mode == GPM_IDLE_MODE_NORMAL) {
		egg_debug ("lid is closed, so we are ignoring ->NORMAL state changes");
		return;
	}

	if (mode == GPM_IDLE_MODE_SLEEP) {
		egg_debug ("Idle state changed: SLEEP");
		if (gpm_manager_is_inhibit_valid (manager, FALSE, "timeout action") == FALSE)
			return;
		gpm_manager_idle_do_sleep (manager);
	}
}

/**
 * gpm_manager_lid_button_pressed:
 * @manager: This class instance
 * @state: TRUE for closed
 *
 * Does actions when the lid is closed, depending on if we are on AC or
 * battery power.
 **/
static void
gpm_manager_lid_button_pressed (GpmManager *manager, gboolean pressed)
{
	if (pressed)
		gpm_manager_play (manager, GPM_MANAGER_SOUND_LID_CLOSE, FALSE);
	else
		gpm_manager_play (manager, GPM_MANAGER_SOUND_LID_OPEN, FALSE);

	if (pressed == FALSE) {
		/* we turn the lid dpms back on unconditionally */
		gpm_manager_unblank_screen (manager, NULL);
		return;
	}

	if (!manager->priv->on_battery) {
		egg_debug ("Performing AC policy");
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_LID_AC,
					    "The lid has been closed on ac power.");
		return;
	}

	egg_debug ("Performing battery policy");
	gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_LID_BATT,
				    "The lid has been closed on battery power.");
}

static void
gpm_manager_update_dpms_throttle (GpmManager *manager)
{
	GpmDpmsMode mode;
	gpm_dpms_get_mode (manager->priv->dpms, &mode, NULL);

	/* Throttle the manager when DPMS is active since we can't see it anyway */
	if (mode == GPM_DPMS_MODE_ON) {
		if (manager->priv->screensaver_dpms_throttle_id != 0) {
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_dpms_throttle_id);
			manager->priv->screensaver_dpms_throttle_id = 0;
		}
	} else {
		/* if throttle already exists then remove */
		if (manager->priv->screensaver_dpms_throttle_id != 0) {
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_dpms_throttle_id);
		}
		/* TRANSLATORS: this is the mate-screensaver throttle */
		manager->priv->screensaver_dpms_throttle_id = gpm_screensaver_add_throttle (manager->priv->screensaver, _("Display DPMS activated"));
	}
}

static void
gpm_manager_update_ac_throttle (GpmManager *manager)
{
	/* Throttle the manager when we are not on AC power so we don't
	   waste the battery */
	if (!manager->priv->on_battery) {
		if (manager->priv->screensaver_ac_throttle_id != 0) {
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_ac_throttle_id);
			manager->priv->screensaver_ac_throttle_id = 0;
		}
	} else {
		/* if throttle already exists then remove */
		if (manager->priv->screensaver_ac_throttle_id != 0)
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_ac_throttle_id);
		/* TRANSLATORS: this is the mate-screensaver throttle */
		manager->priv->screensaver_ac_throttle_id = gpm_screensaver_add_throttle (manager->priv->screensaver, _("On battery power"));
	}
}

static void
gpm_manager_update_lid_throttle (GpmManager *manager, gboolean lid_is_closed)
{
	/* Throttle the screensaver when the lid is close since we can't see it anyway
	   and it may overheat the laptop */
	if (lid_is_closed == FALSE) {
		if (manager->priv->screensaver_lid_throttle_id != 0) {
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_lid_throttle_id);
			manager->priv->screensaver_lid_throttle_id = 0;
		}
	} else {
		/* if throttle already exists then remove */
		if (manager->priv->screensaver_lid_throttle_id != 0)
			gpm_screensaver_remove_throttle (manager->priv->screensaver, manager->priv->screensaver_lid_throttle_id);
		manager->priv->screensaver_lid_throttle_id = gpm_screensaver_add_throttle (manager->priv->screensaver, _("Laptop lid is closed"));
	}
}

/**
 * gpm_manager_button_pressed_cb:
 * @power: The power class instance
 * @type: The button type, e.g. "power"
 * @state: The state, where TRUE is depressed or closed
 * @manager: This class instance
 **/
static void
gpm_manager_button_pressed_cb (GpmButton *button, const gchar *type, GpmManager *manager)
{
	gchar *message;
	egg_debug ("Button press event type=%s", type);

	/* ConsoleKit says we are not on active console */
	if (!egg_console_kit_is_active (manager->priv->console)) {
		egg_debug ("ignoring as not on active console");
		return;
	}

	if (g_strcmp0 (type, GPM_BUTTON_POWER) == 0) {
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_POWER, "The power button has been pressed.");
	} else if (g_strcmp0 (type, GPM_BUTTON_SLEEP) == 0) {
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_SUSPEND, "The suspend button has been pressed.");
	} else if (g_strcmp0 (type, GPM_BUTTON_SUSPEND) == 0) {
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_SUSPEND, "The suspend button has been pressed.");
	} else if (g_strcmp0 (type, GPM_BUTTON_HIBERNATE) == 0) {
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_HIBERNATE, "The hibernate button has been pressed.");
	} else if (g_strcmp0 (type, GPM_BUTTON_LID_OPEN) == 0) {
		gpm_manager_lid_button_pressed (manager, FALSE);
	} else if (g_strcmp0 (type, GPM_BUTTON_LID_CLOSED) == 0) {
		gpm_manager_lid_button_pressed (manager, TRUE);
	} else if (g_strcmp0 (type, GPM_BUTTON_BATTERY) == 0) {
		message = gpm_engine_get_summary (manager->priv->engine);
		gpm_manager_notify (manager, &manager->priv->notification_general,
				    _("Power Information"),
				    message,
				    GPM_MANAGER_NOTIFY_TIMEOUT_LONG,
				    GTK_STOCK_DIALOG_INFO,
				    NOTIFY_URGENCY_NORMAL);
		g_free (message);
	}

	/* really belongs in mate-screensaver */
	if (g_strcmp0 (type, GPM_BUTTON_LOCK) == 0)
		gpm_screensaver_lock (manager->priv->screensaver);

	/* disable or enable the fancy screensaver, as we don't want
	 * this starting when the lid is shut */
	if (g_strcmp0 (type, GPM_BUTTON_LID_CLOSED) == 0)
		gpm_manager_update_lid_throttle (manager, TRUE);
	else if (g_strcmp0 (type, GPM_BUTTON_LID_OPEN) == 0)
		gpm_manager_update_lid_throttle (manager, FALSE);
}

/**
 * gpm_manager_get_spindown_timeout:
 **/
static gint
gpm_manager_get_spindown_timeout (GpmManager *manager)
{
	gboolean enabled;
	gint timeout;

	/* get policy */
	if (!manager->priv->on_battery) {
		enabled = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_DISKS_SPINDOWN_ENABLE_AC, NULL);
		timeout = mateconf_client_get_int (manager->priv->conf, GPM_CONF_DISKS_SPINDOWN_TIMEOUT_AC, NULL);
	} else {
		enabled = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_DISKS_SPINDOWN_ENABLE_BATT, NULL);
		timeout = mateconf_client_get_int (manager->priv->conf, GPM_CONF_DISKS_SPINDOWN_TIMEOUT_BATT, NULL);
	}
	if (!enabled)
		timeout = 0;
	return timeout;
}

/**
 * gpm_manager_client_changed_cb:
 **/
static void
gpm_manager_client_changed_cb (UpClient *client, GpmManager *manager)
{
	gboolean event_when_closed;
	gint timeout;
	gboolean on_battery;
	gboolean lid_is_closed;

	/* get the client state */
	g_object_get (client,
		      "on-battery", &on_battery,
		      "lid-is-closed", &lid_is_closed,
		      NULL);
	if (on_battery == manager->priv->on_battery) {
		egg_debug ("same state as before, ignoring");
		return;
	}

	/* close any discharging notifications */
	if (!on_battery) {
		egg_debug ("clearing notify due ac being present");
		gpm_manager_notify_close (manager, manager->priv->notification_warning_low);
		gpm_manager_notify_close (manager, manager->priv->notification_discharging);
	}

	/* if we are playing a critical charge sound loop, stop it */
	if (!on_battery && manager->priv->critical_alert_timeout_id) {
		egg_debug ("stopping alert loop due to ac being present");
		gpm_manager_play_loop_stop (manager);
	}

	/* save in local cache */
	manager->priv->on_battery = on_battery;

	/* ConsoleKit says we are not on active console */
	if (!egg_console_kit_is_active (manager->priv->console)) {
		egg_debug ("ignoring as not on active console");
		return;
	}

	egg_debug ("on_battery: %d", on_battery);

	/* set disk spindown threshold */
	timeout = gpm_manager_get_spindown_timeout (manager);
	gpm_disks_set_spindown_timeout (manager->priv->disks, timeout);

	gpm_manager_sync_policy_sleep (manager);

	gpm_manager_update_ac_throttle (manager);

	/* simulate user input, but only when the lid is open */
	if (!lid_is_closed)
		gpm_screensaver_poke (manager->priv->screensaver);

	if (!on_battery)
		gpm_manager_play (manager, GPM_MANAGER_SOUND_POWER_PLUG, FALSE);
	else
		gpm_manager_play (manager, GPM_MANAGER_SOUND_POWER_UNPLUG, FALSE);

	/* We do the lid close on battery action if the ac adapter is removed
	   when the laptop is closed and on battery. Fixes #331655 */
	event_when_closed = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_ACTIONS_SLEEP_WHEN_CLOSED, NULL);

	/* We keep track of the lid state so we can do the
	   lid close on battery action if the ac adapter is removed when the laptop
	   is closed. Fixes #331655 */
	if (event_when_closed && on_battery && lid_is_closed) {
		gpm_manager_perform_policy (manager, GPM_CONF_BUTTON_LID_BATT,
					    "The lid has been closed, and the ac adapter "
					    "removed (and mateconf is okay).");
	}
}

/**
 * manager_critical_action_do:
 * @manager: This class instance
 *
 * This is the stub function when we have waited a few seconds for the user to
 * see the message, explaining what we are about to do.
 *
 * Return value: FALSE, as we don't want to repeat this action on resume.
 **/
static gboolean
manager_critical_action_do (GpmManager *manager)
{
	/* stop playing the alert as it's too late to do anything now */
	if (manager->priv->critical_alert_timeout_id)
		gpm_manager_play_loop_stop (manager);

	gpm_manager_perform_policy (manager, GPM_CONF_ACTIONS_CRITICAL_BATT, "Battery is critically low.");
	return FALSE;
}

/**
 * gpm_manager_class_init:
 * @klass: The GpmManagerClass
 **/
static void
gpm_manager_class_init (GpmManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_manager_finalize;
	g_type_class_add_private (klass, sizeof (GpmManagerPrivate));
}

/**
 * gpm_conf_mateconf_key_changed_cb:
 *
 * We might have to do things when the mateconf keys change; do them here.
 **/
static void
gpm_conf_mateconf_key_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, GpmManager *manager)
{
	MateConfValue *value;

	value = mateconf_entry_get_value (entry);
	if (value == NULL)
		return;

	if (g_strcmp0 (entry->key, GPM_CONF_TIMEOUT_SLEEP_COMPUTER_BATT) == 0 ||
	    g_strcmp0 (entry->key, GPM_CONF_TIMEOUT_SLEEP_COMPUTER_AC) == 0 ||
	    g_strcmp0 (entry->key, GPM_CONF_TIMEOUT_SLEEP_DISPLAY_BATT) == 0 ||
	    g_strcmp0 (entry->key, GPM_CONF_TIMEOUT_SLEEP_DISPLAY_AC) == 0)
		gpm_manager_sync_policy_sleep (manager);
}

#if 0
/**
 * gpm_manager_screensaver_auth_request_cb:
 * @manager: This manager class instance
 * @auth: If we are trying to authenticate
 *
 * Called when the user is trying or has authenticated
 **/
static void
gpm_manager_screensaver_auth_request_cb (GpmScreensaver *screensaver, gboolean auth_begin, GpmManager *manager)
{
	GError *error = NULL;

	if (auth_begin) {
		/* We turn on the monitor unconditionally, as we may be using
		 * a smartcard to authenticate and DPMS might still be on.
		 * See #350291 for more details */
		gpm_dpms_set_mode (manager->priv->dpms, GPM_DPMS_MODE_ON, &error);
		if (error != NULL) {
			egg_warning ("Failed to turn on DPMS: %s", error->message);
			g_error_free (error);
			error = NULL;
		}
	}
}
#endif

/**
 * gpm_manager_perhaps_recall_response_cb:
 */
static void
gpm_manager_perhaps_recall_response_cb (GtkDialog *dialog, gint response_id, GpmManager *manager)
{
	GdkScreen *screen;
	GtkWidget *dialog_error;
	GError *error = NULL;
	gboolean ret;
	const gchar *website;

	/* don't show this again */
	if (response_id == GTK_RESPONSE_CANCEL) {
		mateconf_client_set_bool (manager->priv->conf, GPM_CONF_NOTIFY_PERHAPS_RECALL, FALSE, NULL);
		goto out;
	}

	/* visit recall website */
	if (response_id == GTK_RESPONSE_OK) {
		screen = gdk_screen_get_default();
		website = (const gchar *) g_object_get_data (G_OBJECT (manager), "recall-oem-website");
		ret = gtk_show_uri (screen, website, gtk_get_current_event_time (), &error);
		if (!ret) {
			dialog_error = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
							       "Failed to show url %s", error->message);
			gtk_dialog_run (GTK_DIALOG (dialog_error));
			g_error_free (error);
		}
		goto out;
	}
out:
	gtk_widget_destroy (GTK_WIDGET (dialog));
	return;
}

/**
 * gpm_manager_perhaps_recall_delay_cb:
 */
static gboolean
gpm_manager_perhaps_recall_delay_cb (GpmManager *manager)
{
	const gchar *oem_vendor;
	gchar *title = NULL;
	gchar *message = NULL;
	GtkWidget *dialog;

	oem_vendor = (const gchar *) g_object_get_data (G_OBJECT (manager), "recall-oem-vendor");

	/* TRANSLATORS: the battery may be recalled by it's vendor */
	title = g_strdup_printf ("%s: %s", GPM_NAME, _("Battery may be recalled"));
	message = g_strdup_printf (_("A battery in your computer may have been "
				     "recalled by %s and you may be at risk.\n\n"
				     "For more information visit the battery recall website."), oem_vendor);
	dialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
						     "<span size='larger'><b>%s</b></span>", title);

	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog), "%s", message);

	/* TRANSLATORS: button text, visit the manufacturers recall website */
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Visit recall website"), GTK_RESPONSE_OK);

	/* TRANSLATORS: button text, do not show this bubble again */
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Do not show me this again"), GTK_RESPONSE_CANCEL);

	/* wait async for response */
	gtk_widget_show (dialog);
	g_signal_connect (dialog, "response", G_CALLBACK (gpm_manager_perhaps_recall_response_cb), manager);

	g_free (title);
	g_free (message);

	/* never repeat */
	return FALSE;
}

/**
 * gpm_manager_engine_perhaps_recall_cb:
 */
static void
gpm_manager_engine_perhaps_recall_cb (GpmEngine *engine, UpDevice *device, gchar *oem_vendor, gchar *website, GpmManager *manager)
{
	gboolean ret;

	/* don't show when running under GDM */
	if (g_getenv ("RUNNING_UNDER_GDM") != NULL) {
		egg_debug ("running under gdm, so no notification");
		return;
	}

	/* already shown, and dismissed */
	ret = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_NOTIFY_PERHAPS_RECALL, NULL);
	if (!ret) {
		egg_debug ("MateConf prevents notification: %s", GPM_CONF_NOTIFY_PERHAPS_RECALL);
		return;
	}

	g_object_set_data_full (G_OBJECT (manager), "recall-oem-vendor", (gpointer) g_strdup (oem_vendor), (GDestroyNotify) g_free);
	g_object_set_data_full (G_OBJECT (manager), "recall-oem-website", (gpointer) g_strdup (website), (GDestroyNotify) g_free);

	/* delay by a few seconds so the panel can load */
	g_timeout_add_seconds (GPM_MANAGER_RECALL_DELAY, (GSourceFunc) gpm_manager_perhaps_recall_delay_cb, manager);
}

/**
 * gpm_manager_engine_icon_changed_cb:
 */
static void
gpm_manager_engine_icon_changed_cb (GpmEngine  *engine, gchar *icon, GpmManager *manager)
{
	gpm_tray_icon_set_icon (manager->priv->tray_icon, icon);
}

/**
 * gpm_manager_engine_summary_changed_cb:
 */
static void
gpm_manager_engine_summary_changed_cb (GpmEngine *engine, gchar *summary, GpmManager *manager)
{
	gpm_tray_icon_set_tooltip (manager->priv->tray_icon, summary);
}

/**
 * gpm_manager_engine_low_capacity_cb:
 */
static void
gpm_manager_engine_low_capacity_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	gchar *message = NULL;
	const gchar *title;
	gdouble capacity;

	/* don't show when running under GDM */
	if (g_getenv ("RUNNING_UNDER_GDM") != NULL) {
		egg_debug ("running under gdm, so no notification");
		goto out;
	}

	/* get device properties */
	g_object_get (device,
		      "capacity", &capacity,
		      NULL);

	/* We should notify the user if the battery has a low capacity,
	 * where capacity is the ratio of the last_full capacity with that of
	 * the design capacity. (#326740) */

	/* TRANSLATORS: battery is old or broken */
	title = _("Battery may be broken");

	/* TRANSLATORS: notify the user that that battery is broken as the capacity is very low */
	message = g_strdup_printf (_("Battery has a very low capacity (%1.1f%%), "
				     "which means that it may be old or broken."), capacity);
	gpm_manager_notify (manager, &manager->priv->notification_general, title, message, GPM_MANAGER_NOTIFY_TIMEOUT_SHORT,
			    GTK_STOCK_DIALOG_INFO, NOTIFY_URGENCY_LOW);
out:
	g_free (message);
}

/**
 * gpm_manager_engine_fully_charged_cb:
 */
static void
gpm_manager_engine_fully_charged_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	UpDeviceKind kind;
	gchar *native_path = NULL;
	gboolean ret;
	guint plural = 1;
	const gchar *title;

	/* only action this if specified in mateconf */
	ret = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_NOTIFY_FULLY_CHARGED, NULL);
	if (!ret) {
		egg_debug ("no notification");
		goto out;
	}

	/* don't show when running under GDM */
	if (g_getenv ("RUNNING_UNDER_GDM") != NULL) {
		egg_debug ("running under gdm, so no notification");
		goto out;
	}

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "native-path", &native_path,
		      NULL);

	if (kind == UP_DEVICE_KIND_BATTERY) {
		/* is this a dummy composite device, which is plural? */
		if (g_str_has_prefix (native_path, "dummy"))
			plural = 2;

		/* hide the discharging notification */
		gpm_manager_notify_close (manager, manager->priv->notification_warning_low);
		gpm_manager_notify_close (manager, manager->priv->notification_discharging);

		/* TRANSLATORS: show the charged notification */
		title = ngettext ("Battery Charged", "Batteries Charged", plural);
		gpm_manager_notify (manager, &manager->priv->notification_fully_charged,
				    title, NULL, GPM_MANAGER_NOTIFY_TIMEOUT_SHORT,
				    GTK_STOCK_DIALOG_INFO, NOTIFY_URGENCY_LOW);
	}
out:
	g_free (native_path);
}

/**
 * gpm_manager_engine_discharging_cb:
 */
static void
gpm_manager_engine_discharging_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	UpDeviceKind kind;
	gboolean ret;
	const gchar *title;
	const gchar *message;
	gdouble percentage;
	gint64 time_to_empty;
	gchar *remaining_text = NULL;
	gchar *icon = NULL;
	const gchar *kind_desc;

	/* only action this if specified in mateconf */
	ret = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_NOTIFY_DISCHARGING, NULL);
	if (!ret) {
		egg_debug ("no notification");
		goto out;
	}

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "percentage", &percentage,
		      "time-to-empty", &time_to_empty,
		      NULL);

	/* only show text if there is a valid time */
	if (time_to_empty > 0)
		remaining_text = gpm_get_timestring (time_to_empty);
	kind_desc = gpm_device_kind_to_localised_text (kind, 1);

	if (kind == UP_DEVICE_KIND_BATTERY) {
		/* TRANSLATORS: laptop battery is now discharging */
		title = _("Battery Discharging");

		if (remaining_text != NULL) {
			/* TRANSLATORS: tell the user how much time they have got */
			message = g_strdup_printf (_("%s of battery power remaining (%.0f%%)"), remaining_text, percentage);
		} else {
			/* TRANSLATORS: the device is discharging, but we only have a percentage */
			message = g_strdup_printf (_("%s discharging (%.0f%%)"),
						   kind_desc, percentage);
		}
	} else if (kind == UP_DEVICE_KIND_UPS) {
		/* TRANSLATORS: UPS is now discharging */
		title = _("UPS Discharging");

		if (remaining_text != NULL) {
			/* TRANSLATORS: tell the user how much time they have got */
			message = g_strdup_printf (_("%s of UPS backup power remaining (%.0f%%)"), remaining_text, percentage);
		} else {
			/* TRANSLATORS: the device is discharging, but we only have a percentage */
			message = g_strdup_printf (_("%s discharging (%.0f%%)"),
						   kind_desc, percentage);
		}
	} else {
		/* nothing else of interest */
		goto out;
	}

	icon = gpm_upower_get_device_icon (device);
	/* show the notification */
	gpm_manager_notify (manager, &manager->priv->notification_discharging, title, message, GPM_MANAGER_NOTIFY_TIMEOUT_LONG,
			    icon, NOTIFY_URGENCY_NORMAL);
out:
	g_free (icon);
	g_free (remaining_text);
	return;
}

/**
 * gpm_manager_engine_just_laptop_battery:
 */
static gboolean
gpm_manager_engine_just_laptop_battery (GpmManager *manager)
{
	UpDevice *device;
	UpDeviceKind kind;
	GPtrArray *array;
	gboolean ret = TRUE;
	guint i;

	/* find if there are any other device types that mean we have to
	 * be more specific in our wording */
	array = gpm_engine_get_devices (manager->priv->engine);
	for (i=0; i<array->len; i++) {
		device = g_ptr_array_index (array, i);
		g_object_get (device, "kind", &kind, NULL);
		if (kind != UP_DEVICE_KIND_BATTERY) {
			ret = FALSE;
			break;
		}
	}
	g_ptr_array_unref (array);
	return ret;
}

/**
 * gpm_manager_engine_charge_low_cb:
 */
static void
gpm_manager_engine_charge_low_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	const gchar *title = NULL;
	gchar *message = NULL;
	gchar *remaining_text;
	gchar *icon = NULL;
	UpDeviceKind kind;
	gdouble percentage;
	gint64 time_to_empty;
	gboolean ret;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "percentage", &percentage,
		      "time-to-empty", &time_to_empty,
		      NULL);

	/* check to see if the batteries have not noticed we are on AC */
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (!manager->priv->on_battery) {
			egg_warning ("ignoring critically low message as we are not on battery power");
			goto out;
		}
	}

	if (kind == UP_DEVICE_KIND_BATTERY) {

		/* if the user has no other batteries, drop the "Laptop" wording */
		ret = gpm_manager_engine_just_laptop_battery (manager);
		if (ret) {
			/* TRANSLATORS: laptop battery low, and we only have one battery */
			title = _("Battery low");
		} else {
			/* TRANSLATORS: laptop battery low, and we have more than one kind of battery */
			title = _("Laptop battery low");
		}

		remaining_text = gpm_get_timestring (time_to_empty);

		/* TRANSLATORS: tell the user how much time they have got */
		message = g_strdup_printf (_("Approximately <b>%s</b> remaining (%.0f%%)"), remaining_text, percentage);

	} else if (kind == UP_DEVICE_KIND_UPS) {
		/* TRANSLATORS: UPS is starting to get a little low */
		title = _("UPS low");
		remaining_text = gpm_get_timestring (time_to_empty);

		/* TRANSLATORS: tell the user how much time they have got */
		message = g_strdup_printf (_("Approximately <b>%s</b> of remaining UPS backup power (%.0f%%)"),
					   remaining_text, percentage);
	} else if (kind == UP_DEVICE_KIND_MOUSE) {
		/* TRANSLATORS: mouse is getting a little low */
		title = _("Mouse battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Wireless mouse is low in power (%.0f%%)"), percentage);

	} else if (kind == UP_DEVICE_KIND_KEYBOARD) {
		/* TRANSLATORS: keyboard is getting a little low */
		title = _("Keyboard battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Wireless keyboard is low in power (%.0f%%)"), percentage);

	} else if (kind == UP_DEVICE_KIND_PDA) {
		/* TRANSLATORS: PDA is getting a little low */
		title = _("PDA battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("PDA is low in power (%.0f%%)"), percentage);

	} else if (kind == UP_DEVICE_KIND_PHONE) {
		/* TRANSLATORS: cell phone (mobile) is getting a little low */
		title = _("Cell phone battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Cell phone is low in power (%.0f%%)"), percentage);

#if UP_CHECK_VERSION(0,9,5)
	} else if (kind == UP_DEVICE_KIND_MEDIA_PLAYER) {
		/* TRANSLATORS: media player, e.g. mp3 is getting a little low */
		title = _("Media player battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Media player is low in power (%.0f%%)"), percentage);

	} else if (kind == UP_DEVICE_KIND_TABLET) {
		/* TRANSLATORS: graphics tablet, e.g. wacom is getting a little low */
		title = _("Tablet battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Tablet is low in power (%.0f%%)"), percentage);

	} else if (kind == UP_DEVICE_KIND_COMPUTER) {
		/* TRANSLATORS: computer, e.g. ipad is getting a little low */
		title = _("Attached computer battery low");

		/* TRANSLATORS: tell user more details */
		message = g_strdup_printf (_("Attached computer is low in power (%.0f%%)"), percentage);
#endif
	}

	/* get correct icon */
	icon = gpm_upower_get_device_icon (device);
	gpm_manager_notify (manager, &manager->priv->notification_warning_low, title, message, GPM_MANAGER_NOTIFY_TIMEOUT_LONG, icon, NOTIFY_URGENCY_NORMAL);
	gpm_manager_play (manager, GPM_MANAGER_SOUND_BATTERY_CAUTION, TRUE);
out:
	g_free (icon);
	g_free (message);
}

/**
 * gpm_manager_engine_charge_critical_cb:
 */
static void
gpm_manager_engine_charge_critical_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	const gchar *title = NULL;
	gchar *message = NULL;
	gchar *action;
	gchar *icon = NULL;
	UpDeviceKind kind;
	gdouble percentage;
	gint64 time_to_empty;
	GpmActionPolicy policy;
	gboolean ret;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "percentage", &percentage,
		      "time-to-empty", &time_to_empty,
		      NULL);

	/* check to see if the batteries have not noticed we are on AC */
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (!manager->priv->on_battery) {
			egg_warning ("ignoring critically low message as we are not on battery power");
			goto out;
		}
	}

	if (kind == UP_DEVICE_KIND_BATTERY) {

		/* if the user has no other batteries, drop the "Laptop" wording */
		ret = gpm_manager_engine_just_laptop_battery (manager);
		if (ret) {
			/* TRANSLATORS: laptop battery critically low, and only have one kind of battery */
			title = _("Battery critically low");
		} else {
			/* TRANSLATORS: laptop battery critically low, and we have more than one type of battery */
			title = _("Laptop battery critically low");
		}

		/* we have to do different warnings depending on the policy */
		action = mateconf_client_get_string (manager->priv->conf, GPM_CONF_ACTIONS_CRITICAL_BATT, NULL);
		policy = gpm_action_policy_from_string (action);

		/* use different text for different actions */
		if (policy == GPM_ACTION_POLICY_NOTHING) {
			/* TRANSLATORS: tell the use to insert the plug, as we're not going to do anything */
			message = g_strdup (_("Plug in your AC adapter to avoid losing data."));

		} else if (policy == GPM_ACTION_POLICY_SUSPEND) {
			/* TRANSLATORS: give the user a ultimatum */
			message = g_strdup_printf (_("Computer will suspend very soon unless it is plugged in."));

		} else if (policy == GPM_ACTION_POLICY_HIBERNATE) {
			/* TRANSLATORS: give the user a ultimatum */
			message = g_strdup_printf (_("Computer will hibernate very soon unless it is plugged in."));

		} else if (policy == GPM_ACTION_POLICY_SHUTDOWN) {
			/* TRANSLATORS: give the user a ultimatum */
			message = g_strdup_printf (_("Computer will shutdown very soon unless it is plugged in."));
		}

		g_free (action);
	} else if (kind == UP_DEVICE_KIND_UPS) {
		gchar *remaining_text;

		/* TRANSLATORS: the UPS is very low */
		title = _("UPS critically low");
		remaining_text = gpm_get_timestring (time_to_empty);

		/* TRANSLATORS: give the user a ultimatum */
		message = g_strdup_printf (_("Approximately <b>%s</b> of remaining UPS power (%.0f%%). "
					     "Restore AC power to your computer to avoid losing data."),
					   remaining_text, percentage);
		g_free (remaining_text);
	} else if (kind == UP_DEVICE_KIND_MOUSE) {
		/* TRANSLATORS: the mouse battery is very low */
		title = _("Mouse battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Wireless mouse is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);
	} else if (kind == UP_DEVICE_KIND_KEYBOARD) {
		/* TRANSLATORS: the keyboard battery is very low */
		title = _("Keyboard battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Wireless keyboard is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);
	} else if (kind == UP_DEVICE_KIND_PDA) {

		/* TRANSLATORS: the PDA battery is very low */
		title = _("PDA battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("PDA is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);

	} else if (kind == UP_DEVICE_KIND_PHONE) {

		/* TRANSLATORS: the cell battery is very low */
		title = _("Cell phone battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Cell phone is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);

#if UP_CHECK_VERSION(0,9,5)
	} else if (kind == UP_DEVICE_KIND_MEDIA_PLAYER) {

		/* TRANSLATORS: the cell battery is very low */
		title = _("Cell phone battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Media player is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);
	} else if (kind == UP_DEVICE_KIND_TABLET) {

		/* TRANSLATORS: the cell battery is very low */
		title = _("Tablet battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Tablet is very low in power (%.0f%%). "
					     "This device will soon stop functioning if not charged."),
					   percentage);
	} else if (kind == UP_DEVICE_KIND_COMPUTER) {

		/* TRANSLATORS: the cell battery is very low */
		title = _("Attached computer battery low");

		/* TRANSLATORS: the device is just going to stop working */
		message = g_strdup_printf (_("Attached computer is very low in power (%.0f%%). "
					     "The device will soon shutdown if not charged."),
					   percentage);
#endif
	}

	/* get correct icon */
	icon = gpm_upower_get_device_icon (device);
	gpm_manager_notify (manager, &manager->priv->notification_warning_low, title, message, GPM_MANAGER_NOTIFY_TIMEOUT_NEVER, icon, NOTIFY_URGENCY_CRITICAL);

	switch (kind) {

	case UP_DEVICE_KIND_BATTERY:
	case UP_DEVICE_KIND_UPS:
		egg_debug ("critical charge level reached, starting sound loop");
		gpm_manager_play_loop_start (manager,
					     GPM_MANAGER_SOUND_BATTERY_LOW,
					     TRUE,
					     GPM_MANAGER_CRITICAL_ALERT_TIMEOUT);
		break;

	default:
		gpm_manager_play (manager, GPM_MANAGER_SOUND_BATTERY_LOW, TRUE);
	}
out:
	g_free (icon);
	g_free (message);
}

/**
 * gpm_manager_engine_charge_action_cb:
 */
static void
gpm_manager_engine_charge_action_cb (GpmEngine *engine, UpDevice *device, GpmManager *manager)
{
	const gchar *title = NULL;
	gchar *action;
	gchar *message = NULL;
	gchar *icon = NULL;
	UpDeviceKind kind;
	GpmActionPolicy policy;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      NULL);

	/* check to see if the batteries have not noticed we are on AC */
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (!manager->priv->on_battery) {
			egg_warning ("ignoring critically low message as we are not on battery power");
			goto out;
		}
	}

	if (kind == UP_DEVICE_KIND_BATTERY) {

		/* TRANSLATORS: laptop battery is really, really, low */
		title = _("Laptop battery critically low");

		/* we have to do different warnings depending on the policy */
		action = mateconf_client_get_string (manager->priv->conf, GPM_CONF_ACTIONS_CRITICAL_BATT, NULL);
		policy = gpm_action_policy_from_string (action);

		/* use different text for different actions */
		if (policy == GPM_ACTION_POLICY_NOTHING) {
			/* TRANSLATORS: computer will shutdown without saving data */
			message = g_strdup (_("The battery is below the critical level and "
					      "this computer will <b>power-off</b> when the "
					      "battery becomes completely empty."));

		} else if (policy == GPM_ACTION_POLICY_SUSPEND) {
			/* TRANSLATORS: computer will suspend */
			message = g_strdup (_("The battery is below the critical level and "
					      "this computer is about to suspend.<br>"
					      "<b>NOTE:</b> A small amount of power is required "
					      "to keep your computer in a suspended state."));

		} else if (policy == GPM_ACTION_POLICY_HIBERNATE) {
			/* TRANSLATORS: computer will hibernate */
			message = g_strdup (_("The battery is below the critical level and "
					      "this computer is about to hibernate."));

		} else if (policy == GPM_ACTION_POLICY_SHUTDOWN) {
			/* TRANSLATORS: computer will just shutdown */
			message = g_strdup (_("The battery is below the critical level and "
					      "this computer is about to shutdown."));
		}

		g_free (action);

		/* wait 20 seconds for user-panic */
		g_timeout_add_seconds (20, (GSourceFunc) manager_critical_action_do, manager);

	} else if (kind == UP_DEVICE_KIND_UPS) {
		/* TRANSLATORS: UPS is really, really, low */
		title = _("UPS critically low");

		/* we have to do different warnings depending on the policy */
		action = mateconf_client_get_string (manager->priv->conf, GPM_CONF_ACTIONS_CRITICAL_UPS, NULL);
		policy = gpm_action_policy_from_string (action);

		/* use different text for different actions */
		if (policy == GPM_ACTION_POLICY_NOTHING) {
			/* TRANSLATORS: computer will shutdown without saving data */
			message = g_strdup (_("The UPS is below the critical level and "
				              "this computer will <b>power-off</b> when the "
				              "UPS becomes completely empty."));

		} else if (policy == GPM_ACTION_POLICY_HIBERNATE) {
			/* TRANSLATORS: computer will hibernate */
			message = g_strdup (_("The UPS is below the critical level and "
				              "this computer is about to hibernate."));

		} else if (policy == GPM_ACTION_POLICY_SHUTDOWN) {
			/* TRANSLATORS: computer will just shutdown */
			message = g_strdup (_("The UPS is below the critical level and "
				              "this computer is about to shutdown."));
		}

		/* wait 20 seconds for user-panic */
		g_timeout_add_seconds (20, (GSourceFunc) manager_critical_action_do, manager);

		g_free (action);
	}

	/* not all types have actions */
	if (title == NULL)
		return;

	/* get correct icon */
	icon = gpm_upower_get_device_icon (device);
	gpm_manager_notify (manager, &manager->priv->notification_warning_low,
			    title, message, GPM_MANAGER_NOTIFY_TIMEOUT_NEVER,
			    icon, NOTIFY_URGENCY_CRITICAL);
	gpm_manager_play (manager, GPM_MANAGER_SOUND_BATTERY_LOW, TRUE);
out:
	g_free (icon);
	g_free (message);
}

/**
 * gpm_manager_dpms_mode_changed_cb:
 * @mode: The DPMS mode, e.g. GPM_DPMS_MODE_OFF
 * @info: This class instance
 *
 * Log when the DPMS mode is changed.
 **/
static void
gpm_manager_dpms_mode_changed_cb (GpmDpms *dpms, GpmDpmsMode mode, GpmManager *manager)
{
	egg_debug ("DPMS mode changed: %d", mode);

	if (mode == GPM_DPMS_MODE_ON)
		egg_debug ("dpms on");
	else if (mode == GPM_DPMS_MODE_STANDBY)
		egg_debug ("dpms standby");
	else if (mode == GPM_DPMS_MODE_SUSPEND)
		egg_debug ("suspend");
	else if (mode == GPM_DPMS_MODE_OFF)
		egg_debug ("dpms off");

	gpm_manager_update_dpms_throttle (manager);
}

/*
 * gpm_manager_reset_just_resumed_cb
 */
static gboolean
gpm_manager_reset_just_resumed_cb (gpointer user_data)
{
	GpmManager *manager = GPM_MANAGER (user_data);

	if (manager->priv->notification_general != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_general);
	if (manager->priv->notification_warning_low != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_warning_low);
	if (manager->priv->notification_discharging != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_discharging);
	if (manager->priv->notification_fully_charged != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_fully_charged);

	manager->priv->just_resumed = FALSE;
	return FALSE;
}

/**
 * gpm_manager_control_resume_cb
 **/
static void
gpm_manager_control_resume_cb (GpmControl *control, GpmControlAction action, GpmManager *manager)
{
	manager->priv->just_resumed = TRUE;
	g_timeout_add_seconds (1, gpm_manager_reset_just_resumed_cb, manager);
}

/**
 * gpm_manager_init:
 * @manager: This class instance
 **/
static void
gpm_manager_init (GpmManager *manager)
{
	gboolean check_type_cpu;
	gint timeout;
	DBusGConnection *connection;
	GError *error = NULL;
	guint version;

	manager->priv = GPM_MANAGER_GET_PRIVATE (manager);
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

	/* init to unthrottled */
	manager->priv->screensaver_ac_throttle_id = 0;
	manager->priv->screensaver_dpms_throttle_id = 0;
	manager->priv->screensaver_lid_throttle_id = 0;

	manager->priv->critical_alert_timeout_id = 0;
	manager->priv->critical_alert_loop_props = NULL;

	/* init to not just_resumed */
	manager->priv->just_resumed = FALSE;

	/* don't apply policy when not active, so listen to ConsoleKit */
	manager->priv->console = egg_console_kit_new ();

	/* this is a singleton, so we keep a master copy open here */
	manager->priv->prefs_server = gpm_prefs_server_new ();

	manager->priv->notification_general = NULL;
	manager->priv->notification_warning_low = NULL;
	manager->priv->notification_discharging = NULL;
	manager->priv->notification_fully_charged = NULL;
	manager->priv->disks = gpm_disks_new ();
	manager->priv->conf = mateconf_client_get_default ();
	manager->priv->client = up_client_new ();
	g_signal_connect (manager->priv->client, "changed",
			  G_CALLBACK (gpm_manager_client_changed_cb), manager);

	/* use libmatenotify */
	notify_init (GPM_NAME);

	/* watch mate-power-manager keys */
	mateconf_client_add_dir (manager->priv->conf, GPM_CONF_DIR,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	mateconf_client_notify_add (manager->priv->conf, GPM_CONF_DIR,
				 (MateConfClientNotifyFunc) gpm_conf_mateconf_key_changed_cb,
				 manager, NULL, NULL);

	/* check to see if the user has installed the schema properly */
	version = mateconf_client_get_int (manager->priv->conf, GPM_CONF_SCHEMA_VERSION, NULL);
	if (version != GPM_CONF_SCHEMA_ID) {
		gpm_manager_notify (manager, &manager->priv->notification_general,
				    /* TRANSLATORS: there was in install problem */
				    _("Install problem!"),
				    /* TRANSLATORS: the MateConf schema was not installed properly */
				    _("The configuration defaults for MATE Power Manager have not been installed correctly.\n"
				      "Please contact your computer administrator."),
				    GPM_MANAGER_NOTIFY_TIMEOUT_LONG,
				    GTK_STOCK_DIALOG_WARNING,
				    NOTIFY_URGENCY_NORMAL);
		egg_error ("no mateconf schema installed!");
	}

	/* coldplug so we are in the correct state at startup */
	g_object_get (manager->priv->client,
		      "on-battery", &manager->priv->on_battery,
		      NULL);

	manager->priv->button = gpm_button_new ();
	g_signal_connect (manager->priv->button, "button-pressed",
			  G_CALLBACK (gpm_manager_button_pressed_cb), manager);

	/* try and start an interactive service */
	manager->priv->screensaver = gpm_screensaver_new ();
#if 0
	g_signal_connect (manager->priv->screensaver, "auth-request",
			  G_CALLBACK (gpm_manager_screensaver_auth_request_cb), manager);
#endif

	/* try an start an interactive service */
	manager->priv->backlight = gpm_backlight_new ();
	if (manager->priv->backlight != NULL) {
		/* add the new brightness lcd DBUS interface */
		dbus_g_object_type_install_info (GPM_TYPE_BACKLIGHT,
						 &dbus_glib_gpm_backlight_object_info);
		dbus_g_connection_register_g_object (connection, GPM_DBUS_PATH_BACKLIGHT,
						     G_OBJECT (manager->priv->backlight));
	}

	manager->priv->idle = gpm_idle_new ();
	g_signal_connect (manager->priv->idle, "idle-changed",
			  G_CALLBACK (gpm_manager_idle_changed_cb), manager);

	/* set up the check_type_cpu, so we can disable the CPU load check */
	check_type_cpu = mateconf_client_get_bool (manager->priv->conf, GPM_CONF_IDLE_CHECK_CPU, NULL);
	gpm_idle_set_check_cpu (manager->priv->idle, check_type_cpu);

	manager->priv->dpms = gpm_dpms_new ();
	g_signal_connect (manager->priv->dpms, "mode-changed",
			  G_CALLBACK (gpm_manager_dpms_mode_changed_cb), manager);

	/* use the control object */
	egg_debug ("creating new control instance");
	manager->priv->control = gpm_control_new ();
	g_signal_connect (manager->priv->control, "resume",
			  G_CALLBACK (gpm_manager_control_resume_cb), manager);

	egg_debug ("creating new tray icon");
	manager->priv->tray_icon = gpm_tray_icon_new ();

	/* keep a reference for the notifications */
	manager->priv->status_icon = gpm_tray_icon_get_status_icon (manager->priv->tray_icon);

	gpm_manager_sync_policy_sleep (manager);

	manager->priv->engine = gpm_engine_new ();
	g_signal_connect (manager->priv->engine, "perhaps-recall",
			  G_CALLBACK (gpm_manager_engine_perhaps_recall_cb), manager);
	g_signal_connect (manager->priv->engine, "low-capacity",
			  G_CALLBACK (gpm_manager_engine_low_capacity_cb), manager);
	g_signal_connect (manager->priv->engine, "icon-changed",
			  G_CALLBACK (gpm_manager_engine_icon_changed_cb), manager);
	g_signal_connect (manager->priv->engine, "summary-changed",
			  G_CALLBACK (gpm_manager_engine_summary_changed_cb), manager);
	g_signal_connect (manager->priv->engine, "fully-charged",
			  G_CALLBACK (gpm_manager_engine_fully_charged_cb), manager);
	g_signal_connect (manager->priv->engine, "discharging",
			  G_CALLBACK (gpm_manager_engine_discharging_cb), manager);
	g_signal_connect (manager->priv->engine, "charge-low",
			  G_CALLBACK (gpm_manager_engine_charge_low_cb), manager);
	g_signal_connect (manager->priv->engine, "charge-critical",
			  G_CALLBACK (gpm_manager_engine_charge_critical_cb), manager);
	g_signal_connect (manager->priv->engine, "charge-action",
			  G_CALLBACK (gpm_manager_engine_charge_action_cb), manager);

	/* set disk spindown threshold */
	timeout = gpm_manager_get_spindown_timeout (manager);
	gpm_disks_set_spindown_timeout (manager->priv->disks, timeout);

	/* update ac throttle */
	gpm_manager_update_ac_throttle (manager);
}

/**
 * gpm_manager_finalize:
 * @object: The object to finalize
 *
 * Finalise the manager, by unref'ing all the depending modules.
 **/
static void
gpm_manager_finalize (GObject *object)
{
	GpmManager *manager;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_MANAGER (object));

	manager = GPM_MANAGER (object);

	g_return_if_fail (manager->priv != NULL);

	/* close any notifications (also unrefs them) */
	if (manager->priv->notification_general != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_general);
	if (manager->priv->notification_warning_low != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_warning_low);
	if (manager->priv->notification_discharging != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_discharging);
	if (manager->priv->notification_fully_charged != NULL)
		gpm_manager_notify_close (manager, manager->priv->notification_fully_charged);
	if (manager->priv->critical_alert_timeout_id != 0)
		g_source_remove (manager->priv->critical_alert_timeout_id);

	g_object_unref (manager->priv->conf);
	g_object_unref (manager->priv->disks);
	g_object_unref (manager->priv->dpms);
	g_object_unref (manager->priv->idle);
	g_object_unref (manager->priv->engine);
	g_object_unref (manager->priv->tray_icon);
	g_object_unref (manager->priv->screensaver);
	g_object_unref (manager->priv->prefs_server);
	g_object_unref (manager->priv->control);
	g_object_unref (manager->priv->button);
	g_object_unref (manager->priv->backlight);
	g_object_unref (manager->priv->console);
	g_object_unref (manager->priv->client);
	g_object_unref (manager->priv->status_icon);

	G_OBJECT_CLASS (gpm_manager_parent_class)->finalize (object);
}

/**
 * gpm_manager_new:
 *
 * Return value: a new GpmManager object.
 **/
GpmManager *
gpm_manager_new (void)
{
	GpmManager *manager;
	manager = g_object_new (GPM_TYPE_MANAGER, NULL);
	return GPM_MANAGER (manager);
}
