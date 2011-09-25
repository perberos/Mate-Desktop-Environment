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


#ifndef __MDM_WELCOME_SESSION_H
#define __MDM_WELCOME_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_WELCOME_SESSION         (mdm_welcome_session_get_type ())
#define MDM_WELCOME_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_WELCOME_SESSION, MdmWelcomeSession))
#define MDM_WELCOME_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_WELCOME_SESSION, MdmWelcomeSessionClass))
#define MDM_IS_WELCOME_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_WELCOME_SESSION))
#define MDM_IS_WELCOME_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_WELCOME_SESSION))
#define MDM_WELCOME_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_WELCOME_SESSION, MdmWelcomeSessionClass))

typedef struct MdmWelcomeSessionPrivate MdmWelcomeSessionPrivate;

typedef struct
{
        GObject                   parent;
        MdmWelcomeSessionPrivate *priv;
} MdmWelcomeSession;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*start)          (MdmWelcomeSession  *welcome_session);
        gboolean (*stop)           (MdmWelcomeSession  *welcome_session);


        /* signals */
        void (* started)           (MdmWelcomeSession  *welcome_session);
        void (* stopped)           (MdmWelcomeSession  *welcome_session);
        void (* exited)            (MdmWelcomeSession  *welcome_session,
                                    int                 exit_code);
        void (* died)              (MdmWelcomeSession  *welcome_session,
                                    int                 signal_number);
} MdmWelcomeSessionClass;

GType                 mdm_welcome_session_get_type           (void);

void                  mdm_welcome_session_set_server_address (MdmWelcomeSession *welcome_session,
                                                              const char        *server_address);
gboolean              mdm_welcome_session_start              (MdmWelcomeSession *welcome_session);
gboolean              mdm_welcome_session_stop               (MdmWelcomeSession *welcome_session);

G_END_DECLS

#endif /* __MDM_WELCOME_SESSION_H */
