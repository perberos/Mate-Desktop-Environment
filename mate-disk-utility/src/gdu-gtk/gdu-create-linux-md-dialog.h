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

#ifndef __GDU_CREATE_LINUX_MD_DIALOG_H
#define __GDU_CREATE_LINUX_MD_DIALOG_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_CREATE_LINUX_MD_DIALOG         (gdu_create_linux_md_dialog_get_type())
#define GDU_CREATE_LINUX_MD_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_CREATE_LINUX_MD_DIALOG, GduCreateLinuxMdDialog))
#define GDU_CREATE_LINUX_MD_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_CREATE_LINUX_MD_DIALOG, GduCreateLinuxMdDialogClass))
#define GDU_IS_CREATE_LINUX_MD_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_CREATE_LINUX_MD_DIALOG))
#define GDU_IS_CREATE_LINUX_MD_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_CREATE_LINUX_MD_DIALOG))
#define GDU_CREATE_LINUX_MD_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_CREATE_LINUX_MD_DIALOG, GduCreateLinuxMdDialogClass))

typedef struct GduCreateLinuxMdDialogClass   GduCreateLinuxMdDialogClass;
typedef struct GduCreateLinuxMdDialogPrivate GduCreateLinuxMdDialogPrivate;

struct GduCreateLinuxMdDialog
{
        GtkDialog parent;

        /*< private >*/
        GduCreateLinuxMdDialogPrivate *priv;
};

struct GduCreateLinuxMdDialogClass
{
        GtkDialogClass parent_class;
};

GType       gdu_create_linux_md_dialog_get_type           (void) G_GNUC_CONST;
GtkWidget*  gdu_create_linux_md_dialog_new                (GtkWindow               *parent,
                                                           GduPool                 *pool);
gchar      *gdu_create_linux_md_dialog_get_level          (GduCreateLinuxMdDialog  *dialog);
gchar      *gdu_create_linux_md_dialog_get_name           (GduCreateLinuxMdDialog  *dialog);
guint64     gdu_create_linux_md_dialog_get_size           (GduCreateLinuxMdDialog  *dialog);
guint64     gdu_create_linux_md_dialog_get_component_size (GduCreateLinuxMdDialog  *dialog);
guint64     gdu_create_linux_md_dialog_get_stripe_size    (GduCreateLinuxMdDialog  *dialog);
GPtrArray  *gdu_create_linux_md_dialog_get_drives         (GduCreateLinuxMdDialog  *dialog);

G_END_DECLS

#endif  /* __GDU_CREATE_LINUX_MD_DIALOG_H */

