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
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 */

#ifndef TOTEM_GALLERY_PROGRESS_H
#define TOTEM_GALLERY_PROGRESS_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_GALLERY_PROGRESS		(totem_gallery_progress_get_type ())
#define TOTEM_GALLERY_PROGRESS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_GALLERY_PROGRESS, TotemGalleryProgress))
#define TOTEM_GALLERY_PROGRESS_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_GALLERY_PROGRESS, TotemGalleryProgressClass))
#define TOTEM_IS_GALLERY_PROGRESS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_GALLERY_PROGRESS))
#define TOTEM_IS_GALLERY_PROGRESS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_GALLERY_PROGRESS))
#define TOTEM_GALLERY_PROGRESS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_GALLERY_PROGRESS, TotemGalleryProgressClass))

typedef struct _TotemGalleryProgressPrivate	TotemGalleryProgressPrivate;

typedef struct {
	GtkDialog parent;
	TotemGalleryProgressPrivate *priv;
} TotemGalleryProgress;

typedef struct {
	GtkDialogClass parent;
} TotemGalleryProgressClass;

GType totem_gallery_progress_get_type (void);
TotemGalleryProgress *totem_gallery_progress_new (GPid child_pid, const gchar *output_filename);
void totem_gallery_progress_run (TotemGalleryProgress *self, gint stdout_fd);

G_END_DECLS

#endif /* !TOTEM_GALLERY_PROGRESS_H */
