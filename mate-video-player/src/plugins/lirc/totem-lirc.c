/* 
 *  Copyright (C) 2002 James Willcox  <jwillcox@mate.org>
 *            (C) 2007 Jan Arne Petersen <jpetersen@jpetersen.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */


#include <config.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <string.h>

#include <unistd.h>
#include <lirc/lirc_client.h>

#include "totem-plugin.h"
#include "totem.h"

#define TOTEM_TYPE_LIRC_PLUGIN		(totem_lirc_plugin_get_type ())
#define TOTEM_LIRC_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_LIRC_PLUGIN, TotemLircPlugin))
#define TOTEM_LIRC_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_LIRC_PLUGIN, TotemLircPluginClass))
#define TOTEM_IS_LIRC_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_LIRC_PLUGIN))
#define TOTEM_IS_LIRC_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_LIRC_PLUGIN))
#define TOTEM_LIRC_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_LIRC_PLUGIN, TotemLircPluginClass))

typedef struct
{
	TotemPlugin   parent;

	GIOChannel *lirc_channel;
	struct lirc_config *lirc_config;

	TotemObject *totem;
} TotemLircPlugin;

typedef struct
{
	TotemPluginClass parent_class;
} TotemLircPluginClass;

/* strings that we recognize as commands from lirc */
#define TOTEM_IR_COMMAND_PLAY "play"
#define TOTEM_IR_COMMAND_PAUSE "pause"
#define TOTEM_IR_COMMAND_STOP "stop"
#define TOTEM_IR_COMMAND_NEXT "next"
#define TOTEM_IR_COMMAND_PREVIOUS "previous"
#define TOTEM_IR_COMMAND_SEEK_FORWARD "seek_forward"
#define TOTEM_IR_COMMAND_SEEK_BACKWARD "seek_backward"
#define TOTEM_IR_COMMAND_VOLUME_UP "volume_up"
#define TOTEM_IR_COMMAND_VOLUME_DOWN "volume_down"
#define TOTEM_IR_COMMAND_FULLSCREEN "fullscreen"
#define TOTEM_IR_COMMAND_QUIT "quit"
#define TOTEM_IR_COMMAND_UP "up"
#define TOTEM_IR_COMMAND_DOWN "down"
#define TOTEM_IR_COMMAND_LEFT "left"
#define TOTEM_IR_COMMAND_RIGHT "right"
#define TOTEM_IR_COMMAND_SELECT "select"
#define TOTEM_IR_COMMAND_MENU "menu"
#define TOTEM_IR_COMMAND_PLAYPAUSE "play_pause"
#define TOTEM_IR_COMMAND_ZOOM_UP "zoom_up"
#define TOTEM_IR_COMMAND_ZOOM_DOWN "zoom_down"
#define TOTEM_IR_COMMAND_EJECT "eject"
#define TOTEM_IR_COMMAND_PLAY_DVD "play_dvd"
#define TOTEM_IR_COMMAND_MUTE "mute"
#define TOTEM_IR_COMMAND_TOGGLE_ASPECT "toggle_aspect"

#define TOTEM_IR_SETTING "setting_"
#define TOTEM_IR_SETTING_TOGGLE_REPEAT "setting_repeat"
#define TOTEM_IR_SETTING_TOGGLE_SHUFFLE "setting_shuffle"

G_MODULE_EXPORT GType register_totem_plugin	(GTypeModule *module);
GType	totem_lirc_plugin_get_type		(void) G_GNUC_CONST;

static void totem_lirc_plugin_init		(TotemLircPlugin *plugin);
static void totem_lirc_plugin_finalize		(GObject *object);
static gboolean impl_activate			(TotemPlugin *plugin, TotemObject *totem, GError **error);
static void impl_deactivate			(TotemPlugin *plugin, TotemObject *totem);

TOTEM_PLUGIN_REGISTER(TotemLircPlugin, totem_lirc_plugin)

static void
totem_lirc_plugin_class_init (TotemLircPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	object_class->finalize = totem_lirc_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
totem_lirc_plugin_init (TotemLircPlugin *plugin)
{
}

static void
totem_lirc_plugin_finalize (GObject *object)
{
	G_OBJECT_CLASS (totem_lirc_plugin_parent_class)->finalize (object);
}

static char *
totem_lirc_get_url (const char *str)
{
	char *s;

	if (str == NULL)
		return NULL;
	s = strchr (str, ':');
	if (s == NULL)
		return NULL;
	return g_strdup (s + 1);
}

static TotemRemoteSetting
totem_lirc_to_setting (const gchar *str, char **url)
{
	if (strcmp (str, TOTEM_IR_SETTING_TOGGLE_REPEAT) == 0)
		return TOTEM_REMOTE_SETTING_REPEAT;
	else if (strcmp (str, TOTEM_IR_SETTING_TOGGLE_SHUFFLE) == 0)
		return TOTEM_REMOTE_SETTING_SHUFFLE;
	else
		return -1;
}

static TotemRemoteCommand
totem_lirc_to_command (const gchar *str, char **url)
{
	if (strcmp (str, TOTEM_IR_COMMAND_PLAY) == 0)
		return TOTEM_REMOTE_COMMAND_PLAY;
	else if (strcmp (str, TOTEM_IR_COMMAND_PAUSE) == 0)
		return TOTEM_REMOTE_COMMAND_PAUSE;
	else if (strcmp (str, TOTEM_IR_COMMAND_PLAYPAUSE) == 0)
		return TOTEM_REMOTE_COMMAND_PLAYPAUSE;
	else if (strcmp (str, TOTEM_IR_COMMAND_STOP) == 0)
		return TOTEM_REMOTE_COMMAND_STOP;
	else if (strcmp (str, TOTEM_IR_COMMAND_NEXT) == 0)
		return TOTEM_REMOTE_COMMAND_NEXT;
	else if (strcmp (str, TOTEM_IR_COMMAND_PREVIOUS) == 0)
		return TOTEM_REMOTE_COMMAND_PREVIOUS;
	else if (g_str_has_prefix (str, TOTEM_IR_COMMAND_SEEK_FORWARD) != FALSE) {
		*url = totem_lirc_get_url (str);
		return TOTEM_REMOTE_COMMAND_SEEK_FORWARD;
	} else if (g_str_has_prefix (str, TOTEM_IR_COMMAND_SEEK_BACKWARD) != FALSE) {
		*url = totem_lirc_get_url (str);
		return TOTEM_REMOTE_COMMAND_SEEK_BACKWARD;
	} else if (strcmp (str, TOTEM_IR_COMMAND_VOLUME_UP) == 0)
		return TOTEM_REMOTE_COMMAND_VOLUME_UP;
	else if (strcmp (str, TOTEM_IR_COMMAND_VOLUME_DOWN) == 0)
		return TOTEM_REMOTE_COMMAND_VOLUME_DOWN;
	else if (strcmp (str, TOTEM_IR_COMMAND_FULLSCREEN) == 0)
		return TOTEM_REMOTE_COMMAND_FULLSCREEN;
	else if (strcmp (str, TOTEM_IR_COMMAND_QUIT) == 0)
		return TOTEM_REMOTE_COMMAND_QUIT;
	else if (strcmp (str, TOTEM_IR_COMMAND_UP) == 0)
		return TOTEM_REMOTE_COMMAND_UP;
	else if (strcmp (str, TOTEM_IR_COMMAND_DOWN) == 0)
		return TOTEM_REMOTE_COMMAND_DOWN;
	else if (strcmp (str, TOTEM_IR_COMMAND_LEFT) == 0)
		return TOTEM_REMOTE_COMMAND_LEFT;
	else if (strcmp (str, TOTEM_IR_COMMAND_RIGHT) == 0)
		return TOTEM_REMOTE_COMMAND_RIGHT;
	else if (strcmp (str, TOTEM_IR_COMMAND_SELECT) == 0)
		return TOTEM_REMOTE_COMMAND_SELECT;
	else if (strcmp (str, TOTEM_IR_COMMAND_MENU) == 0)
		return TOTEM_REMOTE_COMMAND_DVD_MENU;
	else if (strcmp (str, TOTEM_IR_COMMAND_ZOOM_UP) == 0)
		return TOTEM_REMOTE_COMMAND_ZOOM_UP;
	else if (strcmp (str, TOTEM_IR_COMMAND_ZOOM_DOWN) == 0)
		return TOTEM_REMOTE_COMMAND_ZOOM_DOWN;
	else if (strcmp (str, TOTEM_IR_COMMAND_EJECT) == 0)
		return TOTEM_REMOTE_COMMAND_EJECT;
	else if (strcmp (str, TOTEM_IR_COMMAND_PLAY_DVD) == 0)
		return TOTEM_REMOTE_COMMAND_PLAY_DVD;
	else if (strcmp (str, TOTEM_IR_COMMAND_MUTE) == 0)
		return TOTEM_REMOTE_COMMAND_MUTE;
	else if (strcmp (str, TOTEM_IR_COMMAND_TOGGLE_ASPECT) == 0)
		return TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT;
	else
		return TOTEM_REMOTE_COMMAND_UNKNOWN;
}

static gboolean
totem_lirc_read_code (GIOChannel *source, GIOCondition condition, TotemLircPlugin *pi)
{
	char *code;
	char *str = NULL, *url = NULL;
	int ok;
	TotemRemoteCommand cmd;

	if (condition & (G_IO_ERR | G_IO_HUP)) {
		/* LIRC connection broken. */
		return FALSE;
	}

	/* this _could_ block, but it shouldn't */
	lirc_nextcode (&code);

	if (code == NULL) {
		/* the code was incomplete or something */
		return TRUE;
	}

	do {
		ok = lirc_code2char (pi->lirc_config, code, &str);

		if (ok != 0) {
			/* Couldn't convert lirc code to string. */
			break;
		}

		if (str == NULL) {
			/* there was no command associated with the code */
			break;
		}

		if (g_str_has_prefix (str, TOTEM_IR_SETTING) != FALSE) {
			TotemRemoteSetting setting;

			setting = totem_lirc_to_setting (str, &url);
			if (setting >= 0) {
				gboolean value;

				value = totem_action_remote_get_setting (pi->totem, setting);
				totem_action_remote_set_setting (pi->totem, setting, !value);
			}
		} else {
			cmd = totem_lirc_to_command (str, &url);
			totem_action_remote (pi->totem, cmd, url);
		}
		g_free (url);
	} while (TRUE);

	g_free (code);

	return TRUE;
}

static gboolean
impl_activate (TotemPlugin *plugin,
	       TotemObject *totem,
	       GError **error)
{
	TotemLircPlugin *pi = TOTEM_LIRC_PLUGIN (plugin);
	char *path;
	int fd;

	pi->totem = g_object_ref (totem);

	fd = lirc_init ("Totem", 0);
	if (fd < 0) {
		g_set_error_literal (error, TOTEM_PLUGIN_ERROR, TOTEM_PLUGIN_ERROR_ACTIVATION,
                                     _("Couldn't initialize lirc."));
		return FALSE;
	}

	/* Load the default Totem setup */
	path = totem_plugin_find_file (plugin, "totem_lirc_default");
	if (path == NULL || lirc_readconfig (path, &pi->lirc_config, NULL) == -1) {
		g_free (path);
		g_set_error_literal (error, TOTEM_PLUGIN_ERROR, TOTEM_PLUGIN_ERROR_ACTIVATION,
                                     _("Couldn't read lirc configuration."));
		close (fd);
		return FALSE;
	}
	g_free (path);

	/* Load the user config, doesn't matter if it's not there */
	lirc_readconfig (NULL, &pi->lirc_config, NULL);

	pi->lirc_channel = g_io_channel_unix_new (fd);
	g_io_channel_set_encoding (pi->lirc_channel, NULL, NULL);
	g_io_channel_set_buffered (pi->lirc_channel, FALSE);
	g_io_add_watch (pi->lirc_channel, G_IO_IN | G_IO_ERR | G_IO_HUP,
			(GIOFunc) totem_lirc_read_code, pi);

	return TRUE;
}

static void
impl_deactivate	(TotemPlugin *plugin,
		 TotemObject *totem)
{
	TotemLircPlugin *pi = TOTEM_LIRC_PLUGIN (plugin);
	GError *error = NULL;

	if (pi->lirc_channel) {
		g_io_channel_shutdown (pi->lirc_channel, FALSE, &error);
		if (error != NULL) {
			g_warning ("Couldn't destroy lirc connection: %s",
				   error->message);
			g_error_free (error);
		}
		pi->lirc_channel = NULL;
	}

	if (pi->lirc_config) {
		lirc_freeconfig (pi->lirc_config);
		pi->lirc_config = NULL;

		lirc_deinit ();
	}

	if (pi->totem) {
		g_object_unref (pi->totem);
		pi->totem = NULL;
	}
}
