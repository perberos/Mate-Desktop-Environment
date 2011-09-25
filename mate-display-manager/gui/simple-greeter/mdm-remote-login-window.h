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

#ifndef __MDM_REMOTE_LOGIN_WINDOW_H
#define __MDM_REMOTE_LOGIN_WINDOW_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_REMOTE_LOGIN_WINDOW         (mdm_remote_login_window_get_type ())
#define MDM_REMOTE_LOGIN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_REMOTE_LOGIN_WINDOW, MdmRemoteLoginWindow))
#define MDM_REMOTE_LOGIN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_REMOTE_LOGIN_WINDOW, MdmRemoteLoginWindowClass))
#define MDM_IS_REMOTE_LOGIN_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_REMOTE_LOGIN_WINDOW))
#define MDM_IS_REMOTE_LOGIN_WINDOW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_REMOTE_LOGIN_WINDOW))
#define MDM_REMOTE_LOGIN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_REMOTE_LOGIN_WINDOW, MdmRemoteLoginWindowClass))

typedef struct MdmRemoteLoginWindowPrivate MdmRemoteLoginWindowPrivate;

typedef struct
{
        GtkWindow                    parent;
        MdmRemoteLoginWindowPrivate *priv;
} MdmRemoteLoginWindow;

typedef struct
{
        GtkWindowClass   parent_class;

        /* signals */
        void (* disconnected)                (MdmRemoteLoginWindow *login_window);

} MdmRemoteLoginWindowClass;

GType               mdm_remote_login_window_get_type           (void);
GtkWidget *         mdm_remote_login_window_new                (gboolean display_is_local);

gboolean            mdm_remote_login_window_connect            (MdmRemoteLoginWindow *login_window,
                                                                const char           *host);
gboolean            mdm_remote_login_window_discconnect        (MdmRemoteLoginWindow *login_window);

G_END_DECLS

#endif /* __MDM_REMOTE_LOGIN_WINDOW_H */
