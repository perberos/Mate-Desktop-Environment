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


#ifndef __GDM_SESSION_RELAY_H
#define __GDM_SESSION_RELAY_H

#include <glib-object.h>

#include "gdm-session.h"

G_BEGIN_DECLS

#define GDM_TYPE_SESSION_RELAY         (gdm_session_relay_get_type ())
#define GDM_SESSION_RELAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SESSION_RELAY, GdmSessionRelay))
#define GDM_SESSION_RELAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SESSION_RELAY, GdmSessionRelayClass))
#define GDM_IS_SESSION_RELAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SESSION_RELAY))
#define GDM_IS_SESSION_RELAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SESSION_RELAY))
#define GDM_SESSION_RELAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SESSION_RELAY, GdmSessionRelayClass))

typedef struct GdmSessionRelayPrivate GdmSessionRelayPrivate;

typedef struct
{
        GObject                 parent;
        GdmSessionRelayPrivate *priv;
} GdmSessionRelay;

typedef struct
{
        GObjectClass   parent_class;

        /* Signals */
        void (* connected)               (GdmSessionRelay  *session_relay);
        void (* disconnected)            (GdmSessionRelay  *session_relay);
} GdmSessionRelayClass;

GType              gdm_session_relay_get_type          (void);
GdmSessionRelay *  gdm_session_relay_new               (void);

gboolean           gdm_session_relay_start              (GdmSessionRelay *session_relay);
gboolean           gdm_session_relay_stop               (GdmSessionRelay *session_relay);
char *             gdm_session_relay_get_address        (GdmSessionRelay *session_relay);

G_END_DECLS

#endif /* __GDM_SESSION_RELAY_H */
