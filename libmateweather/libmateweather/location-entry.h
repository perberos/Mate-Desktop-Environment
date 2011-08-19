/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* location-entry.h - Location-selecting text entry
 *
 * Copyright 2008, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef MATEWEATHER_LOCATION_ENTRY_H
#define MATEWEATHER_LOCATION_ENTRY_H 1

#include <gtk/gtk.h>
#include <libmateweather/mateweather-location.h>

#define MATEWEATHER_TYPE_LOCATION_ENTRY            (mateweather_location_entry_get_type ())
#define MATEWEATHER_LOCATION_ENTRY(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MATEWEATHER_TYPE_LOCATION_ENTRY, MateWeatherLocationEntry))
#define MATEWEATHER_LOCATION_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATEWEATHER_TYPE_LOCATION_ENTRY, MateWeatherLocationEntryClass))
#define MATEWEATHER_IS_LOCATION_ENTRY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MATEWEATHER_TYPE_LOCATION_ENTRY))
#define MATEWEATHER_IS_LOCATION_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATEWEATHER_TYPE_LOCATION_ENTRY))
#define MATEWEATHER_LOCATION_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATEWEATHER_TYPE_LOCATION_ENTRY, MateWeatherLocationEntryClass))

typedef struct {
    GtkEntry parent;

    /*< private >*/
    MateWeatherLocation *location, *top;
    guint custom_text : 1;
} MateWeatherLocationEntry;

typedef struct {
    GtkEntryClass parent_class;

} MateWeatherLocationEntryClass;

GType             mateweather_location_entry_get_type     (void);

GtkWidget        *mateweather_location_entry_new          (MateWeatherLocation      *top);

void              mateweather_location_entry_set_location (MateWeatherLocationEntry *entry,
							MateWeatherLocation      *loc);
MateWeatherLocation *mateweather_location_entry_get_location (MateWeatherLocationEntry *entry);

gboolean          mateweather_location_entry_has_custom_text (MateWeatherLocationEntry *entry);

gboolean          mateweather_location_entry_set_city     (MateWeatherLocationEntry *entry,
							const char            *city_name,
							const char            *code);

#endif
