/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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

#ifndef __GSM_APP_DIALOG_H
#define __GSM_APP_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_APP_DIALOG         (gsm_app_dialog_get_type ())
#define GSM_APP_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSM_TYPE_APP_DIALOG, GsmAppDialog))
#define GSM_APP_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSM_TYPE_APP_DIALOG, GsmAppDialogClass))
#define GSM_IS_APP_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSM_TYPE_APP_DIALOG))
#define GSM_IS_APP_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSM_TYPE_APP_DIALOG))
#define GSM_APP_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSM_TYPE_APP_DIALOG, GsmAppDialogClass))

typedef struct GsmAppDialogPrivate GsmAppDialogPrivate;

typedef struct
{
        GtkDialog            parent;
        GsmAppDialogPrivate *priv;
} GsmAppDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} GsmAppDialogClass;

GType                  gsm_app_dialog_get_type           (void);

GtkWidget            * gsm_app_dialog_new                (const char   *name,
                                                          const char   *command,
                                                          const char   *comment);

gboolean               gsm_app_dialog_run               (GsmAppDialog  *dialog,
                                                         char         **name_p,
                                                         char         **command_p,
                                                         char         **comment_p);

const char *           gsm_app_dialog_get_name           (GsmAppDialog *dialog);
const char *           gsm_app_dialog_get_command        (GsmAppDialog *dialog);
const char *           gsm_app_dialog_get_comment        (GsmAppDialog *dialog);

G_END_DECLS

#endif /* __GSM_APP_DIALOG_H */
