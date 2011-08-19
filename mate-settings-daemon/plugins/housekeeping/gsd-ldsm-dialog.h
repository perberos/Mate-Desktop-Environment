/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * gsd-ldsm-dialog.c
 * Copyright (C) Chris Coulson 2009 <chrisccoulson@googlemail.com>
 * 
 * gsd-ldsm-dialog.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gsd-ldsm-dialog.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GSD_LDSM_DIALOG_H_
#define _GSD_LDSM_DIALOG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSD_TYPE_LDSM_DIALOG             (gsd_ldsm_dialog_get_type ())
#define GSD_LDSM_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSD_TYPE_LDSM_DIALOG, GsdLdsmDialog))
#define GSD_LDSM_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSD_TYPE_LDSM_DIALOG, GsdLdsmDialogClass))
#define GSD_IS_LDSM_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSD_TYPE_LDSM_DIALOG))
#define GSD_IS_LDSM_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSD_TYPE_LDSM_DIALOG))
#define GSD_LDSM_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GSD_TYPE_LDSM_DIALOG, GsdLdsmDialogClass))

enum
{
        GSD_LDSM_DIALOG_RESPONSE_EMPTY_TRASH = -20,
        GSD_LDSM_DIALOG_RESPONSE_ANALYZE = -21
};

typedef struct GsdLdsmDialogPrivate GsdLdsmDialogPrivate;
typedef struct _GsdLdsmDialogClass GsdLdsmDialogClass;
typedef struct _GsdLdsmDialog GsdLdsmDialog;

struct _GsdLdsmDialogClass
{
        GtkDialogClass parent_class;
};

struct _GsdLdsmDialog
{
        GtkDialog parent_instance;
        GsdLdsmDialogPrivate *priv;
};

GType gsd_ldsm_dialog_get_type (void) G_GNUC_CONST;

GsdLdsmDialog * gsd_ldsm_dialog_new (gboolean other_usable_partitions,
                                     gboolean other_partitions,
                                     gboolean display_baobab,
                                     gboolean display_empty_trash,
                                     gint64 space_remaining,
                                     const gchar *partition_name,
                                     const gchar *mount_path);

G_END_DECLS

#endif /* _GSD_LDSM_DIALOG_H_ */
