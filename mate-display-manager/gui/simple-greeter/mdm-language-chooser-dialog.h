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

#ifndef __MDM_LANGUAGE_CHOOSER_DIALOG_H
#define __MDM_LANGUAGE_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_LANGUAGE_CHOOSER_DIALOG         (mdm_language_chooser_dialog_get_type ())
#define MDM_LANGUAGE_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG, MdmLanguageChooserDialog))
#define MDM_LANGUAGE_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG, MdmLanguageChooserDialogClass))
#define MDM_IS_LANGUAGE_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG))
#define MDM_IS_LANGUAGE_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG))
#define MDM_LANGUAGE_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG, MdmLanguageChooserDialogClass))

typedef struct MdmLanguageChooserDialogPrivate MdmLanguageChooserDialogPrivate;

typedef struct
{
        GtkDialog                        parent;
        MdmLanguageChooserDialogPrivate *priv;
} MdmLanguageChooserDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} MdmLanguageChooserDialogClass;

GType                  mdm_language_chooser_dialog_get_type                       (void);

GtkWidget            * mdm_language_chooser_dialog_new                            (void);

char *                 mdm_language_chooser_dialog_get_current_language_name      (MdmLanguageChooserDialog *dialog);
void                   mdm_language_chooser_dialog_set_current_language_name      (MdmLanguageChooserDialog *dialog,
     const char               *language_name);

G_END_DECLS

#endif /* __MDM_LANGUAGE_CHOOSER_DIALOG_H */
