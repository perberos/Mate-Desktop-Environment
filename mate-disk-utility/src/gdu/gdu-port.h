/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-port.h
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

#ifndef __GDU_PORT_H
#define __GDU_PORT_H

#include <unistd.h>
#include <sys/types.h>

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>

G_BEGIN_DECLS

#define GDU_TYPE_PORT           (gdu_port_get_type ())
#define GDU_PORT(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_PORT, GduPort))
#define GDU_PORT_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_PORT,  GduPortClass))
#define GDU_IS_PORT(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_PORT))
#define GDU_IS_PORT_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_PORT))
#define GDU_PORT_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_PORT, GduPortClass))

typedef struct _GduPortClass    GduPortClass;
typedef struct _GduPortPrivate  GduPortPrivate;

struct _GduPort
{
        GObject parent;

        /* private */
        GduPortPrivate *priv;
};

struct _GduPortClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed)     (GduPort *port);
        void (*removed)     (GduPort *port);
};

GType        gdu_port_get_type              (void);
const char  *gdu_port_get_object_path       (GduPort   *port);
GduPool     *gdu_port_get_pool              (GduPort   *port);

const gchar *gdu_port_get_native_path       (GduPort   *port);
const gchar *gdu_port_get_adapter           (GduPort   *port);
const gchar *gdu_port_get_parent            (GduPort   *port);
gint         gdu_port_get_number            (GduPort   *port);
const gchar *gdu_port_get_connector_type    (GduPort   *port);

G_END_DECLS

#endif /* __GDU_ADAPTER_H */
