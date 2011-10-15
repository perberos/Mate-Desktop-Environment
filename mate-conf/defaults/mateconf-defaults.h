/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008  Matthias Clasen  <mclasen@redhat.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef MATECONF_DEFAULTS_H
#define MATECONF_DEFAULTS_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECONF_TYPE_DEFAULTS         (mateconf_defaults_get_type ())
#define MATECONF_DEFAULTS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECONF_TYPE_DEFAULTS, MateConfDefaults))
#define MATECONF_DEFAULTS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MATECONF_TYPE_DEFAULTS, MateConfDefaultsClass))
#define MATECONF_IS_DEFAULTS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECONF_TYPE_DEFAULTS))
#define MATECONF_IS_DEFAULTS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MATECONF_TYPE_DEFAULTS))
#define MATECONF_DEFAULTS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MATECONF_TYPE_DEFAULTS, MateConfDefaultsClass))

typedef struct MateConfDefaultsPrivate MateConfDefaultsPrivate;

typedef struct
{
        GObject        parent;
        MateConfDefaultsPrivate *priv;
} MateConfDefaults;

typedef struct
{
        GObjectClass   parent_class;

	void (* system_set) (MateConfDefaults  *defaults,
                             const char    **keys);

} MateConfDefaultsClass;

typedef enum
{
        MATECONF_DEFAULTS_ERROR_GENERAL,
        MATECONF_DEFAULTS_ERROR_NOT_PRIVILEGED,
        MATECONF_DEFAULTS_NUM_ERRORS
} MateConfDefaultsError;

#define MATECONF_DEFAULTS_ERROR mateconf_defaults_error_quark ()

GType mateconf_defaults_error_get_type (void);
#define MATECONF_DEFAULTS_TYPE_ERROR (mateconf_defaults_error_get_type ())


GQuark         mateconf_defaults_error_quark    (void);
GType          mateconf_defaults_get_type       (void);
MateConfDefaults *mateconf_defaults_new            (void);

/* exported methods */
void           mateconf_defaults_set_system          (MateConfDefaults          *mechanism,
                                                   const char            **includes,
                                                   const char            **excludes,
                                                   DBusGMethodInvocation  *context);

void           mateconf_defaults_set_system_value    (MateConfDefaults          *mechanism,
                                                   const char             *path,
                                                   const char             *value,
                                                   DBusGMethodInvocation  *context);

void           mateconf_defaults_set_mandatory       (MateConfDefaults          *mechanism,
                                                   const char            **includes,
                                                   const char            **excludes,
                                                   DBusGMethodInvocation  *context);

void           mateconf_defaults_set_mandatory_value (MateConfDefaults          *mechanism,
                                                   const char             *path,
                                                   const char             *value,
                                                   DBusGMethodInvocation  *context);

void           mateconf_defaults_unset_mandatory     (MateConfDefaults          *mechanism,
                                                   const char            **includes,
                                                   const char            **excludes,
                                                   DBusGMethodInvocation  *context);

void		mateconf_defaults_can_set_system    (MateConfDefaults         *mechanism,
						  const char	       **includes,
                                                  DBusGMethodInvocation  *context);

void		mateconf_defaults_can_set_mandatory (MateConfDefaults         *mechanism,
						  const char	       **includes,
                                                  DBusGMethodInvocation  *context);

G_END_DECLS

#endif /* MATECONF_DEFAULTS_H */
