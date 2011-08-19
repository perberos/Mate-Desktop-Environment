/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Sunil Mohan Adapa <sunilmohan@gnu.org.in>
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
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "bacon-video-widget.h"

#define TOTEM_TYPE_FULLSCREEN            (totem_fullscreen_get_type ())
#define TOTEM_FULLSCREEN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          TOTEM_TYPE_FULLSCREEN, \
                                          TotemFullscreen))
#define TOTEM_FULLSCREEN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                          TOTEM_TYPE_FULLSCREEN, \
                                          TotemFullscreenClass))
#define TOTEM_IS_FULLSCREEN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          TOTEM_TYPE_FULLSCREEN))
#define TOTEM_IS_FULLSCREEN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                          TOTEM_TYPE_FULLSCREEN))

typedef struct TotemFullscreen TotemFullscreen;
typedef struct TotemFullscreenClass TotemFullscreenClass;
typedef struct _TotemFullscreenPrivate TotemFullscreenPrivate;

struct TotemFullscreen {
	GObject                parent;
	
	/* Public Widgets from popups */
	GtkWidget              *time_label;
	GtkWidget              *seek;
	GtkWidget              *volume;
	GtkWidget              *buttons_box;
	GtkWidget              *exit_button;

	/* Read only */
	gboolean                is_fullscreen;

	/* Private */
	TotemFullscreenPrivate *priv;
};

struct TotemFullscreenClass {
	GObjectClass parent_class;
};

GType    totem_fullscreen_get_type           (void);
TotemFullscreen * totem_fullscreen_new       (GtkWindow *toplevel_window);
void     totem_fullscreen_set_video_widget   (TotemFullscreen *fs,
					      BaconVideoWidget *bvw);
void     totem_fullscreen_set_parent_window  (TotemFullscreen *fs,
					      GtkWindow *parent_window);
void     totem_fullscreen_show_popups        (TotemFullscreen *fs,
					      gboolean show_cursor);
void totem_fullscreen_show_popups_or_osd (TotemFullscreen *fs,
					  const char *icon_name,
					  gboolean show_cursor);
gboolean totem_fullscreen_is_fullscreen      (TotemFullscreen *fs);
void     totem_fullscreen_set_fullscreen     (TotemFullscreen *fs,
					      gboolean fullscreen);
void     totem_fullscreen_set_title          (TotemFullscreen *fs,
					      const char *title);
void     totem_fullscreen_set_seekable       (TotemFullscreen *fs,
					      gboolean seekable);
void     totem_fullscreen_set_can_set_volume (TotemFullscreen *fs,
					      gboolean can_set_volume);
