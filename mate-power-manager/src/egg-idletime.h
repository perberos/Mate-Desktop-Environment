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

#ifndef __EGG_IDLETIME_H
#define __EGG_IDLETIME_H

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_IDLETIME_TYPE		(egg_idletime_get_type ())
#define EGG_IDLETIME(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), EGG_IDLETIME_TYPE, EggIdletime))
#define EGG_IDLETIME_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), EGG_IDLETIME_TYPE, EggIdletimeClass))
#define EGG_IS_IDLETIME(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EGG_IDLETIME_TYPE))
#define EGG_IS_IDLETIME_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EGG_IDLETIME_TYPE))
#define EGG_IDLETIME_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EGG_IDLETIME_TYPE, EggIdletimeClass))

typedef struct EggIdletimePrivate EggIdletimePrivate;

typedef struct
{
	GObject			 parent;
	EggIdletimePrivate	*priv;
} EggIdletime;

typedef struct
{
	GObjectClass	parent_class;
	void		(* alarm_expired)		(EggIdletime	*idletime,
							 guint		 timer_id);
	void		(* reset)			(EggIdletime	*idletime);
} EggIdletimeClass;

GType		 egg_idletime_get_type			(void);
EggIdletime	*egg_idletime_new			(void);

void		 egg_idletime_alarm_reset_all		(EggIdletime	*idletime);
gboolean	 egg_idletime_alarm_set			(EggIdletime	*idletime,
							 guint		 alarm_id,
							 guint		 timeout);
gboolean	 egg_idletime_alarm_remove		(EggIdletime	*idletime,
							 guint		 alarm_id);
gint64		 egg_idletime_get_time			(EggIdletime	*idletime);
#ifdef EGG_TEST
void		 egg_idletime_test			(gpointer	 data);
#endif

G_END_DECLS

#endif	/* __EGG_IDLETIME_H */
