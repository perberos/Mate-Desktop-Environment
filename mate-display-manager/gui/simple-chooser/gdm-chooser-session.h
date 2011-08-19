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

#ifndef __GDM_CHOOSER_SESSION_H
#define __GDM_CHOOSER_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_SESSION         (gdm_chooser_session_get_type ())
#define GDM_CHOOSER_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CHOOSER_SESSION, GdmChooserSession))
#define GDM_CHOOSER_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CHOOSER_SESSION, GdmChooserSessionClass))
#define GDM_IS_CHOOSER_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CHOOSER_SESSION))
#define GDM_IS_CHOOSER_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CHOOSER_SESSION))
#define GDM_CHOOSER_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CHOOSER_SESSION, GdmChooserSessionClass))

typedef struct GdmChooserSessionPrivate GdmChooserSessionPrivate;

typedef struct
{
        GObject                   parent;
        GdmChooserSessionPrivate *priv;
} GdmChooserSession;

typedef struct
{
        GObjectClass   parent_class;
} GdmChooserSessionClass;

GType                  gdm_chooser_session_get_type                       (void);

GdmChooserSession    * gdm_chooser_session_new                            (void);

gboolean               gdm_chooser_session_start                          (GdmChooserSession *session,
                                                                           GError           **error);
void                   gdm_chooser_session_stop                           (GdmChooserSession *session);

G_END_DECLS

#endif /* __GDM_CHOOSER_SESSION_H */
