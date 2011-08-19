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


#ifndef __GDM_GREETER_SERVER_H
#define __GDM_GREETER_SERVER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_GREETER_SERVER         (gdm_greeter_server_get_type ())
#define GDM_GREETER_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_GREETER_SERVER, GdmGreeterServer))
#define GDM_GREETER_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_GREETER_SERVER, GdmGreeterServerClass))
#define GDM_IS_GREETER_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_GREETER_SERVER))
#define GDM_IS_GREETER_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_GREETER_SERVER))
#define GDM_GREETER_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_GREETER_SERVER, GdmGreeterServerClass))

typedef struct GdmGreeterServerPrivate GdmGreeterServerPrivate;

typedef struct
{
        GObject                  parent;
        GdmGreeterServerPrivate *priv;
} GdmGreeterServer;

typedef struct
{
        GObjectClass   parent_class;

        void (* begin_auto_login)           (GdmGreeterServer  *greeter_server);
        void (* begin_verification)         (GdmGreeterServer  *greeter_server);
        void (* begin_verification_for_user)(GdmGreeterServer  *greeter_server,
                                             const char        *username);
        void (* query_answer)               (GdmGreeterServer  *greeter_server,
                                             const char        *text);
        void (* session_selected)           (GdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* hostname_selected)          (GdmGreeterServer  *greeter_server,
                                             const char        *hostname);
        void (* language_selected)          (GdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* layout_selected)            (GdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* user_selected)              (GdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* cancelled)                  (GdmGreeterServer  *greeter_server);
        void (* connected)                  (GdmGreeterServer  *greeter_server);
        void (* disconnected)               (GdmGreeterServer  *greeter_server);
        void (* start_session_when_ready)   (GdmGreeterServer  *greeter_server);
        void (* start_session_later)        (GdmGreeterServer  *greeter_server);
} GdmGreeterServerClass;

GType               gdm_greeter_server_get_type              (void);
GdmGreeterServer *  gdm_greeter_server_new                   (const char       *display_id);

gboolean            gdm_greeter_server_start                 (GdmGreeterServer *greeter_server);
gboolean            gdm_greeter_server_stop                  (GdmGreeterServer *greeter_server);
char *              gdm_greeter_server_get_address           (GdmGreeterServer *greeter_server);


gboolean            gdm_greeter_server_info_query            (GdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            gdm_greeter_server_secret_info_query     (GdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            gdm_greeter_server_info                  (GdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            gdm_greeter_server_problem               (GdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            gdm_greeter_server_authentication_failed (GdmGreeterServer *greeter_server);
gboolean            gdm_greeter_server_reset                 (GdmGreeterServer *greeter_server);
gboolean            gdm_greeter_server_ready                 (GdmGreeterServer *greeter_server);
void                gdm_greeter_server_selected_user_changed (GdmGreeterServer *greeter_server,
                                                              const char       *text);
void                gdm_greeter_server_default_language_name_changed (GdmGreeterServer *greeter_server,
                                                                      const char       *text);
void                gdm_greeter_server_default_layout_name_changed (GdmGreeterServer *greeter_server,
                                                                    const char       *text);
void                gdm_greeter_server_default_session_name_changed (GdmGreeterServer *greeter_server,
                                                                     const char       *text);

void                gdm_greeter_server_request_timed_login   (GdmGreeterServer *greeter_server,
                                                              const char       *username,
                                                              int               delay);
void                gdm_greeter_server_user_authorized       (GdmGreeterServer *greeter_server);


G_END_DECLS

#endif /* __GDM_GREETER_SERVER_H */
