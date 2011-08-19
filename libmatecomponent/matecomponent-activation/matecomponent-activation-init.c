/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation: A library for accessing matecomponent-activation-server.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Eazel, Inc.
 *  Copyright (C) 2003 Ximian, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors:
 *      Elliot Lee <sopwith@redhat.com>
 *      Michael Meeks <michael@ximian.com>
 */

#include <config.h>
#include "matecomponent-activation-init.h"
#include "matecomponent-activation-client.h"

#include "MateComponent_ActivationContext.h"
#include <glib/gi18n-lib.h>
#include "matecomponent-activation-private.h"
#include "matecomponent-activation-register.h"
#include "matecomponent-activation-version.h"
#include <sys/types.h>
#include <fcntl.h>
#include <glib.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************** MateCORBA-specific stuff ****************/

#include <matecorba/matecorba.h>

GStaticRecMutex _matecomponent_activation_guard = G_STATIC_REC_MUTEX_INIT;
static CORBA_ORB matecomponent_activation_orb = CORBA_OBJECT_NIL;
static CORBA_Context matecomponent_activation_context;
static gboolean is_initialized = FALSE;

/* prevent registering with OAF when matecomponent_activation_active_server_register() */
gboolean matecomponent_activation_private = FALSE;

#ifdef G_OS_WIN32

#ifndef PIC
#error Must build as a DLL
#endif

#include <io.h>
#include <windows.h>
#include <mbstring.h>

static const char *prefix;
static const char *server_libexecdir;
static const char *serverinfodir;
static const char *server_confdir;
static const char *localedir;

static HMODULE hmodule;

char *
_matecomponent_activation_win32_replace_prefix (const char *runtime_prefix,
                                         const char *configure_time_path)
{
  if (strncmp (configure_time_path, PREFIX "/", strlen (PREFIX) + 1) == 0) {
          return g_strconcat (runtime_prefix,
                              configure_time_path + strlen (PREFIX),
                              NULL);
  } else
          return g_strdup (configure_time_path);
}

/* DllMain that just stores the DLL's hmodule */
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

G_LOCK_DEFINE_STATIC (mutex);

static void
setup_runtime_paths (void)
{
        char cpbfr[1000];
        wchar_t wcbfr[1000];
        char *full_prefix = NULL;
        char *cp_prefix = NULL;
  
        G_LOCK (mutex);
        if (prefix != NULL) {
                G_UNLOCK (mutex);
                return;
        }

        if (G_WIN32_HAVE_WIDECHAR_API ()) {
                /* NT-based Windows */
                if (GetModuleFileNameW (hmodule, wcbfr, G_N_ELEMENTS (wcbfr))) {
                        full_prefix = g_utf16_to_utf8 (wcbfr, -1,
                                                   NULL, NULL, NULL);
                        if (GetShortPathNameW (wcbfr, wcbfr, G_N_ELEMENTS (wcbfr)) &&
                            WideCharToMultiByte (CP_ACP, 0, wcbfr, -1,
                                                 cpbfr, G_N_ELEMENTS (cpbfr),
                                                 NULL, NULL))
                                cp_prefix = g_strdup (cpbfr);
                        else if (full_prefix)
                                cp_prefix = g_locale_from_utf8 (full_prefix, -1,
                                                                NULL, NULL, NULL);
                }
        } else {
                /* Win9x */
                if (GetModuleFileNameA (hmodule, cpbfr, G_N_ELEMENTS (cpbfr))) {
                        full_prefix = g_locale_to_utf8 (cpbfr, -1,
                                                        NULL, NULL, NULL);
                        cp_prefix = g_strdup (cpbfr);
                }
        }

        if (full_prefix != NULL) {
                gchar *p = strrchr (full_prefix, '\\');
                if (p != NULL)
                        *p = '\0';
      
                p = strrchr (full_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
                prefix = full_prefix;
        }
                  
        if (cp_prefix != NULL) {
                gchar *p = _mbsrchr (cp_prefix, '\\');
                if (p != NULL)
                        *p = '\0';
      
                p = _mbsrchr (cp_prefix, '\\');
                if (p && (g_ascii_strcasecmp (p + 1, "bin") == 0))
                        *p = '\0';
                
        }

        server_libexecdir = _matecomponent_activation_win32_replace_prefix (full_prefix, SERVER_LIBEXECDIR);
        serverinfodir = _matecomponent_activation_win32_replace_prefix (full_prefix, SERVERINFODIR);
        server_confdir = _matecomponent_activation_win32_replace_prefix (full_prefix, SERVER_CONFDIR);
        localedir = _matecomponent_activation_win32_replace_prefix (cp_prefix, MATECOMPONENT_ACTIVATION_LOCALEDIR);

        G_UNLOCK (mutex);
}

const char *
_matecomponent_activation_win32_get_prefix (void)
{
        setup_runtime_paths ();

        return prefix;
}

const char *
_matecomponent_activation_win32_get_server_libexecdir (void)
{
        setup_runtime_paths ();

        return server_libexecdir;
}

const char *
_matecomponent_activation_win32_get_serverinfodir (void)
{
        setup_runtime_paths ();

        return serverinfodir;
}

const char *
_matecomponent_activation_win32_get_server_confdir (void)
{
        setup_runtime_paths ();

        return server_confdir;
}

const char *
_matecomponent_activation_win32_get_localedir (void)
{
        setup_runtime_paths ();

        return localedir;
}

#undef MATECOMPONENT_ACTIVATION_LOCALEDIR
#define MATECOMPONENT_ACTIVATION_LOCALEDIR _matecomponent_activation_win32_get_localedir ()

#endif

/**
 * matecomponent_activation_orb_get:
 *
 * Returns the ORB used by OAF.
 *
 * Return value: the ORB used by OAF.
 */
CORBA_ORB
matecomponent_activation_orb_get (void)
{
	return matecomponent_activation_orb;
}

const char *
matecomponent_activation_hostname_get (void)
{
        /*
         * This tolerates a run-time change of
         * hostname [a-la DHCP], and we don't care
         * about this anyway; there is no cross-host
         * activation foo.
         */
        return "localhost";
#if 0
	static char hostname[256] = "";
	if (hostname[0] == '\0') {
		if (gethostname (hostname, sizeof (hostname) - 1))
                        strcpy (hostname, "localhost");
        }
	return hostname;
#endif
}


/**
 * matecomponent_activation_context_get:
 * @void: 
 * 
 * Fetches the internal CORBA_Context used with every activation
 * request. This can be used to manipulate some of the associated
 * fields; particularly 'display'.
 *
 * This method is deprecated.
 * 
 * Return value: the CORBA context for activating with.
 **/
CORBA_Context
matecomponent_activation_context_get (void)
{
	return matecomponent_activation_context;
}

const char *
matecomponent_activation_session_name_get (void)
{
	const char *dumbptr = "local";

	return dumbptr;
}

const char *
matecomponent_activation_domain_get (void)
{
	return NULL;
}

CORBA_Object
matecomponent_activation_internal_activation_context_get_extended (gboolean           existing_only,
                                                            CORBA_Environment *ev)
{
	MateComponentActivationBaseService base_service = { NULL };

	base_service.name = "IDL:MateComponent/ActivationContext:1.0";
	base_service.session_name = matecomponent_activation_session_name_get ();

	return matecomponent_activation_internal_service_get_extended (&base_service, existing_only,
                                                   ev);
}

CORBA_Object
matecomponent_activation_activation_context_get (void)
{
	MateComponentActivationBaseService base_service = { NULL };

	base_service.name = "IDL:MateComponent/ActivationContext:1.0";
	base_service.session_name = matecomponent_activation_session_name_get ();

	return matecomponent_activation_service_get (&base_service);
}

static MateComponent_ObjectDirectory object_directory = CORBA_OBJECT_NIL;

CORBA_Object
matecomponent_activation_object_directory_get (const char *username,
                                        const char *hostname)
{
        CORBA_Environment ev;
        MateComponent_ActivationContext new_ac;
        MateComponent_ObjectDirectoryList *od_list;
        static MateComponent_ActivationContext ac = CORBA_OBJECT_NIL;

        new_ac = matecomponent_activation_activation_context_get ();
        if (ac == new_ac)
                return object_directory;
        ac = new_ac;

        CORBA_exception_init (&ev);

        od_list = MateComponent_ActivationContext__get_directories (ac, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
                CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

        if (od_list->_length != 1) {
                g_warning ("Extremely strange, strange object directories (%d)"
                           "registered with the activation context", od_list->_length);
                CORBA_free (od_list);
                CORBA_exception_free (&ev);
                return CORBA_OBJECT_NIL;
        }

        object_directory = CORBA_Object_duplicate (od_list->_buffer[0], &ev);
        
        CORBA_free (od_list);
        CORBA_exception_free (&ev);

        return object_directory;
}

static int   matecomponent_activation_ior_fd = 1;
static char *matecomponent_activation_activate_iid = NULL;

#ifndef MATECOMPONENT_DISABLE_DEPRECATED_SOURCE
struct poptOption matecomponent_activation_popt_options[] = {
        { NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL },
        { "oaf-ior-fd", '\0', POPT_ARG_INT, &matecomponent_activation_ior_fd, 0,
          N_("File descriptor to print IOR on"), N_("FD") },
        { "oaf-activate-iid", '\0', POPT_ARG_STRING, &matecomponent_activation_activate_iid, 0,
          N_("IID to activate"), "IID" },
        { "oaf-private", '\0', POPT_ARG_NONE, &matecomponent_activation_private, 0,
          N_("Prevent registering of server with OAF"), NULL },
        { NULL }
};
#endif /* MATECOMPONENT_DISABLE_DEPRECATED_SOURCE */

static void
init_gettext (gboolean bind_codeset)
{
	static gboolean initialized = FALSE;
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET	
	static gboolean codeset_bound = FALSE;
#endif

	if (!initialized)
	{
		bindtextdomain (GETTEXT_PACKAGE, MATECOMPONENT_ACTIVATION_LOCALEDIR);
		initialized = TRUE;
	}
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET	
	if (!codeset_bound && bind_codeset)
	{
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
		codeset_bound = TRUE;
	}
#endif
}

/**
 * matecomponent_activation_get_goption_group:
 *
 * Returns a #GOptionGroup for parsing matecomponent-activation options.
 * 
 * Return value: a new #GOptionGroup
 *
 * Since: 2.14
 */
GOptionGroup *
matecomponent_activation_get_goption_group (void)
{
	const GOptionEntry matecomponent_activation_goption_options[] = {
		{ "oaf-ior-fd", '\0', 0,  G_OPTION_ARG_INT, &matecomponent_activation_ior_fd,
		  N_("File descriptor to print IOR on"), N_("FD") },
		{ "oaf-activate-iid", '\0', 0, G_OPTION_ARG_STRING, &matecomponent_activation_activate_iid,
		  N_("IID to activate"), "IID" },
		{ "oaf-private", '\0', 0, G_OPTION_ARG_NONE, &matecomponent_activation_private,
		  N_("Prevent registering of server with OAF"), NULL },
		{ NULL }
	};
	GOptionGroup *group;

	init_gettext (TRUE);

	group = g_option_group_new ("matecomponent-activation",
				    N_("MateComponent Activation options:"),
				    N_("Show MateComponent Activation options"),
				    NULL, NULL);
	g_option_group_set_translation_domain (group, GETTEXT_PACKAGE);
	g_option_group_add_entries (group, matecomponent_activation_goption_options);

	return group;
}

/**
 * matecomponent_activation_activation_iid_get:
 *
 * If this process was launched to activate an exe server, this
 * function gives the IID of the server requested, otherwise it
 * returns NULL.
 * 
 * Return value: The IID of the activated server or NULL.
 */
const char *
matecomponent_activation_iid_get (void)
{
	return matecomponent_activation_activate_iid;
}

int
matecomponent_activation_ior_fd_get (void)
{
	return matecomponent_activation_ior_fd;
}

void
matecomponent_activation_preinit (gpointer app, gpointer mod_info)
{
	init_gettext (FALSE);
}

void
matecomponent_activation_postinit (gpointer app, gpointer mod_info)
{
	matecomponent_activation_base_service_init ();

#if defined (F_SETFD) && defined (FD_CLOEXEC)
	if (matecomponent_activation_ior_fd > 2)
		fcntl (matecomponent_activation_ior_fd, F_SETFD, FD_CLOEXEC);
#endif
#ifdef G_OS_WIN32
	if (matecomponent_activation_ior_fd > 2) {
                HANDLE newhandle;

                if (DuplicateHandle (GetCurrentProcess (), (HANDLE) _get_osfhandle (matecomponent_activation_ior_fd),
                                     GetCurrentProcess (), &newhandle,
                                     0, FALSE, DUPLICATE_SAME_ACCESS)) {
                        close (matecomponent_activation_ior_fd);
                        matecomponent_activation_ior_fd = _open_osfhandle ((int) newhandle, _O_TEXT | _O_NOINHERIT);
                }
        }
#endif

        if (matecomponent_activation_activate_iid)
                g_timeout_add_seconds_full (G_PRIORITY_LOW,
                                            MATECOMPONENT_ACTIVATION_FACTORY_PRIVATE_TIMEOUT,
                                            matecomponent_activation_timeout_reg_check,
                                            NULL, NULL);
        else
                matecomponent_activation_timeout_reg_check_set (FALSE);

	is_initialized = TRUE;
}

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
static void
do_barrier (int signum)
{
	volatile int barrier = 1;

#ifndef HAVE_SIGACTION
        signal (signum, do_barrier);
#endif

	while (barrier);
}
#endif

/**
 * matecomponent_activation_is_initialized:
 *
 * Tells you whether or not matecomponent-activation is initialized.
 *
 * Return value: whether matecomponent-activation is initialized or not.
 */
gboolean
matecomponent_activation_is_initialized (void)
{
	return is_initialized;
}

#ifndef MATECOMPONENT_DISABLE_DEPRECATED_SOURCE
/**
 * matecomponent_activation_get_popt_table_name:
 *
 * Get the table name to use for the oaf popt options table when
 * registering with libmate
 * 
 * Return value: A localized copy of the string "matecomponent activation options"
 */
char *
matecomponent_activation_get_popt_table_name (void)
{
	init_gettext (FALSE);
	return _("MateComponent activation options");
}
#endif /* MATECOMPONENT_DISABLE_DEPRECATED_SOURCE */

/**
 * matecomponent_activation_init:
 * @argc: number of command-line arguments passed to the program.
 * @argv: array of strings containing the command-line 
 *        arguments of the program.
 *
 * Initializes matecomponent-activation. Should be called before any other
 * call to the library.
 *
 * Return value: the ORB used by matecomponent-activation.
 */
CORBA_ORB
matecomponent_activation_init (int argc, char **argv)
{
	CORBA_ORB retval;
	int i;

        if (!is_initialized)
        {
                bindtextdomain (PACKAGE, MATECOMPONENT_ACTIVATION_LOCALEDIR);

                matecomponent_activation_preinit (NULL, NULL);

                retval = matecomponent_activation_orb_init (&argc, argv);
        }
	else
		retval = matecomponent_activation_orb;

        /* Accumulate arguments over multiple inits. Sometimes we are
         * initialized from GTK_MODULEs or mate-vfs with bogus arguments */
	for (i = 1; i < argc; i++) {
                if (!strncmp ("--oaf-ior-fd=", argv[i],
                              strlen ("--oaf-ior-fd="))) {
                        matecomponent_activation_ior_fd =
                                atoi (argv[i] + strlen ("--oaf-ior-fd="));
                        if (!matecomponent_activation_ior_fd)
                                matecomponent_activation_ior_fd = 1;
                } else if (!strncmp
                           ("--oaf-activate-iid=", argv[i],
                            strlen ("--oaf-activate-iid="))) {
			matecomponent_activation_activate_iid =
				g_strdup (argv[i] + strlen ("--oaf-activate-iid="));
                } else if (!strcmp
                           ("--oaf-private", argv[i])) {
                        matecomponent_activation_private = TRUE;
                }     
	}

        if (!is_initialized)
                matecomponent_activation_postinit (NULL, NULL);

	return retval;
}

/**
 * matecomponent_activation_orb_init:
 * @argc: pointer to program's argument count
 * @argv: argument array
 * 
 * Initialize MateComponent Activation's ORB - do this once centrally
 * so it can be easily shared.
 * 
 * Return value: the ORB.
 **/
CORBA_ORB
matecomponent_activation_orb_init (int *argc, char **argv)
{
        CORBA_Context def_ctx;
	CORBA_Environment ev;
	const char *hostname;
	gchar *orb_id;

	CORBA_exception_init (&ev);

#ifdef HAVE_GTHREADS
	orb_id = "matecorba-local-mt-orb";
#else
	orb_id = "matecorba-local-non-threaded-orb";
#endif
	matecomponent_activation_orb = CORBA_ORB_init (argc, argv, orb_id, &ev);
	g_assert (ev._major == CORBA_NO_EXCEPTION);

	matecomponent_activation_init_activation_env ();

	/* Set values in default context */
	CORBA_ORB_get_default_context (matecomponent_activation_orb, &def_ctx, &ev);
        CORBA_Context_create_child (def_ctx, "activation", &matecomponent_activation_context, &ev);
	g_assert (ev._major == CORBA_NO_EXCEPTION);
        CORBA_Object_release ((CORBA_Object) def_ctx, &ev);
	g_assert (ev._major == CORBA_NO_EXCEPTION);

	hostname = matecomponent_activation_hostname_get ();
	CORBA_Context_set_one_value (matecomponent_activation_context, "hostname",
				     (char *) hostname, &ev);
	CORBA_Context_set_one_value (matecomponent_activation_context, "username",
				     (char *) g_get_user_name (), &ev);

	CORBA_exception_free (&ev);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	if (getenv ("MATECOMPONENT_ACTIVATION_TRAP_SEGV")) {
#ifdef HAVE_SIGACTION
		struct sigaction sa;
		sa.sa_handler = do_barrier;
		sigaction (SIGSEGV, &sa, NULL);
		sigaction (SIGPIPE, &sa, NULL);
#else
                signal (SIGSEGV, do_barrier);
#ifdef SIGPIPE
                signal (SIGPIPE, do_barrier);
#endif
#endif
	}
	if (getenv ("MATECOMPONENT_ACTIVATION_BARRIER_INIT")) {
		volatile int barrier = 1;
		while (barrier);
	}
#endif

	return matecomponent_activation_orb;
}

/**
 * matecomponent_activation_debug_shutdown:
 * 
 *   A debugging function to shutdown the ORB and process
 * any reference count leaks that may have occurred.
 * 
 * Return value: FALSE if there were leaks detected, else TRUE
 **/
gboolean
matecomponent_activation_debug_shutdown (void)
{
        int retval = TRUE;
        CORBA_Environment ev;

        if (is_initialized) {
                CORBA_exception_init (&ev);

                matecomponent_activation_base_service_debug_shutdown (&ev);
                if (ev._major != CORBA_NO_EXCEPTION) {
                        retval = FALSE;
                }

                if (matecomponent_activation_context != CORBA_OBJECT_NIL) {
                        CORBA_Object_release (
                                (CORBA_Object) matecomponent_activation_context, &ev);
                        matecomponent_activation_context = CORBA_OBJECT_NIL;
                }

                matecomponent_activation_release_corba_client ();

                if (object_directory != CORBA_OBJECT_NIL) {
                        CORBA_Object_release (object_directory, &ev);
                        object_directory = CORBA_OBJECT_NIL;
                }

                if (matecomponent_activation_orb != CORBA_OBJECT_NIL) {
                        CORBA_ORB_destroy (matecomponent_activation_orb, &ev);
                        if (ev._major != CORBA_NO_EXCEPTION) {
                                retval = FALSE;
                        }
                        CORBA_Object_release (
                                (CORBA_Object) matecomponent_activation_orb, &ev);
                        is_initialized = FALSE;
                }

                CORBA_exception_free (&ev);

        }

        return retval;
}

static const char  matecomponent_activation_version []    = VERSION;
static const guint matecomponent_activation_major_version = MATECOMPONENT_ACTIVATION_MAJOR_VERSION;
static const guint matecomponent_activation_minor_version = MATECOMPONENT_ACTIVATION_MINOR_VERSION;
static const guint matecomponent_activation_micro_version = MATECOMPONENT_ACTIVATION_MICRO_VERSION;

