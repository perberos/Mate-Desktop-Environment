/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __THEMED_ICON_H__
#define __THEMED_ICON_H__

#include <glib.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THEMED_ICON_TYPE            (themed_icon_get_type ())
#define THEMED_ICON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THEMED_ICON_TYPE, ThemedIcon))
#define THEMED_ICON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THEMED_ICON_TYPE, ThemedIconClass))
#define IS_THEMED_ICON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THEMED_ICON_TYPE))
#define IS_THEMED_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THEMED_ICON_TYPE))
#define THEMED_ICON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THEMED_ICON_TYPE, ThemedIconClass))

typedef struct
{
	GtkImage parent;

	GtkIconSize size;
	gchar *id;
} ThemedIcon;

typedef struct
{
	GtkImageClass parent_class;
} ThemedIconClass;

GType themed_icon_get_type (void);
GtkWidget *themed_icon_new (const gchar * id, GtkIconSize size);

#ifdef __cplusplus
}
#endif
#endif /* __THEMED_ICON_H__ */
