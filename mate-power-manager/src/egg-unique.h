/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __EGG_UNIQUE_H
#define __EGG_UNIQUE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_UNIQUE_TYPE		(egg_unique_get_type ())
#define EGG_UNIQUE_OBJECT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), EGG_UNIQUE_TYPE, EggUnique))
#define EGG_UNIQUE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EGG_UNIQUE_TYPE, EggUniqueClass))
#define EGG_IS_UNIQUE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), EGG_UNIQUE_TYPE))
#define EGG_IS_UNIQUE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EGG_UNIQUE_TYPE))
#define EGG_UNIQUE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EGG_UNIQUE_TYPE, EggUniqueClass))

typedef struct EggUniquePrivate EggUniquePrivate;

typedef struct
{
	GObject		parent;
	EggUniquePrivate *priv;
} EggUnique;

typedef struct
{
	GObjectClass	parent_class;
	void		(* activated)		(EggUnique	*unique);
} EggUniqueClass;

GType		 egg_unique_get_type		(void);
EggUnique	*egg_unique_new			(void);

gboolean	 egg_unique_assign		(EggUnique	*unique,
						 const gchar	*service);

G_END_DECLS

#endif	/* __EGG_UNIQUE_H */

