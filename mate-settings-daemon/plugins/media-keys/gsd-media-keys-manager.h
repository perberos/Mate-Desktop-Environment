/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GSD_MEDIA_KEYS_MANAGER_H
#define __GSD_MEDIA_KEYS_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_MEDIA_KEYS_MANAGER         (gsd_media_keys_manager_get_type ())
#define GSD_MEDIA_KEYS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_MEDIA_KEYS_MANAGER, GsdMediaKeysManager))
#define GSD_MEDIA_KEYS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_MEDIA_KEYS_MANAGER, GsdMediaKeysManagerClass))
#define GSD_IS_MEDIA_KEYS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_MEDIA_KEYS_MANAGER))
#define GSD_IS_MEDIA_KEYS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_MEDIA_KEYS_MANAGER))
#define GSD_MEDIA_KEYS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_MEDIA_KEYS_MANAGER, GsdMediaKeysManagerClass))

typedef struct GsdMediaKeysManagerPrivate GsdMediaKeysManagerPrivate;

typedef struct
{
        GObject                     parent;
        GsdMediaKeysManagerPrivate *priv;
} GsdMediaKeysManager;

typedef struct
{
        GObjectClass   parent_class;
        void          (* media_player_key_pressed) (GsdMediaKeysManager *manager,
                                                    const char          *application,
                                                    const char          *key);
} GsdMediaKeysManagerClass;

GType                 gsd_media_keys_manager_get_type                  (void);

GsdMediaKeysManager * gsd_media_keys_manager_new                       (void);
gboolean              gsd_media_keys_manager_start                     (GsdMediaKeysManager *manager,
                                                                        GError             **error);
void                  gsd_media_keys_manager_stop                      (GsdMediaKeysManager *manager);

gboolean              gsd_media_keys_manager_grab_media_player_keys    (GsdMediaKeysManager *manager,
                                                                        const char          *application,
                                                                        guint32              time,
                                                                        GError             **error);
gboolean              gsd_media_keys_manager_release_media_player_keys (GsdMediaKeysManager *manager,
                                                                        const char          *application,
                                                                        GError             **error);

G_END_DECLS

#endif /* __GSD_MEDIA_KEYS_MANAGER_H */
