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

#ifndef __GDM_CHOOSER_CLIENT_H
#define __GDM_CHOOSER_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_CLIENT         (gdm_chooser_client_get_type ())
#define GDM_CHOOSER_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CHOOSER_CLIENT, GdmChooserClient))
#define GDM_CHOOSER_CLIENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CHOOSER_CLIENT, GdmChooserClientClass))
#define GDM_IS_CHOOSER_CLIENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CHOOSER_CLIENT))
#define GDM_IS_CHOOSER_CLIENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CHOOSER_CLIENT))
#define GDM_CHOOSER_CLIENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CHOOSER_CLIENT, GdmChooserClientClass))

typedef struct GdmChooserClientPrivate GdmChooserClientPrivate;

typedef struct
{
        GObject                  parent;
        GdmChooserClientPrivate *priv;
} GdmChooserClient;

typedef struct
{
        GObjectClass   parent_class;
} GdmChooserClientClass;

#define GDM_CHOOSER_CLIENT_ERROR (gdm_chooser_client_error_quark ())

typedef enum _GdmChooserClientError {
        GDM_CHOOSER_CLIENT_ERROR_GENERIC = 0,
} GdmChooserClientError;

GType              gdm_chooser_client_get_type                       (void);
GQuark             gdm_chooser_client_error_quark                    (void);

GdmChooserClient * gdm_chooser_client_new                            (void);

gboolean           gdm_chooser_client_start                          (GdmChooserClient *client,
                                                                         GError          **error);
void               gdm_chooser_client_stop                           (GdmChooserClient *client);

void               gdm_chooser_client_call_disconnect                (GdmChooserClient *client);
void               gdm_chooser_client_call_select_hostname           (GdmChooserClient *client,
                                                                      const char       *text);

G_END_DECLS

#endif /* __GDM_CHOOSER_CLIENT_H */
