/*
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

#ifndef TOTEM_CMML_PARSER_H_
#define TOTEM_CMML_PARSER_H_

#include <glib.h>
#include <libxml/xmlreader.h>

/**
 * TotemCmmlClip:
 * @title: clip title
 * @desc: clip description
 * @time_start: start time of clip in msecs
 * @pixbuf: clip thumbnail
 *
 *  Structure to handle clip data.
 **/
typedef struct {
	gchar		*title;
	gchar		*desc;
	gint64		time_start;
	GdkPixbuf	*pixbuf;
} TotemCmmlClip;

/**
 * TotemCmmlAsyncData:
 * @file: file to read
 * @list: list to store chapters to read in/write
 * @final: function to call at final, %NULL is allowed
 * @user_data: user data passed to @final callback
 * @buf: buffer for writing
 * @error: last error message string, or %NULL if not any
 * @successful: whether operation was successful or not
 * @is_exists: whether @file exists or not
 * @from_dialog: whether read operation was started from open dialog
 * @cancellable: object to cancel operation, %NULL is allowed
 *
 * Structure to handle data for async reading/writing clip data.
 **/
typedef struct {
	gchar		*file;
	GList		*list;
	GFunc		final;
	gpointer	user_data;
	gchar		*buf;
	gchar		*error;
	gboolean	successful;
	gboolean	is_exists;
	gboolean	from_dialog;
	GCancellable	*cancellable;
} TotemCmmlAsyncData;

gchar *	totem_cmml_convert_msecs_to_str (gint64 time_msecs);
TotemCmmlClip * totem_cmml_clip_new (const gchar *title, const gchar *desc, gint64 start, GdkPixbuf *pixbuf);
void totem_cmml_clip_free (TotemCmmlClip *clip);
TotemCmmlClip * totem_cmml_clip_copy (TotemCmmlClip *clip);
gint totem_cmml_read_file_async (TotemCmmlAsyncData *data);
gint totem_cmml_write_file_async (TotemCmmlAsyncData *data);

#endif /* TOTEM_CMML_PARSER_H_ */
