/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004 Bastien Nocera
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

#ifndef TOTEM_SCREENSHOT_H
#define TOTEM_SCREENSHOT_H

#include <gtk/gtk.h>
#include "totem-plugin.h"

G_BEGIN_DECLS

#define TOTEM_TYPE_SCREENSHOT			(totem_screenshot_get_type ())
#define TOTEM_SCREENSHOT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_SCREENSHOT, TotemScreenshot))
#define TOTEM_SCREENSHOT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_SCREENSHOT, TotemScreenshotClass))
#define TOTEM_IS_SCREENSHOT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_SCREENSHOT))
#define TOTEM_IS_SCREENSHOT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_SCREENSHOT))

typedef struct TotemScreenshot			TotemScreenshot;
typedef struct TotemScreenshotClass		TotemScreenshotClass;
typedef struct TotemScreenshotPrivate		TotemScreenshotPrivate;

struct TotemScreenshot {
	GtkDialog parent;
	TotemScreenshotPrivate *priv;
};

struct TotemScreenshotClass {
	GtkDialogClass parent_class;
};

GType totem_screenshot_get_type (void) G_GNUC_CONST;
GtkWidget *totem_screenshot_new (Totem *totem, TotemPlugin *screenshot_plugin, GdkPixbuf *screen_image) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !TOTEM_SCREENSHOT_H */
