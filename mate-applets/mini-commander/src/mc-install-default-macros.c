/*
 * mc-install-default-macros.c:
 * Copyright (C) 2002 Sun Microsystems Inc.
 *
 * Authors: Mark McLoughlin <mark@skynet.ie>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This reads the macro patterns and commands lists
 * schemas and sets the default values to be our
 * default list of macros.
 *
 * This is easier than trying to have this stuff
 * in the .schemas file - you would believe the
 * kind of escaping and hackery needed to get
 * it into the .schemas, and even then its impossible
 * to tell which pattern is for which macro ...
 */

/* This is to have access to mateconf_engine_get_local() */
#define MATECONF_ENABLE_INTERNALS 1

#include <config.h>
#include <stdio.h>

#include <glib/gi18n.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf-engine.h>

/* Okay, I'm VERY bold. I deserve to be spanked.
 *
 * But this isn't installed, its only used at build time,
 * so I don't feel too bad. If these disappear from the
 * ABI or their signature changes, the build will break
 * and someone will have to figure out how to fix it.
 * Too bad, whoever you are :/
 */
void         mateconf_shutdown_daemon  (GError **err);

#include "mc-default-macros.h"

#define MC_PATTERNS_SHEMA_KEY "/schemas/apps/mini-commander-global/macro_patterns"
#define MC_COMMANDS_SHEMA_KEY "/schemas/apps/mini-commander-global/macro_commands"

/*
 * Keep these old schemas around in order to keep
 * old configurations from breaking. See bug #91194
 */
#define MC_DEPRECATED_PATTERNS_SHEMA_KEY "/schemas/apps/mini-commander/prefs/macro_patterns"
#define MC_DEPRECATED_COMMANDS_SHEMA_KEY "/schemas/apps/mini-commander/prefs/macro_commands"

static void
install_default_macros_list (MateConfClient *client,
			     const char  *key,
			     int          offset)
{
	MateConfSchema *schema;
	MateConfValue  *value;
	GSList      *list = NULL;
	GError      *error;
	int          i;

	error = NULL;
	schema = mateconf_client_get_schema (client, key, &error);
	if (error) {
		g_warning (_("Cannot get schema for %s: %s"), key, error->message);
		g_error_free (error);
		return;
	}

	/* mateconf has started to return NULL with no error set. */
	g_return_if_fail (schema != NULL);

	/* Some sanity checks */
	g_assert (mateconf_schema_get_type (schema) == MATECONF_VALUE_LIST);
	g_assert (mateconf_schema_get_list_type (schema) == MATECONF_VALUE_STRING);

	value = mateconf_value_new (MATECONF_VALUE_LIST);
	mateconf_value_set_list_type (value, MATECONF_VALUE_STRING);

	for (i = 0; i < G_N_ELEMENTS (mc_default_macros); i++)
		list = g_slist_prepend (list,
					mateconf_value_new_from_string (MATECONF_VALUE_STRING,
								     G_STRUCT_MEMBER (char *, &mc_default_macros [i], offset),
								     NULL));
	list = g_slist_reverse (list);

	mateconf_value_set_list_nocopy (value, list);
	list = NULL;

	mateconf_schema_set_default_value_nocopy (schema, value);
	value = NULL;

	error = NULL;
	mateconf_client_set_schema (client, key, schema, &error);
	if (error) {
		g_warning (_("Cannot set schema for %s: %s"), key, error->message);
		g_error_free (error);
	}

	mateconf_schema_free (schema);

	printf (_("Set default list value for %s\n"), key);
}

int
main (int argc, char **argv)
{
	MateConfClient *client;
	MateConfEngine *conf;
	GError      *error = NULL;
	const char  *config_source;

	if (g_getenv ("MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL")) {
		fprintf (stderr, _("MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL is set, not installing schemas\n"));
		return 0;
	}

	g_type_init ();

	config_source = g_getenv ("MATECONF_CONFIG_SOURCE");
	if (!config_source) {
		fprintf (stderr, _("Must set the MATECONF_CONFIG_SOURCE environment variable\n"));
		return -1;
	}

	if (*config_source == '\0')
		config_source = NULL;

	/* shut down daemon, this is a race condition, but will usually work. */
	mateconf_shutdown_daemon (NULL);

	if (!config_source)
		conf = mateconf_engine_get_default ();
	else
		conf = mateconf_engine_get_local (config_source, &error);

	if (!conf) {
		g_assert (error != NULL);
		fprintf (stderr, _("Failed to access configuration source(s): %s\n"), error->message);
		g_error_free (error);
		return -1;
	}

	client = mateconf_client_get_for_engine (conf);

	install_default_macros_list (client, MC_PATTERNS_SHEMA_KEY, G_STRUCT_OFFSET (MCDefaultMacro, pattern));
	install_default_macros_list (client, MC_COMMANDS_SHEMA_KEY, G_STRUCT_OFFSET (MCDefaultMacro, command));

	install_default_macros_list (client, MC_DEPRECATED_PATTERNS_SHEMA_KEY, G_STRUCT_OFFSET (MCDefaultMacro, pattern));
	install_default_macros_list (client, MC_DEPRECATED_COMMANDS_SHEMA_KEY, G_STRUCT_OFFSET (MCDefaultMacro, command));

	mateconf_client_suggest_sync (client, &error);
	if (error) {
		fprintf (stderr, _("Error syncing config data: %s"),
			 error->message);
		g_error_free (error);
		return 1;
	}

	mateconf_engine_unref (conf);
	g_object_unref (client);

	return 0;
}
