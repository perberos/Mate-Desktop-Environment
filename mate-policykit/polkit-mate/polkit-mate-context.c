/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-context.c : Convenience functions for using PolicyKit
 * from GTK+ and MATE applications.
 *
 * Copyright (C) 2007 David Zeuthen, <david@fubar.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <polkit/polkit.h>
#include <polkit-dbus/polkit-dbus.h>

#include "polkit-mate-context.h"

/**
 * SECTION:polkit-mate-context
 * @short_description: Convenience functions for using PolicyKit from GTK+ and MATE applications.
 *
 * This class provides convenience functions for using PolicyKit from
 * GTK+ and MATE applications including setting up main loop
 * integration and system bus connections. Rather than using
 * callbacks, GObject signals are provided when external factors
 * change (e.g. the PolicyKit.conf configuration file changes or
 * ConsoleKit reports activity changes).
 *
 * Actual usage of PolicyKit is still through the main PolicyKit API
 * through the public pk_context and pk_tracker variables.
 *
 * This class is implemented as a singleton meaning that several
 * callers will share the underlying #PolKitContext and #PolKitTracker
 * objects. Do not use any of the life cycle methods of these objects;
 * only use them to gather information.
 **/

#define POLKIT_MATE_CONTEXT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), POLKIT_MATE_TYPE_CONTEXT, PolKitMateContextPrivate))

struct _PolKitMateContextPrivate 
{
        DBusGConnection *system_bus;
};

enum 
{
        CONFIG_CHANGED,
        CONSOLE_KIT_DB_CHANGED,
        LAST_SIGNAL
};

/* our static singleton instance */
static PolKitMateContext *_singleton = NULL;

G_DEFINE_TYPE (PolKitMateContext, polkit_mate_context, G_TYPE_OBJECT)

static GObjectClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };

static void
polkit_mate_context_init (PolKitMateContext *context)
{
        context->priv = POLKIT_MATE_CONTEXT_GET_PRIVATE (context);
}

static void
polkit_mate_context_finalize (GObject *object)
{
        PolKitMateContext *context;

        context = POLKIT_MATE_CONTEXT (object);

        /* Remove the match rules we added; pass NULL for error so we
         * don't block; we can't really properly handle the error
         * anyway...
         */
	dbus_bus_remove_match (dbus_g_connection_get_connection (_singleton->priv->system_bus),
                               "type='signal'"
                               ",interface='"DBUS_INTERFACE_DBUS"'"
                               ",sender='"DBUS_SERVICE_DBUS"'"
                               ",member='NameOwnerChanged'",
                               NULL);
	dbus_bus_remove_match (dbus_g_connection_get_connection (_singleton->priv->system_bus),
                               "type='signal',sender='org.freedesktop.ConsoleKit'",
                               NULL);

        if (context->pk_context != NULL) {
                polkit_context_unref (context->pk_context);
        }

        if (context->pk_tracker != NULL) {
                polkit_tracker_unref (context->pk_tracker);
        }

        _singleton = NULL;

        G_OBJECT_CLASS (polkit_mate_context_parent_class)->finalize (object);
}

static void
polkit_mate_context_class_init (PolKitMateContextClass *klass)
{
        GObjectClass *gobject_class;

        parent_class = g_type_class_peek_parent (klass);
        gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->finalize = polkit_mate_context_finalize;

        /**
         * PolKitMateContext::config-changed:
         * @context: the object
         *
         * The ::config-changed signal is emitted when PolicyKit
         * configuration (e.g. /etc/PolicyKit/PolicyKit.conf or
         * .policy files) changes content.
         *
         * As this is one contributing factor to what answer PolicyKit
         * will return, the caller should act on this signal and query
         * PolicyKit for any actions it cares about.
         **/
        signals [CONFIG_CHANGED] =
                g_signal_new ("config-changed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateContextClass, config_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /**
         * PolKitMateContext::console-kit-db-changed:
         * @context: the object
         *
         * The ::console-kit-db-changed signal is emitted when
         * ConsoleKit configuration changes; e.g. when a session
         * becomes active or inactive.
         *
         * As this is one contributing factor to what answer PolicyKit
         * will return, the caller should act on this signal and query
         * PolicyKit for any actions it cares about.
         **/
        signals [CONSOLE_KIT_DB_CHANGED] =
                g_signal_new ("console-kit-db-changed",
                              G_TYPE_FROM_CLASS (gobject_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (PolKitMateContextClass, console_kit_db_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_type_class_add_private (gobject_class, sizeof (PolKitMateContextPrivate));
}


static gboolean
io_watch_have_data (GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
        int fd;
        PolKitContext *pk_context = user_data;
        fd = g_io_channel_unix_get_fd (channel);
        polkit_context_io_func (pk_context, fd);
        return TRUE;
}

static int 
io_add_watch (PolKitContext *pk_context, int fd)
{
        guint id = 0;
        GIOChannel *channel;
        channel = g_io_channel_unix_new (fd);
        if (channel == NULL)
                goto out;
        id = g_io_add_watch (channel, G_IO_IN, io_watch_have_data, pk_context);
        if (id == 0) {
                g_io_channel_unref (channel);
                goto out;
        }
        g_io_channel_unref (channel);
out:
        return id;
}

static void 
io_remove_watch (PolKitContext *pk_context, int watch_id)
{
        g_source_remove (watch_id);
}


static void
pk_config_changed (PolKitContext *pk_context, void *user_data)
{
        PolKitMateContext *context = POLKIT_MATE_CONTEXT (user_data);

        /* g_debug ("ggg PolicyKit config changed"); */
        g_signal_emit (context, signals [CONFIG_CHANGED], 0);
}


static DBusHandlerResult
_filter (DBusConnection *connection, DBusMessage *message, void *user_data)
{
        PolKitMateContext *context = POLKIT_MATE_CONTEXT (user_data);

        /*  pass NameOwnerChanged signals from the bus and ConsoleKit to PolKitTracker */
        if (dbus_message_is_signal (message, DBUS_INTERFACE_DBUS, "NameOwnerChanged") ||
            (dbus_message_get_interface (message) != NULL &&
             g_str_has_prefix (dbus_message_get_interface (message), "org.freedesktop.ConsoleKit"))) {
                if (polkit_tracker_dbus_func (context->pk_tracker, message)) {
                        /* The ConsoleKit database has changed */
                        /* g_debug ("ggg ConsoleKit database changed"); */
                        g_signal_emit (context, signals [CONSOLE_KIT_DB_CHANGED], 0);
                }
        }

        /* other filters might want to process this message too */
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/**
 * polkit_mate_context_get:
 * @error: return location for error
 *
 * Returns a #PolKitMateContext object. The context is a global
 * singleton that may be shared with other callers of this function.
 *
 * This operation can fail if the system message bus is not available.
 *
 * When done with using this object, call g_object_unref(). This is
 * such that resources can be freed when all callers have unreffed it.
 *
 * Returns: a new #PolKitMateContext or NULL if error is set
 */
PolKitMateContext *
polkit_mate_context_get (GError **error)
{
        DBusError dbus_error;
        PolKitError *pk_error;

        if (_singleton != NULL)
                return g_object_ref (_singleton);

        /* g_debug ("Constructing singleton"); */

        _singleton = g_object_new (POLKIT_MATE_TYPE_CONTEXT, NULL);

        if ((_singleton->priv->system_bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, error)) == NULL) {
                goto error;
        }

        _singleton->pk_context = polkit_context_new ();
        polkit_context_set_io_watch_functions (_singleton->pk_context, io_add_watch, io_remove_watch);
        polkit_context_set_config_changed (_singleton->pk_context, pk_config_changed, _singleton);

        pk_error = NULL;
        if (!polkit_context_init (_singleton->pk_context, &pk_error)) {
                g_warning ("Failed to initialize PolicyKit context: %s", polkit_error_get_error_message (pk_error));
                if (error != NULL) {
                        *error = g_error_new_literal (POLKIT_MATE_CONTEXT_ERROR,
                                                      POLKIT_MATE_CONTEXT_ERROR_FAILED,
                                                      polkit_error_get_error_message (pk_error));
                }
                polkit_error_free (pk_error);
                goto error;
        }

        /* TODO FIXME: I'm pretty sure dbus-glib blows in a way that
         * we can't say we're interested in all signals from all
         * members on all interfaces for a given service... So we do
         * this..
         */

        dbus_error_init (&dbus_error);

        /* need to listen to NameOwnerChanged */
	dbus_bus_add_match (dbus_g_connection_get_connection (_singleton->priv->system_bus),
			    "type='signal'"
			    ",interface='"DBUS_INTERFACE_DBUS"'"
			    ",sender='"DBUS_SERVICE_DBUS"'"
			    ",member='NameOwnerChanged'",
			    &dbus_error);

        if (dbus_error_is_set (&dbus_error)) {
                dbus_set_g_error (error, &dbus_error);
                dbus_error_free (&dbus_error);
                goto error;
        }

        /* need to listen to ConsoleKit signals */
	dbus_bus_add_match (dbus_g_connection_get_connection (_singleton->priv->system_bus),
			    "type='signal',sender='org.freedesktop.ConsoleKit'",
			    &dbus_error);

        if (dbus_error_is_set (&dbus_error)) {
                dbus_set_g_error (error, &dbus_error);
                dbus_error_free (&dbus_error);
                goto error;
        }

        if (!dbus_connection_add_filter (dbus_g_connection_get_connection (_singleton->priv->system_bus), 
                                         _filter, 
                                         _singleton, 
                                         NULL)) {
                *error = g_error_new_literal (POLKIT_MATE_CONTEXT_ERROR,
                                              POLKIT_MATE_CONTEXT_ERROR_FAILED,
                                              "Cannot add D-Bus filter");
                goto error;
        }        

        _singleton->pk_tracker = polkit_tracker_new ();
        polkit_tracker_set_system_bus_connection (_singleton->pk_tracker, 
                                                  dbus_g_connection_get_connection (_singleton->priv->system_bus));
        polkit_tracker_init (_singleton->pk_tracker);

        return _singleton;

error:
        g_object_unref (_singleton);
        return NULL;
}

GQuark
polkit_mate_context_error_quark (void)
{
        return g_quark_from_static_string ("polkit-mate-context-error-quark");
}
