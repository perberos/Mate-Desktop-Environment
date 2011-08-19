/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPM_LOAD_H
#define __GPM_LOAD_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_LOAD		(gpm_load_get_type ())
#define GPM_LOAD(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_LOAD, GpmLoad))
#define GPM_LOAD_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_LOAD, GpmLoadClass))
#define GPM_IS_LOAD(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_LOAD))
#define GPM_IS_LOAD_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_LOAD))
#define GPM_LOAD_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_LOAD, GpmLoadClass))

typedef struct GpmLoadPrivate GpmLoadPrivate;

typedef struct
{
	GObject		 parent;
	GpmLoadPrivate	*priv;
} GpmLoad;

typedef struct
{
	GObjectClass	parent_class;
} GpmLoadClass;

GType		 gpm_load_get_type		(void);
GpmLoad		*gpm_load_new			(void);

gdouble		 gpm_load_get_current		(GpmLoad	*load);

G_END_DECLS

#endif /* __GPM_LOAD_H */
