/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-password-dialog.h - A use password prompting dialog widget.

   Copyright (C) 1999, 2000 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

#ifndef MATE_PASSWORD_DIALOG_H
#define MATE_PASSWORD_DIALOG_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_PASSWORD_DIALOG            (mate_password_dialog_get_type ())
#define MATE_PASSWORD_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_PASSWORD_DIALOG, MatePasswordDialog))
#define MATE_PASSWORD_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_PASSWORD_DIALOG, MatePasswordDialogClass))
#define MATE_IS_PASSWORD_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_PASSWORD_DIALOG))
#define MATE_IS_PASSWORD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_PASSWORD_DIALOG))

typedef struct _MatePasswordDialog        MatePasswordDialog;
typedef struct _MatePasswordDialogClass   MatePasswordDialogClass;
typedef struct _MatePasswordDialogDetails MatePasswordDialogDetails;

struct _MatePasswordDialog
{
	GtkDialog gtk_dialog;

	MatePasswordDialogDetails *details;
};

struct _MatePasswordDialogClass
{
	GtkDialogClass parent_class;
};

typedef enum {
	MATE_PASSWORD_DIALOG_REMEMBER_NOTHING,
	MATE_PASSWORD_DIALOG_REMEMBER_SESSION,
	MATE_PASSWORD_DIALOG_REMEMBER_FOREVER
} MatePasswordDialogRemember;

typedef gdouble (* MatePasswordDialogQualityFunc) (MatePasswordDialog *password_dialog,
 						    const char *password,
						    gpointer user_data);

GType    mate_password_dialog_get_type (void);
GtkWidget* mate_password_dialog_new      (const char *dialog_title,
					   const char *message,
					   const char *username,
					   const char *password,
					   gboolean    readonly_username);

gboolean   mate_password_dialog_run_and_block           (MatePasswordDialog *password_dialog);

/* Attribute mutators */
void mate_password_dialog_set_show_username       (MatePasswordDialog *password_dialog,
						    gboolean             show);
void mate_password_dialog_set_show_domain         (MatePasswordDialog *password_dialog,
						    gboolean             show);
void mate_password_dialog_set_show_password       (MatePasswordDialog *password_dialog,
						    gboolean             show);
void mate_password_dialog_set_show_new_password   (MatePasswordDialog *password_dialog,
						    gboolean             show);
void mate_password_dialog_set_show_new_password_quality (MatePasswordDialog *password_dialog,
							  gboolean             show);
void mate_password_dialog_set_username            (MatePasswordDialog *password_dialog,
						    const char          *username);
void mate_password_dialog_set_domain              (MatePasswordDialog *password_dialog,
						    const char          *domain);
void mate_password_dialog_set_password            (MatePasswordDialog *password_dialog,
						    const char          *password);
void mate_password_dialog_set_new_password        (MatePasswordDialog *password_dialog,
						    const char          *password);
void mate_password_dialog_set_password_quality_func (MatePasswordDialog *password_dialog,
						      MatePasswordDialogQualityFunc func,
						      gpointer data,
						      GDestroyNotify dnotify);
void mate_password_dialog_set_readonly_username   (MatePasswordDialog *password_dialog,
						    gboolean             readonly);
void mate_password_dialog_set_readonly_domain     (MatePasswordDialog *password_dialog,
						    gboolean             readonly);

void                        mate_password_dialog_set_show_remember (MatePasswordDialog         *password_dialog,
								     gboolean                     show_remember);
void                        mate_password_dialog_set_remember      (MatePasswordDialog         *password_dialog,
								     MatePasswordDialogRemember  remember);
MatePasswordDialogRemember mate_password_dialog_get_remember      (MatePasswordDialog         *password_dialog);
void                        mate_password_dialog_set_show_userpass_buttons (MatePasswordDialog         *password_dialog,
                                                                     	     gboolean                     show_userpass_buttons);

/* Attribute accessors */
char *     mate_password_dialog_get_username            (MatePasswordDialog *password_dialog);
char *     mate_password_dialog_get_domain              (MatePasswordDialog *password_dialog);
char *     mate_password_dialog_get_password            (MatePasswordDialog *password_dialog);
char *     mate_password_dialog_get_new_password        (MatePasswordDialog *password_dialog);

gboolean   mate_password_dialog_anon_selected 		 (MatePasswordDialog *password_dialog);

#ifdef __cplusplus
}
#endif

#endif /* MATE_PASSWORD_DIALOG_H */
