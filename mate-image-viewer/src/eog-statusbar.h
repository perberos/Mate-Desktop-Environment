/* Eye of Mate - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *	   Jens Finke <jens@mate.org>
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

#ifndef __EOG_STATUSBAR_H__
#define __EOG_STATUSBAR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EogStatusbar        EogStatusbar;
typedef struct _EogStatusbarPrivate EogStatusbarPrivate;
typedef struct _EogStatusbarClass   EogStatusbarClass;

#define EOG_TYPE_STATUSBAR            (eog_statusbar_get_type ())
#define EOG_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_STATUSBAR, EogStatusbar))
#define EOG_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),   EOG_TYPE_STATUSBAR, EogStatusbarClass))
#define EOG_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_STATUSBAR))
#define EOG_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOG_TYPE_STATUSBAR))
#define EOG_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOG_TYPE_STATUSBAR, EogStatusbarClass))

struct _EogStatusbar
{
        GtkStatusbar parent;

        EogStatusbarPrivate *priv;
};

struct _EogStatusbarClass
{
        GtkStatusbarClass parent_class;
};

GType		 eog_statusbar_get_type			(void) G_GNUC_CONST;

GtkWidget	*eog_statusbar_new			(void);

void		 eog_statusbar_set_image_number		(EogStatusbar   *statusbar,
							 gint           num,
							 gint           tot);

void		 eog_statusbar_set_progress		(EogStatusbar   *statusbar,
							 gdouble        progress);

void		 eog_statusbar_set_has_resize_grip	(EogStatusbar   *statusbar,
							 gboolean        has_resize_grip);

G_END_DECLS

#endif /* __EOG_STATUSBAR_H__ */
