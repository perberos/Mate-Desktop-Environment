/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: nil -*- */
/* dllmain.c: DLL entry point for libmate on Win32
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include <glib.h>

#include "mate-init.h"

/* localedir uses system codepage as it is passed to the non-UTF8ified
 * gettext library
 */
static const char *localedir = NULL;

/* The others are in UTF-8 */
static char *prefix;
static const char *libdir;
static const char *datadir;
static const char *localstatedir;
static const char *sysconfdir;

static HMODULE hmodule;
G_LOCK_DEFINE_STATIC (mutex);

/* Silence gcc with prototype. Yes, this is silly. */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

/* DllMain used to tuck away the libmate DLL's HMODULE */
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
            strncmp (configure_time_path, LIBMATE_PREFIX "/",
                     strlen (LIBMATE_PREFIX) + 1) == 0)
                return g_strconcat (runtime_prefix,
                                    configure_time_path + strlen (LIBMATE_PREFIX),
                                    NULL);
        else
                return g_strdup (configure_time_path);
}

/**
 * mate_win32_get_prefixes:
 * @hmodule: The handle to a DLL (a HMODULE).
 * @full_prefix: Where the full UTF-8 path to the DLL's installation folder
 *               will be returned.
 * @cp_prefix: Where a system codepage version of
 *             the installation folder will be returned.
 *
 * This function looks up the installation prefix of the DLL (or EXE)
 * with handle @hmodule. The prefix using long filenames and in UTF-8
 * form is returned in @full_prefix. The prefix using short file names
 * (if present in the file system) and in the system codepage is
 * returned in @cp_prefix. To determine the installation prefix, the
 * full path to the DLL or EXE is first fetched. If the last folder
 * component in that path is called "bin", its parent folder is used,
 * otherwise the folder itself.
 *
 * If either can't be obtained, %NULL is stored. The caller should be
 * prepared to handle that.
 *
 * The returned character pointers are newly allocated and should be
 * freed with g_free when not longer needed.
 */
void
mate_win32_get_prefixes (gpointer  hmodule,
                          char    **full_prefix,
                          char    **cp_prefix)
{
        wchar_t wcbfr[1000];
        char cpbfr[1000];

        g_return_if_fail (full_prefix != NULL);
        g_return_if_fail (cp_prefix != NULL);

        *full_prefix = NULL;
        *cp_prefix = NULL;

        if (GetModuleFileNameW ((HMODULE) hmodule, wcbfr, G_N_ELEMENTS (wcbfr))) {
                *full_prefix = g_utf16_to_utf8 (wcbfr, -1,
                                                NULL, NULL, NULL);
                if (GetShortPathNameW (wcbfr, wcbfr, G_N_ELEMENTS (wcbfr)) &&
                    /* Short pathnames always contain only
                     * ASCII, I think, but just in case, be
                     * prepared.
                     */
                    WideCharToMultiByte (CP_ACP, 0, wcbfr, -1,
                                         cpbfr, G_N_ELEMENTS (cpbfr),
                                         NULL, NULL))
                        *cp_prefix = g_strdup (cpbfr);
                else if (*full_prefix)
                        *cp_prefix = g_locale_from_utf8 (*full_prefix, -1,
                                                         NULL, NULL, NULL);
        }

        if (*full_prefix != NULL) {
                gchar *p = strrchr (*full_prefix, '\\');
                if (p != NULL)
                        *p = '\0';

                p = strrchr (*full_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
        }

        /* cp_prefix is in system codepage */
        if (*cp_prefix != NULL) {
                gchar *p = _mbsrchr (*cp_prefix, '\\');
                if (p != NULL)
                        *p = '\0';

                p = _mbsrchr (*cp_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
        }
}

static void
setup (void)
{
	char *cp_prefix;

        G_LOCK (mutex);
        if (localedir != NULL) {
                G_UNLOCK (mutex);
                return;
        }

        mate_win32_get_prefixes (hmodule, &prefix, &cp_prefix);

        localedir = replace_prefix (cp_prefix, LIBMATE_LOCALEDIR);
        g_free (cp_prefix);

        libdir = replace_prefix (prefix, LIBMATE_LIBDIR);
        datadir = replace_prefix (prefix, LIBMATE_DATADIR);
        localstatedir = replace_prefix (prefix, LIBMATE_LOCALSTATEDIR);
        sysconfdir = replace_prefix (prefix, LIBMATE_SYSCONFDIR);

        G_UNLOCK (mutex);
}

/* Include libmate-private.h now to get prototypes for the getter
 * functions, to silence gcc. Can't include earlier as we need the
 * definitions of the LIBMATE_*DIR macros from the Makefile above.
 */
#include "libmate-private.h"

#define GETTER(varbl)                           \
const char *                                    \
_mate_get_##varbl (void)                       \
{                                               \
        setup ();                               \
        return varbl;                           \
}

GETTER (prefix)
GETTER (localedir)
GETTER (libdir)
GETTER (datadir)
GETTER (localstatedir)
GETTER (sysconfdir)
