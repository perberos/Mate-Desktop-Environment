/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  format-window.h
 *
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
 *  Author: Tomas Bzatek <tbzatek@redhat.com>
 *          David Zeuthen <davidz@redhat.com>
 *
 */

#ifndef __GDU_FORMAT_DIALOG_H
#define __GDU_FORMAT_DIALOG_H

#include <gtk/gtk.h>
#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_FORMAT_DIALOG            gdu_format_dialog_get_type()
#define GDU_FORMAT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_FORMAT_DIALOG, GduFormatDialog))
#define GDU_FORMAT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDU_TYPE_FORMAT_DIALOG, GduFormatDialogClass))
#define GDU_IS_FORMAT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_FORMAT_DIALOG))
#define GDU_IS_FORMAT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDU_TYPE_FORMAT_DIALOG))
#define GDU_FORMAT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_FORMAT_DIALOG, GduFormatDialogClass))

typedef struct GduFormatDialogClass   GduFormatDialogClass;
typedef struct GduFormatDialogPrivate GduFormatDialogPrivate;

struct GduFormatDialog
{
        GduDialog parent;

        /*< private >*/
        GduFormatDialogPrivate *priv;
};

struct GduFormatDialogClass
{
        GduDialogClass parent_class;
};

GType       gdu_format_dialog_get_type           (void) G_GNUC_CONST;
GtkWidget  *gdu_format_dialog_new                (GtkWindow            *parent,
                                                  GduPresentable       *presentable,
                                                  GduFormatDialogFlags  flags);
GtkWidget  *gdu_format_dialog_new_for_drive      (GtkWindow            *parent,
                                                  GduDevice            *device,
                                                  GduFormatDialogFlags  flags);
GtkWidget  *gdu_format_dialog_new_for_volume     (GtkWindow            *parent,
                                                  GduDevice            *device,
                                                  GduFormatDialogFlags  flags);
gchar      *gdu_format_dialog_get_fs_type        (GduFormatDialog *dialog);
gchar      *gdu_format_dialog_get_fs_label       (GduFormatDialog *dialog);
gchar     **gdu_format_dialog_get_fs_options     (GduFormatDialog *dialog);
gboolean    gdu_format_dialog_get_encrypt        (GduFormatDialog *dialog);
gboolean    gdu_format_dialog_get_take_ownership (GduFormatDialog *dialog);

GtkWidget  *gdu_format_dialog_get_table          (GduFormatDialog *dialog);

G_END_DECLS

#endif  /* __GDU_FORMAT_DIALOG_H */

