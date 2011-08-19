/* dllmain.c: DLL entry point for libmatevfs on Win32
 * copyright (C) 2005 Novell, Inc
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

#include <windows.h>
#include <string.h>
#include <mbstring.h>
#include <glib.h>

const char *_mate_vfs_datadir;
const char *_mate_vfs_libdir;
const char *_mate_vfs_prefix;
const char *_mate_vfs_localedir;
const char *_mate_vfs_sysconfdir;

BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

static char *
_mate_vfs_replace_prefix (const char *configure_time_path)
{
  if (strncmp (configure_time_path, MATE_VFS_PREFIX "/", strlen (MATE_VFS_PREFIX) + 1) == 0)
    {
      return g_strconcat (_mate_vfs_prefix,
			  configure_time_path + strlen (MATE_VFS_PREFIX),
			  NULL);
    }
  else
    return g_strdup (configure_time_path);
}

/* DllMain function needed to fetch the DLL name and deduce the
 * installation directory from that, and then form the pathnames for
 * various directories relative to the installation directory.
 */
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  wchar_t wcbfr[1000];
  char cpbfr[1000];
  char *dll_name = NULL;
  
  switch (fdwReason) {
  case DLL_PROCESS_ATTACH:
    /* GLib 2.6 uses UTF-8 file names */
    if (GetVersion () < 0x80000000) {
      /* NT-based Windows has wide char API */
      if (GetModuleFileNameW ((HMODULE) hinstDLL,
			      wcbfr, G_N_ELEMENTS (wcbfr)))
	dll_name = g_utf16_to_utf8 (wcbfr, -1,
				    NULL, NULL, NULL);
    } else {
      /* Win9x, yecch */
      if (GetModuleFileNameA ((HMODULE) hinstDLL,
			      cpbfr, G_N_ELEMENTS (cpbfr)))
	dll_name = g_locale_to_utf8 (cpbfr, -1,
				     NULL, NULL, NULL);
    }

    if (dll_name) {
      gchar *p = strrchr (dll_name, '\\');
      
      if (p != NULL)
	*p = '\0';
      
      p = strrchr (dll_name, '\\');
      if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
	*p = '\0';
      
      _mate_vfs_prefix = dll_name;
    } else {
      _mate_vfs_prefix = g_strdup ("");
    }

    _mate_vfs_datadir = _mate_vfs_replace_prefix (MATE_VFS_DATADIR);
    _mate_vfs_libdir = _mate_vfs_replace_prefix (MATE_VFS_LIBDIR);
    _mate_vfs_localedir = _mate_vfs_replace_prefix (MATE_VFS_LOCALEDIR);
    _mate_vfs_sysconfdir = _mate_vfs_replace_prefix (MATE_VFS_SYSCONFDIR);
    break;
  }

  return TRUE;
}
