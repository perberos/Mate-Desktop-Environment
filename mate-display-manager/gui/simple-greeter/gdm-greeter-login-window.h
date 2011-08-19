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

#ifndef __GDM_GREETER_LOGIN_WINDOW_H
#define __GDM_GREETER_LOGIN_WINDOW_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_GREETER_LOGIN_WINDOW         (gdm_greeter_login_window_get_type ())
#define GDM_GREETER_LOGIN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_GREETER_LOGIN_WINDOW, GdmGreeterLoginWindow))
#define GDM_GREETER_LOGIN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_GREETER_LOGIN_WINDOW, GdmGreeterLoginWindowClass))
#define GDM_IS_GREETER_LOGIN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_GREETER_LOGIN_WINDOW))
#define GDM_IS_GREETER_LOGIN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_GREETER_LOGIN_WINDOW))
#define GDM_GREETER_LOGIN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_GREETER_LOGIN_WINDOW, GdmGreeterLoginWindowClass))

typedef struct GdmGreeterLoginWindowPrivate GdmGreeterLoginWindowPrivate;

typedef struct
{
        GtkWindow                     parent;
        GdmGreeterLoginWindowPrivate *priv;
} GdmGreeterLoginWindow;

typedef struct
{
        GtkWindowClass   parent_class;

        /* signals */
        void (* begin_auto_login)            (GdmGreeterLoginWindow *login_window,
                                              const char            *username);
        void (* begin_verification)          (GdmGreeterLoginWindow *login_window);
        void (* begin_verification_for_user) (GdmGreeterLoginWindow *login_window,
                                              const char            *username);
        void (* query_answer)                (GdmGreeterLoginWindow *login_window,
                                              const char            *text);
        void (* user_selected)               (GdmGreeterLoginWindow *login_window,
                                              const char            *text);
        void (* cancelled)                   (GdmGreeterLoginWindow *login_window);
        void (* disconnected)                (GdmGreeterLoginWindow *login_window);
        void (* start_session)               (GdmGreeterLoginWindow *login_window);

} GdmGreeterLoginWindowClass;

GType               gdm_greeter_login_window_get_type           (void);
GtkWidget *         gdm_greeter_login_window_new                (gboolean display_is_local);


gboolean            gdm_greeter_login_window_reset              (GdmGreeterLoginWindow *login_window);
gboolean            gdm_greeter_login_window_authentication_failed (GdmGreeterLoginWindow *login_window);
gboolean            gdm_greeter_login_window_ready              (GdmGreeterLoginWindow *login_window);
gboolean            gdm_greeter_login_window_info_query         (GdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            gdm_greeter_login_window_secret_info_query  (GdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            gdm_greeter_login_window_info               (GdmGreeterLoginWindow *login_window,
                                                                 const char *text);
gboolean            gdm_greeter_login_window_problem            (GdmGreeterLoginWindow *login_window,
                                                                 const char *text);

void               gdm_greeter_login_window_request_timed_login (GdmGreeterLoginWindow *login_window,
                                                                 const char            *username,
                                                                 int                    delay);
void               gdm_greeter_login_window_user_authorized     (GdmGreeterLoginWindow *login_window);

G_END_DECLS

#endif /* __GDM_GREETER_LOGIN_WINDOW_H */
