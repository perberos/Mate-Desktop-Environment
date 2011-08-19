/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * MATE Screenshot widget
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
 * 
 * MATE Screenshot widget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * MATE Screenshot widget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MATE Screenshot widget.  If not, see <http://www.gnu.org/licenses/>.
 *
 * The Totem project hereby grant permission for non-GPL compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 10th August 2009: Philip Withnall: Add exception clause.
 * Permission from the previous authors granted via e-mail.
 */

#ifndef MATE_SCREENSHOT_WIDGET_H
#define MATE_SCREENSHOT_WIDGET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_SCREENSHOT_WIDGET		(mate_screenshot_widget_get_type ())
#define MATE_SCREENSHOT_WIDGET(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_TYPE_SCREENSHOT_WIDGET, MateScreenshotWidget))
#define MATE_SCREENSHOT_WIDGET_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), MATE_TYPE_SCREENSHOT_WIDGET, MateScreenshotWidgetClass))
#define MATE_IS_SCREENSHOT_WIDGET(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_TYPE_SCREENSHOT_WIDGET))
#define MATE_IS_SCREENSHOT_WIDGET_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MATE_TYPE_SCREENSHOT_WIDGET))
#define MATE_SCREENSHOT_WIDGET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MATE_TYPE_SCREENSHOT_WIDGET, MateScreenshotWidgetClass))

typedef struct _MateScreenshotWidgetPrivate	MateScreenshotWidgetPrivate;

typedef struct {
	GtkBox parent;
	MateScreenshotWidgetPrivate *priv;
} MateScreenshotWidget;

typedef struct {
	GtkBoxClass parent;
} MateScreenshotWidgetClass;

GType mate_screenshot_widget_get_type (void) G_GNUC_CONST;

GtkWidget *mate_screenshot_widget_new (const gchar *interface_filename, GdkPixbuf *screenshot,
					const gchar *initial_uri) G_GNUC_WARN_UNUSED_RESULT;

void mate_screenshot_widget_focus_entry (MateScreenshotWidget *self);
gchar *mate_screenshot_widget_get_uri (MateScreenshotWidget *self) G_GNUC_WARN_UNUSED_RESULT;
gchar *mate_screenshot_widget_get_folder (MateScreenshotWidget *self) G_GNUC_WARN_UNUSED_RESULT;
GdkPixbuf *mate_screenshot_widget_get_screenshot (MateScreenshotWidget *self);

const gchar *mate_screenshot_widget_get_temporary_filename (MateScreenshotWidget *self);
void mate_screenshot_widget_set_temporary_filename (MateScreenshotWidget *self, const gchar *temporary_filename);

G_END_DECLS

#endif /* !MATE_SCREENSHOT_WIDGET_H */
