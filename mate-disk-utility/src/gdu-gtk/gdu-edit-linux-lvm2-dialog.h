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

#ifndef __GDU_EDIT_LINUX_LVM2_DIALOG_H
#define __GDU_EDIT_LINUX_LVM2_DIALOG_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_EDIT_LINUX_LVM2_DIALOG         (gdu_edit_linux_lvm2_dialog_get_type())
#define GDU_EDIT_LINUX_LVM2_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_EDIT_LINUX_LVM2_DIALOG, GduEditLinuxLvm2Dialog))
#define GDU_EDIT_LINUX_LVM2_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_EDIT_LINUX_LVM2_DIALOG, GduEditLinuxLvm2DialogClass))
#define GDU_IS_EDIT_LINUX_LVM2_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_EDIT_LINUX_LVM2_DIALOG))
#define GDU_IS_EDIT_LINUX_LVM2_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_EDIT_LINUX_LVM2_DIALOG))
#define GDU_EDIT_LINUX_LVM2_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_EDIT_LINUX_LVM2_DIALOG, GduEditLinuxLvm2DialogClass))

typedef struct GduEditLinuxLvm2DialogClass   GduEditLinuxLvm2DialogClass;
typedef struct GduEditLinuxLvm2DialogPrivate GduEditLinuxLvm2DialogPrivate;

struct GduEditLinuxLvm2Dialog
{
        GduDialog parent;

        /*< private >*/
        GduEditLinuxLvm2DialogPrivate *priv;
};

struct GduEditLinuxLvm2DialogClass
{
        GduDialogClass parent_class;

        void (*new_button_clicked)    (GduEditLinuxLvm2Dialog *dialog);
        void (*remove_button_clicked) (GduEditLinuxLvm2Dialog *dialog,
                                       const gchar            *pv_uuid);
};

GType       gdu_edit_linux_lvm2_dialog_get_type           (void) G_GNUC_CONST;
GtkWidget*  gdu_edit_linux_lvm2_dialog_new                (GtkWindow                *parent,
                                                           GduLinuxLvm2VolumeGroup  *vg);

G_END_DECLS

#endif  /* __GDU_EDIT_LINUX_LVM2_DIALOG_H */

