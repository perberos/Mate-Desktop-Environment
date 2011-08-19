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

#include <gtk/gtk.h>
#include <libmate/mate-desktop-item.h>

#include "app-shell.h"
#include "app-resizer.h"

static void app_resizer_class_init (AppResizerClass *);
static void app_resizer_init (AppResizer *);
static void app_resizer_destroy (GtkObject *);

static void app_resizer_size_allocate (GtkWidget * resizer, GtkAllocation * allocation);
static gboolean app_resizer_paint_window (GtkWidget * widget, GdkEventExpose * event,
	AppShellData * app_data);

G_DEFINE_TYPE (AppResizer, app_resizer, GTK_TYPE_LAYOUT);


static void
app_resizer_class_init (AppResizerClass * klass)
{
	GtkWidgetClass *widget_class;

	((GtkObjectClass *) klass)->destroy = app_resizer_destroy;

	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->size_allocate = app_resizer_size_allocate;
}

static void
app_resizer_init (AppResizer * window)
{
}

void
remove_container_entries (GtkContainer * widget)
{
	GList *children, *l;

	children = gtk_container_get_children (widget);
	for (l = children; l; l = l->next)
	{
		GtkWidget *child = GTK_WIDGET (l->data);
		gtk_container_remove (GTK_CONTAINER (widget), GTK_WIDGET (child));
	}

	if (children)
		g_list_free (children);
}

static void
resize_table (GtkTable * table, gint columns, GList * launcher_list)
{
	float rows, remainder;

	remove_container_entries (GTK_CONTAINER (table));

	rows = ((float) g_list_length (launcher_list)) / (float) columns;
	remainder = rows - ((int) rows);
	if (remainder != 0.0)
		rows += 1;

	gtk_table_resize (table, (int) rows, columns);
}

static void
relayout_table (GtkTable * table, GList * element_list)
{
	gint maxcols = (GTK_TABLE (table))->ncols;
	gint row = 0, col = 0;
	do
	{
		GtkWidget *element = GTK_WIDGET (element_list->data);
		gtk_table_attach (table, element, col, col + 1, row, row + 1, GTK_EXPAND | GTK_FILL,
			GTK_EXPAND | GTK_FILL, 0, 0);
		col++;
		if (col == maxcols)
		{
			col = 0;
			row++;
		}
	}
	while (NULL != (element_list = g_list_next (element_list)));
}

void
app_resizer_layout_table_default (AppResizer * widget, GtkTable * table, GList * element_list)
{
	resize_table (table, widget->cur_num_cols, element_list);
	relayout_table (table, element_list);
}

static void
relayout_tables (AppResizer * widget, gint num_cols)
{
	GtkTable *table;
	GList *table_list, *launcher_list;

	for (table_list = widget->cached_tables_list; table_list != NULL;
		table_list = g_list_next (table_list))
	{
		table = GTK_TABLE (table_list->data);
		launcher_list = gtk_container_get_children (GTK_CONTAINER (table));
		launcher_list = g_list_reverse (launcher_list);	/* Fixme - ugly hack because table stores prepend */
		resize_table (table, num_cols, launcher_list);
		relayout_table (table, launcher_list);
		g_list_free (launcher_list);
	}
}

static gint
calculate_num_cols (AppResizer * resizer, gint avail_width)
{
	if (resizer->table_elements_homogeneous)
	{
		gint num_cols;

		if (resizer->cached_element_width == -1)
		{
			GtkTable *table = GTK_TABLE (resizer->cached_tables_list->data);
			GList *children = gtk_container_get_children (GTK_CONTAINER (table));
			GtkWidget *table_element = GTK_WIDGET (children->data);
			g_list_free (children);

			resizer->cached_element_width = table_element->allocation.width;
			resizer->cached_table_spacing = gtk_table_get_default_col_spacing (table);
		}

		num_cols =
			(avail_width +
			resizer->cached_table_spacing) / (resizer->cached_element_width +
			resizer->cached_table_spacing);
		return num_cols;
	}
	else
		g_assert_not_reached ();	/* Fixme - implement... */
}

static gint
relayout_tables_if_needed (AppResizer * widget, gint avail_width, gint current_num_cols)
{
	gint num_cols = calculate_num_cols (widget, avail_width);
	if (num_cols < 1)
	{
		num_cols = 1;	/* just horiz scroll if avail_width is less than one column */
	}

	if (current_num_cols != num_cols)
	{
		relayout_tables (widget, num_cols);
		current_num_cols = num_cols;
	}
	return current_num_cols;
}

void
app_resizer_set_table_cache (AppResizer * widget, GList * cache_list)
{
	widget->cached_tables_list = cache_list;
}

void
app_resizer_set_homogeneous (AppResizer * widget, gboolean homogeneous)
{
	widget->table_elements_homogeneous = homogeneous;
}

static void
app_resizer_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
	/* printf("ENTER - app_resizer_size_allocate\n"); */
	AppResizer *resizer = APP_RESIZER (widget);
	GtkWidget *child = GTK_WIDGET (APP_RESIZER (resizer)->child);

	static gboolean first_time = TRUE;
	gint new_num_cols;
	gint useable_area;

	if (first_time)
	{
		/* we are letting the first show be the "natural" size of the child widget so do nothing. */
		if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
			(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);

		first_time = FALSE;
		gtk_layout_set_size (GTK_LAYOUT (resizer), child->allocation.width,
			child->allocation.height);
		return;
	}

	if (!resizer->cached_tables_list)	/* if everthing is currently filtered out - just return */
	{
		GtkAllocation child_allocation;

		if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
			(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);

		/* We want the message to center itself and only scroll if it's bigger than the available real size. */
		child_allocation.x = 0;
		child_allocation.y = 0;
		child_allocation.width = MAX (allocation->width, child->requisition.width);
		child_allocation.height = MAX (allocation->height, child->requisition.height);

		gtk_widget_size_allocate (child, &child_allocation);
		gtk_layout_set_size (GTK_LAYOUT (resizer), child_allocation.width,
			child_allocation.height);
		return;
	}
	useable_area =
		allocation->width - (child->requisition.width -
		GTK_WIDGET (resizer->cached_tables_list->data)->requisition.width);
	new_num_cols =
		relayout_tables_if_needed (APP_RESIZER (resizer), useable_area,
		resizer->cur_num_cols);
	if (resizer->cur_num_cols != new_num_cols)
	{
		GtkRequisition req;

		/* Have to do this so that it requests, and thus gets allocated, new amount */
		gtk_widget_size_request (child, &req);

		resizer->cur_num_cols = new_num_cols;
	}

	if (GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate)
		(*GTK_WIDGET_CLASS (app_resizer_parent_class)->size_allocate) (widget, allocation);
	gtk_layout_set_size (GTK_LAYOUT (resizer), child->allocation.width,
		child->allocation.height);
}

GtkWidget *
app_resizer_new (GtkVBox * child, gint initial_num_columns, gboolean homogeneous,
	AppShellData * app_data)
{
	AppResizer *widget;

	g_assert (child != NULL);
	g_assert (GTK_IS_VBOX (child));

	widget = g_object_new (APP_RESIZER_TYPE, NULL);
	widget->cached_element_width = -1;
	widget->cur_num_cols = initial_num_columns;
	widget->table_elements_homogeneous = homogeneous;
	widget->setting_style = FALSE;
	widget->app_data = app_data;

	g_signal_connect (G_OBJECT (widget), "expose-event", G_CALLBACK (app_resizer_paint_window),
		app_data);

	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (child));
	widget->child = child;

	return GTK_WIDGET (widget);
}

static void
app_resizer_destroy (GtkObject * obj)
{
}

void
app_resizer_set_vadjustment_value (GtkWidget * widget, gdouble value)
{
	GtkAdjustment *adjust = gtk_layout_get_vadjustment (GTK_LAYOUT (widget));
	if (value > adjust->upper - adjust->page_size)
	{
		value = adjust->upper - adjust->page_size;
	}
	gtk_adjustment_set_value (adjust, value);
}

static gboolean
app_resizer_paint_window (GtkWidget * widget, GdkEventExpose * event, AppShellData * app_data)
{
	/*
	printf("ENTER - app_resizer_paint_window\n");
	printf("Area:      %d, %d, %d, %d\n", event->area.x, event->area.y, event->area.width, event->area.height);
	printf("Allocation:%d, %d, %d, %d\n\n", widget->allocation.x, widget->allocation.y, widget->allocation.width, widget->allocation.height);
	*/

	gdk_draw_rectangle (GTK_LAYOUT (widget)->bin_window,
		widget->style->base_gc[GTK_STATE_NORMAL], TRUE, event->area.x, event->area.y,
		event->area.width, event->area.height);

	if (app_data->selected_group)
	{
		GtkWidget *selected_widget = GTK_WIDGET (app_data->selected_group);
		gdk_draw_rectangle (selected_widget->window,	/* drawing on child window and child coordinates */
			selected_widget->style->light_gc[GTK_STATE_SELECTED], TRUE,
			selected_widget->allocation.x, selected_widget->allocation.y,
			widget->allocation.width,	/* drawing with our coordinates here to draw all the way to the edge. */
			selected_widget->allocation.height);
	}

	return FALSE;
}
