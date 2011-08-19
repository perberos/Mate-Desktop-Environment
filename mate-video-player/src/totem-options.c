/* totem-options.c

   Copyright (C) 2004 Bastien Nocera

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

#include "config.h"

#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>

#include "totem-options.h"
#include "totem-uri.h"
#include "bacon-video-widget.h"
#include "totem-private.h"

TotemCmdLineOptions optionstate;	/* Decoded command line options */

G_GNUC_NORETURN static gboolean
option_version_cb (const gchar *option_name,
	           const gchar *value,
	           gpointer     data,
	           GError     **error)
{
	g_print ("%s %s\n", PACKAGE, VERSION);

	exit (0);
}
 
const GOptionEntry all_options[] = {
	{"debug", '\0', 0, G_OPTION_ARG_NONE, &optionstate.debug, N_("Enable debug"), NULL},
	{"play-pause", '\0', 0, G_OPTION_ARG_NONE, &optionstate.playpause, N_("Play/Pause"), NULL},
	{"play", '\0', 0, G_OPTION_ARG_NONE, &optionstate.play, N_("Play"), NULL},
	{"pause", '\0', 0, G_OPTION_ARG_NONE, &optionstate.pause, N_("Pause"), NULL},
	{"next", '\0', 0, G_OPTION_ARG_NONE, &optionstate.next, N_("Next"), NULL},
	{"previous", '\0', 0, G_OPTION_ARG_NONE, &optionstate.previous, N_("Previous"), NULL},
	{"seek-fwd", '\0', 0, G_OPTION_ARG_NONE, &optionstate.seekfwd, N_("Seek Forwards"), NULL},
	{"seek-bwd", '\0', 0, G_OPTION_ARG_NONE, &optionstate.seekbwd, N_("Seek Backwards"), NULL},
	{"volume-up", '\0', 0, G_OPTION_ARG_NONE, &optionstate.volumeup, N_("Volume Up"), NULL},
	{"volume-down", '\0', 0, G_OPTION_ARG_NONE, &optionstate.volumedown, N_("Volume Down"), NULL},
	{"mute", '\0', 0, G_OPTION_ARG_NONE, &optionstate.mute, N_("Mute sound"), NULL},
	{"fullscreen", '\0', 0, G_OPTION_ARG_NONE, &optionstate.fullscreen, N_("Toggle Fullscreen"), NULL},
	{"toggle-controls", '\0', 0, G_OPTION_ARG_NONE, &optionstate.togglecontrols, N_("Show/Hide Controls"), NULL},
	{"quit", '\0', 0, G_OPTION_ARG_NONE, &optionstate.quit, N_("Quit"), NULL},
	{"enqueue", '\0', 0, G_OPTION_ARG_NONE, &optionstate.enqueue, N_("Enqueue"), NULL},
	{"replace", '\0', 0, G_OPTION_ARG_NONE, &optionstate.replace, N_("Replace"), NULL},
	{"no-existing-session", '\0', 0, G_OPTION_ARG_NONE, &optionstate.notconnectexistingsession, N_("Don't connect to an already-running instance"), NULL},
	{"seek", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT64, &optionstate.seek, N_("Seek"), NULL},
	/* Translators: help for a (hidden) command line option to specify a playlist entry to start playing on load */
	{"playlist-idx", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_DOUBLE, &optionstate.playlistidx, N_("Playlist index"), NULL},
	{ "version", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK, option_version_cb, NULL, NULL },
	{G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &optionstate.filenames, N_("Movies to play"), NULL},
	{NULL} /* end the list */
};

void
totem_options_process_late (Totem *totem, const TotemCmdLineOptions *options)
{
	if (options->togglecontrols) 
		totem_action_toggle_controls (totem);

	/* Handle --playlist-idx */
	totem->index = options->playlistidx;

	/* Handle --seek */
	totem->seek_to_start = options->seek;
}

void
totem_options_register_remote_commands (Totem *totem)
{
	GEnumClass *klass;
	guint i;

	klass = (GEnumClass *) g_type_class_ref (TOTEM_TYPE_REMOTE_COMMAND);
	for (i = TOTEM_REMOTE_COMMAND_UNKNOWN + 1; i < klass->n_values; i++) {
		GEnumValue *val;
		val = g_enum_get_value (klass, i);
		unique_app_add_command (totem->app, val->value_name, i);
	}
	g_type_class_unref (klass);
}

void
totem_options_process_early (Totem *totem, const TotemCmdLineOptions* options)
{
	if (options->quit) {
		/* If --quit is one of the commands, just quit */
		gdk_notify_startup_complete ();
		exit (0);
	}

	mateconf_client_set_bool (totem->gc, MATECONF_PREFIX"/debug",
			       options->debug, NULL);
}

void
totem_options_process_for_server (UniqueApp *app,
				  const TotemCmdLineOptions* options)
{
	GList *commands, *l;
	int default_action, i;

	commands = NULL;
	default_action = TOTEM_REMOTE_COMMAND_REPLACE;

	/* Are we quitting ? */
	if (options->quit) {
		unique_app_send_message (app, TOTEM_REMOTE_COMMAND_QUIT, NULL);
		return;
	}

	/* Then handle the things that modify the playlist */
	if (options->replace && options->enqueue) {
		/* FIXME translate that */
		g_warning ("Can't enqueue and replace at the same time");
	} else if (options->replace) {
		default_action = TOTEM_REMOTE_COMMAND_REPLACE;
	} else if (options->enqueue) {
		default_action = TOTEM_REMOTE_COMMAND_ENQUEUE;
	}

	/* Send the files to enqueue */
	for (i = 0; options->filenames && options->filenames[i] != NULL; i++) {
		UniqueMessageData *data;
		char *full_path;

		data = unique_message_data_new ();
		full_path = totem_create_full_path (options->filenames[i]);
		unique_message_data_set_text (data, full_path ? full_path : options->filenames[i], -1);
		full_path = totem_create_full_path (options->filenames[i]);

		unique_app_send_message (app, default_action, data);

		/* Even if the default action is replace, we only want to replace with the
		   first file.  After that, we enqueue. */
		default_action = TOTEM_REMOTE_COMMAND_ENQUEUE;
		unique_message_data_free (data);
		g_free (full_path);
	}

	if (options->playpause) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_PLAYPAUSE));
	}

	if (options->play) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_PLAY));
	}

	if (options->pause) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_PAUSE));
	}

	if (options->next) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_NEXT));
	}

	if (options->previous) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_PREVIOUS));
	}

	if (options->seekfwd) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_SEEK_FORWARD));
	}

	if (options->seekbwd) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_SEEK_BACKWARD));
	}

	if (options->volumeup) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_VOLUME_UP));
	}

	if (options->volumedown) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_VOLUME_DOWN));
	}

	if (options->mute) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_MUTE));
	}

	if (options->fullscreen) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_FULLSCREEN));
	}

	if (options->togglecontrols) {
		commands = g_list_append (commands, GINT_TO_POINTER
					  (TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS));
	}

	/* No commands, no files, show ourselves */
	if (commands == NULL && options->filenames == NULL) {
		unique_app_send_message (app, TOTEM_REMOTE_COMMAND_SHOW, NULL);
		return;
	}

	/* Send commands */
	for (l = commands; l != NULL; l = l->next) {
		int command = GPOINTER_TO_INT (l->data);
		unique_app_send_message (app, command, NULL);
	}
	g_list_free (commands);
}

