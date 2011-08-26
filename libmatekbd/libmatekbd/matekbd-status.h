/*
 * Copyright (C) 2009 Sergey V. Udaltsov <svu@mate.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MATEKBD_STATUS_H__
#define __MATEKBD_STATUS_H__

#include <gtk/gtk.h>

#include <libxklavier/xklavier.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _MatekbdStatus MatekbdStatus;
	typedef struct _MatekbdStatusPrivate MatekbdStatusPrivate;
	typedef struct _MatekbdStatusClass MatekbdStatusClass;

#define MATEKBD_TYPE_STATUS             (matekbd_status_get_type ())
#define MATEKBD_STATUS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATEKBD_TYPE_STATUS, MatekbdStatus))
#define MATEKBD_INDCATOR_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), MATEKBD_TYPE_STATUS,  MatekbdStatusClass))
#define MATEKBD_IS_STATUS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATEKBD_TYPE_STATUS))
#define MATEKBD_IS_STATUS_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), MATEKBD_TYPE_STATUS))
#define MATEKBD_STATUS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATEKBD_TYPE_STATUS, MatekbdStatusClass))

	struct _MatekbdStatus {
		GStatusIcon parent;
		MatekbdStatusPrivate *priv;
	};

	struct _MatekbdStatusClass {
		GtkNotebookClass parent_class;
	};

	extern GType matekbd_status_get_type (void);

	extern GtkStatusIcon *matekbd_status_new (void);

	extern void matekbd_status_reinit_ui (MatekbdStatus * gki);

	extern void matekbd_status_set_angle (MatekbdStatus * gki,
					   gdouble angle);

	extern XklEngine *matekbd_status_get_xkl_engine (void);

	extern gchar **matekbd_status_get_group_names (void);

	extern gchar *matekbd_status_get_image_filename (guint group);

	extern void
	 matekbd_status_set_tooltips_format (const gchar str[]);

#ifdef __cplusplus
}
#endif
#endif
