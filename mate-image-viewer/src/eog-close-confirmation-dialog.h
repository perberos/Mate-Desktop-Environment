/*
 * eog-close-confirmation-dialog.h
 * This file is part of eog
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.h) by gedit Team
 *
 * Copyright (C) 2004-2009 MATE Foundation 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

#ifndef __EOG_CLOSE_CONFIRMATION_DIALOG_H__
#define __EOG_CLOSE_CONFIRMATION_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>

#include <eog-image.h>

#define EOG_TYPE_CLOSE_CONFIRMATION_DIALOG		(eog_close_confirmation_dialog_get_type ())
#define EOG_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_CLOSE_CONFIRMATION_DIALOG, EogCloseConfirmationDialog))
#define EOG_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), EOG_TYPE_CLOSE_CONFIRMATION_DIALOG, EogCloseConfirmationDialogClass))
#define EOG_IS_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define EOG_IS_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define EOG_CLOSE_CONFIRMATION_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),EOG_TYPE_CLOSE_CONFIRMATION_DIALOG, EogCloseConfirmationDialogClass))

typedef struct _EogCloseConfirmationDialog 		EogCloseConfirmationDialog;
typedef struct _EogCloseConfirmationDialogClass 	EogCloseConfirmationDialogClass;
typedef struct _EogCloseConfirmationDialogPrivate 	EogCloseConfirmationDialogPrivate;

struct _EogCloseConfirmationDialog 
{
	GtkDialog parent;

	/*< private > */
	EogCloseConfirmationDialogPrivate *priv;
};

struct _EogCloseConfirmationDialogClass 
{
	GtkDialogClass parent_class;
};

G_GNUC_INTERNAL
GType 		 eog_close_confirmation_dialog_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*eog_close_confirmation_dialog_new			(GtkWindow     *parent,
									 GList         *unsaved_documents);
G_GNUC_INTERNAL
GtkWidget 	*eog_close_confirmation_dialog_new_single 		(GtkWindow     *parent, 
									 EogImage      *image);

G_GNUC_INTERNAL
const GList	*eog_close_confirmation_dialog_get_unsaved_images	(EogCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
GList		*eog_close_confirmation_dialog_get_selected_images	(EogCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
void		 eog_close_confirmation_dialog_set_sensitive		(EogCloseConfirmationDialog *dlg, gboolean value);

#endif /* __EOG_CLOSE_CONFIRMATION_DIALOG_H__ */

