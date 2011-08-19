/*
   Copyright (C) 2009, Nokia

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Carlos Garnacho <carlos@lanedo.com>
 */

#ifndef __TOTEM_PL_PLAYLIST_H__
#define __TOTEM_PL_PLAYLIST_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_PL_PLAYLIST            (totem_pl_playlist_get_type ())
#define TOTEM_PL_PLAYLIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_PL_PLAYLIST, TotemPlPlaylist))
#define TOTEM_PL_PLAYLIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_PL_PLAYLIST, TotemPlPlaylistClass))
#define TOTEM_IS_PL_PLAYLIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_PL_PLAYLIST))
#define TOTEM_IS_PL_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_PL_PLAYLIST))

typedef struct TotemPlPlaylist TotemPlPlaylist;
typedef struct TotemPlPlaylistClass TotemPlPlaylistClass;
typedef struct TotemPlPlaylistIter TotemPlPlaylistIter;

/**
 * TotemPlPlaylist:
 *
 * All the fields in the #TotemPlPlaylist structure are private and should never be accessed directly.
 **/
struct TotemPlPlaylist {
        GObject parent_instance;
};

/**
 * TotemPlPlaylistClass:
 *
 * All the fields in the #TotemPlPlaylistClass structure are private and should never be accessed directly.
 **/
struct TotemPlPlaylistClass {
        GObjectClass parent_class;
};

/**
 * TotemPlPlaylistIter:
 *
 * All the fields in the #TotemPlPlaylistIter structure are private and should never be accessed directly.
 **/
struct TotemPlPlaylistIter {
        gpointer data1;
        gpointer data2;
};

GType totem_pl_playlist_get_type (void) G_GNUC_CONST;

TotemPlPlaylist * totem_pl_playlist_new (void);

guint totem_pl_playlist_size   (TotemPlPlaylist     *playlist);

/* Item insertion methods */
void totem_pl_playlist_prepend (TotemPlPlaylist     *playlist,
                                TotemPlPlaylistIter *iter);
void totem_pl_playlist_append  (TotemPlPlaylist     *playlist,
                                TotemPlPlaylistIter *iter);
void totem_pl_playlist_insert  (TotemPlPlaylist     *playlist,
                                gint                 position,
                                TotemPlPlaylistIter *iter);

/* Navigation methods */
gboolean totem_pl_playlist_iter_first (TotemPlPlaylist     *playlist,
                                       TotemPlPlaylistIter *iter);
gboolean totem_pl_playlist_iter_next  (TotemPlPlaylist     *playlist,
                                       TotemPlPlaylistIter *iter);
gboolean totem_pl_playlist_iter_prev  (TotemPlPlaylist     *playlist,
                                       TotemPlPlaylistIter *iter);

/* Item edition methods */
gboolean totem_pl_playlist_get_value (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      const gchar         *key,
                                      GValue              *value);
void totem_pl_playlist_get_valist    (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      va_list              args);
void totem_pl_playlist_get           (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      ...) G_GNUC_NULL_TERMINATED;

gboolean totem_pl_playlist_set_value (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      const gchar         *key,
                                      GValue              *value);
void totem_pl_playlist_set_valist    (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      va_list              args);
void totem_pl_playlist_set           (TotemPlPlaylist     *playlist,
                                      TotemPlPlaylistIter *iter,
                                      ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /* __TOTEM_PL_PLAYLIST_H__ */
