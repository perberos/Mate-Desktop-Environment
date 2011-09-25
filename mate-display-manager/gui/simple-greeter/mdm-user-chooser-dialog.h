/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#ifndef __MDM_USER_CHOOSER_DIALOG_H
#define __MDM_USER_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_USER_CHOOSER_DIALOG         (mdm_user_chooser_dialog_get_type ())
#define MDM_USER_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_USER_CHOOSER_DIALOG, MdmUserChooserDialog))
#define MDM_USER_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_USER_CHOOSER_DIALOG, MdmUserChooserDialogClass))
#define MDM_IS_USER_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_USER_CHOOSER_DIALOG))
#define MDM_IS_USER_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_USER_CHOOSER_DIALOG))
#define MDM_USER_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_USER_CHOOSER_DIALOG, MdmUserChooserDialogClass))

typedef struct MdmUserChooserDialogPrivate MdmUserChooserDialogPrivate;

typedef struct
{
        GtkDialog                    parent;
        MdmUserChooserDialogPrivate *priv;
} MdmUserChooserDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} MdmUserChooserDialogClass;

GType                  mdm_user_chooser_dialog_get_type                   (void);

GtkWidget            * mdm_user_chooser_dialog_new                        (void);

char *                 mdm_user_chooser_dialog_get_chosen_user_name       (MdmUserChooserDialog *dialog);
void                   mdm_user_chooser_dialog_set_show_other_user        (MdmUserChooserDialog *dialog,
                                                                           gboolean              show);
void                   mdm_user_chooser_dialog_set_show_user_guest        (MdmUserChooserDialog *dialog,
                                                                           gboolean              show);
void                   mdm_user_chooser_dialog_set_show_user_auto         (MdmUserChooserDialog *dialog,
                                                                           gboolean              show);
G_END_DECLS

#endif /* __MDM_USER_CHOOSER_DIALOG_H */
