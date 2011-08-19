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

#ifndef __GDM_GREETER_CLIENT_H
#define __GDM_GREETER_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_GREETER_CLIENT         (gdm_greeter_client_get_type ())
#define GDM_GREETER_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_GREETER_CLIENT, GdmGreeterClient))
#define GDM_GREETER_CLIENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_GREETER_CLIENT, GdmGreeterClientClass))
#define GDM_IS_GREETER_CLIENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_GREETER_CLIENT))
#define GDM_IS_GREETER_CLIENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_GREETER_CLIENT))
#define GDM_GREETER_CLIENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_GREETER_CLIENT, GdmGreeterClientClass))

typedef struct GdmGreeterClientPrivate GdmGreeterClientPrivate;

typedef struct
{
        GObject                  parent;
        GdmGreeterClientPrivate *priv;
} GdmGreeterClient;

typedef struct
{
        GObjectClass   parent_class;

        void (* info_query)              (GdmGreeterClient  *client,
                                          const char        *query_text);

        void (* secret_info_query)       (GdmGreeterClient  *client,
                                          const char        *query_text);

        void (* info)                    (GdmGreeterClient  *client,
                                          const char        *info);

        void (* problem)                 (GdmGreeterClient  *client,
                                          const char        *problem);
        void (* ready)                   (GdmGreeterClient  *client);
        void (* reset)                   (GdmGreeterClient  *client);
        void (* authentication_failed)   (GdmGreeterClient  *client);
        void (* selected_user_changed)   (GdmGreeterClient  *client,
                                          const char        *username);

        void (* default_session_name_changed)  (GdmGreeterClient  *client,
                                                const char        *session_name);
        void (* default_language_name_changed) (GdmGreeterClient  *client,
                                                const char        *language_name);
        void (* default_layout_name_changed) (GdmGreeterClient  *client,
                                              const char        *layout_name);
        void (* timed_login_requested)   (GdmGreeterClient  *client,
                                          const char        *username,
                                          int                delay);
        void (* user_authorized)         (GdmGreeterClient  *client);
} GdmGreeterClientClass;

#define GDM_GREETER_CLIENT_ERROR (gdm_greeter_client_error_quark ())

typedef enum _GdmGreeterClientError {
        GDM_GREETER_CLIENT_ERROR_GENERIC = 0,
} GdmGreeterClientError;

GType              gdm_greeter_client_get_type                       (void);
GQuark             gdm_greeter_client_error_quark                    (void);

GdmGreeterClient * gdm_greeter_client_new                            (void);

gboolean           gdm_greeter_client_start                          (GdmGreeterClient *client,
                                                                         GError          **error);
void               gdm_greeter_client_stop                           (GdmGreeterClient *client);

gboolean           gdm_greeter_client_get_display_is_local           (GdmGreeterClient *client);

char *             gdm_greeter_client_call_get_display_id            (GdmGreeterClient *client);

void               gdm_greeter_client_call_begin_auto_login          (GdmGreeterClient *client,
                                                                      const char       *username);
void               gdm_greeter_client_call_begin_verification        (GdmGreeterClient *client);
void               gdm_greeter_client_call_begin_verification_for_user (GdmGreeterClient *client,
                                                                        const char       *username);
void               gdm_greeter_client_call_cancel                    (GdmGreeterClient *client);
void               gdm_greeter_client_call_disconnect                (GdmGreeterClient *client);
void               gdm_greeter_client_call_select_hostname           (GdmGreeterClient *client,
                                                                      const char       *text);
void               gdm_greeter_client_call_select_user               (GdmGreeterClient *client,
                                                                      const char       *text);
void               gdm_greeter_client_call_select_language           (GdmGreeterClient *client,
                                                                      const char       *text);
void               gdm_greeter_client_call_select_layout             (GdmGreeterClient *client,
                                                                      const char       *text);
void               gdm_greeter_client_call_select_session            (GdmGreeterClient *client,
                                                                      const char       *text);
void               gdm_greeter_client_call_answer_query              (GdmGreeterClient *client,
                                                                      const char       *text);

void               gdm_greeter_client_call_start_session_when_ready  (GdmGreeterClient *client,
                                                                      gboolean          should_start_session);


G_END_DECLS

#endif /* __GDM_GREETER_CLIENT_H */
