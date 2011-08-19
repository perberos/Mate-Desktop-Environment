/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPM_PREFS_SERVER_H
#define __GPM_PREFS_SERVER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_PREFS_SERVER		(gpm_prefs_server_get_type ())
#define GPM_PREFS_SERVER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_PREFS_SERVER, GpmPrefsServer))
#define GPM_PREFS_SERVER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_PREFS_SERVER, GpmPrefsServerClass))
#define GPM_IS_PREFS_SERVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_PREFS_SERVER))
#define GPM_IS_PREFS_SERVER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_PREFS_SERVER))
#define GPM_PREFS_SERVER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_PREFS_SERVER, GpmPrefsServerClass))

#define GPM_PREFS_SERVER_BACKLIGHT	1
#define GPM_PREFS_SERVER_LID		4
#define GPM_PREFS_SERVER_BATTERY	8
#define GPM_PREFS_SERVER_UPS		16
#define GPM_PREFS_SERVER_KEYLIGHT	32

typedef struct GpmPrefsServerPrivate GpmPrefsServerPrivate;

typedef struct
{
	GObject		      parent;
	GpmPrefsServerPrivate *priv;
} GpmPrefsServer;

typedef struct
{
	GObjectClass	parent_class;
	void		(* capability_changed)		(GpmPrefsServer	*server,
							 gint	         capability);
} GpmPrefsServerClass;

GType		 gpm_prefs_server_get_type		(void);
GpmPrefsServer *gpm_prefs_server_new			(void);

gboolean	 gpm_prefs_server_get_capability	(GpmPrefsServer	*server,
							 gint		*capability);
gboolean	 gpm_prefs_server_set_capability	(GpmPrefsServer	*server,
							 gint		 capability);

G_END_DECLS

#endif /* __GPM_PREFS_SERVER_H */
