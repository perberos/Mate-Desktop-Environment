/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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

#ifndef TOTEM_SKIPTO_H
#define TOTEM_SKIPTO_H

#include <gtk/gtk.h>

#include "totem.h"
#include "totem-skipto-plugin.h"

G_BEGIN_DECLS

#define TOTEM_TYPE_SKIPTO		(totem_skipto_get_type ())
#define TOTEM_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_SKIPTO, TotemSkipto))
#define TOTEM_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_SKIPTO, TotemSkiptoClass))
#define TOTEM_IS_SKIPTO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_SKIPTO))
#define TOTEM_IS_SKIPTO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_SKIPTO))

GType totem_skipto_register_type	(GTypeModule *module);

typedef struct TotemSkipto		TotemSkipto;
typedef struct TotemSkiptoClass		TotemSkiptoClass;
typedef struct TotemSkiptoPrivate	TotemSkiptoPrivate;

struct TotemSkipto {
	GtkDialog parent;
	TotemSkiptoPrivate *priv;
};

struct TotemSkiptoClass {
	GtkDialogClass parent_class;
};

GType totem_skipto_get_type	(void);
GtkWidget *totem_skipto_new	(TotemSkiptoPlugin *plugin);
gint64 totem_skipto_get_range	(TotemSkipto *skipto);
void totem_skipto_update_range	(TotemSkipto *skipto, gint64 _time);
void totem_skipto_set_seekable	(TotemSkipto *skipto, gboolean seekable);
void totem_skipto_set_current	(TotemSkipto *skipto, gint64 _time);

G_END_DECLS

#endif /* TOTEM_SKIPTO_H */
