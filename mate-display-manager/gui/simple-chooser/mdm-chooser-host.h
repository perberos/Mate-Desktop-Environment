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

#ifndef __MDM_CHOOSER_HOST__
#define __MDM_CHOOSER_HOST__

#include <glib-object.h>
#include "mdm-address.h"

G_BEGIN_DECLS

#define MDM_TYPE_CHOOSER_HOST         (mdm_chooser_host_get_type ())
#define MDM_CHOOSER_HOST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_CHOOSER_HOST, MdmChooserHost))
#define MDM_CHOOSER_HOST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_CHOOSER_HOST, MdmChooserHostClass))
#define MDM_IS_CHOOSER_HOST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_CHOOSER_HOST))
#define MDM_IS_CHOOSER_HOST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_CHOOSER_HOST))
#define MDM_CHOOSER_HOST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_CHOOSER_HOST, MdmChooserHostClass))

typedef enum {
        MDM_CHOOSER_HOST_KIND_XDMCP = 1 << 0,
} MdmChooserHostKind;

#define MDM_CHOOSER_HOST_KIND_MASK_ALL (MDM_CHOOSER_HOST_KIND_XDMCP)

typedef struct MdmChooserHostPrivate MdmChooserHostPrivate;

typedef struct
{
        GObject                parent;
        MdmChooserHostPrivate *priv;
} MdmChooserHost;

typedef struct
{
        GObjectClass   parent_class;
} MdmChooserHostClass;

GType                 mdm_chooser_host_get_type            (void) G_GNUC_CONST;

const char* mdm_chooser_host_get_description(MdmChooserHost* chooser_host);
MdmAddress *          mdm_chooser_host_get_address         (MdmChooserHost   *chooser_host);
gboolean              mdm_chooser_host_get_willing         (MdmChooserHost   *chooser_host);
MdmChooserHostKind    mdm_chooser_host_get_kind            (MdmChooserHost   *chooser_host);

G_END_DECLS

#endif
