/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  Copyright (C) 2008-2009 Red Hat, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: David Zeuthen <davidz@redhat.com>
 *
 */

#ifndef __GDU_PARTITION_DIALOG_H
#define __GDU_PARTITION_DIALOG_H

#include <gtk/gtk.h>
#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_PARTITION_DIALOG            gdu_partition_dialog_get_type()
#define GDU_PARTITION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_PARTITION_DIALOG, GduPartitionDialog))
#define GDU_PARTITION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDU_TYPE_PARTITION_DIALOG, GduPartitionDialogClass))
#define GDU_IS_PARTITION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_PARTITION_DIALOG))
#define GDU_IS_PARTITION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDU_TYPE_PARTITION_DIALOG))
#define GDU_PARTITION_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_PARTITION_DIALOG, GduPartitionDialogClass))

typedef struct GduPartitionDialogClass   GduPartitionDialogClass;
typedef struct GduPartitionDialogPrivate GduPartitionDialogPrivate;

struct GduPartitionDialog
{
        GduDialog parent;

        /*< private >*/
        GduPartitionDialogPrivate *priv;
};

struct GduPartitionDialogClass
{
        GduDialogClass parent_class;
};

GType       gdu_partition_dialog_get_type      (void) G_GNUC_CONST;
GtkWidget  *gdu_partition_dialog_new           (GtkWindow               *parent,
                                                GduPresentable          *presentable);
GtkWidget  *gdu_partition_dialog_new_for_drive (GtkWindow               *parent,
                                                GduDevice               *device);
gchar      *gdu_partition_dialog_get_scheme    (GduPartitionDialog      *dialog);

G_END_DECLS

#endif  /* __GDU_PARTITION_DIALOG_H */

