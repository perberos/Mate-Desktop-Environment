/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Timezone map - fake widget implementation with hooks for time-admin.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@ximian.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gst.h"
#include "time-tool.h"

#include "tz.h"
#include "e-map/e-map.h"
#include "tz-map.h"


extern GstTimeTool *tool;


/* --- Forward declarations of internal functions --- */


static TzLocation *e_tz_map_location_from_point (ETzMap *tzmap, EMapPoint *point);
static gboolean flash_selected_point (gpointer data);
static gboolean motion (GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gboolean button_pressed (GtkWidget *w, GdkEventButton *event, gpointer data);
static gboolean update_map (GtkWidget *w, gpointer data);
static gboolean in_map (GtkWidget *w, GdkEventCrossing *event, gpointer data);
static gboolean out_map (GtkWidget *w,GdkEventCrossing *event, gpointer data);
static gboolean map (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean unmap (GtkWidget *widget, GdkEvent *event, gpointer data);

ETzMap *
e_tz_map_new (GstTool *tool)
{
	ETzMap *tzmap;
	GPtrArray *locs;
	TzLocation *tzl;
	GtkWidget *location_combo;
	guint i;

	tzmap = g_new0 (ETzMap, 1);
	tzmap->tool = GST_TOOL (tool);
	tzmap->tzdb = tz_load_db ();
	if (!tzmap->tzdb)
		g_error ("Unable to load system timezone database.");
	
	tzmap->map = e_map_new ();
	if (!tzmap->map)
		g_error ("Unable to create map widget.");

	gtk_widget_set_events (GTK_WIDGET (tzmap->map), gtk_widget_get_events (GTK_WIDGET (tzmap->map)) | 
	      GDK_LEAVE_NOTIFY_MASK | GDK_VISIBILITY_NOTIFY_MASK);
	
	locs = tz_get_locations (tzmap->tzdb);

	for (i = 0; i < locs->len; i++)
	{
		tzl = g_ptr_array_index (locs, i);
		e_map_add_point (tzmap->map, NULL, tzl->longitude, tzl->latitude,
				 TZ_MAP_POINT_NORMAL_RGBA);
	}

	g_signal_connect (G_OBJECT (tzmap->map), "map-event",
			  G_CALLBACK (map), tzmap);
	g_signal_connect (G_OBJECT (tzmap->map), "unmap-event",
			  G_CALLBACK (unmap), tzmap);
        g_signal_connect(G_OBJECT (tzmap->map), "motion-notify-event",
			 G_CALLBACK (motion), (gpointer) tzmap);
	g_signal_connect(G_OBJECT(tzmap->map), "button-press-event",
			 G_CALLBACK (button_pressed), (gpointer) tzmap);
        g_signal_connect(G_OBJECT (tzmap->map), "enter-notify-event",
	                 G_CALLBACK (in_map), (gpointer) tzmap);
        g_signal_connect(G_OBJECT (tzmap->map), "leave-notify-event",
	                 G_CALLBACK (out_map), (gpointer) tzmap);
	
	location_combo = gst_dialog_get_widget (tzmap->tool->main_dialog, "location_combo");
	g_signal_connect (G_OBJECT (location_combo), "changed",
	                 G_CALLBACK (update_map), (gpointer) tzmap);
	
	return tzmap;
}

static gboolean
map (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ETzMap *tzmap = (ETzMap *) data;

	if (!tzmap->timeout_id)
		tzmap->timeout_id = g_timeout_add (100, flash_selected_point, (gpointer) tzmap);

	return FALSE;
}

static gboolean
unmap (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	ETzMap *tzmap = (ETzMap *) data;

	if (tzmap->timeout_id) {
		g_source_remove (tzmap->timeout_id);
		tzmap->timeout_id = 0;
	}

	return FALSE;
}

TzDB *
e_tz_map_get_tz_db (ETzMap *tzmap)
{
	g_return_val_if_fail (tzmap != NULL, NULL);
	return tzmap->tzdb;
}

TzLocation *
e_tz_map_get_location_by_name (ETzMap *tzmap, const gchar *name)
{
	TzLocation *tz_loc = NULL;
	TzDB *tz_db;
	GPtrArray *locs;
	guint i;

	tz_db = e_tz_map_get_tz_db (tzmap);
	locs = tz_get_locations (tz_db);

	for (i = 0; i < locs->len; i++)
	{
		TzLocation *tz_loc_temp;

		tz_loc_temp = g_ptr_array_index (locs, i);

		if (!strcmp (tz_location_get_zone (tz_loc_temp), name))
		{
			tz_loc = tz_loc_temp;
			break;
		}
	}

	return tz_loc;
}

static void
e_tz_map_set_location_text (ETzMap *tzmap, const gchar *name)
{
	GtkWidget    *location_combo;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;
	gchar        *location;

	location_combo = gst_dialog_get_widget (tzmap->tool->main_dialog, "location_combo");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (location_combo));
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &location, -1);

		if (strcmp (location, name) == 0) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (location_combo), &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
		}

		g_free (location);
	}
}

void
e_tz_map_set_tz_from_name (ETzMap *tzmap, const gchar *name)
{
	TzLocation *tz_loc = NULL;
	TzDB *tz_db;
	GPtrArray *locs;
	double l_longitude = 0.0, l_latitude = 0.0;
	guint i;

	g_return_if_fail (tzmap != NULL);

	tz_db = e_tz_map_get_tz_db (tzmap);
	locs = tz_get_locations (tz_db);

	if (name) {
		for (i = 0; i < locs->len; i++)	{
			tz_loc = g_ptr_array_index (locs, i);

			if (!strcmp(tz_location_get_zone(tz_loc), name)) {
				tz_location_get_position (tz_loc, &l_longitude, &l_latitude);
				break;
			}
		}
	}

	if (tzmap->point_selected)
	        e_map_point_set_color_rgba (tzmap->map,
					    tzmap->point_selected,
					    TZ_MAP_POINT_NORMAL_RGBA);
	tzmap->point_selected =
		e_map_get_closest_point (tzmap->map, l_longitude, l_latitude, FALSE);
	
	e_tz_map_set_location_text (tzmap, tz_location_get_zone (e_tz_map_location_from_point (tzmap, tzmap->point_selected)));
}

static gboolean
update_map (GtkWidget *w, gpointer data)
{
	ETzMap       *tzmap;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *location;
   
	tzmap = (ETzMap *) data;

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (w), &iter)) {
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (w));
		gtk_tree_model_get (model, &iter, 0, &location, -1);
		e_tz_map_set_tz_from_name (tzmap, location);
	}

	return TRUE;
}
                                 
gchar *
e_tz_map_get_selected_tz_name (ETzMap *tzmap)
{
	GtkWidget    *location_combo;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *location = NULL;

	location_combo = gst_dialog_get_widget (tzmap->tool->main_dialog, "location_combo");

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (location_combo), &iter)) {
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (location_combo));
		gtk_tree_model_get (model, &iter, 0, &location, -1);
	}

	return location;
}


static TzLocation *
e_tz_map_location_from_point (ETzMap *tzmap, EMapPoint *point)
{
	TzLocation *tz_loc = NULL;
	TzDB *tz_db;
	GPtrArray *locs;
	double p_longitude, p_latitude;
	double l_longitude, l_latitude;
	guint i;

	tz_db = e_tz_map_get_tz_db (tzmap);
	locs = tz_get_locations (tz_db);
	e_map_point_get_location (point, &p_longitude, &p_latitude);

	for (i = 0; i < locs->len; i++)
	{
		tz_location_get_position (g_ptr_array_index (locs, i),
					  &l_longitude, &l_latitude);
		
		if (l_longitude - 0.005 <= p_longitude &&
		    l_longitude + 0.005 >= p_longitude &&
		    l_latitude - 0.005 <= p_latitude &&
		    l_latitude + 0.005 >= p_latitude)
		{
			tz_loc = g_ptr_array_index (locs, i);
			break;
		}
	}

	return (tz_loc);
}


static gboolean
flash_selected_point (gpointer data)
{
	ETzMap *tzmap;

	tzmap = (ETzMap *) data;
	if (!tzmap->point_selected) return TRUE;

        if (e_map_point_get_color_rgba (tzmap->point_selected) ==
	    TZ_MAP_POINT_SELECTED_1_RGBA)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_selected,
					    TZ_MAP_POINT_SELECTED_2_RGBA);
	else
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_selected,
					    TZ_MAP_POINT_SELECTED_1_RGBA);

	return TRUE;
}

static void
update_hover_point (ETzMap  *tzmap,
		    gdouble  x_coord,
		    gdouble  y_coord)
{
	double longitude, latitude;

	e_map_window_to_world (tzmap->map, x_coord, y_coord,
			       &longitude, &latitude);

	if (tzmap->point_hover && tzmap->point_hover != tzmap->point_selected)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_hover,
					    TZ_MAP_POINT_NORMAL_RGBA);

	tzmap->point_hover =
	  e_map_get_closest_point (tzmap->map, longitude, latitude, TRUE);

	if (tzmap->point_hover != tzmap->point_selected)
	        e_map_point_set_color_rgba (tzmap->map, tzmap->point_hover,
					    TZ_MAP_POINT_HOVER_RGBA);

	/* e_tz_map_location_from_point() can in theory return NULL, but in
	 * practice there are no reasons why it should */

	gtk_label_set_text (GTK_LABEL (((GstTimeTool *) tzmap->tool)->map_hover_label),
			    tz_location_get_zone (e_tz_map_location_from_point (tzmap, tzmap->point_hover)));
}

static gboolean
motion (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	ETzMap *tzmap;
	gdouble x_coord, y_coord;

	tzmap = (ETzMap *) data;

	gdk_event_get_coords ((GdkEvent *) event, &x_coord, &y_coord);
	update_hover_point (tzmap, x_coord, y_coord);
	return TRUE;
}

static gboolean
in_map (GtkWidget *w, GdkEventCrossing *event, gpointer data)
{
	ETzMap *tzmap;
	gdouble x_coord, y_coord;

	tzmap = (ETzMap *) data;

	gdk_event_get_coords ((GdkEvent *) event, &x_coord, &y_coord);
	update_hover_point (tzmap, x_coord, y_coord);
	return TRUE;
}

static gboolean
out_map (GtkWidget *w, GdkEventCrossing *event, gpointer data)
{
	ETzMap *tzmap;
	const char *old_zone;
   
	tzmap = (ETzMap *) data;

	if (event->mode != GDK_CROSSING_NORMAL)
		return FALSE;
   
	if (tzmap->point_hover && tzmap->point_hover != tzmap->point_selected)
		e_map_point_set_color_rgba (tzmap->map, tzmap->point_hover, TZ_MAP_POINT_NORMAL_RGBA);

	tzmap->point_hover = NULL;
   
	old_zone = gtk_label_get_text (GTK_LABEL (((GstTimeTool *) tzmap->tool)->map_hover_label));
	if (strcmp (old_zone, ""))
		gtk_label_set_text (GTK_LABEL (((GstTimeTool *) tzmap->tool)->map_hover_label), "");
   
	return TRUE;
}
   
static gboolean
button_pressed (GtkWidget *w, GdkEventButton *event, gpointer data)
{
	ETzMap *tzmap;
	double longitude, latitude;
	
	tzmap = (ETzMap *) data;

	e_map_window_to_world (tzmap->map, (double) event->x, (double) event->y,
			       &longitude, &latitude);
	
	if (event->button != 1)
	        e_map_zoom_out (tzmap->map);
	else
	{
		GtkWidget  *location_combo;
		TzLocation *tz_location;
		gchar      *entry_text, *entry_text_new;

		if (e_map_get_magnification (tzmap->map) <= 1.0)
		        e_map_zoom_to_location (tzmap->map, longitude, latitude);

		if (tzmap->point_selected)
		        e_map_point_set_color_rgba (tzmap->map,
						    tzmap->point_selected,
						    TZ_MAP_POINT_NORMAL_RGBA);
		tzmap->point_selected = tzmap->point_hover;

		location_combo = gst_dialog_get_widget (tzmap->tool->main_dialog, "location_combo");
		tz_location    = e_tz_map_location_from_point (tzmap, tzmap->point_selected);

		entry_text     = e_tz_map_get_selected_tz_name (tzmap);
		entry_text_new = tz_location_get_zone (tz_location);

		if (!entry_text || !entry_text_new || strcmp (entry_text, entry_text_new))
			e_tz_map_set_location_text (tzmap, entry_text_new);
	}

	return TRUE;
}
