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

/**
 * SECTION:totem-pl-playlist
 * @short_description: playlist object
 * @stability: Stable
 * @include: totem-pl-playlist.h
 *
 * #TotemPlPlaylist represents a playlist, provides API to either navigate through
 * the playlist elements, or perform additions or modifications. See also
 * totem_pl_parser_save().
 *
 **/

/**
 * SECTION:totem-pl-playlist-iter
 * @short_description: playlist manipulation object
 * @stability: Stable
 * @totem-pl-playlist.h
 *
 * #TotemPlPlaylistIter refers to an element in a playlist and is designed
 * to bridge between application provided playlist widgets or objects,
 * and #TotemPlParser's saving code.
 *
 **/

#include "totem-pl-playlist.h"

typedef struct TotemPlPlaylistPrivate TotemPlPlaylistPrivate;

struct TotemPlPlaylistPrivate {
        GList *items;
};

#define TOTEM_PL_PLAYLIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TOTEM_TYPE_PL_PLAYLIST, TotemPlPlaylistPrivate))

static void totem_pl_playlist_finalize (GObject *object);


G_DEFINE_TYPE (TotemPlPlaylist, totem_pl_playlist, G_TYPE_OBJECT)


static void
totem_pl_playlist_class_init (TotemPlPlaylistClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = totem_pl_playlist_finalize;

        g_type_class_add_private (klass, sizeof (TotemPlPlaylistPrivate));
}

static void
totem_pl_playlist_init (TotemPlPlaylist *playlist)
{
}

static void
totem_pl_playlist_finalize (GObject *object)
{
        TotemPlPlaylistPrivate *priv;

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (object);

        g_list_foreach (priv->items, (GFunc) g_hash_table_destroy, NULL);
        g_list_free (priv->items);

        G_OBJECT_CLASS (totem_pl_playlist_parent_class)->finalize (object);
}

/**
 * totem_pl_playlist_new:
 *
 * Creates a new #TotemPlPlaylist object.
 *
 * Returns: The newly created #TotemPlPlaylist
 **/
TotemPlPlaylist *
totem_pl_playlist_new (void)
{
        return g_object_new (TOTEM_TYPE_PL_PLAYLIST, NULL);
}

/**
 * totem_pl_playlist_size:
 * @playlist: a #TotemPlPlaylist
 *
 * Returns the number of elements in @playlist.
 *
 * Returns: The number of elements
 **/
guint
totem_pl_playlist_size (TotemPlPlaylist *playlist)
{
        TotemPlPlaylistPrivate *priv;

        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), 0);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        return g_list_length (priv->items);
}

static GHashTable *
create_playlist_item (void)
{
        return g_hash_table_new_full (g_str_hash,
                                      g_str_equal,
                                      (GDestroyNotify) g_free,
                                      (GDestroyNotify) g_free);
}

/**
 * totem_pl_playlist_prepend:
 * @playlist: a #TotemPlPlaylist
 * @iter: an unset #TotemPlPlaylistIter for returning the location
 *
 * Prepends a new empty element to @playlist, and modifies @iter so
 * it points to it. To fill in values, you need to call
 * totem_pl_playlist_set() or totem_pl_playlist_set_value().
 **/
void
totem_pl_playlist_prepend (TotemPlPlaylist     *playlist,
                           TotemPlPlaylistIter *iter)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (iter != NULL);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        item = create_playlist_item ();
        priv->items = g_list_prepend (priv->items, item);

        iter->data1 = playlist;
        iter->data2 = priv->items;
}

/**
 * totem_pl_playlist_append:
 * @playlist: a #TotemPlPlaylist
 * @iter: an unset #TotemPlPlaylistIter for returning the location
 *
 * Appends a new empty element to @playlist, and modifies @iter so
 * it points to it. To fill in values, you need to call
 * totem_pl_playlist_set() or totem_pl_playlist_set_value().
 **/
void
totem_pl_playlist_append (TotemPlPlaylist     *playlist,
                          TotemPlPlaylistIter *iter)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item;
        GList *list_item;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (iter != NULL);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        item = create_playlist_item ();

        list_item = g_list_alloc ();
        list_item->data = item;

        priv->items = g_list_concat (priv->items, list_item);

        iter->data1 = playlist;
        iter->data2 = list_item;
}

/**
 * totem_pl_playlist_insert:
 * @playlist: a #TotemPlPlaylist
 * @position: position in the playlist
 * @iter: an unset #TotemPlPlaylistIter for returning the location
 *
 * Inserts a new empty element to @playlist at @position, and modifies
 * @iter so it points to it. To fill in values, you need to call
 * totem_pl_playlist_set() or totem_pl_playlist_set_value().
 *
 * @position may be minor than 0 to prepend elements, or bigger than
 * the current @playlist size to append elements.
 **/
void
totem_pl_playlist_insert (TotemPlPlaylist     *playlist,
                          gint                 position,
                          TotemPlPlaylistIter *iter)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (iter != NULL);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        item = create_playlist_item ();
        priv->items = g_list_insert (priv->items, item, position);

        iter->data1 = playlist;
        iter->data2 = g_list_find (priv->items, item);
}

static gboolean
check_iter (TotemPlPlaylist     *playlist,
            TotemPlPlaylistIter *iter)
{
        TotemPlPlaylistPrivate *priv;

        if (!iter) {
                return FALSE;
        }

        if (iter->data1 != playlist) {
                return FALSE;
        }

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        if (g_list_position (priv->items, iter->data2) == -1) {
                return FALSE;
        }

        return TRUE;
}

/**
 * totem_pl_playlist_iter_first:
 * @playlist: a #TotemPlPlaylist
 * @iter: an unset #TotemPlPlaylistIter for returning the location
 *
 * Modifies @iter so it points to the first element in @playlist.
 *
 * Returns: %TRUE if there is such first element.
 **/
gboolean
totem_pl_playlist_iter_first (TotemPlPlaylist     *playlist,
                              TotemPlPlaylistIter *iter)
{
        TotemPlPlaylistPrivate *priv;

        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), FALSE);
        g_return_val_if_fail (iter != NULL, FALSE);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);

        if (!priv->items) {
                /* Empty playlist */
                return FALSE;
        }

        iter->data1 = playlist;
        iter->data2 = priv->items;

        return TRUE;
}

/**
 * totem_pl_playlist_iter_next:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 *
 * Modifies @iter so it points to the next element it previously
 * pointed to. This function will return %FALSE if there was no
 * next element, or @iter didn't actually point to any element
 * in @playlist.
 *
 * Returns: %TRUE if there was next element.
 **/
gboolean
totem_pl_playlist_iter_next (TotemPlPlaylist     *playlist,
                             TotemPlPlaylistIter *iter)
{
        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), FALSE);
        g_return_val_if_fail (check_iter (playlist, iter), FALSE);

        iter->data2 = ((GList *) iter->data2)->next;

        return (iter->data2 != NULL);
}

/**
 * totem_pl_playlist_iter_prev:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 *
 * Modifies @iter so it points to the previous element it previously
 * pointed to. This function will return %FALSE if there was no
 * previous element, or @iter didn't actually point to any element
 * in @playlist.
 *
 * Returns: %TRUE if there was previous element.
 **/
gboolean
totem_pl_playlist_iter_prev (TotemPlPlaylist     *playlist,
                             TotemPlPlaylistIter *iter)
{
        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), FALSE);
        g_return_val_if_fail (check_iter (playlist, iter), FALSE);

        iter->data2 = ((GList *) iter->data2)->prev;

        return (iter->data2 != NULL);
}

/**
 * totem_pl_playlist_get_value:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @key: data key
 * @value: an empty #GValue to set
 *
 * Gets the value for @key (Such as %TOTEM_PL_PARSER_FIELD_URI) in
 * the playlist item pointed by @iter.
 *
 * Returns: %TRUE if @iter contains data for @key.
 **/
gboolean
totem_pl_playlist_get_value (TotemPlPlaylist     *playlist,
                             TotemPlPlaylistIter *iter,
                             const gchar         *key,
                             GValue              *value)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item_data;
        gchar *str;

        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), FALSE);
        g_return_val_if_fail (check_iter (playlist, iter), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);
        g_return_val_if_fail (value != NULL, FALSE);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);
        item_data = ((GList *) iter->data2)->data;

        str = g_hash_table_lookup (item_data, key);

        if (!str) {
                return FALSE;
        }

        g_value_init (value, G_TYPE_STRING);
        g_value_set_string (value, str);

        return TRUE;
}

/**
 * totem_pl_playlist_get_valist:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @args: a va_list
 *
 * See totem_pl_playlist_get(), this function takes a va_list.
 **/
void
totem_pl_playlist_get_valist (TotemPlPlaylist     *playlist,
                              TotemPlPlaylistIter *iter,
                              va_list              args)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item_data;
        gchar *key, **value;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (check_iter (playlist, iter));

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);
        item_data = ((GList *) iter->data2)->data;

        key = va_arg (args, gchar *);

        while (key) {
                value = va_arg (args, gchar **);

                if (value) {
                        gchar *str;

                        str = g_hash_table_lookup (item_data, key);
                        *value = g_strdup (str);
                }

                key = va_arg (args, gchar *);
        }
}

/**
 * totem_pl_playlist_get:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @...: pairs of key/return location for value, terminated by %NULL
 *
 * Gets the value for one or more keys from the element pointed
 * by @iter.
 **/
void
totem_pl_playlist_get (TotemPlPlaylist     *playlist,
                       TotemPlPlaylistIter *iter,
                       ...)
{
        va_list args;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (check_iter (playlist, iter));

        va_start (args, iter);
        totem_pl_playlist_get_valist (playlist, iter, args);
        va_end (args);
}

/**
 * totem_pl_playlist_set_value:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @key: key to set the value for
 * @value: #GValue containing the key value
 *
 * Sets the value for @key in the element pointed by @iter.
 *
 * Returns: %TRUE if the value could be stored in @playlist
 **/
gboolean
totem_pl_playlist_set_value (TotemPlPlaylist     *playlist,
                             TotemPlPlaylistIter *iter,
                             const gchar         *key,
                             GValue              *value)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item_data;
        gchar *str;

        g_return_val_if_fail (TOTEM_IS_PL_PLAYLIST (playlist), FALSE);
        g_return_val_if_fail (check_iter (playlist, iter), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);
        g_return_val_if_fail (value != NULL, FALSE);

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);
        item_data = ((GList *) iter->data2)->data;

        if (G_VALUE_TYPE (value) == G_TYPE_STRING) {
                str = g_value_dup_string (value);
        } else {
                GValue str_value = { 0 };

                g_value_init (&str_value, G_TYPE_STRING);

                if (g_value_transform (value, &str_value)) {
                        str = g_value_dup_string (&str_value);
                } else {
                        str = NULL;
                }

                g_value_unset (&str_value);
        }

        if (!str) {
                g_critical ("Value could not be transformed to string");
                return FALSE;
        }

        g_hash_table_replace (item_data, g_strdup (key), str);

        return TRUE;
}

/**
 * totem_pl_playlist_set_valist:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @args: a va_list
 *
 * See totem_pl_playlist_set(), this function takes a va_list.
 **/
void
totem_pl_playlist_set_valist (TotemPlPlaylist     *playlist,
                              TotemPlPlaylistIter *iter,
                              va_list              args)
{
        TotemPlPlaylistPrivate *priv;
        GHashTable *item_data;
        gchar *key, *value;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (check_iter (playlist, iter));

        priv = TOTEM_PL_PLAYLIST_GET_PRIVATE (playlist);
        item_data = ((GList *) iter->data2)->data;

        key = va_arg (args, gchar *);

        while (key) {
                value = va_arg (args, gchar *);

                g_hash_table_replace (item_data,
                                      g_strdup (key),
                                      g_strdup (value));

                key = va_arg (args, gchar *);
        }
}

/**
 * totem_pl_playlist_set:
 * @playlist: a #TotemPlPlaylist
 * @iter: a #TotemPlPlaylistIter pointing to some item in @playlist
 * @...: key/value string pairs, terminated with %NULL
 *
 * Sets the value for one or several keys in the element pointed
 * by @iter.
 **/
void
totem_pl_playlist_set (TotemPlPlaylist     *playlist,
                       TotemPlPlaylistIter *iter,
                       ...)
{
        va_list args;

        g_return_if_fail (TOTEM_IS_PL_PLAYLIST (playlist));
        g_return_if_fail (check_iter (playlist, iter));

        va_start (args, iter);
        totem_pl_playlist_set_valist (playlist, iter, args);
        va_end (args);
}
