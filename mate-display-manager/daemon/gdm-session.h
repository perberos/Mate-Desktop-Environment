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


#ifndef __GDM_SESSION_H
#define __GDM_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SESSION         (gdm_session_get_type ())
#define GDM_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SESSION, GdmSession))
#define GDM_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SESSION, GdmSessionClass))
#define GDM_IS_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SESSION))
#define GDM_SESSION_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), GDM_TYPE_SESSION, GdmSessionIface))

typedef struct _GdmSession      GdmSession; /* Dummy typedef */
typedef struct _GdmSessionIface GdmSessionIface;

enum {
        GDM_SESSION_CRED_ESTABLISH = 0,
        GDM_SESSION_CRED_REFRESH,
};

struct _GdmSessionIface
{
        GTypeInterface base_iface;

        /* Methods */
        void (* start_conversation)          (GdmSession   *session);
        void (* setup)                       (GdmSession   *session,
                                              const char   *service_name);
        void (* setup_for_user)              (GdmSession   *session,
                                              const char   *service_name,
                                              const char   *username);
        void (* reset)                       (GdmSession   *session);
        void (* authenticate)                (GdmSession   *session);
        void (* authorize)                   (GdmSession   *session);
        void (* accredit)                    (GdmSession   *session,
                                              int           cred_flag);
        void (* open_session)                (GdmSession   *session);
        void (* answer_query)                (GdmSession   *session,
                                              const char   *text);
        void (* select_language)             (GdmSession   *session,
                                              const char   *text);
        void (* select_layout)               (GdmSession   *session,
                                              const char   *text);
        void (* select_session)              (GdmSession   *session,
                                              const char   *text);
        void (* select_user)                 (GdmSession   *session,
                                              const char   *text);
        void (* start_session)               (GdmSession   *session);
        void (* close)                       (GdmSession   *session);
        void (* cancel)                      (GdmSession   *session);

        /* Signals */
        void (* setup_complete)              (GdmSession   *session);
        void (* setup_failed)                (GdmSession   *session,
                                              const char   *message);
        void (* reset_complete)              (GdmSession   *session);
        void (* reset_failed)                (GdmSession   *session,
                                              const char   *message);
        void (* authenticated)               (GdmSession   *session);
        void (* authentication_failed)       (GdmSession   *session,
                                              const char   *message);
        void (* authorized)                  (GdmSession   *session);
        void (* authorization_failed)        (GdmSession   *session,
                                              const char   *message);
        void (* accredited)                  (GdmSession   *session);
        void (* accreditation_failed)        (GdmSession   *session,
                                              const char   *message);

        void (* info_query)                  (GdmSession   *session,
                                              const char   *query_text);
        void (* secret_info_query)           (GdmSession   *session,
                                              const char   *query_text);
        void (* info)                        (GdmSession   *session,
                                              const char   *info);
        void (* problem)                     (GdmSession   *session,
                                              const char   *problem);
        void (* session_opened)              (GdmSession   *session);
        void (* session_open_failed)         (GdmSession   *session,
                                              const char   *message);
        void (* session_started)             (GdmSession   *session,
                                              int           pid);
        void (* session_start_failed)        (GdmSession   *session,
                                              const char   *message);
        void (* session_exited)              (GdmSession   *session,
                                              int           exit_code);
        void (* session_died)                (GdmSession   *session,
                                              int           signal_number);
        void (* conversation_started)        (GdmSession   *session);
        void (* closed)                      (GdmSession   *session);
        void (* selected_user_changed)       (GdmSession   *session,
                                              const char   *text);

        void (* default_language_name_changed)    (GdmSession   *session,
                                                   const char   *text);
        void (* default_layout_name_changed)      (GdmSession   *session,
                                                   const char   *text);
        void (* default_session_name_changed)     (GdmSession   *session,
                                                   const char   *text);
};

GType    gdm_session_get_type                    (void) G_GNUC_CONST;

void     gdm_session_start_conversation          (GdmSession *session);
void     gdm_session_setup                       (GdmSession *session,
                                                  const char *service_name);
void     gdm_session_setup_for_user              (GdmSession *session,
                                                  const char *service_name,
                                                  const char *username);
void     gdm_session_reset                       (GdmSession *session);
void     gdm_session_authenticate                (GdmSession *session);
void     gdm_session_authorize                   (GdmSession *session);
void     gdm_session_accredit                    (GdmSession *session,
                                                  int         cred_flag);
void     gdm_session_open_session                (GdmSession *session);
void     gdm_session_start_session               (GdmSession *session);
void     gdm_session_close                       (GdmSession *session);

void     gdm_session_answer_query                (GdmSession *session,
                                                  const char *text);
void     gdm_session_select_session              (GdmSession *session,
                                                  const char *session_name);
void     gdm_session_select_language             (GdmSession *session,
                                                  const char *language);
void     gdm_session_select_layout               (GdmSession *session,
                                                  const char *language);
void     gdm_session_select_user                 (GdmSession *session,
                                                  const char *username);
void     gdm_session_cancel                      (GdmSession *session);

G_END_DECLS

#endif /* __GDM_SESSION_H */
