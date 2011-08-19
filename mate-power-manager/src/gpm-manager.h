/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2005-2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GPM_MANAGER_H
#define __GPM_MANAGER_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define GPM_TYPE_MANAGER	 (gpm_manager_get_type ())
#define GPM_MANAGER(o)		 (G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_MANAGER, GpmManager))
#define GPM_MANAGER_CLASS(k)	 (G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_MANAGER, GpmManagerClass))
#define GPM_IS_MANAGER(o)	 (G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_MANAGER))
#define GPM_IS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_MANAGER))
#define GPM_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_MANAGER, GpmManagerClass))
#define GPM_MANAGER_ERROR	 (gpm_manager_error_quark ())
#define GPM_MANAGER_TYPE_ERROR	 (gpm_manager_error_get_type ()) 

typedef struct GpmManagerPrivate GpmManagerPrivate;

typedef struct
{
	 GObject		 parent;
	 GpmManagerPrivate	*priv;
} GpmManager;

typedef struct
{
	GObjectClass	parent_class;
} GpmManagerClass;

typedef enum
{
	GPM_MANAGER_ERROR_DENIED,
	GPM_MANAGER_ERROR_NO_HW,
	GPM_MANAGER_ERROR_LAST
} GpmManagerError;


GQuark		 gpm_manager_error_quark		(void);
GType		 gpm_manager_error_get_type		(void);
GType		 gpm_manager_get_type		  	(void);
GpmManager	*gpm_manager_new			(void);

gboolean	 gpm_manager_suspend			(GpmManager	*manager,
							 GError		**error);
gboolean	 gpm_manager_hibernate			(GpmManager	*manager,
							 GError		**error);
gboolean	 gpm_manager_can_suspend		(GpmManager	*manager,
							 gboolean	*can_suspend,
							 GError		**error);
gboolean	 gpm_manager_can_hibernate		(GpmManager	*manager,
							 gboolean	*can_hibernate,
							 GError		**error);
gboolean	 gpm_manager_get_preferences_options	(GpmManager	*manager,
							 gint		*capability,
							 GError		**error);

G_END_DECLS

#endif /* __GPM_MANAGER_H */
