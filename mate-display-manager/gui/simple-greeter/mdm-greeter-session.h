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

#ifndef __MDM_GREETER_SESSION_H
#define __MDM_GREETER_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_GREETER_SESSION         (mdm_greeter_session_get_type ())
#define MDM_GREETER_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_GREETER_SESSION, MdmGreeterSession))
#define MDM_GREETER_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_GREETER_SESSION, MdmGreeterSessionClass))
#define MDM_IS_GREETER_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_GREETER_SESSION))
#define MDM_IS_GREETER_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_GREETER_SESSION))
#define MDM_GREETER_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_GREETER_SESSION, MdmGreeterSessionClass))

typedef struct MdmGreeterSessionPrivate MdmGreeterSessionPrivate;

typedef struct
{
        GObject                   parent;
        MdmGreeterSessionPrivate *priv;
} MdmGreeterSession;

typedef struct
{
        GObjectClass   parent_class;
} MdmGreeterSessionClass;

GType                  mdm_greeter_session_get_type                       (void);

MdmGreeterSession    * mdm_greeter_session_new                            (void);

gboolean               mdm_greeter_session_start                          (MdmGreeterSession *session,
                                                                           GError           **error);
void                   mdm_greeter_session_stop                           (MdmGreeterSession *session);

G_END_DECLS

#endif /* __MDM_GREETER_SESSION_H */
