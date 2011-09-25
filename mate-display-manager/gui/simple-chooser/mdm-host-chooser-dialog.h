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

#ifndef __MDM_HOST_CHOOSER_DIALOG_H
#define __MDM_HOST_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "mdm-chooser-host.h"

G_BEGIN_DECLS

#define MDM_TYPE_HOST_CHOOSER_DIALOG         (mdm_host_chooser_dialog_get_type ())
#define MDM_HOST_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_HOST_CHOOSER_DIALOG, MdmHostChooserDialog))
#define MDM_HOST_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_HOST_CHOOSER_DIALOG, MdmHostChooserDialogClass))
#define MDM_IS_HOST_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_HOST_CHOOSER_DIALOG))
#define MDM_IS_HOST_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_HOST_CHOOSER_DIALOG))
#define MDM_HOST_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_HOST_CHOOSER_DIALOG, MdmHostChooserDialogClass))

typedef struct MdmHostChooserDialogPrivate MdmHostChooserDialogPrivate;

typedef struct
{
        GtkDialog                    parent;
        MdmHostChooserDialogPrivate *priv;
} MdmHostChooserDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} MdmHostChooserDialogClass;

GType                  mdm_host_chooser_dialog_get_type           (void);

GtkWidget            * mdm_host_chooser_dialog_new                (int                   kind_mask);
void                   mdm_host_chooser_dialog_set_kind_mask      (MdmHostChooserDialog *dialog,
                                                                   int                   kind_mask);

MdmChooserHost *       mdm_host_chooser_dialog_get_host           (MdmHostChooserDialog *dialog);

G_END_DECLS

#endif /* __MDM_HOST_CHOOSER_DIALOG_H */
