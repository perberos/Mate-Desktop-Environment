/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-expander.h
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

#ifndef __GDU_EXPANDER_H
#define __GDU_EXPANDER_H

#include <unistd.h>
#include <sys/types.h>

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>

G_BEGIN_DECLS

#define GDU_TYPE_EXPANDER           (gdu_expander_get_type ())
#define GDU_EXPANDER(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_EXPANDER, GduExpander))
#define GDU_EXPANDER_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_EXPANDER,  GduExpanderClass))
#define GDU_IS_EXPANDER(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_EXPANDER))
#define GDU_IS_EXPANDER_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_EXPANDER))
#define GDU_EXPANDER_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_EXPANDER, GduExpanderClass))

typedef struct _GduExpanderClass    GduExpanderClass;
typedef struct _GduExpanderPrivate  GduExpanderPrivate;

struct _GduExpander
{
        GObject parent;

        /* private */
        GduExpanderPrivate *priv;
};

struct _GduExpanderClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed)     (GduExpander *expander);
        void (*removed)     (GduExpander *expander);
};

GType        gdu_expander_get_type              (void);
const char  *gdu_expander_get_object_path       (GduExpander   *expander);
GduPool     *gdu_expander_get_pool              (GduExpander   *expander);

const gchar *gdu_expander_get_native_path       (GduExpander   *expander);
const gchar *gdu_expander_get_vendor            (GduExpander   *expander);
const gchar *gdu_expander_get_model             (GduExpander   *expander);
const gchar *gdu_expander_get_revision          (GduExpander   *expander);
guint        gdu_expander_get_num_ports         (GduExpander   *expander);
gchar      **gdu_expander_get_upstream_ports    (GduExpander   *expander);
const gchar *gdu_expander_get_adapter           (GduExpander   *expander);

G_END_DECLS

#endif /* __GDU_EXPANDER_H */
