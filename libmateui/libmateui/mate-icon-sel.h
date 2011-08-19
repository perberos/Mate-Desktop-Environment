/* mate-icon-sel.h:
 * Copyright (C) 1998 Free Software Foundation
 * All rights reserved.
 *
 * For selecting an icon.
 * Written by: Havoc Pennington, based on John Ellis's code.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef MATE_ICON_SEL_H
#define MATE_ICON_SEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _MateIconSelection        MateIconSelection;
typedef struct _MateIconSelectionPrivate MateIconSelectionPrivate;
typedef struct _MateIconSelectionClass   MateIconSelectionClass;

#define MATE_TYPE_ICON_SELECTION            (mate_icon_selection_get_type ())
#define MATE_ICON_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ICON_SELECTION, MateIconSelection))
#define MATE_ICON_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ICON_SELECTION, MateIconSelectionClass))
#define MATE_IS_ICON_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ICON_SELECTION))
#define MATE_IS_ICON_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ICON_SELECTION))
#define MATE_ICON_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ICON_SELECTION, MateIconSelectionClass))

struct _MateIconSelection {
  GtkVBox vbox;

  /*< private >*/
  MateIconSelectionPrivate *_priv;
};

struct _MateIconSelectionClass {
  GtkVBoxClass parent_class;

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

GType mate_icon_selection_get_type     (void) G_GNUC_CONST;

GtkWidget * mate_icon_selection_new      (void);

/* Add default Mate icon directories */
void  mate_icon_selection_add_defaults   (MateIconSelection * gis);

/* Add icons from this directory */
void  mate_icon_selection_add_directory  (MateIconSelection * gis,
					   const gchar * dir);

/* Loads and displays the icons that were added using mate_icon_selection_add_* */
void  mate_icon_selection_show_icons     (MateIconSelection * gis);

/* Clear all icons (even the non shown ones if not_shown is set)*/
void  mate_icon_selection_clear          (MateIconSelection * gis,
					   gboolean not_shown);

/* if (full_path) return the whole filename, otherwise just the 
   last component */
gchar * 
mate_icon_selection_get_icon             (MateIconSelection * gis,
					   gboolean full_path);

/* Filename is only the last part, not the full path */
void  mate_icon_selection_select_icon    (MateIconSelection * gis,
					   const gchar * filename);

/* Stop the loading of images when we are in the loop in show_icons */
void  mate_icon_selection_stop_loading   (MateIconSelection * gis);

/* accessors for the internal widgets, icon_list is the actual 
   icon list, and box is the vertical box*/
GtkWidget *mate_icon_selection_get_gil   (MateIconSelection * gis);
GtkWidget *mate_icon_selection_get_box   (MateIconSelection * gis);

G_END_DECLS
   
#endif /* MATE_ICON_SEL_H */
