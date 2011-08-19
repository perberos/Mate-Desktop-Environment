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

#ifndef __GDM_HOST_CHOOSER_DIALOG_H
#define __GDM_HOST_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gdm-chooser-host.h"

G_BEGIN_DECLS

#define GDM_TYPE_HOST_CHOOSER_DIALOG         (gdm_host_chooser_dialog_get_type ())
#define GDM_HOST_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_HOST_CHOOSER_DIALOG, GdmHostChooserDialog))
#define GDM_HOST_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_HOST_CHOOSER_DIALOG, GdmHostChooserDialogClass))
#define GDM_IS_HOST_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_HOST_CHOOSER_DIALOG))
#define GDM_IS_HOST_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_HOST_CHOOSER_DIALOG))
#define GDM_HOST_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_HOST_CHOOSER_DIALOG, GdmHostChooserDialogClass))

typedef struct GdmHostChooserDialogPrivate GdmHostChooserDialogPrivate;

typedef struct
{
        GtkDialog                    parent;
        GdmHostChooserDialogPrivate *priv;
} GdmHostChooserDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} GdmHostChooserDialogClass;

GType                  gdm_host_chooser_dialog_get_type           (void);

GtkWidget            * gdm_host_chooser_dialog_new                (int                   kind_mask);
void                   gdm_host_chooser_dialog_set_kind_mask      (GdmHostChooserDialog *dialog,
                                                                   int                   kind_mask);

GdmChooserHost *       gdm_host_chooser_dialog_get_host           (GdmHostChooserDialog *dialog);

G_END_DECLS

#endif /* __GDM_HOST_CHOOSER_DIALOG_H */
