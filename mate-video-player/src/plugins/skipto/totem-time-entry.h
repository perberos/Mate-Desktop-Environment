/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008 Philip Withnall <philip@tecnocode.co.uk>
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
 * Author: Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef TOTEM_TIME_ENTRY_H
#define TOTEM_TIME_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_TIME_ENTRY		(totem_time_entry_get_type ())
#define TOTEM_TIME_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_TIME_ENTRY, TotemTimeEntry))
#define TOTEM_TIME_ENTRY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_TIME_ENTRY, TotemTimeEntryClass))
#define TOTEM_IS_TIME_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_TIME_ENTRY))
#define TOTEM_IS_TIME_ENTRY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_TIME_ENTRY))
#define TOTEM_TIME_ENTRY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_TIME_ENTRY, TotemTimeEntryClass))

typedef struct {
	GtkSpinButton parent;
} TotemTimeEntry;

typedef struct {
	GtkSpinButtonClass parent;
} TotemTimeEntryClass;

GType totem_time_entry_get_type (void);
GtkWidget *totem_time_entry_new (GtkAdjustment *adjustment, gdouble climb_rate);

G_END_DECLS

#endif /* !TOTEM_TIME_ENTRY_H */
