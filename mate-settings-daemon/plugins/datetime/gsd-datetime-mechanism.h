/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef GSD_DATETIME_MECHANISM_H
#define GSD_DATETIME_MECHANISM_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GSD_DATETIME_TYPE_MECHANISM         (gsd_datetime_mechanism_get_type ())
#define GSD_DATETIME_MECHANISM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_DATETIME_TYPE_MECHANISM, GsdDatetimeMechanism))
#define GSD_DATETIME_MECHANISM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_DATETIME_TYPE_MECHANISM, GsdDatetimeMechanismClass))
#define GSD_DATETIME_IS_MECHANISM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_DATETIME_TYPE_MECHANISM))
#define GSD_DATETIME_IS_MECHANISM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_DATETIME_TYPE_MECHANISM))
#define GSD_DATETIME_MECHANISM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_DATETIME_TYPE_MECHANISM, GsdDatetimeMechanismClass))

typedef struct GsdDatetimeMechanismPrivate GsdDatetimeMechanismPrivate;

typedef struct
{
        GObject        parent;
        GsdDatetimeMechanismPrivate *priv;
} GsdDatetimeMechanism;

typedef struct
{
        GObjectClass   parent_class;
} GsdDatetimeMechanismClass;

typedef enum
{
        GSD_DATETIME_MECHANISM_ERROR_GENERAL,
        GSD_DATETIME_MECHANISM_ERROR_NOT_PRIVILEGED,
        GSD_DATETIME_MECHANISM_ERROR_INVALID_TIMEZONE_FILE,
        GSD_DATETIME_MECHANISM_NUM_ERRORS
} GsdDatetimeMechanismError;

#define GSD_DATETIME_MECHANISM_ERROR gsd_datetime_mechanism_error_quark ()

GType gsd_datetime_mechanism_error_get_type (void);
#define GSD_DATETIME_MECHANISM_TYPE_ERROR (gsd_datetime_mechanism_error_get_type ())


GQuark                     gsd_datetime_mechanism_error_quark         (void);
GType                      gsd_datetime_mechanism_get_type            (void);
GsdDatetimeMechanism      *gsd_datetime_mechanism_new                 (void);

/* exported methods */
gboolean            gsd_datetime_mechanism_get_timezone (GsdDatetimeMechanism   *mechanism,
                                                         DBusGMethodInvocation  *context);
gboolean            gsd_datetime_mechanism_set_timezone (GsdDatetimeMechanism   *mechanism,
                                                         const char             *zone_file,
                                                         DBusGMethodInvocation  *context);

gboolean            gsd_datetime_mechanism_can_set_timezone (GsdDatetimeMechanism  *mechanism,
                                                             DBusGMethodInvocation *context);

gboolean            gsd_datetime_mechanism_set_time     (GsdDatetimeMechanism  *mechanism,
                                                         gint64                 seconds_since_epoch,
                                                         DBusGMethodInvocation *context);

gboolean            gsd_datetime_mechanism_can_set_time (GsdDatetimeMechanism  *mechanism,
                                                         DBusGMethodInvocation *context);

gboolean            gsd_datetime_mechanism_adjust_time  (GsdDatetimeMechanism  *mechanism,
                                                         gint64                 seconds_to_add,
                                                         DBusGMethodInvocation *context);

gboolean            gsd_datetime_mechanism_get_hardware_clock_using_utc  (GsdDatetimeMechanism  *mechanism,
                                                                          DBusGMethodInvocation *context);

gboolean            gsd_datetime_mechanism_set_hardware_clock_using_utc  (GsdDatetimeMechanism  *mechanism,
                                                                          gboolean               using_utc,
                                                                          DBusGMethodInvocation *context);

#ifdef __cplusplus
}
#endif

#endif /* GSD_DATETIME_MECHANISM_H */
