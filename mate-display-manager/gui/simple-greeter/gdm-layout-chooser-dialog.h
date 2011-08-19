/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Matthias Clasen <mclasen@redhat.com>
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

#ifndef __GDM_LAYOUT_CHOOSER_DIALOG_H
#define __GDM_LAYOUT_CHOOSER_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_LAYOUT_CHOOSER_DIALOG         (gdm_layout_chooser_dialog_get_type ())
#define GDM_LAYOUT_CHOOSER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_LAYOUT_CHOOSER_DIALOG, GdmLayoutChooserDialog))
#define GDM_LAYOUT_CHOOSER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_LAYOUT_CHOOSER_DIALOG, GdmLayoutChooserDialogClass))
#define GDM_IS_LAYOUT_CHOOSER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_LAYOUT_CHOOSER_DIALOG))
#define GDM_IS_LAYOUT_CHOOSER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_LAYOUT_CHOOSER_DIALOG))
#define GDM_LAYOUT_CHOOSER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_LAYOUT_CHOOSER_DIALOG, GdmLayoutChooserDialogClass))

typedef struct GdmLayoutChooserDialogPrivate GdmLayoutChooserDialogPrivate;

typedef struct
{
        GtkDialog                        parent;
        GdmLayoutChooserDialogPrivate *priv;
} GdmLayoutChooserDialog;

typedef struct
{
        GtkDialogClass   parent_class;
} GdmLayoutChooserDialogClass;

GType                  gdm_layout_chooser_dialog_get_type                       (void);

GtkWidget            * gdm_layout_chooser_dialog_new                            (void);

char *                 gdm_layout_chooser_dialog_get_current_layout_name      (GdmLayoutChooserDialog *dialog);
void                   gdm_layout_chooser_dialog_set_current_layout_name      (GdmLayoutChooserDialog *dialog,
     const char               *layout_name);

G_END_DECLS

#endif /* __GDM_LAYOUT_CHOOSER_DIALOG_H */
