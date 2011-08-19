/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GDM_CHOOSER_HOST__
#define __GDM_CHOOSER_HOST__

#include <glib-object.h>
#include "gdm-address.h"

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_HOST         (gdm_chooser_host_get_type ())
#define GDM_CHOOSER_HOST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CHOOSER_HOST, GdmChooserHost))
#define GDM_CHOOSER_HOST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CHOOSER_HOST, GdmChooserHostClass))
#define GDM_IS_CHOOSER_HOST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CHOOSER_HOST))
#define GDM_IS_CHOOSER_HOST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CHOOSER_HOST))
#define GDM_CHOOSER_HOST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CHOOSER_HOST, GdmChooserHostClass))

typedef enum {
        GDM_CHOOSER_HOST_KIND_XDMCP = 1 << 0,
} GdmChooserHostKind;

#define GDM_CHOOSER_HOST_KIND_MASK_ALL (GDM_CHOOSER_HOST_KIND_XDMCP)

typedef struct GdmChooserHostPrivate GdmChooserHostPrivate;

typedef struct
{
        GObject                parent;
        GdmChooserHostPrivate *priv;
} GdmChooserHost;

typedef struct
{
        GObjectClass   parent_class;
} GdmChooserHostClass;

GType                 gdm_chooser_host_get_type            (void) G_GNUC_CONST;

G_CONST_RETURN char  *gdm_chooser_host_get_description     (GdmChooserHost   *chooser_host);
GdmAddress *          gdm_chooser_host_get_address         (GdmChooserHost   *chooser_host);
gboolean              gdm_chooser_host_get_willing         (GdmChooserHost   *chooser_host);
GdmChooserHostKind    gdm_chooser_host_get_kind            (GdmChooserHost   *chooser_host);

G_END_DECLS

#endif
