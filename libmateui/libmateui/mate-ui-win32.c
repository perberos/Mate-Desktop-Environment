/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
/* mate-ui-win32.c: Win32-specific code in libmateui
 * Copyright (C) 2005 Novell, Inc
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <windows.h>
#include <string.h>
#include <glib.h>

#include <libmate/mate-init.h>

/* localedir uses system codepage as it is passed to the non-UTF8ified
 * gettext library
 */
static char *localedir = NULL;

/* The others are in UTF-8 */
static char *datadir;

static HMODULE hmodule;
G_LOCK_DEFINE_STATIC (mutex);

/* Silence gcc with prototypes. Yes, this is silly. */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

const char *_mate_ui_get_localedir (void);
const char *_mate_ui_get_datadir (void);

/* DllMain used to tuck away the DLL's HMODULE */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
        switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
                hmodule = hinstDLL;
                break;
        }
        return TRUE;
}

static char *
replace_prefix (const char *runtime_prefix,
                const char *configure_time_path)
{
        if (runtime_prefix &&
            strncmp (configure_time_path, LIBMATEUI_PREFIX "/",
                     strlen (LIBMATEUI_PREFIX) + 1) == 0) {
                return g_strconcat (runtime_prefix,
                                    configure_time_path + strlen (LIBMATEUI_PREFIX),
                                    NULL);
        } else
                return g_strdup (configure_time_path);
}

static void
setup (void)
{
	char *full_prefix;  
	char *cp_prefix; 

        G_LOCK (mutex);
        if (localedir != NULL) {
                G_UNLOCK (mutex);
                return;
        }

        mate_win32_get_prefixes (hmodule, &full_prefix, &cp_prefix);

        localedir = replace_prefix (cp_prefix, MATEUILOCALEDIR);
        g_free (cp_prefix);

        datadir = replace_prefix (full_prefix, LIBMATEUI_DATADIR);
        g_free (full_prefix);

	G_UNLOCK (mutex);
}

#define GETTER(varbl)                           \
const char *                                    \
_mate_ui_get_##varbl (void)                    \
{                                               \
        setup ();                               \
        return varbl;                           \
}

GETTER (localedir)
GETTER (datadir)
