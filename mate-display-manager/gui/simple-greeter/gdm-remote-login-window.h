/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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

#ifndef __GDM_REMOTE_LOGIN_WINDOW_H
#define __GDM_REMOTE_LOGIN_WINDOW_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_REMOTE_LOGIN_WINDOW         (gdm_remote_login_window_get_type ())
#define GDM_REMOTE_LOGIN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_REMOTE_LOGIN_WINDOW, GdmRemoteLoginWindow))
#define GDM_REMOTE_LOGIN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_REMOTE_LOGIN_WINDOW, GdmRemoteLoginWindowClass))
#define GDM_IS_REMOTE_LOGIN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_REMOTE_LOGIN_WINDOW))
#define GDM_IS_REMOTE_LOGIN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_REMOTE_LOGIN_WINDOW))
#define GDM_REMOTE_LOGIN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_REMOTE_LOGIN_WINDOW, GdmRemoteLoginWindowClass))

typedef struct GdmRemoteLoginWindowPrivate GdmRemoteLoginWindowPrivate;

typedef struct
{
        GtkWindow                    parent;
        GdmRemoteLoginWindowPrivate *priv;
} GdmRemoteLoginWindow;

typedef struct
{
        GtkWindowClass   parent_class;

        /* signals */
        void (* disconnected)                (GdmRemoteLoginWindow *login_window);

} GdmRemoteLoginWindowClass;

GType               gdm_remote_login_window_get_type           (void);
GtkWidget *         gdm_remote_login_window_new                (gboolean display_is_local);

gboolean            gdm_remote_login_window_connect            (GdmRemoteLoginWindow *login_window,
                                                                const char           *host);
gboolean            gdm_remote_login_window_discconnect        (GdmRemoteLoginWindow *login_window);

G_END_DECLS

#endif /* __GDM_REMOTE_LOGIN_WINDOW_H */
