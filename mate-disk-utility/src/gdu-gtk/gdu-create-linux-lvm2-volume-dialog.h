/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  Copyright (C) 2008-2010 Red Hat, Inc.
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

#ifndef __GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG_H
#define __GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG_H

#include <gtk/gtk.h>
#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG            gdu_create_linux_lvm2_volume_dialog_get_type()
#define GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG, GduCreateLinuxLvm2VolumeDialog))
#define GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG, GduCreateLinuxLvm2VolumeDialogClass))
#define GDU_IS_CREATE_LINUX_LVM2_VOLUME_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG))
#define GDU_IS_CREATE_LINUX_LVM2_VOLUME_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG))
#define GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_CREATE_LINUX_LVM2_VOLUME_DIALOG, GduCreateLinuxLvm2VolumeDialogClass))

typedef struct GduCreateLinuxLvm2VolumeDialogClass   GduCreateLinuxLvm2VolumeDialogClass;
typedef struct GduCreateLinuxLvm2VolumeDialogPrivate GduCreateLinuxLvm2VolumeDialogPrivate;

struct GduCreateLinuxLvm2VolumeDialog
{
        GduFormatDialog parent;

        /*< private >*/
        GduCreateLinuxLvm2VolumeDialogPrivate *priv;
};

struct GduCreateLinuxLvm2VolumeDialogClass
{
        GduFormatDialogClass parent_class;
};

GType       gdu_create_linux_lvm2_volume_dialog_get_type       (void) G_GNUC_CONST;
GtkWidget  *gdu_create_linux_lvm2_volume_dialog_new            (GtkWindow                *parent,
                                                                GduPresentable           *presentable,
                                                                guint64                   max_size,
                                                                GduFormatDialogFlags      flags);
guint64     gdu_create_linux_lvm2_volume_dialog_get_size       (GduCreateLinuxLvm2VolumeDialog *dialog);
guint64     gdu_create_linux_lvm2_volume_dialog_get_max_size   (GduCreateLinuxLvm2VolumeDialog *dialog);

G_END_DECLS

#endif  /* __GDU_CREATE_LINUX_LVM2_VOLUME_DIALOG_H */

