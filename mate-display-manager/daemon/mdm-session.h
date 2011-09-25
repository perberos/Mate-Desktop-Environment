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


#ifndef __MDM_SESSION_H
#define __MDM_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_SESSION         (mdm_session_get_type ())
#define MDM_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SESSION, MdmSession))
#define MDM_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SESSION, MdmSessionClass))
#define MDM_IS_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SESSION))
#define MDM_SESSION_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), MDM_TYPE_SESSION, MdmSessionIface))

typedef struct _MdmSession      MdmSession; /* Dummy typedef */
typedef struct _MdmSessionIface MdmSessionIface;

enum {
        MDM_SESSION_CRED_ESTABLISH = 0,
        MDM_SESSION_CRED_REFRESH,
};

struct _MdmSessionIface
{
        GTypeInterface base_iface;

        /* Methods */
        void (* start_conversation)          (MdmSession   *session);
        void (* setup)                       (MdmSession   *session,
                                              const char   *service_name);
        void (* setup_for_user)              (MdmSession   *session,
                                              const char   *service_name,
                                              const char   *username);
        void (* reset)                       (MdmSession   *session);
        void (* authenticate)                (MdmSession   *session);
        void (* authorize)                   (MdmSession   *session);
        void (* accredit)                    (MdmSession   *session,
                                              int           cred_flag);
        void (* open_session)                (MdmSession   *session);
        void (* answer_query)                (MdmSession   *session,
                                              const char   *text);
        void (* select_language)             (MdmSession   *session,
                                              const char   *text);
        void (* select_layout)               (MdmSession   *session,
                                              const char   *text);
        void (* select_session)              (MdmSession   *session,
                                              const char   *text);
        void (* select_user)                 (MdmSession   *session,
                                              const char   *text);
        void (* start_session)               (MdmSession   *session);
        void (* close)                       (MdmSession   *session);
        void (* cancel)                      (MdmSession   *session);

        /* Signals */
        void (* setup_complete)              (MdmSession   *session);
        void (* setup_failed)                (MdmSession   *session,
                                              const char   *message);
        void (* reset_complete)              (MdmSession   *session);
        void (* reset_failed)                (MdmSession   *session,
                                              const char   *message);
        void (* authenticated)               (MdmSession   *session);
        void (* authentication_failed)       (MdmSession   *session,
                                              const char   *message);
        void (* authorized)                  (MdmSession   *session);
        void (* authorization_failed)        (MdmSession   *session,
                                              const char   *message);
        void (* accredited)                  (MdmSession   *session);
        void (* accreditation_failed)        (MdmSession   *session,
                                              const char   *message);

        void (* info_query)                  (MdmSession   *session,
                                              const char   *query_text);
        void (* secret_info_query)           (MdmSession   *session,
                                              const char   *query_text);
        void (* info)                        (MdmSession   *session,
                                              const char   *info);
        void (* problem)                     (MdmSession   *session,
                                              const char   *problem);
        void (* session_opened)              (MdmSession   *session);
        void (* session_open_failed)         (MdmSession   *session,
                                              const char   *message);
        void (* session_started)             (MdmSession   *session,
                                              int           pid);
        void (* session_start_failed)        (MdmSession   *session,
                                              const char   *message);
        void (* session_exited)              (MdmSession   *session,
                                              int           exit_code);
        void (* session_died)                (MdmSession   *session,
                                              int           signal_number);
        void (* conversation_started)        (MdmSession   *session);
        void (* closed)                      (MdmSession   *session);
        void (* selected_user_changed)       (MdmSession   *session,
                                              const char   *text);

        void (* default_language_name_changed)    (MdmSession   *session,
                                                   const char   *text);
        void (* default_layout_name_changed)      (MdmSession   *session,
                                                   const char   *text);
        void (* default_session_name_changed)     (MdmSession   *session,
                                                   const char   *text);
};

GType    mdm_session_get_type                    (void) G_GNUC_CONST;

void     mdm_session_start_conversation          (MdmSession *session);
void     mdm_session_setup                       (MdmSession *session,
                                                  const char *service_name);
void     mdm_session_setup_for_user              (MdmSession *session,
                                                  const char *service_name,
                                                  const char *username);
void     mdm_session_reset                       (MdmSession *session);
void     mdm_session_authenticate                (MdmSession *session);
void     mdm_session_authorize                   (MdmSession *session);
void     mdm_session_accredit                    (MdmSession *session,
                                                  int         cred_flag);
void     mdm_session_open_session                (MdmSession *session);
void     mdm_session_start_session               (MdmSession *session);
void     mdm_session_close                       (MdmSession *session);

void     mdm_session_answer_query                (MdmSession *session,
                                                  const char *text);
void     mdm_session_select_session              (MdmSession *session,
                                                  const char *session_name);
void     mdm_session_select_language             (MdmSession *session,
                                                  const char *language);
void     mdm_session_select_layout               (MdmSession *session,
                                                  const char *language);
void     mdm_session_select_user                 (MdmSession *session,
                                                  const char *username);
void     mdm_session_cancel                      (MdmSession *session);

G_END_DECLS

#endif /* __MDM_SESSION_H */
