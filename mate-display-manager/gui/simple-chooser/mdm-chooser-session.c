/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-chooser-session.h"
#include "mdm-chooser-client.h"

#include "mdm-host-chooser-dialog.h"

#define MDM_CHOOSER_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_SESSION, MdmChooserSessionPrivate))

struct MdmChooserSessionPrivate
{
        MdmChooserClient      *client;
        GtkWidget             *chooser_dialog;
};

enum {
        PROP_0,
};

static void     mdm_chooser_session_class_init  (MdmChooserSessionClass *klass);
static void     mdm_chooser_session_init        (MdmChooserSession      *chooser_session);
static void     mdm_chooser_session_finalize    (GObject                *object);

G_DEFINE_TYPE (MdmChooserSession, mdm_chooser_session, G_TYPE_OBJECT)

static gpointer session_object = NULL;

static gboolean
launch_compiz (MdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("MdmChooserSession: Launching compiz");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async ("gtk-window-decorator --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        error = NULL;
        g_spawn_command_line_async ("compiz --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

        /* FIXME: should try to detect if it actually works */

 out:
        return ret;
}

static gboolean
launch_marco (MdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("MdmChooserSession: Launching marco");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async ("marco --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static void
start_window_manager (MdmChooserSession *session)
{
        if (! launch_marco (session)) {
                launch_compiz (session);
        }
}

static gboolean
start_settings_daemon (MdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("MdmChooserSession: Launching settings daemon");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async (LIBEXECDIR "/mate-settings-daemon --mateconf-prefix=/apps/mdm/simple-chooser/settings-manager-plugins", &error);
        if (error != NULL) {
                g_warning ("Error starting settings daemon: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static void
on_dialog_response (GtkDialog         *dialog,
                    int                response_id,
                    MdmChooserSession *session)
{
        MdmChooserHost *host;

        host = NULL;
        switch (response_id) {
        case GTK_RESPONSE_OK:
                host = mdm_host_chooser_dialog_get_host (MDM_HOST_CHOOSER_DIALOG (dialog));
        case GTK_RESPONSE_NONE:
                /* delete event */
        default:
                break;
        }

        if (host != NULL) {
                char *hostname;

                /* only support XDMCP hosts in remote chooser */
                g_assert (mdm_chooser_host_get_kind (host) == MDM_CHOOSER_HOST_KIND_XDMCP);

                hostname = NULL;
                mdm_address_get_hostname (mdm_chooser_host_get_address (host), &hostname);
                /* FIXME: fall back to numerical address? */
                if (hostname != NULL) {
                        g_debug ("MdmChooserSession: Selected hostname '%s'", hostname);
                        mdm_chooser_client_call_select_hostname (session->priv->client, hostname);
                        g_free (hostname);
                }
        }

        mdm_chooser_client_call_disconnect (session->priv->client);
}

gboolean
mdm_chooser_session_start (MdmChooserSession *session,
                           GError           **error)
{
        gboolean res;

        g_return_val_if_fail (MDM_IS_CHOOSER_SESSION (session), FALSE);

        res = mdm_chooser_client_start (session->priv->client, error);

        if (res) {
                start_settings_daemon (session);
                start_window_manager (session);

                /* Only support XDMCP on remote choosers */
                session->priv->chooser_dialog = mdm_host_chooser_dialog_new (MDM_CHOOSER_HOST_KIND_XDMCP);
                g_signal_connect (session->priv->chooser_dialog,
                                  "response",
                                  G_CALLBACK (on_dialog_response),
                                  session);
                gtk_widget_show (session->priv->chooser_dialog);
        }

        return res;
}

void
mdm_chooser_session_stop (MdmChooserSession *session)
{
        g_return_if_fail (MDM_IS_CHOOSER_SESSION (session));

}

static void
mdm_chooser_session_set_property (GObject        *object,
                                  guint           prop_id,
                                  const GValue   *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_chooser_session_get_property (GObject        *object,
                                  guint           prop_id,
                                  GValue         *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_chooser_session_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmChooserSession      *chooser_session;

        chooser_session = MDM_CHOOSER_SESSION (G_OBJECT_CLASS (mdm_chooser_session_parent_class)->constructor (type,
                                                                                                               n_construct_properties,
                                                                                                               construct_properties));

        return G_OBJECT (chooser_session);
}

static void
mdm_chooser_session_dispose (GObject *object)
{
        g_debug ("MdmChooserSession: Disposing chooser_session");

        G_OBJECT_CLASS (mdm_chooser_session_parent_class)->dispose (object);
}

static void
mdm_chooser_session_class_init (MdmChooserSessionClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_chooser_session_get_property;
        object_class->set_property = mdm_chooser_session_set_property;
        object_class->constructor = mdm_chooser_session_constructor;
        object_class->dispose = mdm_chooser_session_dispose;
        object_class->finalize = mdm_chooser_session_finalize;

        g_type_class_add_private (klass, sizeof (MdmChooserSessionPrivate));
}

static void
mdm_chooser_session_init (MdmChooserSession *session)
{

        session->priv = MDM_CHOOSER_SESSION_GET_PRIVATE (session);

        session->priv->client = mdm_chooser_client_new ();
}

static void
mdm_chooser_session_finalize (GObject *object)
{
        MdmChooserSession *chooser_session;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_SESSION (object));

        chooser_session = MDM_CHOOSER_SESSION (object);

        g_return_if_fail (chooser_session->priv != NULL);

        G_OBJECT_CLASS (mdm_chooser_session_parent_class)->finalize (object);
}

MdmChooserSession *
mdm_chooser_session_new (void)
{
        if (session_object != NULL) {
                g_object_ref (session_object);
        } else {
                session_object = g_object_new (MDM_TYPE_CHOOSER_SESSION, NULL);
                g_object_add_weak_pointer (session_object,
                                           (gpointer *) &session_object);
        }

        return MDM_CHOOSER_SESSION (session_object);
}
