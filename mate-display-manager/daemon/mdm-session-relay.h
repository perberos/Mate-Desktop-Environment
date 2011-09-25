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


#ifndef __MDM_SESSION_RELAY_H
#define __MDM_SESSION_RELAY_H

#include <glib-object.h>

#include "mdm-session.h"

G_BEGIN_DECLS

#define MDM_TYPE_SESSION_RELAY         (mdm_session_relay_get_type ())
#define MDM_SESSION_RELAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SESSION_RELAY, MdmSessionRelay))
#define MDM_SESSION_RELAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SESSION_RELAY, MdmSessionRelayClass))
#define MDM_IS_SESSION_RELAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SESSION_RELAY))
#define MDM_IS_SESSION_RELAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SESSION_RELAY))
#define MDM_SESSION_RELAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SESSION_RELAY, MdmSessionRelayClass))

typedef struct MdmSessionRelayPrivate MdmSessionRelayPrivate;

typedef struct
{
        GObject                 parent;
        MdmSessionRelayPrivate *priv;
} MdmSessionRelay;

typedef struct
{
        GObjectClass   parent_class;

        /* Signals */
        void (* connected)               (MdmSessionRelay  *session_relay);
        void (* disconnected)            (MdmSessionRelay  *session_relay);
} MdmSessionRelayClass;

GType              mdm_session_relay_get_type          (void);
MdmSessionRelay *  mdm_session_relay_new               (void);

gboolean           mdm_session_relay_start              (MdmSessionRelay *session_relay);
gboolean           mdm_session_relay_stop               (MdmSessionRelay *session_relay);
char *             mdm_session_relay_get_address        (MdmSessionRelay *session_relay);

G_END_DECLS

#endif /* __MDM_SESSION_RELAY_H */
