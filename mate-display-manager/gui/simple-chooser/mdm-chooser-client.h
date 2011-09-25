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

#ifndef __MDM_CHOOSER_CLIENT_H
#define __MDM_CHOOSER_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_CHOOSER_CLIENT         (mdm_chooser_client_get_type ())
#define MDM_CHOOSER_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_CHOOSER_CLIENT, MdmChooserClient))
#define MDM_CHOOSER_CLIENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_CHOOSER_CLIENT, MdmChooserClientClass))
#define MDM_IS_CHOOSER_CLIENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_CHOOSER_CLIENT))
#define MDM_IS_CHOOSER_CLIENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_CHOOSER_CLIENT))
#define MDM_CHOOSER_CLIENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_CHOOSER_CLIENT, MdmChooserClientClass))

typedef struct MdmChooserClientPrivate MdmChooserClientPrivate;

typedef struct
{
        GObject                  parent;
        MdmChooserClientPrivate *priv;
} MdmChooserClient;

typedef struct
{
        GObjectClass   parent_class;
} MdmChooserClientClass;

#define MDM_CHOOSER_CLIENT_ERROR (mdm_chooser_client_error_quark ())

typedef enum _MdmChooserClientError {
        MDM_CHOOSER_CLIENT_ERROR_GENERIC = 0,
} MdmChooserClientError;

GType              mdm_chooser_client_get_type                       (void);
GQuark             mdm_chooser_client_error_quark                    (void);

MdmChooserClient * mdm_chooser_client_new                            (void);

gboolean           mdm_chooser_client_start                          (MdmChooserClient *client,
                                                                         GError          **error);
void               mdm_chooser_client_stop                           (MdmChooserClient *client);

void               mdm_chooser_client_call_disconnect                (MdmChooserClient *client);
void               mdm_chooser_client_call_select_hostname           (MdmChooserClient *client,
                                                                      const char       *text);

G_END_DECLS

#endif /* __MDM_CHOOSER_CLIENT_H */
