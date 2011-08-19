/* Eye Of Mate - Thumbnail Navigator
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

#ifndef __EOG_THUMB_NAV_H__
#define __EOG_THUMB_NAV_H__

#include "eog-thumb-view.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _EogThumbNav EogThumbNav;
typedef struct _EogThumbNavClass EogThumbNavClass;
typedef struct _EogThumbNavPrivate EogThumbNavPrivate;

#define EOG_TYPE_THUMB_NAV            (eog_thumb_nav_get_type ())
#define EOG_THUMB_NAV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_THUMB_NAV, EogThumbNav))
#define EOG_THUMB_NAV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_THUMB_NAV, EogThumbNavClass))
#define EOG_IS_THUMB_NAV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_THUMB_NAV))
#define EOG_IS_THUMB_NAV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_THUMB_NAV))
#define EOG_THUMB_NAV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_THUMB_NAV, EogThumbNavClass))

typedef enum {
	EOG_THUMB_NAV_MODE_ONE_ROW,
	EOG_THUMB_NAV_MODE_ONE_COLUMN,
	EOG_THUMB_NAV_MODE_MULTIPLE_ROWS,
	EOG_THUMB_NAV_MODE_MULTIPLE_COLUMNS
} EogThumbNavMode;

struct _EogThumbNav {
	GtkHBox base_instance;

	EogThumbNavPrivate *priv;
};

struct _EogThumbNavClass {
	GtkHBoxClass parent_class;
};

GType	         eog_thumb_nav_get_type          (void) G_GNUC_CONST;

GtkWidget       *eog_thumb_nav_new               (GtkWidget         *thumbview,
						  EogThumbNavMode    mode,
	             			          gboolean           show_buttons);

gboolean         eog_thumb_nav_get_show_buttons  (EogThumbNav       *nav);

void             eog_thumb_nav_set_show_buttons  (EogThumbNav       *nav,
                                                  gboolean           show_buttons);

EogThumbNavMode  eog_thumb_nav_get_mode          (EogThumbNav       *nav);

void             eog_thumb_nav_set_mode          (EogThumbNav       *nav,
                                                  EogThumbNavMode    mode);

G_END_DECLS

#endif /* __EOG_THUMB_NAV_H__ */
