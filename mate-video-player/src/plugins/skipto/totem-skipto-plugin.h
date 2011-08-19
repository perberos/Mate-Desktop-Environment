/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef TOTEM_SKIPTO_PLUGIN_H
#define TOTEM_SKIPTO_PLUGIN_H

#include <glib.h>

#include "totem.h"
#include "totem-plugin.h"

G_BEGIN_DECLS

#define TOTEM_TYPE_SKIPTO_PLUGIN		(totem_skipto_plugin_get_type ())
#define TOTEM_SKIPTO_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPlugin))
#define TOTEM_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPluginClass))
#define TOTEM_IS_SKIPTO_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_SKIPTO_PLUGIN))
#define TOTEM_IS_SKIPTO_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_SKIPTO_PLUGIN))
#define TOTEM_SKIPTO_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_SKIPTO_PLUGIN, TotemSkiptoPluginClass))

typedef struct TotemSkiptoPluginPrivate         TotemSkiptoPluginPrivate;

typedef struct
{
	TotemPlugin	          parent;

	TotemObject              *totem;
	TotemSkiptoPluginPrivate *priv;
} TotemSkiptoPlugin;

typedef struct
{
	TotemPluginClass parent_class;
} TotemSkiptoPluginClass;

GType totem_skipto_plugin_get_type			(void) G_GNUC_CONST;

G_END_DECLS

#endif /* TOTEM_SKIPTO_PLUGIN_H */
