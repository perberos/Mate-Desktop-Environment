/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* MATE Library - mate-mateconf.c
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/


#include <config.h>
#include <stdlib.h>

#define MATECONF_ENABLE_INTERNALS
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include <glib/gi18n-lib.h>

#include "libmate.h"

#include "mate-mateconfP.h"

/**
 * mate_mateconf_get_mate_libs_settings_relative:
 * @subkey: key part below the mate desktop settings directory
 *
 * Description:  Gets the full key name for a MATE desktop specific
 * setting for a specific application. Those keys are used to store application-specific
 * configuration, for example the history of a MateEntry. This
 * config space should only be used by libraries.
 *
 * Returns:  A newly allocated string
 **/
gchar* mate_mateconf_get_mate_libs_settings_relative(const gchar* subkey)
{
	gchar* dir;
	gchar* key;
	gchar* tmp;

	tmp = mateconf_escape_key(mate_program_get_app_id(mate_program_get()), -1);

	dir = g_strconcat("/apps/mate-settings/", tmp, NULL);
	g_free(tmp);

	if (subkey && *subkey)
	{
		key = mateconf_concat_dir_and_key(dir, subkey);
		g_free(dir);
	}
	else
	{
		/* subkey == "" */
		key = dir;
	}

	return key;
}

/**
 * mate_mateconf_get_app_settings_relative:
 * @program: #MateProgram pointer or %NULL for the default
 * @subkey: key part below the mate desktop settings directory
 *
 * Description:  Gets the full key name for an application specific
 * setting.  That is "/apps/&lt;application_id&gt;/@subkey".
 *
 * Returns:  A newly allocated string
 **/
gchar* mate_mateconf_get_app_settings_relative(MateProgram* program, const gchar* subkey)
{
	gchar* dir;
	gchar* key;

	if (program == NULL)
		program = mate_program_get();

	dir = g_strconcat("/apps/", mate_program_get_app_id(program), NULL);

	if (subkey && *subkey)
	{
		key = mateconf_concat_dir_and_key(dir, subkey);
		g_free(dir);
	}
	else
	{
		/* subkey == "" */
		key = dir;
	}

	return key;
}

/**
 * mate_mateconf_lazy_init:
 *
 * Description:  Internal libmate/ui routine.  You never have
 * to do this from your code.  But all places in libmate/ui
 * that need mateconf should call this before calling any mateconf
 * calls.
 **/
void _mate_mateconf_lazy_init(void)
{
	/* Note this is the same as in libmateui/libmateui/mate-mateconf-ui.c,
	* keep this in sync (it's named mateui_mateconf_lazy_init) */
	gchar* settings_dir;
	MateConfClient* client = NULL;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	client = mateconf_client_get_default();

	mateconf_client_add_dir(client, "/desktop/mate", MATECONF_CLIENT_PRELOAD_NONE, NULL);

	settings_dir = mate_mateconf_get_mate_libs_settings_relative("");

	/* Possibly we should turn preload on for this MATECONF_CLIENT_PRELOAD_NONE */
	mateconf_client_add_dir(client, settings_dir, MATECONF_CLIENT_PRELOAD_NONE, NULL);
	g_free(settings_dir);

	/* Leak the MateConfClient reference, we want to keep
	* the client alive forever.
	*/
}

/**
 * mate_mateconf_module_info_get
 *
 * An internal libmate/ui routine. This will never be needed in public code.
 *
 * Returns: A #MateModuleInfo instance representing the MateConf module.
 */
const MateModuleInfo* _mate_mateconf_module_info_get(void)
{
	static MateModuleInfo module_info = {
		"mate-mateconf",
		mateconf_version,
		NULL /* description */,
		NULL /* requirements */,
		NULL /* instance init */,
		NULL /* pre_args_parse */,
		NULL /* post_args_parse */,
		NULL /* options */,
		NULL /* init_pass */,
		NULL /* class_init */,
		NULL, NULL /* expansions */
	};

	module_info.description = _("MATE MateConf Support");

	return &module_info;
}
