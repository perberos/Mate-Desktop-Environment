/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-win32.c: Win32-specific code for libmatecomponentui
 *
 * Author:
 *    Tor Lillqvist  (tml@novell.com)
 *
 * Copyright 2005 Novell, Inc.
 */
#include <config.h>
#include <glib.h>

#ifdef G_OS_WIN32

#ifndef PIC
#error Must build as a DLL
#endif

#include <string.h>
#include <stdio.h>
#include <windows.h>

/* Silence gcc with a prototype */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
		     DWORD     fdwReason,
		     LPVOID    lpvReserved);

/* Ditto. We can't include matecomponent-ui-private.h where the prototypes
 * are in this file.
 */
const char *_matecomponent_ui_get_localedir (void);
const char *_matecomponent_ui_get_datadir (void);
const char *_matecomponent_ui_get_uidir (void);

static HMODULE hmodule;
G_LOCK_DEFINE_STATIC (mutex);

/* Using short filenames, in system codepage */
static char *_matecomponent_localedir = NULL;

/* Using long filenames, in UTF-8 */
static char *_matecomponent_datadir;
static char *_matecomponent_uidir;

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
	if (strncmp (configure_time_path, PREFIX "/", strlen (PREFIX) + 1) == 0) {
		return g_strconcat (runtime_prefix,
				    configure_time_path + strlen (PREFIX),
				    NULL);
	} else
		return g_strdup (configure_time_path);
}

static void
setup (void)
{
	char *full_prefix = NULL;  
	char *short_prefix = NULL; 
	char *p;

	G_LOCK (mutex);
	if (_matecomponent_localedir != NULL) {
		G_UNLOCK (mutex);
		return;
	}

	if (GetVersion () < 0x80000000) {
		wchar_t wcbfr[1000];

		if (GetModuleFileNameW (hmodule, wcbfr,
					G_N_ELEMENTS (wcbfr))) {
			full_prefix = g_utf16_to_utf8 (wcbfr, -1,
						       NULL, NULL, NULL);
			if (GetShortPathNameW (wcbfr, wcbfr,
					       G_N_ELEMENTS (wcbfr)))
				short_prefix = g_utf16_to_utf8 (wcbfr, -1,
								NULL, NULL, NULL);
		}
	} else {
		char cpbfr[1000];
		if (GetModuleFileNameA (hmodule, cpbfr, G_N_ELEMENTS (cpbfr)))
			full_prefix = short_prefix =
				g_locale_to_utf8 (cpbfr, -1,
						  NULL, NULL, NULL);
	}

	if (full_prefix != NULL) {
		p = strrchr (full_prefix, '\\');
		if (p != NULL)
			*p = '\0';
		
		p = strrchr (full_prefix, '\\');
		if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
			*p = '\0';
	} else {
		full_prefix = "";
	}
		
	if (short_prefix != NULL) {
		p = strrchr (short_prefix, '\\');
		if (p != NULL)
			*p = '\0';
			
		p = strrchr (short_prefix, '\\');
		if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
			*p = '\0';
	} else {
		short_prefix = "";
	}

	_matecomponent_localedir = replace_prefix (short_prefix, MATECOMPONENT_LOCALEDIR);
	p = _matecomponent_localedir;
	_matecomponent_localedir = g_locale_from_utf8 (_matecomponent_localedir, -1,
						NULL, NULL, NULL);
	g_free (p);

	_matecomponent_datadir = replace_prefix (full_prefix, MATECOMPONENT_DATADIR);
	_matecomponent_uidir = replace_prefix (full_prefix, MATECOMPONENT_UIDIR);

	G_UNLOCK (mutex);
}

const char *
_matecomponent_ui_get_localedir (void)
{
	setup ();
	return _matecomponent_localedir;
}

const char *
_matecomponent_ui_get_datadir (void)
{
	setup ();
	return _matecomponent_datadir;
}

const char *
_matecomponent_ui_get_uidir (void)
{
	setup ();
	return _matecomponent_uidir;
}

#endif	/* G_OS_WIN32 */
