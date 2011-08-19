/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef GSD_MEDIA_KEYS_WINDOW_H
#define GSD_MEDIA_KEYS_WINDOW_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSD_TYPE_MEDIA_KEYS_WINDOW            (gsd_media_keys_window_get_type ())
#define GSD_MEDIA_KEYS_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  GSD_TYPE_MEDIA_KEYS_WINDOW, GsdMediaKeysWindow))
#define GSD_MEDIA_KEYS_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),   GSD_TYPE_MEDIA_KEYS_WINDOW, GsdMediaKeysWindowClass))
#define GSD_IS_MEDIA_KEYS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  GSD_TYPE_MEDIA_KEYS_WINDOW))
#define GSD_IS_MEDIA_KEYS_WINDOW_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), GSD_TYPE_MEDIA_KEYS_WINDOW))

typedef struct GsdMediaKeysWindow                   GsdMediaKeysWindow;
typedef struct GsdMediaKeysWindowClass              GsdMediaKeysWindowClass;
typedef struct GsdMediaKeysWindowPrivate            GsdMediaKeysWindowPrivate;

struct GsdMediaKeysWindow {
        GtkWindow                   parent;

        GsdMediaKeysWindowPrivate  *priv;
};

struct GsdMediaKeysWindowClass {
        GtkWindowClass parent_class;
};

typedef enum {
        GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME,
        GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM
} GsdMediaKeysWindowAction;

GType                 gsd_media_keys_window_get_type          (void);

GtkWidget *           gsd_media_keys_window_new               (void);
void                  gsd_media_keys_window_set_action        (GsdMediaKeysWindow      *window,
                                                               GsdMediaKeysWindowAction action);
void                  gsd_media_keys_window_set_action_custom (GsdMediaKeysWindow      *window,
                                                               const char              *icon_name,
                                                               gboolean                 show_level);
void                  gsd_media_keys_window_set_volume_muted  (GsdMediaKeysWindow      *window,
                                                               gboolean                 muted);
void                  gsd_media_keys_window_set_volume_level  (GsdMediaKeysWindow      *window,
                                                               int                      level);
gboolean              gsd_media_keys_window_is_valid          (GsdMediaKeysWindow      *window);

G_END_DECLS

#endif
