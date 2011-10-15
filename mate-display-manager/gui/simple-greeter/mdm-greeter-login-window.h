/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __MDM_GREETER_LOGIN_WINDOW_H
#define __MDM_GREETER_LOGIN_WINDOW_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_GREETER_LOGIN_WINDOW         (mdm_greeter_login_window_get_type ())
#define MDM_GREETER_LOGIN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_GREETER_LOGIN_WINDOW, MdmGreeterLoginWindow))
#define MDM_GREETER_LOGIN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_GREETER_LOGIN_WINDOW, MdmGreeterLoginWindowClass))
#define MDM_IS_GREETER_LOGIN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_GREETER_LOGIN_WINDOW))
#define MDM_IS_GREETER_LOGIN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_GREETER_LOGIN_WINDOW))
#define MDM_GREETER_LOGIN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_GREETER_LOGIN_WINDOW, MdmGreeterLoginWindowClass))

typedef struct MdmGreeterLoginWindowPrivate MdmGreeterLoginWindowPrivate;

typedef struct
{
        GtkWindow                     parent;
        MdmGreeterLoginWindowPrivate *priv;
} MdmGreeterLoginWindow;

typedef struct
{
        GtkWindowClass   parent_class;

        /* signals */
        void (* begin_auto_login)            (MdmGreeterLoginWindow *login_window,
                                              const char            *username);
        void (* begin_verification)          (MdmGreeterLoginWindow *login_window);
        void (* begin_verification_for_user) (MdmGreeterLoginWindow *login_window,
                                              const char            *username);
        void (* query_answer)                (MdmGreeterLoginWindow *login_window,
                                              const char            *text);
        void (* user_selected)               (MdmGreeterLoginWindow *login_window,
                                              const char            *text);
        void (* cancelled)                   (MdmGreeterLoginWindow *login_window);
        void (* disconnected)                (MdmGreeterLoginWindow *login_window);
        void (* start_session)               (MdmGreeterLoginWindow *login_window);

} MdmGreeterLoginWindowClass;

GType               mdm_greeter_login_window_get_type           (void);
GtkWidget *         mdm_greeter_login_window_new                (gboolean display_is_local);


gboolean            mdm_greeter_login_window_reset              (MdmGreeterLoginWindow *login_window);
gboolean            mdm_greeter_login_window_authentication_failed (MdmGreeterLoginWindow *login_window);
gboolean            mdm_greeter_login_window_ready              (MdmGreeterLoginWindow *login_window);
gboolean            mdm_greeter_login_window_info_query         (MdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            mdm_greeter_login_window_secret_info_query  (MdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            mdm_greeter_login_window_info               (MdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            mdm_greeter_login_window_problem            (MdmGreeterLoginWindow *login_window,
                                                                 const char *text);

void               mdm_greeter_login_window_request_timed_login (MdmGreeterLoginWindow *login_window,
                                                                 const char            *username,
                                                                 int                    delay);
void               mdm_greeter_login_window_user_authorized     (MdmGreeterLoginWindow *login_window);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_GREETER_LOGIN_WINDOW_H */
