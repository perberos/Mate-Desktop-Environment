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

#ifndef __MDM_GREETER_CLIENT_H
#define __MDM_GREETER_CLIENT_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_GREETER_CLIENT         (mdm_greeter_client_get_type ())
#define MDM_GREETER_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_GREETER_CLIENT, MdmGreeterClient))
#define MDM_GREETER_CLIENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_GREETER_CLIENT, MdmGreeterClientClass))
#define MDM_IS_GREETER_CLIENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_GREETER_CLIENT))
#define MDM_IS_GREETER_CLIENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_GREETER_CLIENT))
#define MDM_GREETER_CLIENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_GREETER_CLIENT, MdmGreeterClientClass))

typedef struct MdmGreeterClientPrivate MdmGreeterClientPrivate;

typedef struct
{
        GObject                  parent;
        MdmGreeterClientPrivate *priv;
} MdmGreeterClient;

typedef struct
{
        GObjectClass   parent_class;

        void (* info_query)              (MdmGreeterClient  *client,
                                          const char        *query_text);

        void (* secret_info_query)       (MdmGreeterClient  *client,
                                          const char        *query_text);

        void (* info)                    (MdmGreeterClient  *client,
                                          const char        *info);

        void (* problem)                 (MdmGreeterClient  *client,
                                          const char        *problem);
        void (* ready)                   (MdmGreeterClient  *client);
        void (* reset)                   (MdmGreeterClient  *client);
        void (* authentication_failed)   (MdmGreeterClient  *client);
        void (* selected_user_changed)   (MdmGreeterClient  *client,
                                          const char        *username);

        void (* default_session_name_changed)  (MdmGreeterClient  *client,
                                                const char        *session_name);
        void (* default_language_name_changed) (MdmGreeterClient  *client,
                                                const char        *language_name);
        void (* default_layout_name_changed) (MdmGreeterClient  *client,
                                              const char        *layout_name);
        void (* timed_login_requested)   (MdmGreeterClient  *client,
                                          const char        *username,
                                          int                delay);
        void (* user_authorized)         (MdmGreeterClient  *client);
} MdmGreeterClientClass;

#define MDM_GREETER_CLIENT_ERROR (mdm_greeter_client_error_quark ())

typedef enum _MdmGreeterClientError {
        MDM_GREETER_CLIENT_ERROR_GENERIC = 0,
} MdmGreeterClientError;

GType              mdm_greeter_client_get_type                       (void);
GQuark             mdm_greeter_client_error_quark                    (void);

MdmGreeterClient * mdm_greeter_client_new                            (void);

gboolean           mdm_greeter_client_start                          (MdmGreeterClient *client,
                                                                         GError          **error);
void               mdm_greeter_client_stop                           (MdmGreeterClient *client);

gboolean           mdm_greeter_client_get_display_is_local           (MdmGreeterClient *client);

char *             mdm_greeter_client_call_get_display_id            (MdmGreeterClient *client);

void               mdm_greeter_client_call_begin_auto_login          (MdmGreeterClient *client,
                                                                      const char       *username);
void               mdm_greeter_client_call_begin_verification        (MdmGreeterClient *client);
void               mdm_greeter_client_call_begin_verification_for_user (MdmGreeterClient *client,
                                                                        const char       *username);
void               mdm_greeter_client_call_cancel                    (MdmGreeterClient *client);
void               mdm_greeter_client_call_disconnect                (MdmGreeterClient *client);
void               mdm_greeter_client_call_select_hostname           (MdmGreeterClient *client,
                                                                      const char       *text);
void               mdm_greeter_client_call_select_user               (MdmGreeterClient *client,
                                                                      const char       *text);
void               mdm_greeter_client_call_select_language           (MdmGreeterClient *client,
                                                                      const char       *text);
void               mdm_greeter_client_call_select_layout             (MdmGreeterClient *client,
                                                                      const char       *text);
void               mdm_greeter_client_call_select_session            (MdmGreeterClient *client,
                                                                      const char       *text);
void               mdm_greeter_client_call_answer_query              (MdmGreeterClient *client,
                                                                      const char       *text);

void               mdm_greeter_client_call_start_session_when_ready  (MdmGreeterClient *client,
                                                                      gboolean          should_start_session);


#ifdef __cplusplus
}
#endif

#endif /* __MDM_GREETER_CLIENT_H */
