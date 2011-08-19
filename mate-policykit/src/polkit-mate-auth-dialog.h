/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef POLKIT_MATE_AUTH_DIALOG_H
#define POLKIT_MATE_AUTH_DIALOG_H

#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION "/desktop/mate/policykit/auth_dialog_retain_authorization"
#define KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST "/desktop/mate/policykit/auth_dialog_retain_authorization_blacklist"

#define POLKIT_MATE_TYPE_AUTH_DIALOG            (polkit_mate_auth_dialog_get_type ())
#define POLKIT_MATE_AUTH_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POLKIT_MATE_TYPE_AUTH_DIALOG, PolkitMateAuthDialog))
#define POLKIT_MATE_AUTH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POLKIT_MATE_TYPE_AUTH_DIALOG, PolkitMateAuthDialogClass))
#define POLKIT_MATE_IS_AUTH_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POLKIT_MATE_TYPE_AUTH_DIALOG))
#define POLKIT_MATE_IS_AUTH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POLKIT_MATE_TYPE_AUTH_DIALOG))

typedef struct _PolkitMateAuthDialog        PolkitMateAuthDialog;
typedef struct _PolkitMateAuthDialogClass   PolkitMateAuthDialogClass;
typedef struct _PolkitMateAuthDialogPrivate PolkitMateAuthDialogPrivate;

struct _PolkitMateAuthDialog
{
	GtkDialog gtk_dialog;
	PolkitMateAuthDialogPrivate *priv;
};

struct _PolkitMateAuthDialogClass
{
	GtkDialogClass parent_class;

	/* Signals: */

	void (*user_selected) (PolkitMateAuthDialog *auth_dialog, const char *user_name);
};

GType    polkit_mate_auth_dialog_get_type      (void);
GtkWidget *polkit_mate_auth_dialog_new (const char *path_to_program,
					 const char *action_id,
					 const char *vendor,
					 const char *vendor_url,
					 const char *icon_name,
					 const char *message_markup);

void        polkit_mate_auth_dialog_set_prompt (PolkitMateAuthDialog *auth_dialog,
						 const char *prompt,
						 gboolean show_chars);

const char *polkit_mate_auth_dialog_get_password (PolkitMateAuthDialog *auth_dialog);

void        polkit_mate_auth_dialog_set_options (PolkitMateAuthDialog *auth_dialog, 
						  gboolean session, 
						  gboolean always,
						  gboolean requires_admin,
						  char **admin_users);

gboolean    polkit_mate_auth_dialog_get_remember_session (PolkitMateAuthDialog *auth_dialog);
gboolean    polkit_mate_auth_dialog_get_remember_always  (PolkitMateAuthDialog *auth_dialog);
gboolean    polkit_mate_auth_dialog_get_apply_all        (PolkitMateAuthDialog *auth_dialog);

void        polkit_mate_auth_dialog_select_admin_user (PolkitMateAuthDialog *auth_dialog, const char *admin_user);

void        polkit_mate_auth_dialog_indicate_auth_error (PolkitMateAuthDialog *auth_dialog);

G_END_DECLS

#endif /* POLKIT_MATE_AUTH_DIALOG_H */
