/* gdict-source-dialog.h - source dialog
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __GDICT_SOURCE_DIALOG_H__
#define __GDICT_SOURCE_DIALOG_H__

#include <gtk/gtk.h>
#include <libgdict/gdict.h>

G_BEGIN_DECLS

#define GDICT_TYPE_SOURCE_DIALOG 	(gdict_source_dialog_get_type ())
#define GDICT_SOURCE_DIALOG(obj) 	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SOURCE_DIALOG, GdictSourceDialog))
#define GDICT_IS_SOURCE_DIALOG(obj) 	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SOURCE_DIALOG))

typedef enum
{
  GDICT_SOURCE_DIALOG_VIEW,
  GDICT_SOURCE_DIALOG_CREATE,
  GDICT_SOURCE_DIALOG_EDIT
} GdictSourceDialogAction;

typedef struct _GdictSourceDialog      GdictSourceDialog;
typedef struct _GdictSourceDialogClass GdictSourceDialogClass;

GType      gdict_source_dialog_get_type (void) G_GNUC_CONST;
GtkWidget *gdict_source_dialog_new      (GtkWindow               *parent,
					 const gchar             *title,
					 GdictSourceDialogAction  action,
					 GdictSourceLoader       *loader,
					 const gchar             *source_name);

G_END_DECLS

#endif /* __GDICT_SOURCE_DIALOG_H__ */
