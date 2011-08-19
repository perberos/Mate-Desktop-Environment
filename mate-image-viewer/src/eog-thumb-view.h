/* Eye Of Mate - Thumbnail View
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#ifndef EOG_THUMB_VIEW_H
#define EOG_THUMB_VIEW_H

#include "eog-image.h"
#include "eog-list-store.h"

G_BEGIN_DECLS

#define EOG_TYPE_THUMB_VIEW            (eog_thumb_view_get_type ())
#define EOG_THUMB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_THUMB_VIEW, EogThumbView))
#define EOG_THUMB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOG_TYPE_THUMB_VIEW, EogThumbViewClass))
#define EOG_IS_THUMB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_THUMB_VIEW))
#define EOG_IS_THUMB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOG_TYPE_THUMB_VIEW))
#define EOG_THUMB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOG_TYPE_THUMB_VIEW, EogThumbViewClass))

typedef struct _EogThumbView EogThumbView;
typedef struct _EogThumbViewClass EogThumbViewClass;
typedef struct _EogThumbViewPrivate EogThumbViewPrivate;

typedef enum {
	EOG_THUMB_VIEW_SELECT_CURRENT = 0,
	EOG_THUMB_VIEW_SELECT_LEFT,
	EOG_THUMB_VIEW_SELECT_RIGHT,
	EOG_THUMB_VIEW_SELECT_FIRST,
	EOG_THUMB_VIEW_SELECT_LAST,
	EOG_THUMB_VIEW_SELECT_RANDOM
} EogThumbViewSelectionChange;

struct _EogThumbView {
	GtkIconView icon_view;
	EogThumbViewPrivate *priv;
};

struct _EogThumbViewClass {
	 GtkIconViewClass icon_view_class;
};

GType       eog_thumb_view_get_type 		    (void) G_GNUC_CONST;

GtkWidget  *eog_thumb_view_new 			    (void);

void	    eog_thumb_view_set_model 		    (EogThumbView *thumbview,
						     EogListStore *store);

void        eog_thumb_view_set_item_height          (EogThumbView *thumbview,
						     gint          height);

guint	    eog_thumb_view_get_n_selected 	    (EogThumbView *thumbview);

EogImage   *eog_thumb_view_get_first_selected_image (EogThumbView *thumbview);

GList      *eog_thumb_view_get_selected_images 	    (EogThumbView *thumbview);

void        eog_thumb_view_select_single 	    (EogThumbView *thumbview,
						     EogThumbViewSelectionChange change);

void        eog_thumb_view_set_current_image	    (EogThumbView *thumbview,
						     EogImage     *image,
						     gboolean     deselect_other);

void        eog_thumb_view_set_thumbnail_popup      (EogThumbView *thumbview,
						     GtkMenu      *menu);

G_END_DECLS

#endif /* EOG_THUMB_VIEW_H */
