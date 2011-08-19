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

#ifndef __EGG_CONSOLE_KIT_H
#define __EGG_CONSOLE_KIT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define EGG_TYPE_CONSOLE_KIT		(egg_console_kit_get_type ())
#define EGG_CONSOLE_KIT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EGG_TYPE_CONSOLE_KIT, EggConsoleKit))
#define EGG_CONSOLE_KIT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), EGG_TYPE_CONSOLE_KIT, EggConsoleKitClass))
#define EGG_IS_CONSOLE_KIT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EGG_TYPE_CONSOLE_KIT))
#define EGG_IS_CONSOLE_KIT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EGG_TYPE_CONSOLE_KIT))
#define EGG_CONSOLE_KIT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EGG_TYPE_CONSOLE_KIT, EggConsoleKitClass))
#define EGG_CONSOLE_KIT_ERROR		(egg_console_kit_error_quark ())
#define EGG_CONSOLE_KIT_TYPE_ERROR	(egg_console_kit_error_get_type ())

typedef struct EggConsoleKitPrivate EggConsoleKitPrivate;

typedef struct
{
	 GObject		 parent;
	 EggConsoleKitPrivate	*priv;
} EggConsoleKit;

typedef struct
{
	GObjectClass	parent_class;
	void		(* active_changed)		(EggConsoleKit	*console,
							 gboolean	 active);
} EggConsoleKitClass;

GType		 egg_console_kit_get_type	  	(void);
EggConsoleKit	*egg_console_kit_new			(void);
gboolean	 egg_console_kit_is_local		(EggConsoleKit	*console);
gboolean	 egg_console_kit_is_active		(EggConsoleKit	*console);
gboolean	 egg_console_kit_stop			(EggConsoleKit	*console,
							 GError		**error);
gboolean	 egg_console_kit_restart		(EggConsoleKit	*console,
							 GError		**error);
gboolean	 egg_console_kit_can_stop		(EggConsoleKit	*console,
							 gboolean	*can_stop,
							 GError		**error);
gboolean	 egg_console_kit_can_restart		(EggConsoleKit	*console,
							 gboolean	*can_restart,
							 GError		**error);

G_END_DECLS

#endif /* __EGG_CONSOLE_KIT_H */

