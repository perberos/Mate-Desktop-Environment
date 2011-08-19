/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Jacob Berkman <jacob@ximian.com>
 */

#ifndef GST_DIALOG_H
#define GST_DIALOG_H

G_BEGIN_DECLS

#include "gst-tool.h"

#define GST_TYPE_DIALOG        (gst_dialog_get_type ())
#define GST_DIALOG(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o),  GST_TYPE_DIALOG, GstDialog))
#define GST_DIALOG_CLASS(c)    (G_TYPE_CHECK_CLASS_CAST ((c), GST_TYPE_DIALOG, GstDialogClass))
#define GST_IS_DIALOG(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DIALOG))
#define GST_IS_DIALOG_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), GST_TYPE_DIALOG))

typedef struct _GstDialog       GstDialog;
typedef struct _GstDialogClass  GstDialogClass;
typedef struct _GstDialogSignal GstDialogSignal;

struct _GstDialogSignal {
	const char *widget;
	const char *signal_name;
	GCallback   func;
};

struct _GstDialog {
	GtkDialog dialog;
};

struct _GstDialogClass {
	GtkDialogClass parent_class;

	void (* lock_changed) (GstDialogClass *button);
};

GType               gst_dialog_get_type            (void);

GstDialog          *gst_dialog_new                 (GstTool *tool, 
						    const char *widget, 
						    const char *title,
						    gboolean    lock_button);

void                gst_dialog_connect_signals     (GstDialog *xd, GstDialogSignal *signals);
void                gst_dialog_connect_signals_after (GstDialog *xd, GstDialogSignal *signals);

void                gst_dialog_freeze              (GstDialog *xd);
void                gst_dialog_thaw                (GstDialog *xd);
guint               gst_dialog_get_freeze_level    (GstDialog *dialog);

gboolean            gst_dialog_get_modified        (GstDialog *xd);
void                gst_dialog_set_modified        (GstDialog *xd, gboolean state);
void                gst_dialog_modify              (GstDialog *xd);
void                gst_dialog_modify_cb           (GtkWidget *w, gpointer data);

GtkWidget          *gst_dialog_get_widget          (GstDialog *xd, const char *widget);
void                gst_dialog_try_set_sensitive   (GstDialog *xd, GtkWidget *w, gboolean sensitive);

GstTool            *gst_dialog_get_tool            (GstDialog *xd);

void                gst_dialog_authenticate        (GstDialog *dialog);
gboolean            gst_dialog_is_authenticated    (GstDialog *dialog);
void                gst_dialog_require_authentication_for_widget  (GstDialog *xd, GtkWidget *w);
void                gst_dialog_require_authentication_for_widgets (GstDialog *xd, const gchar **names);

void                gst_dialog_add_edit_dialog     (GstDialog *dialog,
						    GtkWidget *edit_dialog);
void                gst_dialog_remove_edit_dialog  (GstDialog *dialog,
						    GtkWidget *edit_dialog);
void                gst_dialog_stop_editing        (GstDialog *dialog);
gboolean            gst_dialog_get_editing         (GstDialog *dialog);
GtkWidget *         gst_dialog_get_topmost_edit_dialog (GstDialog *dialog);


G_END_DECLS

#endif /* GST_DIALOG_H */
