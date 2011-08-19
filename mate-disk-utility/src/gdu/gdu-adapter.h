/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-adapter.h
 *
 * Copyright (C) 2009 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_ADAPTER_H
#define __GDU_ADAPTER_H

#include <unistd.h>
#include <sys/types.h>

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>

G_BEGIN_DECLS

#define GDU_TYPE_ADAPTER           (gdu_adapter_get_type ())
#define GDU_ADAPTER(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_ADAPTER, GduAdapter))
#define GDU_ADAPTER_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_ADAPTER,  GduAdapterClass))
#define GDU_IS_ADAPTER(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_ADAPTER))
#define GDU_IS_ADAPTER_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_ADAPTER))
#define GDU_ADAPTER_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_ADAPTER, GduAdapterClass))

typedef struct _GduAdapterClass    GduAdapterClass;
typedef struct _GduAdapterPrivate  GduAdapterPrivate;

struct _GduAdapter
{
        GObject parent;

        /* private */
        GduAdapterPrivate *priv;
};

struct _GduAdapterClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed)     (GduAdapter *adapter);
        void (*removed)     (GduAdapter *adapter);
};

GType        gdu_adapter_get_type              (void);
const char  *gdu_adapter_get_object_path       (GduAdapter   *adapter);
GduPool     *gdu_adapter_get_pool              (GduAdapter   *adapter);

const gchar *gdu_adapter_get_native_path       (GduAdapter   *adapter);
const gchar *gdu_adapter_get_vendor            (GduAdapter   *adapter);
const gchar *gdu_adapter_get_model             (GduAdapter   *adapter);
const gchar *gdu_adapter_get_driver            (GduAdapter   *adapter);
const gchar *gdu_adapter_get_fabric            (GduAdapter   *adapter);
guint        gdu_adapter_get_num_ports         (GduAdapter   *adapter);

G_END_DECLS

#endif /* __GDU_ADAPTER_H */
