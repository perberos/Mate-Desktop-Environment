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


#ifndef __MDM_GREETER_SERVER_H
#define __MDM_GREETER_SERVER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_GREETER_SERVER         (mdm_greeter_server_get_type ())
#define MDM_GREETER_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_GREETER_SERVER, MdmGreeterServer))
#define MDM_GREETER_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_GREETER_SERVER, MdmGreeterServerClass))
#define MDM_IS_GREETER_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_GREETER_SERVER))
#define MDM_IS_GREETER_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_GREETER_SERVER))
#define MDM_GREETER_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_GREETER_SERVER, MdmGreeterServerClass))

typedef struct MdmGreeterServerPrivate MdmGreeterServerPrivate;

typedef struct
{
        GObject                  parent;
        MdmGreeterServerPrivate *priv;
} MdmGreeterServer;

typedef struct
{
        GObjectClass   parent_class;

        void (* begin_auto_login)           (MdmGreeterServer  *greeter_server);
        void (* begin_verification)         (MdmGreeterServer  *greeter_server);
        void (* begin_verification_for_user)(MdmGreeterServer  *greeter_server,
                                             const char        *username);
        void (* query_answer)               (MdmGreeterServer  *greeter_server,
                                             const char        *text);
        void (* session_selected)           (MdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* hostname_selected)          (MdmGreeterServer  *greeter_server,
                                             const char        *hostname);
        void (* language_selected)          (MdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* layout_selected)            (MdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* user_selected)              (MdmGreeterServer  *greeter_server,
                                             const char        *name);
        void (* cancelled)                  (MdmGreeterServer  *greeter_server);
        void (* connected)                  (MdmGreeterServer  *greeter_server);
        void (* disconnected)               (MdmGreeterServer  *greeter_server);
        void (* start_session_when_ready)   (MdmGreeterServer  *greeter_server);
        void (* start_session_later)        (MdmGreeterServer  *greeter_server);
} MdmGreeterServerClass;

GType               mdm_greeter_server_get_type              (void);
MdmGreeterServer *  mdm_greeter_server_new                   (const char       *display_id);

gboolean            mdm_greeter_server_start                 (MdmGreeterServer *greeter_server);
gboolean            mdm_greeter_server_stop                  (MdmGreeterServer *greeter_server);
char *              mdm_greeter_server_get_address           (MdmGreeterServer *greeter_server);


gboolean            mdm_greeter_server_info_query            (MdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            mdm_greeter_server_secret_info_query     (MdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            mdm_greeter_server_info                  (MdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            mdm_greeter_server_problem               (MdmGreeterServer *greeter_server,
                                                              const char       *text);
gboolean            mdm_greeter_server_authentication_failed (MdmGreeterServer *greeter_server);
gboolean            mdm_greeter_server_reset                 (MdmGreeterServer *greeter_server);
gboolean            mdm_greeter_server_ready                 (MdmGreeterServer *greeter_server);
void                mdm_greeter_server_selected_user_changed (MdmGreeterServer *greeter_server,
                                                              const char       *text);
void                mdm_greeter_server_default_language_name_changed (MdmGreeterServer *greeter_server,
                                                                      const char       *text);
void                mdm_greeter_server_default_layout_name_changed (MdmGreeterServer *greeter_server,
                                                                    const char       *text);
void                mdm_greeter_server_default_session_name_changed (MdmGreeterServer *greeter_server,
                                                                     const char       *text);

void                mdm_greeter_server_request_timed_login   (MdmGreeterServer *greeter_server,
                                                              const char       *username,
                                                              int               delay);
void                mdm_greeter_server_user_authorized       (MdmGreeterServer *greeter_server);


#ifdef __cplusplus
}
#endif

#endif /* __MDM_GREETER_SERVER_H */
