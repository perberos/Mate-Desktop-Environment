/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2005 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2010 Red Hat, Inc.
 * Copyright (C) 2011 Perberos <perberos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <X11/Xproto.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "daemon.h"
#include "engines.h"
#include "stack.h"
#include "sound.h"
#include "notificationdaemon-dbus-glue.h"

#define MAX_NOTIFICATIONS 20

#define IMAGE_SIZE 48
#define IDLE_SECONDS 30
#define NOTIFICATION_BUS_NAME      "org.freedesktop.Notifications"
#define NOTIFICATION_BUS_PATH      "/org/freedesktop/Notifications"

#define NW_GET_NOTIFY_ID(nw) \
	(GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(nw), "_notify_id")))
#define NW_GET_NOTIFY_SENDER(nw) \
	(g_object_get_data(G_OBJECT(nw), "_notify_sender"))
#define NW_GET_DAEMON(nw) \
	(g_object_get_data(G_OBJECT(nw), "_notify_daemon"))

typedef struct {
	NotifyStackLocation type;
	const char* identifier;
} PopupNotifyStackLocation;

const PopupNotifyStackLocation popup_stack_locations[] = {
	{NOTIFY_STACK_LOCATION_TOP_LEFT, "top_left"},
	{NOTIFY_STACK_LOCATION_TOP_RIGHT, "top_right"},
	{NOTIFY_STACK_LOCATION_BOTTOM_LEFT, "bottom_left"},
	{NOTIFY_STACK_LOCATION_BOTTOM_RIGHT, "bottom_right"},
	{NOTIFY_STACK_LOCATION_UNKNOWN, NULL}
};

#define POPUP_STACK_DEFAULT_INDEX 3     /* XXX Hack! */

typedef struct {
	NotifyDaemon* daemon;
	GTimeVal expiration;
	GTimeVal paused_diff;
	guint id;
	GtkWindow* nw;
	Window src_window_xid;
	guint has_timeout: 1;
	guint paused: 1;
} NotifyTimeout;

typedef struct {
	NotifyStack** stacks;
	int n_stacks;
	Atom workarea_atom;
} NotifyScreen;

struct _NotifyDaemonPrivate {
	guint next_id;
	guint timeout_source;
	guint exit_timeout_source;
	GHashTable* idle_reposition_notify_ids;
	GHashTable* monitored_window_hash;
	GHashTable* notification_hash;
	gboolean url_clicked_lock;

	guint mateconf_cnxn;

	NotifyStackLocation stack_location;
	NotifyScreen** screens;
	int n_screens;
};

typedef struct {
	guint id;
	NotifyDaemon* daemon;
} _NotifyPendingClose;

static DBusConnection* dbus_conn = NULL;

static void notify_daemon_finalize(GObject* object);
static void _notification_destroyed_cb(GtkWindow* nw, NotifyDaemon* daemon);
static void _close_notification(NotifyDaemon* daemon, guint id, gboolean hide_notification, NotifydClosedReason reason);
static GdkFilterReturn _notify_x11_filter(GdkXEvent* xevent, GdkEvent* event, NotifyDaemon* daemon);
static void _emit_closed_signal(GtkWindow* nw, NotifydClosedReason reason);
static void _action_invoked_cb(GtkWindow* nw, const char* key);
static NotifyStackLocation get_stack_location_from_string(const char* slocation);
static void sync_notification_position(NotifyDaemon* daemon, GtkWindow* nw, Window source);
static void monitor_notification_source_windows(NotifyDaemon* daemon, NotifyTimeout* nt, Window source);

G_DEFINE_TYPE(NotifyDaemon, notify_daemon, G_TYPE_OBJECT);

static void notify_daemon_class_init(NotifyDaemonClass* daemon_class)
{
	GObjectClass* object_class = G_OBJECT_CLASS(daemon_class);

	object_class->finalize = notify_daemon_finalize;

	g_type_class_add_private(daemon_class, sizeof(NotifyDaemonPrivate));
}

static void _notify_timeout_destroy(NotifyTimeout* nt)
{
	/*
	 * Disconnect the destroy handler to avoid a loop since the id
	 * won't be removed from the hash table before the widget is
	 * destroyed.
	 */
	g_signal_handlers_disconnect_by_func(nt->nw, _notification_destroyed_cb, nt->daemon);
	gtk_widget_destroy(GTK_WIDGET(nt->nw));
	g_free(nt);
}

static gboolean do_exit(gpointer user_data)
{
	//g_debug("Exiting due to inactivity");
	exit(1);

	return FALSE;
}

static void add_exit_timeout(NotifyDaemon* daemon)
{
    if (daemon->priv->exit_timeout_source > 0)
    {
		return;
	}

	daemon->priv->exit_timeout_source = g_timeout_add_seconds(IDLE_SECONDS, do_exit, NULL);
}

static void remove_exit_timeout(NotifyDaemon* daemon)
{
	if (daemon->priv->exit_timeout_source == 0)
	{
		return;
	}

	g_source_remove(daemon->priv->exit_timeout_source);
	daemon->priv->exit_timeout_source = 0;
}

static void create_stack_for_monitor(NotifyDaemon* daemon, GdkScreen* screen, int monitor_num)
{
	int screen_num = gdk_screen_get_number(screen);
	NotifyScreen* nscreen = daemon->priv->screens[screen_num];

	nscreen->stacks[monitor_num] = notify_stack_new(daemon, screen, monitor_num, daemon->priv->stack_location);
}

static void on_screen_monitors_changed(GdkScreen* screen, NotifyDaemon* daemon)
{
	NotifyScreen* nscreen;
	int screen_num;
	int n_monitors;
	int i;

	screen_num = gdk_screen_get_number(screen);
	nscreen = daemon->priv->screens[screen_num];

	n_monitors = gdk_screen_get_n_monitors(screen);

	if (n_monitors > nscreen->n_stacks)
	{
		/* grow */
		nscreen->stacks = g_renew(NotifyStack *, nscreen->stacks, n_monitors);

		/* add more stacks */
		for (i = nscreen->n_stacks; i < n_monitors; i++)
		{
			create_stack_for_monitor(daemon, screen, i);
		}

		nscreen->n_stacks = n_monitors;
	}
	else if (n_monitors < nscreen->n_stacks)
	{
		NotifyStack* last_stack;

		last_stack = nscreen->stacks[n_monitors - 1];

		/* transfer items before removing stacks */
		for (i = n_monitors; i < nscreen->n_stacks; i++)
		{
			NotifyStack* stack = nscreen->stacks[i];
			GList* windows = g_list_copy(notify_stack_get_windows(stack));
			GList* l;

			for (l = windows; l != NULL; l = l->next)
			{
				/* skip removing the window from the old stack since it will try
				 * to unrealize the window.
				 * And the stack is going away anyhow. */
				notify_stack_add_window(last_stack, l->data, TRUE);
			}

			g_list_free(windows);
			notify_stack_destroy(stack);
			nscreen->stacks[i] = NULL;
		}

		/* remove the extra stacks */
		nscreen->stacks = g_renew(NotifyStack*, nscreen->stacks, n_monitors);
		nscreen->n_stacks = n_monitors;
	}
}

static void create_stacks_for_screen(NotifyDaemon* daemon, GdkScreen    *screen)
{
	NotifyScreen* nscreen;
	int screen_num;
	int i;

	screen_num = gdk_screen_get_number(screen);
	nscreen = daemon->priv->screens[screen_num];

	nscreen->n_stacks = gdk_screen_get_n_monitors(screen);

	nscreen->stacks = g_renew(NotifyStack*, nscreen->stacks, nscreen->n_stacks);

	for (i = 0; i < nscreen->n_stacks; i++)
	{
		create_stack_for_monitor(daemon, screen, i);
	}
}

static GdkFilterReturn screen_xevent_filter(GdkXEvent* xevent, GdkEvent* event, NotifyScreen* nscreen)
{
	XEvent* xev = (XEvent*) xevent;

	if (xev->type == PropertyNotify && xev->xproperty.atom == nscreen->workarea_atom)
	{
		int i;

		for (i = 0; i < nscreen->n_stacks; i++)
		{
			notify_stack_queue_update_position(nscreen->stacks[i]);
		}
	}

	return GDK_FILTER_CONTINUE;
}

static void create_screens(NotifyDaemon* daemon)
{
	GdkDisplay* display;
	int i;

	g_assert(daemon->priv->screens == NULL);

	display = gdk_display_get_default();
	daemon->priv->n_screens = gdk_display_get_n_screens(display);

	daemon->priv->screens = g_new0(NotifyScreen *, daemon->priv->n_screens);

	for (i = 0; i < daemon->priv->n_screens; i++)
	{
		GdkScreen* screen;
		GdkWindow* gdkwindow;

		screen = gdk_display_get_screen(display, i);
		g_signal_connect(screen, "monitors-changed", G_CALLBACK(on_screen_monitors_changed), daemon);

		daemon->priv->screens[i] = g_new0(NotifyScreen, 1);

		#if GTK_CHECK_VERSION(3, 0, 0)
			daemon->priv->screens[i]->workarea_atom = XInternAtom(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), "_NET_WORKAREA", True);
		#else
			daemon->priv->screens[i]->workarea_atom = XInternAtom(GDK_DISPLAY(), "_NET_WORKAREA", True);
		#endif

		gdkwindow = gdk_screen_get_root_window(screen);
		gdk_window_add_filter(gdkwindow, (GdkFilterFunc) screen_xevent_filter, daemon->priv->screens[i]);
		gdk_window_set_events(gdkwindow, gdk_window_get_events(gdkwindow) | GDK_PROPERTY_CHANGE_MASK);

		create_stacks_for_screen(daemon, gdk_display_get_screen(display, i));
	}
}

static void on_popup_location_changed(MateConfClient* client, guint cnxn_id, MateConfEntry* entry, NotifyDaemon* daemon)
{
	NotifyStackLocation stack_location;
	const char* slocation;
	MateConfValue* value;
	int i;

	value = mateconf_entry_get_value(entry);
	slocation = (value != NULL ? mateconf_value_get_string(value) : NULL);

	if (slocation != NULL && *slocation != '\0')
	{
		stack_location = get_stack_location_from_string(slocation);
	}
	else
	{
		mateconf_client_set_string(client, MATECONF_KEY_POPUP_LOCATION, popup_stack_locations[POPUP_STACK_DEFAULT_INDEX].identifier, NULL);

		stack_location = NOTIFY_STACK_LOCATION_DEFAULT;
	}

	daemon->priv->stack_location = stack_location;

	for (i = 0; i < daemon->priv->n_screens; i++)
	{
		int j;

		for (j = 0; j < daemon->priv->n_screens; j++)
		{
			NotifyStack* stack;
			stack = daemon->priv->screens[i]->stacks[j];
			notify_stack_set_location(stack, stack_location);
		}
	}
}

static void notify_daemon_init(NotifyDaemon* daemon)
{
	MateConfClient* client;
	char* location;

	daemon->priv = G_TYPE_INSTANCE_GET_PRIVATE(daemon, NOTIFY_TYPE_DAEMON, NotifyDaemonPrivate);

	daemon->priv->next_id = 1;
	daemon->priv->timeout_source = 0;

	add_exit_timeout(daemon);

	client = mateconf_client_get_default();
	mateconf_client_add_dir(client, MATECONF_KEY_DAEMON, MATECONF_CLIENT_PRELOAD_NONE, NULL);

	location = mateconf_client_get_string(client, MATECONF_KEY_POPUP_LOCATION, NULL);
	daemon->priv->stack_location = get_stack_location_from_string(location);
	g_free(location);

	daemon->priv->mateconf_cnxn = mateconf_client_notify_add(client, MATECONF_KEY_POPUP_LOCATION, (MateConfClientNotifyFunc) on_popup_location_changed, daemon, NULL, NULL);
	g_object_unref(client);

	daemon->priv->n_screens = 0;
	daemon->priv->screens = NULL;

	create_screens(daemon);

	daemon->priv->idle_reposition_notify_ids = g_hash_table_new(NULL, NULL);
	daemon->priv->monitored_window_hash = g_hash_table_new(NULL, NULL);
	daemon->priv->notification_hash = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, (GDestroyNotify) _notify_timeout_destroy);
}

static void destroy_screens(NotifyDaemon* daemon)
{
	GdkDisplay* display;
	int i;
	int j;

	display = gdk_display_get_default();

	for (i = 0; i < daemon->priv->n_screens; i++)
	{
		GdkScreen* screen;
		GdkWindow* gdkwindow;

		screen = gdk_display_get_screen(display, i);
		g_signal_handlers_disconnect_by_func(screen, G_CALLBACK(on_screen_monitors_changed), daemon);

		gdkwindow = gdk_screen_get_root_window(screen);
		gdk_window_remove_filter(gdkwindow, (GdkFilterFunc) screen_xevent_filter, daemon->priv->screens[i]);

		for (j = 0; i < daemon->priv->screens[i]->n_stacks; j++)
		{
			notify_stack_destroy(daemon->priv->screens[i]->stacks[j]);
			daemon->priv->screens[i]->stacks[j] = NULL;
		}

		g_free(daemon->priv->screens[i]->stacks);
	}

	g_free(daemon->priv->screens);
	daemon->priv->screens = NULL;
}

static void notify_daemon_finalize(GObject* object)
{
	NotifyDaemon* daemon;
	MateConfClient* client;

	daemon = NOTIFY_DAEMON(object);

	client = mateconf_client_get_default();
	mateconf_client_notify_remove(client, daemon->priv->mateconf_cnxn);
	g_object_unref(client);

	if (g_hash_table_size(daemon->priv->monitored_window_hash) > 0)
	{
		gdk_window_remove_filter(NULL, (GdkFilterFunc) _notify_x11_filter, daemon);
	}

	remove_exit_timeout(daemon);

	g_hash_table_destroy(daemon->priv->monitored_window_hash);
	g_hash_table_destroy(daemon->priv->idle_reposition_notify_ids);
	g_hash_table_destroy(daemon->priv->notification_hash);

	destroy_screens(daemon);

	g_free(daemon->priv);

	G_OBJECT_CLASS(notify_daemon_parent_class)->finalize(object);
}

static NotifyStackLocation get_stack_location_from_string(const char* slocation)
{
	NotifyStackLocation stack_location = NOTIFY_STACK_LOCATION_DEFAULT;

	if (slocation == NULL || *slocation == '\0')
	{
		return NOTIFY_STACK_LOCATION_DEFAULT;
	}
	else
	{
		const PopupNotifyStackLocation* l;

		for (l = popup_stack_locations; l->type != NOTIFY_STACK_LOCATION_UNKNOWN; l++)
		{
			if (!strcmp(slocation, l->identifier))
			{
				stack_location = l->type;
			}
		}
	}

	return stack_location;
}

static DBusMessage* create_signal(GtkWindow* nw, const char* signal_name)
{
	guint id;
	char* dest;
	DBusMessage* message;

	id = NW_GET_NOTIFY_ID(nw);
	dest = NW_GET_NOTIFY_SENDER(nw);

	g_assert(dest != NULL);

	message = dbus_message_new_signal(NOTIFICATION_BUS_PATH, NOTIFICATION_BUS_NAME, signal_name);

	dbus_message_set_destination(message, dest);
	dbus_message_append_args(message, DBUS_TYPE_UINT32, &id, DBUS_TYPE_INVALID);

	return message;
}

static void _action_invoked_cb(GtkWindow* nw, const char *key)
{
	NotifyDaemon* daemon;
	guint id;
	DBusMessage* message;

	daemon = NW_GET_DAEMON(nw);
	id = NW_GET_NOTIFY_ID(nw);

	message = create_signal(nw, "ActionInvoked");
	dbus_message_append_args(message, DBUS_TYPE_STRING, &key, DBUS_TYPE_INVALID);

	dbus_connection_send(dbus_conn, message, NULL);
	dbus_message_unref(message);

	_close_notification(daemon, id, TRUE, NOTIFYD_CLOSED_USER);
}

static void _emit_closed_signal(GtkWindow* nw, NotifydClosedReason reason)
{
	DBusMessage* message = create_signal(nw, "NotificationClosed");
	dbus_message_append_args(message, DBUS_TYPE_UINT32, &reason, DBUS_TYPE_INVALID);
	dbus_connection_send(dbus_conn, message, NULL);
	dbus_message_unref(message);
}

static void _close_notification(NotifyDaemon* daemon, guint id, gboolean hide_notification, NotifydClosedReason reason)
{
	NotifyDaemonPrivate* priv = daemon->priv;
	NotifyTimeout* nt;

	nt = (NotifyTimeout*) g_hash_table_lookup(priv->notification_hash, &id);

	if (nt != NULL)
	{
		_emit_closed_signal(nt->nw, reason);

		if (hide_notification)
		{
			theme_hide_notification(nt->nw);
		}

		g_hash_table_remove(priv->notification_hash, &id);

		if (g_hash_table_size(daemon->priv->notification_hash) == 0)
		{
			add_exit_timeout(daemon);
		}
	}
}

static gboolean _close_notification_not_shown(_NotifyPendingClose* data)
{
	_close_notification(data->daemon, data->id, TRUE, NOTIFYD_CLOSED_RESERVED);
	g_object_unref(data->daemon);
	g_free(data);

	return FALSE;
}

static void _notification_destroyed_cb(GtkWindow* nw, NotifyDaemon* daemon)
{
	/*
	 * This usually won't happen, but can if notification-daemon dies before
	 * all notifications are closed. Mark them as expired.
	 */
	_close_notification(daemon, NW_GET_NOTIFY_ID(nw), FALSE, NOTIFYD_CLOSED_EXPIRED);
}

typedef struct {
	NotifyDaemon* daemon;
	gint id;
} IdleRepositionData;

static gboolean idle_reposition_notification(IdleRepositionData* data)
{
	NotifyDaemon* daemon;
	NotifyTimeout* nt;
	gint notify_id;

	daemon = data->daemon;
	notify_id = data->id;

	/* Look up the timeout, if it's completed we don't need to do anything */
	nt = (NotifyTimeout*) g_hash_table_lookup(daemon->priv->notification_hash, &notify_id);

	if (nt != NULL)
	{
		sync_notification_position(daemon, nt->nw, nt->src_window_xid);
	}

	g_hash_table_remove(daemon->priv->idle_reposition_notify_ids, GINT_TO_POINTER(notify_id));
	g_object_unref(daemon);
	g_free(data);

	return FALSE;
}

static void _queue_idle_reposition_notification(NotifyDaemon* daemon, gint notify_id)
{
	IdleRepositionData* data;
	gpointer orig_key;
	gpointer value;
	guint idle_id;

	/* Do we already have an idle update pending? */
	if (g_hash_table_lookup_extended(daemon->priv->idle_reposition_notify_ids, GINT_TO_POINTER(notify_id), &orig_key, &value))
	{
		return;
	}

	data = g_new0(IdleRepositionData, 1);
	data->daemon = g_object_ref(daemon);
	data->id = notify_id;

	/* We do this as a short timeout to avoid repositioning spam */
	idle_id = g_timeout_add_full(G_PRIORITY_LOW, 50, (GSourceFunc) idle_reposition_notification, data, NULL);
	g_hash_table_insert(daemon->priv->idle_reposition_notify_ids, GINT_TO_POINTER(notify_id), GUINT_TO_POINTER(idle_id));
}

static GdkFilterReturn _notify_x11_filter(GdkXEvent* xevent, GdkEvent* event, NotifyDaemon* daemon)
{
	XEvent* xev;
	gpointer orig_key;
	gpointer value;
	gint notify_id;
	NotifyTimeout* nt;

	xev = (XEvent*) xevent;

	if (xev->xany.type == DestroyNotify)
	{
		g_hash_table_remove(daemon->priv->monitored_window_hash, GUINT_TO_POINTER(xev->xany.window));

		if (g_hash_table_size(daemon->priv->monitored_window_hash) == 0)
		{
			gdk_window_remove_filter(NULL, (GdkFilterFunc) _notify_x11_filter, daemon);
		}

		return GDK_FILTER_CONTINUE;
	}

	if (!g_hash_table_lookup_extended(daemon->priv->monitored_window_hash, GUINT_TO_POINTER(xev->xany.window), &orig_key, &value))
	{
		return GDK_FILTER_CONTINUE;
	}

	notify_id = GPOINTER_TO_INT(value);

	if (xev->xany.type == ConfigureNotify || xev->xany.type == MapNotify)
	{
		_queue_idle_reposition_notification(daemon, notify_id);
	}
	else if (xev->xany.type == ReparentNotify)
	{
		nt = (NotifyTimeout *) g_hash_table_lookup(daemon->priv->notification_hash, &notify_id);

		if (nt == NULL)
		{
			return GDK_FILTER_CONTINUE;
		}

		/*
		 * If the window got reparented, we need to start monitoring the
		 * new parents.
		 */
		monitor_notification_source_windows(daemon, nt, nt->src_window_xid);
		sync_notification_position(daemon, nt->nw, nt->src_window_xid);
	}

	return GDK_FILTER_CONTINUE;
}

static void _mouse_entered_cb(GtkWindow* nw, GdkEventCrossing* event, NotifyDaemon* daemon)
{
	NotifyTimeout* nt;
	guint id;
	GTimeVal now;

	if (event->detail == GDK_NOTIFY_INFERIOR)
	{
		return;
	}

	id = NW_GET_NOTIFY_ID(nw);
	nt = (NotifyTimeout*) g_hash_table_lookup(daemon->priv->notification_hash, &id);

	nt->paused = TRUE;
	g_get_current_time(&now);

	nt->paused_diff.tv_usec = nt->expiration.tv_usec - now.tv_usec;
	nt->paused_diff.tv_sec = nt->expiration.tv_sec - now.tv_sec;

	if (nt->paused_diff.tv_usec < 0)
	{
		nt->paused_diff.tv_usec += G_USEC_PER_SEC;
		nt->paused_diff.tv_sec--;
	}
}

static void _mouse_exitted_cb(GtkWindow* nw, GdkEventCrossing* event, NotifyDaemon* daemon)
{
	if (event->detail == GDK_NOTIFY_INFERIOR)
	{
		return;
	}

	guint id = NW_GET_NOTIFY_ID(nw);
	NotifyTimeout* nt = (NotifyTimeout*) g_hash_table_lookup(daemon->priv->notification_hash, &id);

	nt->paused = FALSE;
}

static gboolean _is_expired(gpointer key, NotifyTimeout* nt, gboolean* phas_more_timeouts)
{
	time_t now_time;
	time_t expiration_time;
	GTimeVal now;

	if (!nt->has_timeout)
	{
		return FALSE;
	}

	g_get_current_time(&now);

	expiration_time = (nt->expiration.tv_sec * 1000) + (nt->expiration.tv_usec / 1000);
	now_time = (now.tv_sec * 1000) + (now.tv_usec / 1000);

	if (now_time > expiration_time)
	{
		theme_notification_tick(nt->nw, 0);
		_emit_closed_signal(nt->nw, NOTIFYD_CLOSED_EXPIRED);
		return TRUE;
	}
	else if (nt->paused)
	{
		nt->expiration.tv_usec = nt->paused_diff.tv_usec + now.tv_usec;
		nt->expiration.tv_sec = nt->paused_diff.tv_sec + now.tv_sec;

		if (nt->expiration.tv_usec >= G_USEC_PER_SEC)
		{
			nt->expiration.tv_usec -= G_USEC_PER_SEC;
			nt->expiration.tv_sec++;
		}
	}
	else
	{
		theme_notification_tick(nt->nw, expiration_time - now_time);
	}

	*phas_more_timeouts = TRUE;

	return FALSE;
}

static gboolean _check_expiration(NotifyDaemon* daemon)
{
	gboolean has_more_timeouts = FALSE;

	g_hash_table_foreach_remove(daemon->priv->notification_hash, (GHRFunc) _is_expired, (gpointer) &has_more_timeouts);

	if (!has_more_timeouts)
	{
		daemon->priv->timeout_source = 0;

		if (g_hash_table_size (daemon->priv->notification_hash) == 0)
		{
			add_exit_timeout(daemon);
		}
	}

	return has_more_timeouts;
}

static void _calculate_timeout(NotifyDaemon* daemon, NotifyTimeout* nt, int timeout)
{
	if (timeout == 0)
	{
		nt->has_timeout = FALSE;
	}
	else
	{
		nt->has_timeout = TRUE;

		if (timeout == -1)
		{
			timeout = NOTIFY_DAEMON_DEFAULT_TIMEOUT;
		}

		theme_set_notification_timeout(nt->nw, timeout);

		glong usec = timeout * 1000;  /* convert from msec to usec */

		/*
		 * If it's less than 0, wrap around back to MAXLONG.
		 * g_time_val_add() requires a glong, and anything larger than
		 * MAXLONG will be treated as a negative value.
		 */
		if (usec < 0)
		{
			usec = G_MAXLONG;
		}

		g_get_current_time(&nt->expiration);
		g_time_val_add(&nt->expiration, usec);

		if (daemon->priv->timeout_source == 0)
		{
			daemon->priv->timeout_source = g_timeout_add(100, (GSourceFunc) _check_expiration, daemon);
		}
	}
}

static NotifyTimeout* _store_notification(NotifyDaemon* daemon, GtkWindow* nw, int timeout)
{
	NotifyDaemonPrivate* priv = daemon->priv;
	NotifyTimeout* nt;
	guint id = 0;

	do {
		id = priv->next_id;

		if (id != UINT_MAX)
		{
			priv->next_id++;
		}
		else
		{
			priv->next_id = 1;
		}

		if (g_hash_table_lookup (priv->notification_hash, &id) != NULL)
		{
			id = 0;
		}

	} while (id == 0);

	nt = g_new0(NotifyTimeout, 1);
	nt->id = id;
	nt->nw = nw;
	nt->daemon = daemon;

	_calculate_timeout(daemon, nt, timeout);

	g_hash_table_insert(priv->notification_hash, g_memdup(&id, sizeof(guint)), nt);
	remove_exit_timeout(daemon);

	return nt;
}

static GdkPixbuf* _notify_daemon_pixbuf_from_data_hint(GValue* icon_data)
{
	const guchar* data = NULL;
	gboolean has_alpha;
	int bits_per_sample;
	int width;
	int height;
	int rowstride;
	int n_channels;
	gsize expected_len;
	GdkPixbuf* pixbuf;
	GValueArray* image_struct;
	GValue* value;
	GArray* tmp_array;
	GType struct_type;

	struct_type = dbus_g_type_get_struct("GValueArray", G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_INT, dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR), G_TYPE_INVALID);

	if (!G_VALUE_HOLDS (icon_data, struct_type))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected a GValue of type GValueArray");
		return NULL;
	}

	image_struct = (GValueArray *) g_value_get_boxed (icon_data);
	value = g_value_array_get_nth (image_struct, 0);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 0 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_INT))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 0 of the GValueArray to be of type int");
		return NULL;
	}

	width = g_value_get_int (value);
	value = g_value_array_get_nth (image_struct, 1);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 1 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_INT))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 1 of the GValueArray to be of type int");
		return NULL;
	}

	height = g_value_get_int (value);
	value = g_value_array_get_nth (image_struct, 2);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 2 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_INT))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 2 of the GValueArray to be of type int");
		return NULL;
	}

	rowstride = g_value_get_int (value);
	value = g_value_array_get_nth (image_struct, 3);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 3 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_BOOLEAN))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 3 of the GValueArray to be of type gboolean");
		return NULL;
	}

	has_alpha = g_value_get_boolean (value);
	value = g_value_array_get_nth (image_struct, 4);

	if (value == NULL) {
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 4 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_INT))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 4 of the GValueArray to be of type int");
		return NULL;
	}

	bits_per_sample = g_value_get_int (value);
	value = g_value_array_get_nth (image_struct, 5);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 5 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, G_TYPE_INT))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 5 of the GValueArray to be of type int");
		return NULL;
	}

	n_channels = g_value_get_int (value);
	value = g_value_array_get_nth (image_struct, 6);

	if (value == NULL)
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 6 of the GValueArray to exist");
		return NULL;
	}

	if (!G_VALUE_HOLDS (value, dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR)))
	{
		g_warning ("_notify_daemon_pixbuf_from_data_hint expected position 6 of the GValueArray to be of type GArray");
		return NULL;
	}

	tmp_array = (GArray*) g_value_get_boxed(value);
	expected_len = (height - 1) * rowstride + width * ((n_channels * bits_per_sample + 7) / 8);

	if (expected_len != tmp_array->len)
	{
		g_warning("_notify_daemon_pixbuf_from_data_hint expected image data to be of length %" G_GSIZE_FORMAT " but got a " "length of %u", expected_len, tmp_array->len);
		return NULL;
	}

	data = (guchar *) g_memdup(tmp_array->data, tmp_array->len);
	pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, has_alpha, bits_per_sample, width, height, rowstride, (GdkPixbufDestroyNotify) g_free, NULL);

	return pixbuf;
}

static GdkPixbuf* _notify_daemon_pixbuf_from_path(const char* path)
{
	GdkPixbuf* pixbuf = NULL;

	if (!strncmp (path, "file://", 7) || *path == '/')
	{
		if (!strncmp (path, "file://", 7))
		{
			path += 7;
		}

			/* Load file */
		pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	}
	else
	{
		/* Load icon theme icon */
		GtkIconTheme *theme;
		GtkIconInfo  *icon_info;

		theme = gtk_icon_theme_get_default ();
		icon_info = gtk_icon_theme_lookup_icon (theme, path, IMAGE_SIZE, GTK_ICON_LOOKUP_USE_BUILTIN);

		if (icon_info != NULL)
		{
			gint icon_size;

			icon_size = MIN (IMAGE_SIZE, gtk_icon_info_get_base_size (icon_info));

			if (icon_size == 0)
			{
				icon_size = IMAGE_SIZE;
			}

			pixbuf = gtk_icon_theme_load_icon (theme, path, icon_size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

			gtk_icon_info_free (icon_info);
		}

		if (pixbuf == NULL)
		{
			/* Well... maybe this is a file afterall. */
			pixbuf = gdk_pixbuf_new_from_file (path, NULL);
		}
	}

	return pixbuf;
}

static void window_clicked_cb(GtkWindow* nw, GdkEventButton* button, NotifyDaemon* daemon)
{
	if (daemon->priv->url_clicked_lock)
	{
		daemon->priv->url_clicked_lock = FALSE;
		return;
	}

	_action_invoked_cb (nw, "default");
	_close_notification (daemon, NW_GET_NOTIFY_ID (nw), TRUE, NOTIFYD_CLOSED_USER);
}

static void url_clicked_cb(GtkWindow* nw, const char *url)
{
	NotifyDaemon* daemon;
	char* escaped_url;
	char* cmd = NULL;

	daemon = NW_GET_DAEMON(nw);

	/* Somewhat of a hack.. */
	daemon->priv->url_clicked_lock = TRUE;

	escaped_url = g_shell_quote (url);

	if (g_find_program_in_path ("gvfs-open") != NULL)
	{
		cmd = g_strdup_printf ("gvfs-open %s", escaped_url);
	}
	else if (g_find_program_in_path ("xdg-open") != NULL)
	{
		cmd = g_strdup_printf ("xdg-open %s", escaped_url);
	}
	else if (g_find_program_in_path ("firefox") != NULL)
	{
		cmd = g_strdup_printf ("firefox %s", escaped_url);
	}
	else
	{
		g_warning ("Unable to find a browser.");
	}

	g_free (escaped_url);

	if (cmd != NULL)
	{
		g_spawn_command_line_async (cmd, NULL);
		g_free (cmd);
	}
}

static gboolean screensaver_active(GtkWidget* nw)
{
	GError* error = NULL;
	gboolean active = FALSE;

	DBusGConnection* connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		//g_warning("Failed to get dbus connection: %s", error->message);
		g_error_free(error);
		goto out;
	}

	DBusGProxy* gs_proxy = dbus_g_proxy_new_for_name(connection, "org.mate.ScreenSaver", "/", "org.mate.ScreenSaver");

	if (!dbus_g_proxy_call(gs_proxy, "GetActive", &error, G_TYPE_INVALID, G_TYPE_BOOLEAN, &active, G_TYPE_INVALID))
	{
		//g_warning("Failed to call mate-screensaver: %s", error->message);
		g_error_free(error);
	}

	g_object_unref(gs_proxy);

	out:

	return active;
}

static gboolean fullscreen_window_exists(GtkWidget* nw)
{
	WnckScreen* wnck_screen;
	WnckWorkspace* wnck_workspace;
	GList* l;

	#if (GDK_PIXBUF_MAJOR == 2) && (DK_PIXBUF_MINOR < 22)
		wnck_screen = wnck_screen_get(GDK_SCREEN_XNUMBER(gdk_window_get_screen(gtk_widget_get_window(nw))));
	#else
		wnck_screen = wnck_screen_get(GDK_SCREEN_XNUMBER(gdk_drawable_get_screen(GDK_DRAWABLE(GTK_WIDGET(nw)->window))));
	#endif

    wnck_screen_force_update (wnck_screen);

	wnck_workspace = wnck_screen_get_active_workspace (wnck_screen);

	for (l = wnck_screen_get_windows_stacked (wnck_screen); l != NULL; l = l->next)
	{
		WnckWindow *wnck_win = (WnckWindow *) l->data;

		if (wnck_window_is_on_workspace (wnck_win, wnck_workspace) && wnck_window_is_fullscreen (wnck_win) && wnck_window_is_active (wnck_win))
		{
			/*
			 * Sanity check if the window is _really_ fullscreen to
			 * work around a bug in libwnck that doesn't get all
			 * unfullscreen events.
			 */
			int sw = wnck_screen_get_width (wnck_screen);
			int sh = wnck_screen_get_height (wnck_screen);
			int x, y, w, h;

			wnck_window_get_geometry (wnck_win, &x, &y, &w, &h);

			if (sw == w && sh == h)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

static Window get_window_parent(Display* display, Window window, Window* root)
{
	Window parent;
	Window* children = NULL;
	guint nchildren;
	gboolean result;

	gdk_error_trap_push();
	result = XQueryTree(display, window, root, &parent, &children, &nchildren);

	if (gdk_error_trap_pop() || !result)
	{
		return None;
	}

	if (children)
	{
		XFree(children);
	}

	return parent;
}

/*
 * Recurse over X Window and parents, up to root, and start watching them
 * for position changes.
 */
static void monitor_notification_source_windows(NotifyDaemon  *daemon, NotifyTimeout *nt, Window source)
{
	Display* display;
	Window root = None;
	Window parent;

	display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

	/* Start monitoring events if necessary.  We don't want to
	   filter events unless we absolutely have to. */
	if (g_hash_table_size (daemon->priv->monitored_window_hash) == 0)
	{
		gdk_window_add_filter (NULL, (GdkFilterFunc) _notify_x11_filter, daemon);
	}

	/* Store the window in the timeout */
	g_assert (nt != NULL);
	nt->src_window_xid = source;

	for (parent = get_window_parent (display, source, &root); parent != None && root != parent; parent = get_window_parent (display, parent, &root))
	{
		XSelectInput (display, parent, StructureNotifyMask);

		g_hash_table_insert(daemon->priv->monitored_window_hash, GUINT_TO_POINTER (parent), GINT_TO_POINTER (nt->id));
	}
}

/* Use a source X Window ID to reposition a notification. */
static void sync_notification_position(NotifyDaemon* daemon, GtkWindow* nw, Window source)
{
	Display* display;
	Status result;
	Window root;
	Window child;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;

	display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default ());

	gdk_error_trap_push();

	/* Get the root for this window */
	result = XGetGeometry(display, source, &root, &x, &y, &width, &height, &border_width, &depth);

	if (gdk_error_trap_pop () || !result)
	{
		return;
	}

	/*
	 * Now calculate the offset coordinates for the source window from
	 * the root.
	 */
	gdk_error_trap_push ();
	result = XTranslateCoordinates (display, source, root, 0, 0, &x, &y, &child);
	if (gdk_error_trap_pop () || !result)
	{
		return;
	}

	x += width / 2;
	y += height / 2;

	theme_set_notification_arrow (nw, TRUE, x, y);
	theme_move_notification (nw, x, y);
	theme_show_notification (nw);

	/*
	 * We need to manually queue a draw here as the default theme recalculates
	 * its position in the draw handler and moves the window (which seems
	 * fairly broken), so just calling move/show above isn't enough to cause
	 * its position to be calculated.
	 */
	gtk_widget_queue_draw (GTK_WIDGET (nw));
}

GQuark notify_daemon_error_quark(void)
{
	static GQuark q = 0;

	if (q == 0)
	{
		q = g_quark_from_static_string ("notification-daemon-error-quark");
	}

	return q;
}

gboolean notify_daemon_notify_handler(NotifyDaemon* daemon, const char* app_name, guint id, const char* icon, const char* summary, const char* body, char** actions, GHashTable* hints, int timeout, DBusGMethodInvocation* context)
{
	NotifyDaemonPrivate *priv = daemon->priv;
	NotifyTimeout* nt = NULL;
	GtkWindow* nw = NULL;
	GValue* data;
	gboolean use_pos_data = FALSE;
	gboolean new_notification = FALSE;
	gint x = 0;
	gint y = 0;
	Window window_xid = None;
	guint return_id;
	char* sender;
	char* sound_file = NULL;
	gboolean sound_enabled;
	gint i;
	GdkPixbuf* pixbuf;
	MateConfClient* mateconf_client;

	if (g_hash_table_size (priv->notification_hash) > MAX_NOTIFICATIONS)
	{
		GError *error;

		error = g_error_new (notify_daemon_error_quark (), 1, _("Exceeded maximum number of notifications"));
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return TRUE;
	}

	if (id > 0)
	{
		nt = (NotifyTimeout *) g_hash_table_lookup (priv->notification_hash, &id);

		if (nt != NULL)
		{
			nw = nt->nw;
		}
		else
		{
			id = 0;
		}
	}

	if (nw == NULL)
	{
		nw = theme_create_notification (url_clicked_cb);
		g_object_set_data (G_OBJECT (nw), "_notify_daemon", daemon);
		gtk_widget_realize (GTK_WIDGET (nw));
		new_notification = TRUE;

		g_signal_connect (G_OBJECT (nw), "button-release-event", G_CALLBACK (window_clicked_cb), daemon);
		g_signal_connect (G_OBJECT (nw), "destroy", G_CALLBACK (_notification_destroyed_cb), daemon);
		g_signal_connect (G_OBJECT (nw), "enter-notify-event", G_CALLBACK (_mouse_entered_cb), daemon);
		g_signal_connect (G_OBJECT (nw), "leave-notify-event", G_CALLBACK (_mouse_exitted_cb), daemon);
	}
	else
	{
		theme_clear_notification_actions (nw);
	}

	theme_set_notification_text (nw, summary, body);
	theme_set_notification_hints (nw, hints);

	/*
	 *XXX This needs to handle file URIs and all that.
	 */


	if ((data = (GValue *) g_hash_table_lookup (hints, "window-xid")) != NULL)
	{
		window_xid = (Window) g_value_get_uint (data);
	}
	/* deal with x, and y hints */
	else if ((data = (GValue *) g_hash_table_lookup (hints, "x")) != NULL)
	{
		x = g_value_get_int (data);

		if ((data = (GValue *) g_hash_table_lookup (hints, "y")) != NULL)
		{
			y = g_value_get_int (data);
			use_pos_data = TRUE;
		}
	}

	/* Deal with sound hints */
	mateconf_client = mateconf_client_get_default ();
	sound_enabled = mateconf_client_get_bool (mateconf_client, MATECONF_KEY_SOUND_ENABLED, NULL);
	g_object_unref (mateconf_client);

	data = (GValue *) g_hash_table_lookup (hints, "suppress-sound");

	if (data != NULL)
	{
		if (G_VALUE_HOLDS_BOOLEAN (data))
		{
			sound_enabled = !g_value_get_boolean (data);
		}
		else if (G_VALUE_HOLDS_INT (data))
		{
			sound_enabled = (g_value_get_int (data) != 0);
		}
		else
		{
			g_warning ("suppress-sound is of type %s (expected bool or int)\n", g_type_name (G_VALUE_TYPE (data)));
		}
	}

	if (sound_enabled)
	{
		data = (GValue *) g_hash_table_lookup (hints, "sound-file");

		if (data != NULL)
		{
			sound_file = g_value_dup_string (data);

			if (*sound_file == '\0' || !g_file_test (sound_file, G_FILE_TEST_EXISTS))
			{
				g_free (sound_file);
				sound_file = NULL;
			}
		}
	}

	/* set up action buttons */
	for (i = 0; actions[i] != NULL; i += 2)
	{
		char* l = actions[i + 1];

		if (l == NULL)
		{
			g_warning ("Label not found for action %s. The protocol specifies that a label must follow an action in the actions array", actions[i]);

			break;
		}

		if (strcasecmp (actions[i], "default"))
		{
			theme_add_notification_action (nw, l, actions[i], G_CALLBACK (_action_invoked_cb));
		}
	}

	pixbuf = NULL;

	if ((data = (GValue *) g_hash_table_lookup (hints, "image_data")))
	{
		pixbuf = _notify_daemon_pixbuf_from_data_hint (data);
	}
	else if ((data = (GValue *) g_hash_table_lookup (hints, "image_path")))
	{
		if (G_VALUE_HOLDS_STRING (data))
		{
			const char *path = g_value_get_string (data);
			pixbuf = _notify_daemon_pixbuf_from_path (path);
		}
		else
		{
			g_warning ("notify_daemon_notify_handler expected image_path hint to be of type string");
		}
	}
	else if (*icon != '\0')
	{
		pixbuf = _notify_daemon_pixbuf_from_path (icon);
	}
	else if ((data = (GValue *) g_hash_table_lookup (hints, "icon_data")))
	{
		g_warning("\"icon_data\" hint is deprecated, please use \"image_data\" instead");
		pixbuf = _notify_daemon_pixbuf_from_data_hint (data);
	}

	if (pixbuf != NULL)
	{
		theme_set_notification_icon (nw, pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	}

	if (window_xid != None && !theme_get_always_stack (nw))
	{
		/*
		 * Do nothing here if we were passed an XID; we'll call
		 * sync_notification_position later.
		 */
	}
	else if (use_pos_data && !theme_get_always_stack (nw))
	{
		/*
		 * Typically, the theme engine will set its own position based on
		 * the arrow X, Y hints. However, in case, move the notification to
		 * that position.
		 */
		theme_set_notification_arrow (nw, TRUE, x, y);
		theme_move_notification (nw, x, y);
	}
	else
	{
		int monitor_num;
		int screen_num;
		GdkScreen* screen;
		gint x, y;

		theme_set_notification_arrow (nw, FALSE, 0, 0);

		gdk_display_get_pointer (gdk_display_get_default (), &screen, &x, &y, NULL);
		screen_num = gdk_screen_get_number (screen);
		monitor_num = gdk_screen_get_monitor_at_point (screen, x, y);

		if (monitor_num >= priv->screens[screen_num]->n_stacks)
		{
			/* screw it - dump it on the last one we'll get
			 a monitors-changed signal soon enough*/
			monitor_num = priv->screens[screen_num]->n_stacks - 1;
		}

		notify_stack_add_window (priv->screens[screen_num]->stacks[monitor_num], nw, new_notification);
	}

	if (id == 0)
	{
		nt = _store_notification (daemon, nw, timeout);
		return_id = nt->id;
	}
	else
	{
		return_id = id;
	}

	/*
	 * If we have a source Window XID, start monitoring the tree
	 * for changes, and reposition the window based on the source
	 * window.  We need to do this after return_id is calculated.
	 */
	if (window_xid != None && !theme_get_always_stack (nw))
	{
		monitor_notification_source_windows (daemon, nt, window_xid);
		sync_notification_position (daemon, nw, window_xid);
	}

	if (!screensaver_active (GTK_WIDGET (nw)) && !fullscreen_window_exists (GTK_WIDGET (nw)))
	{
		theme_show_notification (nw);

		if (sound_file != NULL)
		{
			sound_play_file (GTK_WIDGET (nw), sound_file);
		}
	}
	else
	{
		_NotifyPendingClose* data;

		/* The notification was not shown, so queue up a close
		 * for it */
		data = g_new0 (_NotifyPendingClose, 1);
		data->id = id;
		data->daemon = g_object_ref (daemon);
		g_idle_add ((GSourceFunc) _close_notification_not_shown, data);
	}

	g_free (sound_file);

	sender = dbus_g_method_get_sender (context);

	g_object_set_data (G_OBJECT (nw), "_notify_id", GUINT_TO_POINTER (return_id));
	g_object_set_data_full (G_OBJECT (nw), "_notify_sender", sender, (GDestroyNotify) g_free);

	if (nt)
	{
		_calculate_timeout (daemon, nt, timeout);
	}

	dbus_g_method_return (context, return_id);

	return TRUE;
}

gboolean notify_daemon_close_notification_handler(NotifyDaemon* daemon, guint id, GError** error)
{
	if (id == 0)
	{
		g_set_error (error, notify_daemon_error_quark (), 100, _("%u is not a valid notification ID"), id);
		return FALSE;
	}
	else
	{
		_close_notification (daemon, id, TRUE, NOTIFYD_CLOSED_API);
		return TRUE;
	}
}

gboolean notify_daemon_get_capabilities(NotifyDaemon* daemon, char*** caps)
{
	GPtrArray* a = g_ptr_array_new ();
	g_ptr_array_add (a, g_strdup ("actions"));
	g_ptr_array_add (a, g_strdup ("body"));
	g_ptr_array_add (a, g_strdup ("body-hyperlinks"));
	g_ptr_array_add (a, g_strdup ("body-markup"));
	g_ptr_array_add (a, g_strdup ("icon-static"));
	g_ptr_array_add (a, g_strdup ("sound"));
	g_ptr_array_add (a, NULL);
	char** _caps = (char**) g_ptr_array_free (a, FALSE);

	*caps = _caps;

	return TRUE;
}

gboolean notify_daemon_get_server_information(NotifyDaemon *daemon, char** out_name, char** out_vendor, char** out_version, char** out_spec_ver)
{
	*out_name = g_strdup("Notification Daemon");
	*out_vendor = g_strdup("MATE");
	*out_version = g_strdup(PACKAGE_VERSION);
	*out_spec_ver = g_strdup("1.1");

	return TRUE;
}

int main(int argc, char** argv)
{
	NotifyDaemon* daemon;
	DBusGConnection* connection;
	DBusGProxy* bus_proxy;
	GError* error;
	gboolean res;
	guint request_name_result;

	g_log_set_always_fatal(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);

	gtk_init(&argc, &argv);

	error = NULL;
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

	if (connection == NULL)
	{
		g_printerr("Failed to open connection to bus: %s\n", error->message);
		g_error_free(error);
		exit(1);
	}

	dbus_conn = dbus_g_connection_get_connection(connection);

	dbus_g_object_type_install_info(NOTIFY_TYPE_DAEMON, &dbus_glib_notification_daemon_object_info);

	bus_proxy = dbus_g_proxy_new_for_name(connection, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus");

	res = dbus_g_proxy_call(bus_proxy, "RequestName", &error, G_TYPE_STRING, NOTIFICATION_BUS_NAME, G_TYPE_UINT, 0, G_TYPE_INVALID, G_TYPE_UINT, &request_name_result, G_TYPE_INVALID);

	if (! res || request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		if (error != NULL)
		{
			g_warning("Failed to acquire name %s: %s", NOTIFICATION_BUS_NAME, error->message);
			g_error_free(error);
		}
		else
		{
			g_warning("Failed to acquire name %s", NOTIFICATION_BUS_NAME);
		}

		goto out;
	}

	daemon = g_object_new(NOTIFY_TYPE_DAEMON, NULL);

	dbus_g_connection_register_g_object(connection, "/org/freedesktop/Notifications", G_OBJECT(daemon));

	gtk_main();

	g_object_unref(daemon);

	out:

	return 0;
}
