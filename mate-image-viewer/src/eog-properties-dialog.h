/* Eye Of Mate - Image Properties Dialog
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

#ifndef __EOG_PROPERTIES_DIALOG_H__
#define __EOG_PROPERTIES_DIALOG_H__

#include "eog-dialog.h"
#include "eog-image.h"
#include "eog-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EogPropertiesDialog EogPropertiesDialog;
typedef struct _EogPropertiesDialogClass EogPropertiesDialogClass;
typedef struct _EogPropertiesDialogPrivate EogPropertiesDialogPrivate;

#define EOG_TYPE_PROPERTIES_DIALOG            (eog_properties_dialog_get_type ())
#define EOG_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_PROPERTIES_DIALOG, EogPropertiesDialog))
#define EOG_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_PROPERTIES_DIALOG, EogPropertiesDialogClass))
#define EOG_IS_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_PROPERTIES_DIALOG))
#define EOG_IS_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_PROPERTIES_DIALOG))
#define EOG_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_PROPERTIES_DIALOG, EogPropertiesDialogClass))

typedef enum {
	EOG_PROPERTIES_DIALOG_PAGE_GENERAL = 0,
	EOG_PROPERTIES_DIALOG_PAGE_EXIF,
	EOG_PROPERTIES_DIALOG_PAGE_DETAILS,
	EOG_PROPERTIES_DIALOG_N_PAGES
} EogPropertiesDialogPage;

struct _EogPropertiesDialog {
	EogDialog dialog;

	EogPropertiesDialogPrivate *priv;
};

struct _EogPropertiesDialogClass {
	EogDialogClass parent_class;
};

GType	    eog_properties_dialog_get_type	(void) G_GNUC_CONST;

GObject    *eog_properties_dialog_new	  	(GtkWindow               *parent,
                                                 EogThumbView            *thumbview,
						 GtkAction               *next_image_action,
						 GtkAction               *previous_image_action);

void	    eog_properties_dialog_update  	(EogPropertiesDialog     *prop,
						 EogImage                *image);

void	    eog_properties_dialog_set_page  	(EogPropertiesDialog     *prop,
						 EogPropertiesDialogPage  page);

void	    eog_properties_dialog_set_netbook_mode (EogPropertiesDialog *dlg,
						    gboolean enable);
G_END_DECLS

#endif /* __EOG_PROPERTIES_DIALOG_H__ */
