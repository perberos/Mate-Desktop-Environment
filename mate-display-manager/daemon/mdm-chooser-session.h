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


#ifndef __MDM_CHOOSER_SESSION_H
#define __MDM_CHOOSER_SESSION_H

#include <glib-object.h>

#include "mdm-welcome-session.h"

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
        MdmWelcomeSession         parent;
        MdmChooserSessionPrivate *priv;
} MdmChooserSession;

typedef struct
{
        MdmWelcomeSessionClass    parent_class;
} MdmChooserSessionClass;

GType                 mdm_chooser_session_get_type           (void);
MdmChooserSession *   mdm_chooser_session_new                (const char        *display_name,
                                                              const char        *display_device,
                                                              const char        *display_hostname);

G_END_DECLS

#endif /* __MDM_CHOOSER_SESSION_H */
