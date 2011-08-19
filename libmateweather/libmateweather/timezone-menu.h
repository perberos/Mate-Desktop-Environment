/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* timezone-menu.h - Timezone-selecting menu
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

#ifndef MATEWEATHER_TIMEZONE_MENU_H
#define MATEWEATHER_TIMEZONE_MENU_H 1

#include <gtk/gtk.h>
#include <libmateweather/mateweather-location.h>

#define MATEWEATHER_TYPE_TIMEZONE_MENU            (mateweather_timezone_menu_get_type ())
#define MATEWEATHER_TIMEZONE_MENU(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), MATEWEATHER_TYPE_TIMEZONE_MENU, MateWeatherTimezoneMenu))
#define MATEWEATHER_TIMEZONE_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATEWEATHER_TYPE_TIMEZONE_MENU, MateWeatherTimezoneMenuClass))
#define MATEWEATHER_IS_TIMEZONE_MENU(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), MATEWEATHER_TYPE_TIMEZONE_MENU))
#define MATEWEATHER_IS_TIMEZONE_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATEWEATHER_TYPE_TIMEZONE_MENU))
#define MATEWEATHER_TIMEZONE_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATEWEATHER_TYPE_TIMEZONE_MENU, MateWeatherTimezoneMenuClass))

typedef struct {
    GtkComboBox parent;

    /*< private >*/
    MateWeatherTimezone *zone;
} MateWeatherTimezoneMenu;

typedef struct {
    GtkComboBoxClass parent_class;

} MateWeatherTimezoneMenuClass;

GType       mateweather_timezone_menu_get_type         (void);

GtkWidget  *mateweather_timezone_menu_new              (MateWeatherLocation     *top);

void        mateweather_timezone_menu_set_tzid         (MateWeatherTimezoneMenu *menu,
						     const char           *tzid);
const char *mateweather_timezone_menu_get_tzid         (MateWeatherTimezoneMenu *menu);

#endif
