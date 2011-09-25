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

#ifndef __MDM_CHOOSER_SESSION_H
#define __MDM_CHOOSER_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_CHOOSER_SESSION         (mdm_chooser_session_get_type ())
#define MDM_CHOOSER_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_CHOOSER_SESSION, MdmChooserSession))
#define MDM_CHOOSER_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_CHOOSER_SESSION, MdmChooserSessionClass))
#define MDM_IS_CHOOSER_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_CHOOSER_SESSION))
#define MDM_IS_CHOOSER_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_CHOOSER_SESSION))
#define MDM_CHOOSER_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_CHOOSER_SESSION, MdmChooserSessionClass))

typedef struct MdmChooserSessionPrivate MdmChooserSessionPrivate;

typedef struct
{
        GObject                   parent;
        MdmChooserSessionPrivate *priv;
} MdmChooserSession;

typedef struct
{
        GObjectClass   parent_class;
} MdmChooserSessionClass;

GType                  mdm_chooser_session_get_type                       (void);

MdmChooserSession    * mdm_chooser_session_new                            (void);

gboolean               mdm_chooser_session_start                          (MdmChooserSession *session,
                                                                           GError           **error);
void                   mdm_chooser_session_stop                           (MdmChooserSession *session);

G_END_DECLS

#endif /* __MDM_CHOOSER_SESSION_H */
