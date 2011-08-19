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

#ifndef __GPMBUTTON_H
#define __GPMBUTTON_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_BUTTON		(gpm_button_get_type ())
#define GPM_BUTTON(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_BUTTON, GpmButton))
#define GPM_BUTTON_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_BUTTON, GpmButtonClass))
#define GPM_IS_BUTTON(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_BUTTON))
#define GPM_IS_BUTTON_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_BUTTON))
#define GPM_BUTTON_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_BUTTON, GpmButtonClass))

typedef struct GpmButtonPrivate GpmButtonPrivate;

#define GPM_BUTTON_POWER		"power"
#define GPM_BUTTON_SLEEP		"sleep"
#define GPM_BUTTON_SUSPEND		"suspend"
#define GPM_BUTTON_HIBERNATE		"hibernate"
#define GPM_BUTTON_LID_DEP		"lid"		/* Remove when HAL drops input support */
#define GPM_BUTTON_LID_OPEN		"lid-up"
#define GPM_BUTTON_LID_CLOSED		"lid-down"
#define GPM_BUTTON_BRIGHT_UP		"brightness-up"
#define GPM_BUTTON_BRIGHT_DOWN		"brightness-down"
#define GPM_BUTTON_KBD_BRIGHT_UP	"kbd-illum-up"
#define GPM_BUTTON_KBD_BRIGHT_DOWN	"kbd-illum-down"
#define GPM_BUTTON_KBD_BRIGHT_TOGGLE	"kbd-illum-toggle"
#define GPM_BUTTON_LOCK			"lock"
#define GPM_BUTTON_BATTERY		"battery"

typedef struct
{
	GObject		 parent;
	GpmButtonPrivate *priv;
} GpmButton;

typedef struct
{
	GObjectClass	parent_class;
	void		(* button_pressed)	(GpmButton	*button,
						 const gchar	*type);
} GpmButtonClass;

GType		 gpm_button_get_type		(void);
GpmButton	*gpm_button_new			(void);
gboolean	 gpm_button_is_lid_closed	(GpmButton *button);
gboolean	 gpm_button_reset_time		(GpmButton *button);

G_END_DECLS

#endif	/* __GPMBUTTON_H */
