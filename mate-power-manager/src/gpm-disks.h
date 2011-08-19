/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPM_DISKS_H
#define __GPM_DISKS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_DISKS		(gpm_disks_get_type ())
#define GPM_DISKS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_DISKS, GpmDisks))
#define GPM_DISKS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_DISKS, GpmDisksClass))
#define GPM_IS_DISKS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_DISKS))
#define GPM_IS_DISKS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_DISKS))
#define GPM_DISKS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_DISKS, GpmDisksClass))

typedef struct GpmDisksPrivate GpmDisksPrivate;

typedef struct
{
	GObject		 parent;
	GpmDisksPrivate	*priv;
} GpmDisks;

typedef struct
{
	GObjectClass	parent_class;
} GpmDisksClass;

GType		 gpm_disks_get_type			(void);
GpmDisks	*gpm_disks_new				(void);

gboolean	 gpm_disks_set_spindown_timeout		(GpmDisks	*disks,
							 gint		 timeout);

G_END_DECLS

#endif /* __GPM_DISKS_H */
