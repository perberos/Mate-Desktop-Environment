/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-ata-smart-dialog.h
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

#ifndef __GDU_SLOW_UNMOUNT_DIALOG_H
#define __GDU_SLOW_UNMOUNT_DIALOG_H

#include <gdu/gdu.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_SLOW_UNMOUNT_DIALOG         gdu_slow_unmount_dialog_get_type()
#define GDU_SLOW_UNMOUNT_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SLOW_UNMOUNT_DIALOG, GduSlowUnmountDialog))
#define GDU_SLOW_UNMOUNT_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SLOW_UNMOUNT_DIALOG, GduSlowUnmountDialogClass))
#define GDU_IS_SLOW_UNMOUNT_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SLOW_UNMOUNT_DIALOG))
#define GDU_IS_SLOW_UNMOUNT_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SLOW_UNMOUNT_DIALOG))
#define GDU_SLOW_UNMOUNT_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SLOW_UNMOUNT_DIALOG, GduSlowUnmountDialogClass))

typedef struct GduSlowUnmountDialog        GduSlowUnmountDialog;
typedef struct GduSlowUnmountDialogClass   GduSlowUnmountDialogClass;
typedef struct GduSlowUnmountDialogPrivate GduSlowUnmountDialogPrivate;

struct GduSlowUnmountDialog
{
        GtkDialog parent;

        /*< private >*/
        GduSlowUnmountDialogPrivate *priv;
};

struct GduSlowUnmountDialogClass
{
        GtkDialogClass parent_class;
};

GType       gdu_slow_unmount_dialog_get_type (void) G_GNUC_CONST;
GtkWidget*  gdu_slow_unmount_dialog_new      (GtkWindow *parent,
                                              GduDevice *device);

G_END_DECLS

#endif /* __GDU_SLOW_UNMOUNT_DIALOG_H */
