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

#ifndef __APP_RESIZER_H__
#define __APP_RESIZER_H__

#include <glib.h>
#include <gtk/gtk.h>

#include <libslab/app-shell.h>

G_BEGIN_DECLS

#define INITIAL_NUM_COLS 3
#define APP_RESIZER_TYPE            (app_resizer_get_type ())
#define APP_RESIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), APP_RESIZER_TYPE, AppResizer))
#define APP_RESIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), APP_RESIZER_TYPE, AppResizerClass))
#define IS_APP_RESIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APP_RESIZER_TYPE))
#define IS_APP_RESIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), APP_RESIZER_TYPE))
#define APP_RESIZER_GET_CLASS(obj)  (G_TYPE_CHECK_GET_CLASS ((obj), APP_RESIZER_TYPE, AppResizerClass))

typedef struct _AppResizer AppResizer;
typedef struct _AppResizerClass AppResizerClass;

struct _AppResizer
{
	GtkLayout parent;

	GtkVBox *child;
	GList *cached_tables_list;
	gint cached_element_width;
	gint cached_table_spacing;
	gboolean table_elements_homogeneous;
	gint cur_num_cols;
	gboolean setting_style;
	AppShellData *app_data;
};

struct _AppResizerClass
{
	GtkLayoutClass parent_class;
};

void app_resizer_set_homogeneous (AppResizer * widget, gboolean value);
void remove_container_entries (GtkContainer * widget);

GType app_resizer_get_type (void);
GtkWidget *app_resizer_new (GtkVBox * child, gint initial_num_columns, gboolean homogeneous,
	AppShellData * app_data);
void app_resizer_set_table_cache (AppResizer * widget, GList * cache_list);
void app_resizer_layout_table_default (AppResizer * widget, GtkTable * table, GList * element_list);
void app_resizer_set_vadjustment_value (GtkWidget * widget, gdouble value);

G_END_DECLS
#endif /* __APP_RESIZER_H__ */
