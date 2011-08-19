/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 Raffaele Sandrini
 * Copyright (C) 2005 Red Hat, Inc.
 * Copyright (C) 2002, 2003 George Lebl
 * Copyright (C) 2001 Queen of England,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *      Raffaele Sandrini <rasa@gmx.ch>
 *      George Lebl <jirka@5z.com>
 *      Mark McLoughlin <mark@skynet.ie>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <X11/Xauth.h>
#include <gdk/gdk.h>

#include "gdm.h"

#define GDM_PROTOCOL_UPDATE_INTERVAL 1 /* seconds */

#define GDM_PROTOCOL_SOCKET_PATH "/var/run/gdm_socket"

#define GDM_PROTOCOL_MSG_CLOSE         "CLOSE"
#define GDM_PROTOCOL_MSG_VERSION       "VERSION"
#define GDM_PROTOCOL_MSG_AUTHENTICATE  "AUTH_LOCAL"
#define GDM_PROTOCOL_MSG_QUERY_ACTION  "QUERY_LOGOUT_ACTION"
#define GDM_PROTOCOL_MSG_SET_ACTION    "SET_SAFE_LOGOUT_ACTION"
#define GDM_PROTOCOL_MSG_FLEXI_XSERVER "FLEXI_XSERVER"

#define GDM_ACTION_STR_NONE     "NONE"
#define GDM_ACTION_STR_SHUTDOWN "HALT"
#define GDM_ACTION_STR_REBOOT   "REBOOT"
#define GDM_ACTION_STR_SUSPEND  "SUSPEND"

typedef struct {
        int             fd;
        char           *auth_cookie;

        GdmLogoutAction available_actions;
        GdmLogoutAction current_actions;

        time_t          last_update;
} GdmProtocolData;

static GdmProtocolData gdm_protocol_data = {
        0,
        NULL,
        GDM_LOGOUT_ACTION_NONE,
        GDM_LOGOUT_ACTION_NONE,
        0
};

static char *
gdm_send_protocol_msg (GdmProtocolData *data,
                       const char      *msg)
{
        GString *retval;
        char buf[256];
        char *p;
        int len;

        p = g_strconcat (msg, "\n", NULL);

        if (write (data->fd, p, strlen (p)) < 0) {
                g_free (p);

                g_warning ("Failed to send message to GDM: %s",
                           g_strerror (errno));

                return NULL;
        }

        g_free (p);

        p = NULL;
        retval = NULL;

        while ((len = read (data->fd, buf, sizeof (buf) - 1)) > 0) {
                buf[len] = '\0';

                if (!retval) {
                        retval = g_string_new (buf);
                } else {
                        retval = g_string_append (retval, buf);
                }

                if ((p = strchr (retval->str, '\n'))) {
                        break;
                }
        }

        if (p) {
                *p = '\0';
        }

        return retval ? g_string_free (retval, FALSE) : NULL;
}

static char *
get_display_number (void)
{
        const char *display_name;
        char       *retval;
        char       *p;

        display_name = gdk_display_get_name (gdk_display_get_default ());

        p = strchr (display_name, ':');

        if (!p) {
                return g_strdup ("0");
        }

        while (*p == ':') {
                p++;
        }

        retval = g_strdup (p);

        p = strchr (retval, '.');

        if (p != NULL) {
                *p = '\0';
        }

        return retval;
}

static gboolean
gdm_authenticate_connection (GdmProtocolData *data)
{
#define GDM_MIT_MAGIC_COOKIE_LEN 16

        const char *xau_path;
        FILE *f;
        Xauth *xau;
        char *display_number;
        gboolean retval;

        if (data->auth_cookie) {
                char *msg;
                char *response;

                msg = g_strdup_printf (GDM_PROTOCOL_MSG_AUTHENTICATE " %s",
                                       data->auth_cookie);
                response = gdm_send_protocol_msg (data, msg);
                g_free (msg);

                if (response && !strcmp (response, "OK")) {
                        g_free (response);
                        return TRUE;
                } else {
                        g_free (response);
                        g_free (data->auth_cookie);
                        data->auth_cookie = NULL;
                }
        }

        if (!(xau_path = XauFileName ())) {
                return FALSE;
        }

        if (!(f = fopen (xau_path, "r"))) {
                return FALSE;
        }

        retval = FALSE;
        display_number = get_display_number ();

        while ((xau = XauReadAuth (f))) {
                char buffer[40]; /* 2*16 == 32, so 40 is enough */
                char *msg;
                char *response;
                int   i;

                if (xau->family != FamilyLocal ||
                    strncmp (xau->number, display_number, xau->number_length) ||
                    strncmp (xau->name, "MIT-MAGIC-COOKIE-1", xau->name_length) ||
                    xau->data_length != GDM_MIT_MAGIC_COOKIE_LEN) {
                        XauDisposeAuth (xau);
                        continue;
                }

                for (i = 0; i < GDM_MIT_MAGIC_COOKIE_LEN; i++) {
                        g_snprintf (buffer + 2*i, 3, "%02x", (guint)(guchar)xau->data[i]);
                }

                XauDisposeAuth (xau);

                msg = g_strdup_printf (GDM_PROTOCOL_MSG_AUTHENTICATE " %s", buffer);
                response = gdm_send_protocol_msg (data, msg);
                g_free (msg);

                if (response && !strcmp (response, "OK")) {
                        data->auth_cookie = g_strdup (buffer);
                        g_free (response);
                        retval = TRUE;
                        break;
                }

                g_free (response);
        }

        g_free (display_number);

        fclose (f);

        return retval;

#undef GDM_MIT_MAGIC_COOKIE_LEN
}

static void
gdm_shutdown_protocol_connection (GdmProtocolData *data)
{
        if (data->fd) {
                close (data->fd);
        }

        data->fd = 0;
}

static gboolean
gdm_init_protocol_connection (GdmProtocolData *data)
{
        struct sockaddr_un addr;
        char              *response;

        g_assert (data->fd <= 0);

        if (g_file_test (GDM_PROTOCOL_SOCKET_PATH, G_FILE_TEST_EXISTS)) {
                strcpy (addr.sun_path, GDM_PROTOCOL_SOCKET_PATH);
        } else if (g_file_test ("/tmp/.gdm_socket", G_FILE_TEST_EXISTS)) {
                strcpy (addr.sun_path, "/tmp/.gdm_socket");
        } else {
                return FALSE;
        }

        data->fd = socket (AF_UNIX, SOCK_STREAM, 0);

        if (data->fd < 0) {
                g_warning ("Failed to create GDM socket: %s",
                           g_strerror (errno));

                gdm_shutdown_protocol_connection (data);

                return FALSE;
        }

        addr.sun_family = AF_UNIX;

        if (connect (data->fd, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
                g_warning ("Failed to establish a connection with GDM: %s",
                           g_strerror (errno));

                gdm_shutdown_protocol_connection (data);

                return FALSE;
        }

        response = gdm_send_protocol_msg (data, GDM_PROTOCOL_MSG_VERSION);

        if (!response || strncmp (response, "GDM ", strlen ("GDM ") != 0)) {
                g_free (response);

                g_warning ("Failed to get protocol version from GDM");
                gdm_shutdown_protocol_connection (data);

                return FALSE;
        }

        g_free (response);

        if (!gdm_authenticate_connection (data)) {
                g_warning ("Failed to authenticate with GDM");
                gdm_shutdown_protocol_connection (data);
                return FALSE;
        }

        return TRUE;
}

static void
gdm_parse_query_response (GdmProtocolData *data,
                          const char      *response)
{
        char **actions;
        int i;

        data->available_actions = GDM_LOGOUT_ACTION_NONE;
        data->current_actions   = GDM_LOGOUT_ACTION_NONE;

        if (strncmp (response, "OK ", 3) != 0) {
                return;
        }

        response += 3;

        actions = g_strsplit (response, ";", -1);

        for (i = 0; actions[i]; i++) {
                GdmLogoutAction action = GDM_LOGOUT_ACTION_NONE;
                gboolean        selected = FALSE;
                char           *str = actions [i];
                int             len;

                len = strlen (str);

                if (!len) {
                        continue;
                }

                if (str[len - 1] == '!') {
                        selected = TRUE;
                        str[len - 1] = '\0';
                }

                if (!strcmp (str, GDM_ACTION_STR_SHUTDOWN)) {
                        action = GDM_LOGOUT_ACTION_SHUTDOWN;
                } else if (!strcmp (str, GDM_ACTION_STR_REBOOT)) {
                        action = GDM_LOGOUT_ACTION_REBOOT;
                } else if (!strcmp (str, GDM_ACTION_STR_SUSPEND)) {
                        action = GDM_LOGOUT_ACTION_SUSPEND;
                }

                data->available_actions |= action;

                if (selected) {
                        data->current_actions |= action;
                }
        }

        g_strfreev (actions);
}

static void
gdm_update_logout_actions (GdmProtocolData *data)
{
        time_t current_time;
        char  *response;

        current_time = time (NULL);

        if (current_time <= (data->last_update + GDM_PROTOCOL_UPDATE_INTERVAL)) {
                return;
        }

        data->last_update = current_time;

        if (!gdm_init_protocol_connection (data)) {
                return;
        }

        if ((response = gdm_send_protocol_msg (data, GDM_PROTOCOL_MSG_QUERY_ACTION))) {
                gdm_parse_query_response (data, response);
                g_free (response);
        }

        gdm_shutdown_protocol_connection (data);
}

gboolean
gdm_is_available (void)
{
        if (!gdm_init_protocol_connection (&gdm_protocol_data)) {
                return FALSE;
        }

        gdm_shutdown_protocol_connection (&gdm_protocol_data);

        return TRUE;
}

gboolean
gdm_supports_logout_action (GdmLogoutAction action)
{
        gdm_update_logout_actions (&gdm_protocol_data);

        return (gdm_protocol_data.available_actions & action) != 0;
}

GdmLogoutAction
gdm_get_logout_action (void)
{
        gdm_update_logout_actions (&gdm_protocol_data);

        return gdm_protocol_data.current_actions;
}

void
gdm_set_logout_action (GdmLogoutAction action)
{
        char *action_str = NULL;
        char *msg;
        char *response;

        if (!gdm_init_protocol_connection (&gdm_protocol_data)) {
                return;
        }

        switch (action) {
        case GDM_LOGOUT_ACTION_NONE:
                action_str = GDM_ACTION_STR_NONE;
                break;
        case GDM_LOGOUT_ACTION_SHUTDOWN:
                action_str = GDM_ACTION_STR_SHUTDOWN;
                break;
        case GDM_LOGOUT_ACTION_REBOOT:
                action_str = GDM_ACTION_STR_REBOOT;
                break;
        case GDM_LOGOUT_ACTION_SUSPEND:
                action_str = GDM_ACTION_STR_SUSPEND;
                break;
        }

        msg = g_strdup_printf (GDM_PROTOCOL_MSG_SET_ACTION " %s", action_str);

        response = gdm_send_protocol_msg (&gdm_protocol_data, msg);

        g_free (msg);
        g_free (response);

        gdm_protocol_data.last_update = 0;

        gdm_shutdown_protocol_connection (&gdm_protocol_data);
}

void
gdm_new_login (void)
{
        char *response;

        if (!gdm_init_protocol_connection (&gdm_protocol_data)) {
                return;
        }

        response = gdm_send_protocol_msg (&gdm_protocol_data,
                                          GDM_PROTOCOL_MSG_FLEXI_XSERVER);

        g_free (response);

        gdm_protocol_data.last_update = 0;

        gdm_shutdown_protocol_connection (&gdm_protocol_data);
}
