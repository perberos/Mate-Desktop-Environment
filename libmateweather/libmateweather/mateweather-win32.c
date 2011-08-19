/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mateweather-win32.c - Win32 portability
 *
 * Copyright 2008, Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#ifdef G_OS_WIN32

#include <windows.h>

#include "mateweather-win32.h"

static HMODULE dll = NULL;

/* Prototype first to silence gcc warning */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH)
      dll = hinstDLL;

  return TRUE;
}

char *
_mateweather_win32_get_zoneinfo_dir (void)
{
    static char *retval = NULL;
    char *root;

    if (retval)
	return retval;

    root = g_win32_get_package_installation_directory_of_module (dll);
    retval = g_build_filename (root, "share/zoneinfo", NULL);
    g_free (root);

    return retval;
}

char *
_mateweather_win32_get_locale_dir (void)
{
    static char *retval = NULL;
    char *root;

    if (retval)
	return retval;

    root = g_win32_get_package_installation_directory_of_module (dll);
    retval = g_build_filename (root, "share/locale", NULL);
    g_free (root);

    return retval;
}

char *
_mateweather_win32_get_xml_location_dir (void)
{
    static char *retval = NULL;
    char *root;

    if (retval)
	return retval;

    root = g_win32_get_package_installation_directory_of_module (dll);
    retval = g_build_filename (root, "share/libmateweather", NULL);
    g_free (root);

    return retval;
}

#endif
