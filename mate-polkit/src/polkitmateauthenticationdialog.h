/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __POLKIT_MATE_AUTHENTICATION_DIALOG_H
#define __POLKIT_MATE_AUTHENTICATION_DIALOG_H

#include <gtk/gtk.h>
#include <polkit/polkit.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG            (polkit_mate_authentication_dialog_get_type ())
#define POLKIT_MATE_AUTHENTICATION_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG, PolkitMateAuthenticationDialog))
#define POLKIT_MATE_AUTHENTICATION_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG, PolkitMateAuthenticationDialogClass))
#define POLKIT_MATE_IS_AUTHENTICATION_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG))
#define POLKIT_MATE_IS_AUTHENTICATION_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), POLKIT_MATE_TYPE_AUTHENTICATION_DIALOG))

typedef struct _PolkitMateAuthenticationDialog        PolkitMateAuthenticationDialog;
typedef struct _PolkitMateAuthenticationDialogClass   PolkitMateAuthenticationDialogClass;
typedef struct _PolkitMateAuthenticationDialogPrivate PolkitMateAuthenticationDialogPrivate;

struct _PolkitMateAuthenticationDialog
{
  GtkDialog parent_instance;
  PolkitMateAuthenticationDialogPrivate *priv;
};

struct _PolkitMateAuthenticationDialogClass
{
  GtkDialogClass parent_class;
};

GType      polkit_mate_authentication_dialog_get_type                      (void);
GtkWidget *polkit_mate_authentication_dialog_new                           (const gchar    *action_id,
                                                                             const gchar    *vendor,
                                                                             const gchar    *vendor_url,
                                                                             const gchar    *icon_name,
                                                                             const gchar    *message_markup,
                                                                             PolkitDetails  *details,
                                                                             gchar         **users);
gchar     *polkit_mate_authentication_dialog_get_selected_user             (PolkitMateAuthenticationDialog *dialog);
gboolean   polkit_mate_authentication_dialog_run_until_user_is_selected    (PolkitMateAuthenticationDialog *dialog);
gchar     *polkit_mate_authentication_dialog_run_until_response_for_prompt (PolkitMateAuthenticationDialog *dialog,
                                                                             const gchar                     *prompt,
                                                                             gboolean                         echo_chars,
                                                                             gboolean                        *was_cancelled,
                                                                             gboolean                        *new_user_selected);
gboolean   polkit_mate_authentication_dialog_cancel                        (PolkitMateAuthenticationDialog *dialog);
void       polkit_mate_authentication_dialog_indicate_error                (PolkitMateAuthenticationDialog *dialog);
void       polkit_mate_authentication_dialog_set_info_message              (PolkitMateAuthenticationDialog *dialog,
                                                                             const gchar                     *info_markup);

#ifdef __cplusplus
}
#endif

#endif /* __POLKIT_MATE_AUTHENTICATION_DIALOG_H */
