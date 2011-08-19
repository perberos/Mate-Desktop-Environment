/* totem-playlist.h: Simple playlist dialog

   Copyright (C) 2002, 2003, 2004, 2005 Bastien Nocera <hadess@hadess.net>

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

   Author: Bastien Nocera <hadess@hadess.net>
 */

#ifndef TOTEM_PLAYLIST_H
#define TOTEM_PLAYLIST_H

#include <gtk/gtk.h>
#include <totem-pl-parser.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_PLAYLIST            (totem_playlist_get_type ())
#define TOTEM_PLAYLIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_PLAYLIST, TotemPlaylist))
#define TOTEM_PLAYLIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_PLAYLIST, TotemPlaylistClass))
#define TOTEM_IS_PLAYLIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_PLAYLIST))
#define TOTEM_IS_PLAYLIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_PLAYLIST))

typedef enum {
	TOTEM_PLAYLIST_STATUS_NONE,
	TOTEM_PLAYLIST_STATUS_PLAYING,
	TOTEM_PLAYLIST_STATUS_PAUSED
} TotemPlaylistStatus;

typedef enum {
	TOTEM_PLAYLIST_DIRECTION_NEXT,
	TOTEM_PLAYLIST_DIRECTION_PREVIOUS
} TotemPlaylistDirection;

typedef enum {
	TOTEM_PLAYLIST_DIALOG_SELECTED,
	TOTEM_PLAYLIST_DIALOG_PLAYING
} TotemPlaylistSelectDialog;


typedef struct TotemPlaylist	       TotemPlaylist;
typedef struct TotemPlaylistClass      TotemPlaylistClass;
typedef struct TotemPlaylistPrivate    TotemPlaylistPrivate;

typedef void (*TotemPlaylistForeachFunc) (TotemPlaylist *playlist,
					  const gchar   *filename,
					  const gchar   *uri,
					  gpointer       user_data);

struct TotemPlaylist {
	GtkVBox parent;
	TotemPlaylistPrivate *priv;
};

struct TotemPlaylistClass {
	GtkVBoxClass parent_class;

	void (*changed) (TotemPlaylist *playlist);
	void (*item_activated) (TotemPlaylist *playlist);
	void (*active_name_changed) (TotemPlaylist *playlist);
	void (*current_removed) (TotemPlaylist *playlist);
	void (*repeat_toggled) (TotemPlaylist *playlist, gboolean repeat);
	void (*shuffle_toggled) (TotemPlaylist *playlist, gboolean toggled);
	void (*subtitle_changed) (TotemPlaylist *playlist);
	void (*item_added) (TotemPlaylist *playlist, const gchar *filename, const gchar *uri);
	void (*item_removed) (TotemPlaylist *playlist, const gchar *filename, const gchar *uri);
};

GType    totem_playlist_get_type (void);
GtkWidget *totem_playlist_new      (void);

/* The application is responsible for checking that the mrl is correct
 * @display_name is if you have a preferred display string for the mrl,
 * NULL otherwise
 */
void totem_playlist_add_mrl (TotemPlaylist *playlist,
                             const char *mrl,
                             const char *display_name,
                             gboolean cursor,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data);
gboolean totem_playlist_add_mrl_finish (TotemPlaylist *playlist,
                                        GAsyncResult *result);
gboolean totem_playlist_add_mrl_sync (TotemPlaylist *playlist,
                                      const char *mrl,
                                      const char *display_name);

void totem_playlist_save_current_playlist (TotemPlaylist *playlist,
					   const char *output);
void totem_playlist_save_current_playlist_ext (TotemPlaylist *playlist,
					   const char *output, TotemPlParserType type);
void totem_playlist_select_subtitle_dialog (TotemPlaylist *playlist,
					    TotemPlaylistSelectDialog mode);

/* totem_playlist_clear doesn't emit the current_removed signal, even if it does
 * because the caller should know what to do after it's done with clearing */
gboolean   totem_playlist_clear (TotemPlaylist *playlist);
void       totem_playlist_clear_with_g_mount (TotemPlaylist *playlist,
					      GMount *mount);
char      *totem_playlist_get_current_mrl (TotemPlaylist *playlist,
					   char **subtitle);
char      *totem_playlist_get_current_title (TotemPlaylist *playlist,
					     gboolean *custom);
char      *totem_playlist_get_title (TotemPlaylist *playlist,
				     guint title_index);

gboolean   totem_playlist_set_title (TotemPlaylist *playlist,
				     const char *title,
				     gboolean force);
void       totem_playlist_set_current_subtitle (TotemPlaylist *playlist,
						const char *subtitle_uri);

#define    totem_playlist_has_direction(playlist, direction) (direction == TOTEM_PLAYLIST_DIRECTION_NEXT ? totem_playlist_has_next_mrl (playlist) : totem_playlist_has_previous_mrl (playlist))
gboolean   totem_playlist_has_previous_mrl (TotemPlaylist *playlist);
gboolean   totem_playlist_has_next_mrl (TotemPlaylist *playlist);

#define    totem_playlist_set_direction(playlist, direction) (direction == TOTEM_PLAYLIST_DIRECTION_NEXT ? totem_playlist_set_next (playlist) : totem_playlist_set_previous (playlist))
void       totem_playlist_set_previous (TotemPlaylist *playlist);
void       totem_playlist_set_next (TotemPlaylist *playlist);

gboolean   totem_playlist_get_repeat (TotemPlaylist *playlist);
void       totem_playlist_set_repeat (TotemPlaylist *playlist, gboolean repeat);

gboolean   totem_playlist_get_shuffle (TotemPlaylist *playlist);
void       totem_playlist_set_shuffle (TotemPlaylist *playlist,
				       gboolean shuffle);

gboolean   totem_playlist_set_playing (TotemPlaylist *playlist, TotemPlaylistStatus state);
TotemPlaylistStatus totem_playlist_get_playing (TotemPlaylist *playlist);

void       totem_playlist_set_at_start (TotemPlaylist *playlist);
void       totem_playlist_set_at_end (TotemPlaylist *playlist);

int        totem_playlist_get_current (TotemPlaylist *playlist);
int        totem_playlist_get_last (TotemPlaylist *playlist);
void       totem_playlist_set_current (TotemPlaylist *playlist, guint current_index);

void       totem_playlist_foreach (TotemPlaylist *playlist,
				   TotemPlaylistForeachFunc callback,
				   gpointer user_data);

G_END_DECLS

#endif /* TOTEM_PLAYLIST_H */
