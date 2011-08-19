/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/***************************************************************************
 *
 * polkit-mate-auth.c : Show authentication dialogs to gain privileges
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

#include "polkit-mate-auth.h"

/**
 * SECTION:polkit-mate-auth
 * @title: Authentication Dialogs
 * @short_description: Show authentication dialogs to gain privileges
 *
 * Show authentication dialogs to gain privileges.
 *
 **/


typedef struct {
        PolKitAction *action;
        PolKitMateAuthCB callback;
        gpointer user_data;
} CallClosure;

static void
_notify_callback (DBusGProxy *proxy, DBusGProxyCall *call, void *user_data)
{
        GError *error;
        CallClosure *c = (CallClosure *) user_data;
        gboolean gained_privilege;

        error = NULL;
        if (!dbus_g_proxy_end_call (proxy, call, &error, G_TYPE_BOOLEAN, &gained_privilege, G_TYPE_INVALID)) {
                gained_privilege = FALSE;
        }

        /* perform the callback */
        c->callback (c->action, gained_privilege, error, c->user_data);

        g_object_unref (proxy);
        polkit_action_unref (c->action);
}

/**
 * polkit_mate_auth_obtain:
 * @action: The #PolKitAction to make the user authenticate for
 * @xid: X11 window ID for the window that the dialog will be transient for. If there is no window, pass 0.
 * @pid: Process ID of process to grant authorization to. Normally one wants to pass result of getpid().
 * @callback: Function to call when authentication is done
 * @user_data: Data to pass to the callback function
 * @error: Return location for error
 *
 * Applications can use this function to show a dialog for the user
 * asking her to authenticate in order to gain privileges to do the
 * given action. The authentication, for security reasons, happens in
 * a separate process; this function is merely a wrapper around a
 * D-Bus call across the session message bus to the
 * <literal>org.freedesktop.PolicyKit.AuthenticationAgent</literal>
 * service. Depending on the setup, this may be the Authentication
 * Agent shipped with PolicyKit-mate or it may be another
 * implementation. For example, if the user is in KDE it may be an
 * Authentication Agent using the Qt toolkit.
 *
 * The Authentication Agent shipped with PolicyKit-mate is described
 * in <link linkend="ref-auth-daemon">this section</link>.
 *
 * This function is similar to the polkit_auth_obtain() function
 * supplied in <literal>libpolkit-dbus</literal> except that this
 * function is asynchronous.
 *
 * Returns: #TRUE if the authentication session was scheduled to
 * start. #FALSE if error is set (and no callback will be made).
 */
gboolean 
polkit_mate_auth_obtain (PolKitAction *action, 
                          guint xid,
                          pid_t pid,
                          PolKitMateAuthCB callback, 
                          gpointer user_data, 
                          GError **error)
{
        char *polkit_action_id;
        gboolean ret;
        CallClosure *c;
        DBusGConnection *session_bus;
        DBusGProxy *polkit_mate_proxy;

        ret = FALSE;

        if ((session_bus = dbus_g_bus_get (DBUS_BUS_SESSION, error)) == NULL) {
                goto error;
        }

        /* TODO: this can fail.. */
        polkit_action_get_action_id (action, &polkit_action_id);

	polkit_mate_proxy = dbus_g_proxy_new_for_name (session_bus,
                                                        "org.freedesktop.PolicyKit.AuthenticationAgent", /* bus name */
                                                        "/",                                             /* object */
                                                        "org.freedesktop.PolicyKit.AuthenticationAgent");/* interface */

        c = g_new0 (CallClosure, 1);
        c->action = polkit_action_ref (action);
        c->callback = callback;
        c->user_data = user_data;

        dbus_g_proxy_begin_call_with_timeout (polkit_mate_proxy,
                                              "ObtainAuthorization",
                                              _notify_callback,
                                              c,
                                              g_free,
                                              INT_MAX,
                                              /* parameters: */
                                              G_TYPE_STRING, polkit_action_id,  /* action_id */
                                              G_TYPE_UINT, xid,                 /* X11 window ID */
                                              G_TYPE_UINT, pid,                 /* process id */
                                              G_TYPE_INVALID);

        ret = TRUE;
error:
        return ret;
}

