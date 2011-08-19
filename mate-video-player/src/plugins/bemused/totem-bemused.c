/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

//FIXME we shouldn't use the video widget directly
#include "bacon-video-widget.h"
#include "totem-plugin.h"
#include "totem.h"

#define TOTEM_TYPE_BEMUSED_PLUGIN		(totem_bemused_plugin_get_type ())
#define TOTEM_BEMUSED_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_BEMUSED_PLUGIN, TotemBemusedPlugin))
#define TOTEM_BEMUSED_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_BEMUSED_PLUGIN, TotemBemusedPluginClass))
#define TOTEM_IS_BEMUSED_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_BEMUSED_PLUGIN))
#define TOTEM_IS_BEMUSED_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_BEMUSED_PLUGIN))
#define TOTEM_BEMUSED_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_BEMUSED_PLUGIN, TotemBemusedPluginClass))

typedef struct
{
	TotemPlugin   parent;

	TotemObject *totem;
	BaconVideoWidget *bvw;
	guint server_watch_id;
	guint client_watch_id;
	GIOChannel *server_iochan, *client_iochan;
	sdp_session_t *sdp_session;
} TotemBemusedPlugin;

typedef struct
{
	TotemPluginClass parent_class;
} TotemBemusedPluginClass;

G_MODULE_EXPORT GType register_totem_plugin		(GTypeModule *module);
GType	totem_bemused_plugin_get_type		(void) G_GNUC_CONST;

static void totem_bemused_plugin_init		(TotemBemusedPlugin *plugin);
static void totem_bemused_plugin_finalize		(GObject *object);
static gboolean impl_activate				(TotemPlugin *plugin, TotemObject *totem, GError **error);
static void impl_deactivate				(TotemPlugin *plugin, TotemObject *totem);

TOTEM_PLUGIN_REGISTER(TotemBemusedPlugin, totem_bemused_plugin)

/* Bluetooth functions */

#define CMD_IS(x) if (cmd != NULL && strcmp (cmd, x) == 0)

static void
flush_response (TotemBemusedPlugin *tp, GIOChannel *source)
{
	GError *error = NULL;

	if (g_io_channel_flush (source, &error) != G_IO_STATUS_NORMAL) {
		g_message ("error flushing: %s", error->message);
		g_error_free (error);
	}
}

static void
send_response_flush (TotemBemusedPlugin *tp, GIOChannel *source, const char *resp, gssize len, gboolean flush)
{
	GError *error = NULL;
	gsize written = 0;

	if (g_io_channel_write_chars (source, resp, len, &written, &error) != G_IO_STATUS_NORMAL) {
		g_message ("error writing response: %s", error->message);
		g_error_free (error);
	}

	if (flush != FALSE)
		flush_response (tp, source);

	if (len != written)
		g_message ("sent response: %d chars but len %d chars", (int) written, (int) len);
}

static void
send_response (TotemBemusedPlugin *tp, GIOChannel *source, const char *resp, gssize len)
{
	send_response_flush (tp, source, resp, len, TRUE);
}

static void
read_response (TotemBemusedPlugin *tp, GIOChannel *source, char *buf, gssize len)
{
	GError *error = NULL;
	gsize read;

	if (g_io_channel_read_chars (source, buf, len, &read, &error) != G_IO_STATUS_NORMAL) {
		g_message ("error reading response: %s", error->message);
		g_error_free (error);
	} else if (len != read) {
		g_message ("read %d chars but len %d chars", (int) read, (int) len);
	}
}

static char *
read_filename (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char namelenbuf[2], *filename;
	int namelen;

	/* Read length */
	read_response (tp, source, namelenbuf, 2);
	namelen = (namelenbuf[0] << 8) + namelenbuf[1];

	/* Read filename */
	filename = g_malloc0 (namelen + 1);
	read_response (tp, source, filename, namelen);
	filename[namelen] = '\0';

	//FIXME if outside the proper dirs, return NULL

	return filename;
}

static void
write_playlist (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf[11];
	int playlist_pos, playlist_len, i;

	playlist_pos = totem_get_playlist_pos (tp->totem);
	playlist_len = totem_get_playlist_length (tp->totem);

	strncpy(buf, "PLSTACK", strlen ("PLSTACK"));
	if (playlist_len == 0) {
		buf[7] = buf[8] = 0;
	} else {
		buf[7] = (playlist_pos >> 8) & 0xFF;
		buf[8] = playlist_pos & 0xFF;
	}
	buf[9] = '#';
	buf[10] = '\n';

	send_response (tp, source, buf, 11);

	for (i = 0; i < playlist_len; i++) {
		char *title;

		title = totem_get_title_at_playlist_pos (tp->totem, i);
		if (title == NULL) {
			/* Translators: the parameter is a number used to identify this playlist entry */
			title = g_strdup_printf (_("Untitled %d"), i);
		}
		g_message ("pushing entry %s", title);
		send_response_flush (tp, source, title, strlen (title), FALSE);
		g_free (title);
		send_response (tp, source, "\n", 1);
	}

	send_response (tp, source, "", 1);
}

static void
write_current_volume (TotemBemusedPlugin *tp, GIOChannel *source)
{
	double volume;
	char buf[8];

	strncpy(buf, "GVOLACK", strlen ("GVOLACK"));
	volume = bacon_video_widget_get_volume (tp->bvw);
	if (volume >= 1.0)
		buf[7] = (unsigned char) 255;
	else
		buf[7] = (unsigned char) (volume * 256);
	send_response (tp, source, buf, 8);
}

static void
set_volume (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf;
	double volume;

	read_response (tp, source, &buf, 1);
	volume = (double) buf / (double) 256;
	bacon_video_widget_set_volume (tp->bvw, volume);
}

static void
write_version (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf[10];

	strncpy (buf, "VERSACK", strlen ("VERSACK"));
	buf[7] = (char) 1;
	buf[8] = (char) 73;
	buf[9] = '\0';
	send_response (tp, source, buf, strlen (buf));
}

static void
set_setting (TotemBemusedPlugin *tp, GIOChannel *source, TotemRemoteSetting setting)
{
	char buf;

	read_response (tp, source, &buf, 1);
	totem_action_remote_set_setting (tp->totem, setting, buf != 0);
}

static void
seek_to_pos (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf[4];
	int time;

	read_response (tp, source, buf, 4);
	time = buf[0] << 24;
	time += buf[1] << 16;
	time += buf[2] << 8;
	time += buf[3];

	totem_action_seek_time (tp->totem, (gint64) time * 1000, FALSE);
}

static void
write_playlist_length (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf[2];
	int len;

	len = totem_get_playlist_length (tp->totem);

	buf[0] = (len >> 8) & 0xFF;
	buf[1] = len & 0xFF;

	send_response (tp, source, buf, 2);
}

static void
add_or_enqueue (TotemBemusedPlugin *tp, GIOChannel *source, TotemRemoteCommand cmd)
{
	char *filename;

	filename = read_filename (tp, source);
	if (filename == NULL)
		return;

	//FIXME mangle the filename
	g_message ("%s file '%s'", cmd == TOTEM_REMOTE_COMMAND_ENQUEUE ? "Enqueuing" : "Replacing with", filename);

	//FIXME do something with it
	g_free (filename);
}

static void
download_file (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char *filename;
	GMappedFile *file;

	filename = read_filename (tp, source);
	if (filename == NULL)
		return;

	/* Replace all the backslashes */
	filename = g_strdelimit (filename, "\\", '/');

	file = g_mapped_file_new (filename, FALSE, NULL);
	if (file == NULL)
		return;
	send_response_flush (tp, source, "DOWNACK", strlen ("DOWNACK"), FALSE);
	send_response (tp, source, g_mapped_file_get_contents (file), g_mapped_file_get_length (file));
	g_mapped_file_unref (file);
}

#define STUFF4(x) {						\
	char buf[4];						\
	buf[0] = x >> 24;					\
	buf[1] = (x >> 16) & 0xFF;				\
	buf[2] = (x >> 8) & 0xFF;				\
	buf[3] = x & 0xFF;					\
	send_response_flush (tp, source, buf, 4, FALSE);	\
}

static void
write_detailed_song_info (TotemBemusedPlugin *tp, GIOChannel *source)
{
	int bitrate, samplerate, numchans;

	//FIXME don't lie about this
	bitrate = 256;
	samplerate = 44;
	numchans = 2;

	send_response (tp, source, "DINFACK", strlen("DINFACK"));
	STUFF4(bitrate);
	STUFF4(samplerate);
	STUFF4(numchans);

	flush_response (tp, source);
}

static void
write_detailed_file_info (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char *filename;
	struct stat stat_buf;

	filename = read_filename (tp, source);
	if (filename == NULL)
		return;

	if (g_stat (filename, &stat_buf) < 0)
		return;

	send_response (tp, source, "FINFACK", strlen ("FINFACK"));
	STUFF4((int) stat_buf.st_size);

	flush_response (tp, source);
}

static void
write_song_info (TotemBemusedPlugin *tp, GIOChannel *source, gboolean send_null)
{
	char *title;
	char status;

	/* The reference Bemused Linux server says INF2ACK if we send the NULL
	 * the protocol docs say INFOACK */
	if (send_null == FALSE)
		send_response_flush (tp, source, "INFOACK", strlen ("INFOACK"), FALSE);
	else
		send_response_flush (tp, source, "INF2ACK", strlen ("INF2ACK"), FALSE);
	if (totem_is_playing (tp->totem) != FALSE)
		status = 1;
	else if (totem_is_paused (tp->totem) != FALSE)
		status = 2;
	else
		status = 0;

	send_response_flush (tp, source, &status, 1, FALSE);
	STUFF4((int) bacon_video_widget_get_stream_length (tp->bvw) / 1000);
	STUFF4((int) bacon_video_widget_get_current_time (tp->bvw) / 1000);
	status = totem_action_remote_get_setting (tp->totem, TOTEM_REMOTE_SETTING_SHUFFLE);
	send_response_flush (tp, source, &status, 1, FALSE);
	status = totem_action_remote_get_setting (tp->totem, TOTEM_REMOTE_SETTING_REPEAT);
	send_response_flush (tp, source, &status, 1, FALSE);

	title = totem_get_short_title (tp->totem);
	g_message ("written info for %s", title);
	if (title == NULL) {
		flush_response (tp, source);
		return;
	}

	if (send_null == FALSE)
		send_response (tp, source, title, strlen (title));
	else
		send_response (tp, source, title, strlen (title) + 1);
	g_free (title);
}

static void
set_playlist_at_pos (TotemBemusedPlugin *tp, GIOChannel *source)
{
	char buf[2];
	int index;

	read_response (tp, source, buf, 2);
	index = (buf[0] << 8) + buf[1];
	totem_action_set_playlist_index (tp->totem, index);
}

#if 0
static void
write_directory_listing_at_dir (TotemBemusedPlugin *tp, GIOChannel *source, const char *directory)
{
}
#endif
static void
write_directory_listing (TotemBemusedPlugin *tp, GIOChannel *source, gboolean at_root)
{
	char *filename;
	char buf[2];

	if (at_root == FALSE)
		filename = read_filename (tp, source);
	else
		filename = g_strdup ("\\");

	if (filename == NULL)
		goto endoflist;

	/* Replace all the backslashes */
	filename = g_strdelimit (filename, "\\", '/');

#if 0
	root = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_MOVIES), filename, NULL);
	write_directory_listing_at_dir (tp, source, root);
	g_free (root);

	root = g_build_filename (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC), filename, NULL);
	write_directory_listing_at_dir (tp, source, root);
	g_free (root);
#endif

	g_free (filename);

endoflist:
	/* Send end of list */
	buf[0] = 0xFF;
	buf[1] = 0x00;
	send_response (tp, source, buf, 2);
}

static void
handle_command (TotemBemusedPlugin *tp, GIOChannel *source, const char *cmd)
{
	g_message ("cmd: %s", cmd);

	CMD_IS("CHCK") {
		send_response (tp, source, "Y", 1);
	} else CMD_IS("DINF") {
		write_detailed_song_info (tp, source);
	} else CMD_IS("DLST") {
		write_directory_listing (tp, source, FALSE);
	} else CMD_IS("DOWN") {
		download_file (tp, source);
	} else CMD_IS("FADE") {
		//stop
	} else CMD_IS("FFWD") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_SEEK_FORWARD, NULL);
	} else CMD_IS("FINF") {
		write_detailed_file_info (tp, source);
	} else CMD_IS("GVOL") {
		write_current_volume (tp, source);
	} else CMD_IS("INFO") {
		write_song_info (tp, source, FALSE);
	} else CMD_IS("INF2") {
		write_song_info (tp, source, TRUE);
	} else CMD_IS("LADD") {
		add_or_enqueue (tp, source, TOTEM_REMOTE_COMMAND_ENQUEUE);
	} else CMD_IS("LIST") {
		write_directory_listing (tp, source, TRUE);
	} else CMD_IS("NEXT") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_NEXT, NULL);
	} else CMD_IS("PAUS") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_PAUSE, NULL);
	} else CMD_IS("PLAY") {
		add_or_enqueue (tp, source, TOTEM_REMOTE_COMMAND_REPLACE);
	} else CMD_IS("PLEN") {
		write_playlist_length (tp, source);
	} else CMD_IS("PLST") {
		write_playlist (tp, source);
	} else CMD_IS("PREV") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_PREVIOUS, NULL);
	} else CMD_IS("REPT") {
		set_setting (tp, source, TOTEM_REMOTE_SETTING_REPEAT);
	} else CMD_IS("RMAL") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_REPLACE, NULL);
	} else CMD_IS("RWND") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_SEEK_BACKWARD, NULL);
	} else CMD_IS("SHFL") {
		set_setting (tp, source, TOTEM_REMOTE_SETTING_SHUFFLE);
	} else CMD_IS("SEEK") {
		seek_to_pos (tp, source);
	} else CMD_IS("SHUT") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_QUIT, NULL);
	} else CMD_IS("STEN") {
		//stop at end of track
	} else CMD_IS("SLCT") {
		set_playlist_at_pos (tp, source);
	} else CMD_IS("STOP") {
		//stop
	} else CMD_IS("STRT") {
		totem_action_remote (tp->totem, TOTEM_REMOTE_COMMAND_PLAY, NULL);
	} else CMD_IS("VOLM") {
		set_volume (tp, source);
	} else CMD_IS("VERS") {
		write_version (tp, source);
	} else {
		g_warning ("command '%s' is unhandled", cmd);
	}
}

static gboolean
client_watch_func (GIOChannel *source, GIOCondition condition, gpointer data)
{
	TotemBemusedPlugin *tp = (TotemBemusedPlugin *) data;
	GIOStatus status;

	g_message ("client_watch_func");

	if (condition & G_IO_IN || condition & G_IO_PRI) {
		char buf[5];
		gsize read;

		status = G_IO_STATUS_NORMAL;

		while (status == G_IO_STATUS_NORMAL) {
			memset(buf, 0, 5);
			status = g_io_channel_read_chars (source, buf, 4, &read, NULL);

			if (status == G_IO_STATUS_NORMAL)
				handle_command (tp, source, buf);
		}

		if (status == G_IO_STATUS_AGAIN) {
			return TRUE;
		} else {
			g_message ("error reading from socket");
			return FALSE;
		}
	} else {
		g_message ("other condition");
		return FALSE;
	}

	return TRUE;
}

static gboolean
server_watch_func (GIOChannel *source, GIOCondition condition, gpointer data)
{
	TotemBemusedPlugin *tp = (TotemBemusedPlugin *) data;

	g_message ("server_watch_func");

	if (condition & G_IO_IN) {
		struct sockaddr_rc caddr;
		int client_fd;
		guint addrlen = sizeof(struct sockaddr_rc);

		g_message ("new connection");

		client_fd = accept (g_io_channel_unix_get_fd (source),
				    (struct sockaddr *)&caddr,
				    &addrlen);
		if (client_fd < 0) {
			g_message ("accpet failed");
		} else {
			bdaddr_t ba;

			g_message ("managed to accept it!");

			baswap (&ba, &caddr.rc_bdaddr);
			//FIXME check batostr(&ba) is our expected client
			g_message ("connected from %s", batostr(&ba));

			if (tp->bvw != NULL)
				g_object_unref (G_OBJECT (tp->bvw));
			tp->bvw = BACON_VIDEO_WIDGET (totem_get_video_widget (tp->totem));

			tp->client_iochan = g_io_channel_unix_new (client_fd);
			g_io_channel_set_encoding (tp->client_iochan, NULL, NULL);
			g_io_channel_set_buffered (tp->client_iochan, FALSE);
			g_io_channel_set_flags (tp->client_iochan,
						g_io_channel_get_flags (tp->client_iochan) | G_IO_FLAG_NONBLOCK,
						NULL);
			tp->client_watch_id = g_io_add_watch (tp->client_iochan,
						       G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
						       client_watch_func,
						       tp);
			g_io_channel_unref (tp->client_iochan);
		}
	}

	return TRUE;
}

/* SDP functions */

//FIXME is this right?
static guint BEMUSED_SVC_UUID[] = { 0x95C4, 0x5DF4, 0x5E73, 0x03C7 };
#define BEMUSED_SVC_NAME N_("Totem Bemused Server")
//FIXME version
#define BEMUSED_SVC_DESC N_("Totem Bemused Server version 1.0")
#define BEMUSED_SVC_PROV "http://www.mate.org/projects/totem/"

static sdp_session_t *
sdp_svc_add_spp(u_int8_t port,
		const char *name,
		const char *dsc,
		const char *prov,
		const uint32_t uuid[])
{
	uuid_t	root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid, svc_class_uuid;
	sdp_list_t *l2cap_list = 0, *rfcomm_list = 0, *root_list = 0, *proto_list = 0,
		   *access_proto_list = 0, *svc_class_list = 0, *profile_list = 0;
	sdp_data_t *channel = 0;
	sdp_profile_desc_t profile;
	sdp_record_t record;
	sdp_session_t *session = 0;

	/* Set the general service ID */
	sdp_uuid128_create (&svc_uuid, &uuid);
	memset(&record, 0, sizeof(sdp_record_t));
	sdp_set_service_id (&record, svc_uuid);

	/* Set the service class */
	sdp_uuid16_create (&svc_class_uuid, SERIAL_PORT_SVCLASS_ID);
	svc_class_list = sdp_list_append (0, &svc_class_uuid);
	sdp_set_service_classes (&record, svc_class_list);

	/* Set the Bluetooth profile information */
	memset (&profile, 0, sizeof(sdp_profile_desc_t));
	sdp_uuid16_create (&profile.uuid, SERIAL_PORT_PROFILE_ID);
	profile.version = 0x0100;
	profile_list = sdp_list_append (0, &profile);
	sdp_set_profile_descs (&record, profile_list);

	/* Make the service record publicly browsable */
	sdp_uuid16_create (&root_uuid, PUBLIC_BROWSE_GROUP);
	root_list = sdp_list_append (0, &root_uuid);
	sdp_set_browse_groups (&record, root_list);

	/* Set l2cap information */
	sdp_uuid16_create (&l2cap_uuid, L2CAP_UUID);
	l2cap_list = sdp_list_append (0, &l2cap_uuid);
	proto_list = sdp_list_append (0, l2cap_list);

	/* Register the RFCOMM channel for RFCOMM sockets */
	sdp_uuid16_create (&rfcomm_uuid, RFCOMM_UUID);
	channel = sdp_data_alloc (SDP_UINT8, &port);
	rfcomm_list = sdp_list_append (0, &rfcomm_uuid);
	sdp_list_append (rfcomm_list, channel);
	sdp_list_append (proto_list, rfcomm_list);
	access_proto_list = sdp_list_append (0, proto_list);
	sdp_set_access_protos (&record, access_proto_list);

	/* Set the name, provider, and description */
	sdp_set_info_attr (&record, name, prov, dsc);

	/* Connect to the local SDP server, register the service record */
	session = sdp_connect (BDADDR_ANY, BDADDR_LOCAL, 0);
	sdp_record_register (session, &record, 0);

	/* Cleanup */
	sdp_data_free (channel);
	sdp_list_free (l2cap_list, 0);
	sdp_list_free (rfcomm_list, 0);
	sdp_list_free (root_list, 0);
	sdp_list_free (access_proto_list, 0);

	return session;
}

static void
sdp_svc_del (sdp_session_t *session)
{
	sdp_close (session);
}

/* Object functions */

static void
totem_bemused_plugin_class_init (TotemBemusedPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	object_class->finalize = totem_bemused_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
totem_bemused_plugin_init (TotemBemusedPlugin *plugin)
{
}

static void
totem_bemused_plugin_finalize (GObject *object)
{
	G_OBJECT_CLASS (totem_bemused_plugin_parent_class)->finalize (object);
}

static gboolean
impl_activate (TotemPlugin *plugin,
	       TotemObject *totem,
	       GError **error)
{
	TotemBemusedPlugin *tp = TOTEM_BEMUSED_PLUGIN (plugin);
	int fd, channel;
	struct sockaddr_rc addr;

	tp->totem = totem;

	fd = socket (PF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (fd < 0) {
		g_message ("couldn't create socket");
		//FIXME set error
		return FALSE;
	}

	g_message ("socket created");

	channel = 1;
	tp->sdp_session = sdp_svc_add_spp (channel,
					   BEMUSED_SVC_NAME,
					   BEMUSED_SVC_DESC,
					   BEMUSED_SVC_PROV,
					   BEMUSED_SVC_UUID);
	if (tp->sdp_session == NULL) {
		close (fd);
		g_message ("registering service failed");
		return FALSE;
	}

	addr.rc_family = AF_BLUETOOTH;
	bacpy(&addr.rc_bdaddr, BDADDR_ANY);
	addr.rc_channel = (guint8) channel;

	if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		//FIXME
		sdp_svc_del (tp->sdp_session);
		g_message ("couldn't bind");
		return FALSE;
	}

	g_message ("bind launched");

	if (listen (fd, 10) < 0) {
		//FIXME
		g_message ("couldn't listen");
		sdp_svc_del (tp->sdp_session);
		return FALSE;
	}

	g_message ("listen launched");

	tp->server_iochan = g_io_channel_unix_new (fd);
	g_io_channel_set_encoding (tp->server_iochan, NULL, NULL);
	tp->server_watch_id = g_io_add_watch (tp->server_iochan,
					      G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
					      server_watch_func,
					      plugin);
	g_io_channel_unref (tp->server_iochan);

	g_message ("io chan set");

	//FIXME

	return TRUE;
}

static void
impl_deactivate	(TotemPlugin *plugin,
		 TotemObject *totem)
{
	totem_remove_sidebar_page (totem, "sidebar-test");
	g_message ("Just removed a test sidebar");
}

