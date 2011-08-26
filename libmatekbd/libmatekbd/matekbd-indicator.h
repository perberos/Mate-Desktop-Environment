/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
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

#ifndef __MATEKBD_INDICATOR_H__
#define __MATEKBD_INDICATOR_H__

#include <gtk/gtk.h>

#include <libxklavier/xklavier.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct _MatekbdIndicator MatekbdIndicator;
	typedef struct _MatekbdIndicatorPrivate MatekbdIndicatorPrivate;
	typedef struct _MatekbdIndicatorClass MatekbdIndicatorClass;

#define MATEKBD_TYPE_INDICATOR             (matekbd_indicator_get_type ())
#define MATEKBD_INDICATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATEKBD_TYPE_INDICATOR, MatekbdIndicator))
#define MATEKBD_INDCATOR_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), MATEKBD_TYPE_INDICATOR,  MatekbdIndicatorClass))
#define MATEKBD_IS_INDICATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATEKBD_TYPE_INDICATOR))
#define MATEKBD_IS_INDICATOR_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), MATEKBD_TYPE_INDICATOR))
#define MATEKBD_INDICATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATEKBD_TYPE_INDICATOR, MatekbdIndicatorClass))

	struct _MatekbdIndicator {
		GtkNotebook parent;
		MatekbdIndicatorPrivate *priv;
	};

	struct _MatekbdIndicatorClass {
		GtkNotebookClass parent_class;

		void (*reinit_ui) (MatekbdIndicator * gki);
	};

	extern GType matekbd_indicator_get_type (void);

	extern GtkWidget *matekbd_indicator_new (void);

	extern void matekbd_indicator_reinit_ui (MatekbdIndicator * gki);

	extern void matekbd_indicator_set_angle (MatekbdIndicator * gki,
					      gdouble angle);

	extern XklEngine *matekbd_indicator_get_xkl_engine (void);

	extern gchar **matekbd_indicator_get_group_names (void);

	extern gchar *matekbd_indicator_get_image_filename (guint group);

	extern gdouble matekbd_indicator_get_max_width_height_ratio (void);

	extern void
	 matekbd_indicator_set_parent_tooltips (MatekbdIndicator *
					     gki, gboolean ifset);

	extern void
	 matekbd_indicator_set_tooltips_format (const gchar str[]);

#ifdef __cplusplus
}
#endif
#endif
