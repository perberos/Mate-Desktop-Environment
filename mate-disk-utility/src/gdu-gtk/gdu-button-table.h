/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
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
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __GDU_BUTTON_TABLE_H
#define __GDU_BUTTON_TABLE_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_BUTTON_TABLE         (gdu_button_table_get_type())
#define GDU_BUTTON_TABLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_BUTTON_TABLE, GduButtonTable))
#define GDU_BUTTON_TABLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_BUTTON_TABLE, GduButtonTableClass))
#define GDU_IS_BUTTON_TABLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_BUTTON_TABLE))
#define GDU_IS_BUTTON_TABLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_BUTTON_TABLE))
#define GDU_BUTTON_TABLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_BUTTON_TABLE, GduButtonTableClass))

typedef struct GduButtonTableClass   GduButtonTableClass;
typedef struct GduButtonTablePrivate GduButtonTablePrivate;

struct GduButtonTable
{
        GtkHBox parent;

        /*< private >*/
        GduButtonTablePrivate *priv;
};

struct GduButtonTableClass
{
        GtkHBoxClass parent_class;
};

GType       gdu_button_table_get_type        (void) G_GNUC_CONST;
GtkWidget*  gdu_button_table_new             (guint            num_columns,
                                              GPtrArray       *elements);
guint       gdu_button_table_get_num_columns (GduButtonTable *table);
GPtrArray  *gdu_button_table_get_elements    (GduButtonTable *table);
void        gdu_button_table_set_num_columns (GduButtonTable *table,
                                              guint            num_columns);
void        gdu_button_table_set_elements    (GduButtonTable *table,
                                              GPtrArray       *elements);

G_END_DECLS

#endif  /* __GDU_BUTTON_TABLE_H */

