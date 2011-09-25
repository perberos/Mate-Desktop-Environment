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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-session.h"
#include "mdm-session-private.h"

enum {
        CONVERSATION_STARTED = 0,
        SETUP_COMPLETE,
        SETUP_FAILED,
        RESET_COMPLETE,
        RESET_FAILED,
        AUTHENTICATED,
        AUTHENTICATION_FAILED,
        AUTHORIZED,
        AUTHORIZATION_FAILED,
        ACCREDITED,
        ACCREDITATION_FAILED,
        CLOSED,
        INFO,
        PROBLEM,
        INFO_QUERY,
        SECRET_INFO_QUERY,
        SESSION_OPENED,
        SESSION_OPEN_FAILED,
        SESSION_STARTED,
        SESSION_START_FAILED,
        SESSION_EXITED,
        SESSION_DIED,
        SELECTED_USER_CHANGED,
        DEFAULT_LANGUAGE_NAME_CHANGED,
        DEFAULT_LAYOUT_NAME_CHANGED,
        DEFAULT_SESSION_NAME_CHANGED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void mdm_session_class_init (gpointer g_iface);

GType
mdm_session_get_type (void)
{
        static GType session_type = 0;

        if (!session_type) {
                session_type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                                              "MdmSession",
                                                              sizeof (MdmSessionIface),
                                                              (GClassInitFunc) mdm_session_class_init,
                                                              0, NULL, 0);

                g_type_interface_add_prerequisite (session_type, G_TYPE_OBJECT);
        }

        return session_type;
}

void
mdm_session_start_conversation (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->start_conversation (session);
}

void
mdm_session_close (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->close (session);
}

void
mdm_session_setup (MdmSession *session,
                   const char *service_name)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->setup (session, service_name);
}

void
mdm_session_setup_for_user (MdmSession *session,
                            const char *service_name,
                            const char *username)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->setup_for_user (session, service_name, username);
}

void
mdm_session_authenticate (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->authenticate (session);
}

void
mdm_session_authorize (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->authorize (session);
}

void
mdm_session_accredit (MdmSession *session,
                      int         flag)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->accredit (session, flag);
}

void
mdm_session_answer_query (MdmSession *session,
                          const char *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->answer_query (session, text);
}

void
mdm_session_select_session (MdmSession *session,
                            const char *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->select_session (session, text);
}

void
mdm_session_select_language (MdmSession *session,
                             const char *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->select_language (session, text);
}

void
mdm_session_select_layout (MdmSession *session,
                           const char *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->select_layout (session, text);
}

void
mdm_session_select_user (MdmSession *session,
                         const char *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->select_user (session, text);
}

void
mdm_session_cancel (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->cancel (session);
}

void
mdm_session_open_session (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->open_session (session);
}

void
mdm_session_start_session (MdmSession *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        MDM_SESSION_GET_IFACE (session)->start_session (session);
}

static void
mdm_session_class_init (gpointer g_iface)
{
        GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

        signals [CONVERSATION_STARTED] =
                g_signal_new ("conversation-started",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, conversation_started),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [SETUP_COMPLETE] =
                g_signal_new ("setup-complete",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, setup_complete),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [SETUP_FAILED] =
                g_signal_new ("setup-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, setup_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [RESET_COMPLETE] =
                g_signal_new ("reset-complete",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, reset_complete),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [RESET_FAILED] =
                g_signal_new ("reset-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, reset_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [AUTHENTICATED] =
                g_signal_new ("authenticated",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, authenticated),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [AUTHENTICATION_FAILED] =
                g_signal_new ("authentication-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, authentication_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [AUTHORIZED] =
                g_signal_new ("authorized",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, authorized),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [AUTHORIZATION_FAILED] =
                g_signal_new ("authorization-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, authorization_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [ACCREDITED] =
                g_signal_new ("accredited",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, accredited),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [ACCREDITATION_FAILED] =
                g_signal_new ("accreditation-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, accreditation_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

         signals [INFO_QUERY] =
                g_signal_new ("info-query",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, info_query),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [SECRET_INFO_QUERY] =
                g_signal_new ("secret-info-query",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, secret_info_query),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [INFO] =
                g_signal_new ("info",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, info),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [PROBLEM] =
                g_signal_new ("problem",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, problem),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [SESSION_OPENED] =
                g_signal_new ("session-opened",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_opened),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [SESSION_OPEN_FAILED] =
                g_signal_new ("session-open-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_open_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [SESSION_STARTED] =
                g_signal_new ("session-started",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_started),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
        signals [SESSION_START_FAILED] =
                g_signal_new ("session-start-failed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_start_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [SESSION_EXITED] =
                g_signal_new ("session-exited",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_exited),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
        signals [SESSION_DIED] =
                g_signal_new ("session-died",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, session_died),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
        signals [CLOSED] =
                g_signal_new ("closed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, closed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [SELECTED_USER_CHANGED] =
                g_signal_new ("selected-user-changed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, selected_user_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [DEFAULT_LANGUAGE_NAME_CHANGED] =
                g_signal_new ("default-language-name-changed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, default_language_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [DEFAULT_LAYOUT_NAME_CHANGED] =
                g_signal_new ("default-layout-name-changed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, default_layout_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [DEFAULT_SESSION_NAME_CHANGED] =
                g_signal_new ("default-session-name-changed",
                              iface_type,
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionIface, default_session_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
}

void
_mdm_session_setup_complete (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [SETUP_COMPLETE], 0);
}

void
_mdm_session_setup_failed (MdmSession   *session,
                           const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SETUP_FAILED], 0, text);
}

void
_mdm_session_reset_complete (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [RESET_COMPLETE], 0);
}

void
_mdm_session_reset_failed (MdmSession   *session,
                           const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [RESET_FAILED], 0, text);
}

void
_mdm_session_authenticated (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [AUTHENTICATED], 0);
}

void
_mdm_session_authentication_failed (MdmSession   *session,
                                    const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [AUTHENTICATION_FAILED], 0, text);
}

void
_mdm_session_authorized (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [AUTHORIZED], 0);
}

void
_mdm_session_authorization_failed (MdmSession   *session,
                                   const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [AUTHORIZATION_FAILED], 0, text);
}

void
_mdm_session_accredited (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [ACCREDITED], 0);
}

void
_mdm_session_accreditation_failed (MdmSession   *session,
                                   const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [ACCREDITATION_FAILED], 0, text);
}

void
_mdm_session_info_query (MdmSession   *session,
                         const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [INFO_QUERY], 0, text);
}

void
_mdm_session_secret_info_query (MdmSession   *session,
                                const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SECRET_INFO_QUERY], 0, text);
}

void
_mdm_session_info (MdmSession   *session,
                   const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [INFO], 0, text);
}

void
_mdm_session_problem (MdmSession   *session,
                      const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [PROBLEM], 0, text);
}

void
_mdm_session_session_opened (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_OPENED], 0);
}

void
_mdm_session_session_open_failed (MdmSession   *session,
                                  const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_OPEN_FAILED], 0, text);
}

void
_mdm_session_session_started (MdmSession   *session,
                              int           pid)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_STARTED], 0, pid);
}

void
_mdm_session_session_start_failed (MdmSession   *session,
                                   const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_START_FAILED], 0, text);
}

void
_mdm_session_session_exited (MdmSession   *session,
                             int           exit_code)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_EXITED], 0, exit_code);
}

void
_mdm_session_session_died (MdmSession   *session,
                           int           signal_number)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SESSION_DIED], 0, signal_number);
}

void
_mdm_session_conversation_started (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [CONVERSATION_STARTED], 0);
}

void
_mdm_session_closed (MdmSession   *session)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [CLOSED], 0);
}

void
_mdm_session_default_language_name_changed (MdmSession   *session,
                                            const char   *language_name)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [DEFAULT_LANGUAGE_NAME_CHANGED], 0, language_name);
}

void
_mdm_session_default_layout_name_changed (MdmSession   *session,
                                          const char   *layout_name)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [DEFAULT_LAYOUT_NAME_CHANGED], 0, layout_name);
}

void
_mdm_session_default_session_name_changed (MdmSession   *session,
                                           const char   *session_name)
{
        g_return_if_fail (MDM_IS_SESSION (session));

        g_signal_emit (session, signals [DEFAULT_SESSION_NAME_CHANGED], 0, session_name);
}

void
_mdm_session_selected_user_changed (MdmSession   *session,
                                    const char   *text)
{
        g_return_if_fail (MDM_IS_SESSION (session));
        g_signal_emit (session, signals [SELECTED_USER_CHANGED], 0, text);
}
