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

#ifndef __LOCATIONS_COMBO_H__
#define __LOCATIONS_COMBO_H__

G_BEGIN_DECLS

#include <glib.h>
#include <gtk/gtk.h>
#include "network-locations.h"
#include "gst.h"

#define GST_TYPE_LOCATIONS_COMBO           (gst_locations_combo_get_type ())
#define GST_LOCATIONS_COMBO(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_LOCATIONS_COMBO, GstLocationsCombo))
#define GST_LOCATIONS_COMBO_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_LOCATIONS_COMBO, GstLocationsComboClass))
#define GST_IS_LOCATIONS_COMBO(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_LOCATIONS_COMBO))
#define GST_IS_LOCATIONS_COMBO_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_LOCATIONS_COMBO))
#define GST_LOCATIONS_COMBO_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_LOCATIONS_COMBO, GstLocationsComboClass))

typedef struct _GstLocationsCombo      GstLocationsCombo;
typedef struct _GstLocationsComboClass GstLocationsComboClass;

struct _GstLocationsCombo
{
  GstNetworkLocations parent_instance;
  gpointer _priv;
};

struct _GstLocationsComboClass
{
  GstNetworkLocationsClass parent_class;
};

GType                  gst_locations_combo_get_type        (void);
GstLocationsCombo*     gst_locations_combo_new             (GstTool   *tool,
							    GtkWidget *combo,
							    GtkWidget *add,
							    GtkWidget *remove);


G_END_DECLS

#endif /* __LOCATIONS_COMBO_H__ */
