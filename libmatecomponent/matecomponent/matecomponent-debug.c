/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-debug.c: A runtime-controllable debugging API.
 *
 * Author:
 *   Jaka Mocnik  <jaka@gnu.org>
 */

#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include <matecomponent/matecomponent-debug.h>

#include <glib/gstdio.h>

MateComponentDebugFlags _matecomponent_debug_flags;
static FILE *_matecomponent_debug_file;

void
matecomponent_debug_init(void)
{
	const GDebugKey debug_keys[] = {
		{ "refs",       MATECOMPONENT_DEBUG_REFS },
		{ "aggregate",  MATECOMPONENT_DEBUG_AGGREGATE },
		{ "lifecycle",  MATECOMPONENT_DEBUG_LIFECYCLE },
		{ "running",    MATECOMPONENT_DEBUG_RUNNING },
		{ "object",     MATECOMPONENT_DEBUG_OBJECT }
	};
	const char *env_string;

	_matecomponent_debug_flags = MATECOMPONENT_DEBUG_NONE;
	env_string = g_getenv ("MATECOMPONENT_DEBUG");
	if (env_string)
	  _matecomponent_debug_flags |=
			  g_parse_debug_string (env_string,
						      debug_keys,
						      G_N_ELEMENTS (debug_keys));
	_matecomponent_debug_file = NULL;
	env_string = g_getenv ("MATECOMPONENT_DEBUG_DIR");
	if(env_string) {
	  gchar *dbg_filename;
	  dbg_filename = g_strdup_printf("%s/matecomponent-debug-%d", env_string, getpid());
	  _matecomponent_debug_file = g_fopen(dbg_filename, "w");
	  g_free(dbg_filename);
	}
	if(_matecomponent_debug_file == NULL)
	  _matecomponent_debug_file = stderr;
}

void
matecomponent_debug_print (const char *name, char *fmt, ...)
{
	va_list args;
           
	va_start (args, fmt);
	
	fprintf (_matecomponent_debug_file, "[%06d]:%-15s ", getpid (), name); 
	vfprintf (_matecomponent_debug_file, fmt, args);
	fprintf (_matecomponent_debug_file, "\n"); 
	fflush (_matecomponent_debug_file);

	va_end (args);
}
