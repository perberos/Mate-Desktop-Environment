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


#ifndef __GDM_WELCOME_SESSION_H
#define __GDM_WELCOME_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_WELCOME_SESSION         (gdm_welcome_session_get_type ())
#define GDM_WELCOME_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_WELCOME_SESSION, GdmWelcomeSession))
#define GDM_WELCOME_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_WELCOME_SESSION, GdmWelcomeSessionClass))
#define GDM_IS_WELCOME_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_WELCOME_SESSION))
#define GDM_IS_WELCOME_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_WELCOME_SESSION))
#define GDM_WELCOME_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_WELCOME_SESSION, GdmWelcomeSessionClass))

typedef struct GdmWelcomeSessionPrivate GdmWelcomeSessionPrivate;

typedef struct
{
        GObject                   parent;
        GdmWelcomeSessionPrivate *priv;
} GdmWelcomeSession;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*start)          (GdmWelcomeSession  *welcome_session);
        gboolean (*stop)           (GdmWelcomeSession  *welcome_session);


        /* signals */
        void (* started)           (GdmWelcomeSession  *welcome_session);
        void (* stopped)           (GdmWelcomeSession  *welcome_session);
        void (* exited)            (GdmWelcomeSession  *welcome_session,
                                    int                 exit_code);
        void (* died)              (GdmWelcomeSession  *welcome_session,
                                    int                 signal_number);
} GdmWelcomeSessionClass;

GType                 gdm_welcome_session_get_type           (void);

void                  gdm_welcome_session_set_server_address (GdmWelcomeSession *welcome_session,
                                                              const char        *server_address);
gboolean              gdm_welcome_session_start              (GdmWelcomeSession *welcome_session);
gboolean              gdm_welcome_session_stop               (GdmWelcomeSession *welcome_session);

G_END_DECLS

#endif /* __GDM_WELCOME_SESSION_H */
