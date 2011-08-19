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

#ifndef __GPM_TRAY_ICON_H
#define __GPM_TRAY_ICON_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_TRAY_ICON		(gpm_tray_icon_get_type ())
#define GPM_TRAY_ICON(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_TRAY_ICON, GpmTrayIcon))
#define GPM_TRAY_ICON_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_TRAY_ICON, GpmTrayIconClass))
#define GPM_IS_TRAY_ICON(o)	 	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_TRAY_ICON))
#define GPM_IS_TRAY_ICON_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_TRAY_ICON))
#define GPM_TRAY_ICON_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_TRAY_ICON, GpmTrayIconClass))

typedef struct GpmTrayIconPrivate GpmTrayIconPrivate;

typedef struct
{
	GObject		    parent;
	GpmTrayIconPrivate *priv;
} GpmTrayIcon;

typedef struct
{
	GObjectClass	parent_class;
	void	(* suspend)				(GpmTrayIcon	*tray_icon);
	void	(* hibernate)				(GpmTrayIcon	*tray_icon);
} GpmTrayIconClass;

GType		 gpm_tray_icon_get_type			(void);
GpmTrayIcon	*gpm_tray_icon_new			(void);

gboolean	 gpm_tray_icon_set_tooltip		(GpmTrayIcon	*icon,
							 const gchar	*tooltip);
gboolean	 gpm_tray_icon_set_icon			(GpmTrayIcon	*icon,
							 const gchar	*filename);
GtkStatusIcon	*gpm_tray_icon_get_status_icon		(GpmTrayIcon	*icon);

G_END_DECLS

#endif /* __GPM_TRAY_ICON_H */
