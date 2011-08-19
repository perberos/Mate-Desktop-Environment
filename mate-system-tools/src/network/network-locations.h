/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#ifndef __NETWORK_LOCATIONS_H
#define __NETWORK_LOCATIONS_H

G_BEGIN_DECLS

#include <glib.h>
#include <glib-object.h>
#include <oobs/oobs.h>

#define GST_TYPE_NETWORK_LOCATIONS           (gst_network_locations_get_type ())
#define GST_NETWORK_LOCATIONS(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_NETWORK_LOCATIONS, GstNetworkLocations))
#define GST_NETWORK_LOCATIONS_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_NETWORK_LOCATIONS, GstNetworkLocationsClass))
#define GST_IS_NETWORK_LOCATIONS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_NETWORK_LOCATIONS))
#define GST_IS_NETWORK_LOCATIONS_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_NETWORK_LOCATIONS))
#define GST_NETWORK_LOCATIONS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_NETWORK_LOCATIONS, GstNetworkLocationsClass))

typedef struct _GstNetworkLocations      GstNetworkLocations;
typedef struct _GstNetworkLocationsClass GstNetworkLocationsClass;

struct _GstNetworkLocations
{
  GObject parent_instance;
  OobsObject *ifaces_config;
  OobsObject *hosts_config;
  gpointer _priv;
};

struct _GstNetworkLocationsClass
{
  GObjectClass parent_class;

  void (*changed) (GstNetworkLocations *locations);
};


GType                  gst_network_locations_get_type        (void);
GstNetworkLocations*   gst_network_locations_get             (void);

GList*                 gst_network_locations_get_names       (GstNetworkLocations *locations);
gchar*                 gst_network_locations_get_current     (GstNetworkLocations *locations);
gboolean               gst_network_locations_set_location    (GstNetworkLocations *locations,
							      const gchar         *name);
gboolean               gst_network_locations_save_current    (GstNetworkLocations *locations,
							      const gchar         *name);
void                   gst_network_locations_delete_location (GstNetworkLocations *locations,
							      const gchar         *name);

G_END_DECLS

#endif /* __NETWORK_LOCATIONS_H */
