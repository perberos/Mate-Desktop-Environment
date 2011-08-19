/* Eye Of Mate - EOG Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
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
 */

#ifndef __EOG_DIALOG_H__
#define __EOG_DIALOG_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EogDialog EogDialog;
typedef struct _EogDialogClass EogDialogClass;
typedef struct _EogDialogPrivate EogDialogPrivate;

#define EOG_TYPE_DIALOG            (eog_dialog_get_type ())
#define EOG_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_DIALOG, EogDialog))
#define EOG_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_DIALOG, EogDialogClass))
#define EOG_IS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_DIALOG))
#define EOG_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_DIALOG))
#define EOG_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_DIALOG, EogDialogClass))

struct _EogDialog {
	GObject object;

	EogDialogPrivate *priv;
};

struct _EogDialogClass {
	GObjectClass parent_class;

	void    (* construct)   (EogDialog   *dialog,
				 const gchar *glade_file,
				 const gchar *dlg_node);

	void    (* show)        (EogDialog   *dialog);

	void    (* hide)        (EogDialog   *dialog);
};

GType   eog_dialog_get_type      (void) G_GNUC_CONST;

void    eog_dialog_construct     (EogDialog   *dialog,
				  const gchar *glade_file,
				  const gchar *dlg_node);

void    eog_dialog_show	         (EogDialog *dialog);

void    eog_dialog_hide	         (EogDialog *dialog);

void    eog_dialog_get_controls  (EogDialog   *dialog,
				  const gchar *property_id,
				  ...);

G_END_DECLS

#endif /* __EOG_DIALOG_H__ */
