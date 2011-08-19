/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  oafd: OAF CORBA dameon.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>,
 *
 */

#include <config.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <MateCORBAservices/CosNaming.h>
#include <MateCORBAservices/CosNaming_impl.h>
#include <libmatecomponent.h>

#include "matecomponent-activation/matecomponent-activation-private.h"

#include "server.h"
#include "activation-context.h"
#include "activation-context-query.h"
#include "object-directory-config-file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#include <libxml/parser.h>

#include <glib/gstdio.h>

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <io.h>
#include <conio.h>
#define _WIN32_WINNT 0x0500 
#include <windows.h>

static gboolean allocated_new_console = FALSE;

#endif

static gboolean        server_threaded = FALSE;
static glong           server_guard_depth = 0;
static GStaticRecMutex server_guard = G_STATIC_REC_MUTEX_INIT;

static PortableServer_POA
server_get_poa (void)
{
        PortableServer_POA poa;

        if (!g_getenv ("MATECOMPONENT_ACTIVATION_DISABLE_THREADING")) {
                server_threaded = TRUE;
                poa = matecomponent_poa_get_threaded (MATECORBA_THREAD_HINT_PER_REQUEST);
        } else {
                g_warning ("b-a-s running in non-threaded mode");
                server_threaded = FALSE;
                poa = matecomponent_poa();
        }
        matecomponent_activation_server_fork_init (server_threaded);

        return poa;
}

ServerLockState
server_lock_drop (void)
{
        glong i, state = server_guard_depth;

        if (!server_threaded)
                return 0;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        fprintf (stderr, "thread %p dropping server guard with depth %ld\n",
                 g_thread_self (), state);
#endif
        server_guard_depth = 0;
        for (i = 0; i < state; i++)
                g_static_rec_mutex_unlock (&server_guard);
        return state;
}

void
server_lock_resume (ServerLockState state)
{
        long i;

        if (!server_threaded)
                return;

        for (i = 0; i < state; i++)
                g_static_rec_mutex_lock (&server_guard);
        server_guard_depth = state;
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        fprintf (stderr, "thread %p re-taken server guard with depth %ld\n",
                 g_thread_self (), state);
#endif
}

void
server_lock (void)
{
        if (!server_threaded)
                return;

        g_static_rec_mutex_lock (&server_guard);
        server_guard_depth++;
        fprintf (stderr, "thread %p take guard [%ld]\n",
                 g_thread_self (), server_guard_depth);
}

void
server_unlock (void)
{
        if (!server_threaded)
                return;

        fprintf (stderr, "thread %p release guard [%ld]\n",
                 g_thread_self (), server_guard_depth);
        server_guard_depth--;
        g_static_rec_mutex_unlock (&server_guard);
}

#ifdef G_OS_WIN32

#undef SERVERINFODIR
#define SERVERINFODIR _matecomponent_activation_win32_get_serverinfodir ()

#undef SERVER_LOCALEDIR
#define SERVER_LOCALEDIR _matecomponent_activation_win32_get_localedir ()

#endif  /* Win32 */

/* Option values */
static gchar *od_source_dir = NULL;
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
static void debug_queries (void);
static gchar *ac_evaluate = NULL;
#endif
static gboolean server_reg = FALSE;
static gboolean output_debug = FALSE;
static gboolean server_ac = FALSE;
static gint ior_fd = -1;

static const GOptionEntry options[] = {

	{"od-source-dir", '\0', 0, G_OPTION_ARG_FILENAME, &od_source_dir,
	 N_("Directory to read .server files from"), N_("DIRECTORY")},

	{"ac-activate", '\0', 0, G_OPTION_ARG_NONE, &server_ac,
	 N_("Serve as an ActivationContext (default is as an ObjectDirectory only)"),
	 NULL},

	{"ior-output-fd", '\0', 0, G_OPTION_ARG_INT, &ior_fd,
	 N_("File descriptor to write IOR to"), N_("FD")},

	{"register-server", '\0', 0, G_OPTION_ARG_NONE, &server_reg,
	 N_("Register as the user's activation server without locking."
            "  Warning: this option can have dangerous side effects"
            " on the stability of the user's running session,"
            " and should only be used for debugging purposes"),
	 NULL},

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	{"evaluate", '\0', 0, G_OPTION_ARG_STRING, &ac_evaluate,
	 N_("Query expression to evaluate"), N_("EXPRESSION")},
#endif

        {NULL}
};

/* Referenced from object-directory-corba.c */
GMainLoop *main_loop = NULL;

static GString *
build_src_dir (void)
{
        const char *env_od_source_dir;
        const char *mate_env_od_source_dir;
        char *config_file_od_source_dir;
        char **mate_dirs;
        GString *real_od_source_dir;
        int i;

        real_od_source_dir = g_string_new ("");
        env_od_source_dir = g_getenv ("MATECOMPONENT_ACTIVATION_PATH");
        mate_env_od_source_dir = g_getenv ("MATE2_PATH");
        config_file_od_source_dir = object_directory_load_config_file ();

        if (od_source_dir) {
                g_string_append (real_od_source_dir, od_source_dir);
                g_string_append_c (real_od_source_dir, G_SEARCHPATH_SEPARATOR);
        }

        if (env_od_source_dir) {
                g_string_append (real_od_source_dir,
                                 env_od_source_dir);
                g_string_append_c (real_od_source_dir, G_SEARCHPATH_SEPARATOR);
        }

        if (config_file_od_source_dir) {
                g_string_append (real_od_source_dir,
                                 config_file_od_source_dir);
                g_free (config_file_od_source_dir);
                g_string_append_c (real_od_source_dir, G_SEARCHPATH_SEPARATOR);
        }

        if (mate_env_od_source_dir) {
                GString *mate_od_source_dir;
                mate_dirs = g_strsplit (mate_env_od_source_dir, G_SEARCHPATH_SEPARATOR_S, -1);
                mate_od_source_dir = g_string_new("");
                for (i=0; mate_dirs[i]; i++) {
                        g_string_append (mate_od_source_dir,
                                         mate_dirs[i]);
                        g_string_append (mate_od_source_dir,
                                         "/lib/matecomponent/servers" G_SEARCHPATH_SEPARATOR_S);
                }
                g_strfreev (mate_dirs);
                g_string_append (real_od_source_dir,
                                 mate_od_source_dir->str);
                g_string_append_c (real_od_source_dir, G_SEARCHPATH_SEPARATOR);
		g_string_free (mate_od_source_dir, TRUE);
        }

        g_string_append (real_od_source_dir, SERVERINFODIR);

        return real_od_source_dir;
}

#ifdef G_OS_WIN32

static void
wait_console_key (void)
{
        SetConsoleTitle ("MateComponent activation server exiting. Type any character to close this window.");
        printf ("\n"
                "(MateComponent activation server exiting. Type any character to close this window)\n");
        _getch ();
}

#endif

static void
log_handler (const gchar *log_domain,
             GLogLevelFlags log_level,
             const gchar *message,
             gpointer user_data)
{
#ifdef HAVE_SYSLOG_H
	int syslog_priority;
#endif
	gchar *converted_message;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	if (log_level & G_LOG_LEVEL_DEBUG && !output_debug)
		return;
#else
	if (log_level & G_LOG_LEVEL_DEBUG)
		return;
#endif

	converted_message = g_locale_from_utf8 (message, -1, NULL, NULL, NULL);
	if (converted_message)
		message = converted_message;

	if (output_debug)
                fprintf (stderr, "%s%s", message,
                         (message[strlen (message) - 1] == '\n' ? "" : "\n"));

#ifdef HAVE_SYSLOG_H
	/* syslog uses reversed meaning of LEVEL_ERROR and LEVEL_CRITICAL */
	if (log_level & G_LOG_LEVEL_ERROR)
		syslog_priority = LOG_CRIT;
	else if (log_level & G_LOG_LEVEL_CRITICAL)
		syslog_priority = LOG_ERR;
	else if (log_level & G_LOG_LEVEL_WARNING)
		syslog_priority = LOG_WARNING;
	else if (log_level & G_LOG_LEVEL_MESSAGE)
		syslog_priority = LOG_NOTICE;
	else if (log_level & G_LOG_LEVEL_INFO)
		syslog_priority = LOG_INFO;
	else if (log_level & G_LOG_LEVEL_DEBUG)
		syslog_priority = LOG_DEBUG;
	else
		syslog_priority = LOG_NOTICE;

	syslog (syslog_priority, "%s", message);
#else
        if (!(log_level & G_LOG_FLAG_FATAL))
                fprintf (stderr, "%s%s", message,
                         (message[strlen (message) - 1] == '\n' ? "" : "\n"));
#endif

	if (log_level & G_LOG_FLAG_FATAL) {
		fprintf (stderr, "%s%s", message,
                         (message[strlen (message) - 1] == '\n' ? "" : "\n"));
#ifdef G_OS_WIN32
                if (allocated_new_console)
                        wait_console_key ();
#endif
		_exit (1);
	}

	g_free (converted_message);
}

static int
redirect_output (int ior_fd)
{
        int dev_null_fd = -1;

	if (output_debug)
		return dev_null_fd;

#ifdef G_OS_WIN32
	dev_null_fd = open ("NUL:", O_RDWR);
#else
	dev_null_fd = open ("/dev/null", O_RDWR);
#endif
	if (ior_fd != 0)
		dup2 (dev_null_fd, 0);
	if (ior_fd != 1)
		dup2 (dev_null_fd, 1);
	if (ior_fd != 2)
		dup2 (dev_null_fd, 2);

        return dev_null_fd;
}

static void
nameserver_destroy (PortableServer_POA  poa,
                    const CORBA_Object  reference,
                    CORBA_Environment  *ev)
{
        PortableServer_ObjectId *oid;

        oid = PortableServer_POA_reference_to_id (poa, reference, ev);
	PortableServer_POA_deactivate_object (poa, oid, ev);
	CORBA_free (oid);
}

static void
dump_ior (CORBA_ORB orb, int dev_null_fd, CORBA_Environment *ev)
{
        char *ior;
        FILE *fh;

	ior = CORBA_ORB_object_to_string (orb, activation_context_get (), ev);

	fh = NULL;
	if (ior_fd >= 0)
		fh = fdopen (ior_fd, "w");
	if (fh) {
		fprintf (fh, "%s\n", ior);
		fclose (fh);
		if (ior_fd <= 2)
                        dup2 (dev_null_fd, ior_fd);
	} else {
		printf ("%s\n", ior);
		fflush (stdout);
	}
        if (dev_null_fd != -1)
                close (dev_null_fd);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	debug_queries ();
#endif
        if (server_reg) {
                char *fname;
                fname = _matecomponent_activation_ior_fname_get ();
                fh = g_fopen (fname, "w+");
		fprintf (fh, "%s\n", ior);
		fclose (fh);
                g_free (fname);
        }

	CORBA_free (ior);
}

static void
cleanup_ior_and_lock_files (void)
{
    char *fname;

    fname = _matecomponent_activation_ior_fname_get ();
    g_unlink (fname);
    g_free (fname);

    fname = _matecomponent_activation_lock_fname_get ();
    g_unlink (fname);
    g_free (fname);
}

#ifdef HAVE_DBUS

static void
session_bus_closed_cb (GDBusConnection *conection,
                       gboolean         remote_peer_vanished,
                       GError          *error,
                       gpointer         user_data)
{
        GMainLoop *loop = (GMainLoop *) user_data;

        if (g_main_loop_is_running (loop))
                g_main_loop_quit (loop);
}

#endif

#ifdef G_OS_WIN32

static void
ensure_output_visible (gboolean asked_for_it)
{
        if (fileno (stdout) != -1 &&
            _get_osfhandle (fileno (stdout)) != -1)
                return;

        typedef BOOL (* WINAPI AttachConsole_t) (DWORD);

        AttachConsole_t p_AttachConsole =
                (AttachConsole_t) GetProcAddress (GetModuleHandle ("kernel32.dll"), "AttachConsole");

        if (p_AttachConsole != NULL) {
                if (!AttachConsole (ATTACH_PARENT_PROCESS)) {
                        if (AllocConsole ())
                                allocated_new_console = TRUE;
                }

                freopen ("CONOUT$", "w", stdout);
                dup2 (fileno (stdout), 1);
                freopen ("CONOUT$", "w", stderr);
                dup2 (fileno (stderr), 2);

                if (allocated_new_console) {
                        SetConsoleTitle ("MateComponent activation server output. You can minimize this window, but don't close it.");
                        if (asked_for_it)
                                printf ("You asked for debugging output by setting the MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT\n"
                                        "environment variable, so here it is.\n"
                                        "\n");
                        atexit (wait_console_key);
                }
        }
}

#endif

int
main (int argc, char *argv[])
{
        PortableServer_POA            threaded_poa;
        CORBA_ORB                     orb;
        CORBA_Environment             real_ev, *ev;
        CORBA_Object                  naming_service, existing;
        MateComponent_ActivationEnvironment  environment;
        MateComponent_ObjectDirectory        od;
	MateComponent_EventSource            event_source;
        GOptionContext               *ctx;
        int                           dev_null_fd;
#ifdef HAVE_SIGACTION
        struct sigaction              sa;
#endif
        GString                      *src_dir;
#ifdef HAVE_SYSLOG_H
	gchar                        *syslog_ident;
#endif
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	const gchar                  *debug_output_env;
#endif
	GError                       *error = NULL;
#ifdef HAVE_DBUS
        GDBusConnection              *connection;
#endif

#ifdef HAVE_SETSID
        /*
         *    Become process group leader, detach from controlling
         * terminal, etc.
         */
        setsid ();
#endif
	/* internationalization. */
	setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, SERVER_LOCALEDIR);
        textdomain (GETTEXT_PACKAGE);

#ifdef SIGPIPE
        /* Ignore sig-pipe - as normal */
#ifdef HAVE_SIGACTION
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &sa, NULL);
#else
        signal (SIGPIPE, SIG_IGN);
#endif
#endif

        g_thread_init (NULL);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	debug_output_env = g_getenv ("MATECOMPONENT_ACTIVATION_DEBUG_OUTPUT");
	if (debug_output_env && debug_output_env[0] != '\0') {
		output_debug = TRUE;
#ifdef G_OS_WIN32
                ensure_output_visible (TRUE);
#endif
        }
#endif

	g_set_prgname ("matecomponent-activation-server");
	ctx = g_option_context_new (NULL);
	g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
	if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_printerr (_("Run '%s --help' to see a full list of available command line options.\n"), g_get_prgname ());
		exit(1);
	}
	g_option_context_free (ctx);

        LIBXML_TEST_VERSION;
	xmlKeepBlanksDefault(0);

#ifdef HAVE_SYSLOG_H
	syslog_ident = g_strdup_printf ("matecomponent-activation-server (%s-%u)", g_get_user_name (), (guint) getpid ());

	/* openlog does not copy ident string, so we free it on shutdown */
	openlog (syslog_ident, 0, LOG_USER);
#endif

        if (server_reg)
                output_debug = TRUE;

        if (!output_debug) {
                g_log_set_fatal_mask (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
                g_log_set_handler (G_LOG_DOMAIN,
                                   G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                                   log_handler,
                                   NULL);
        }

        dev_null_fd = redirect_output (ior_fd);

        if (!matecomponent_init (&argc, argv))
                g_warning ("Failed to initialize libmatecomponent");
	orb = matecomponent_activation_orb_get ();
	main_loop = g_main_loop_new (NULL, FALSE);

	threaded_poa = server_get_poa ();

        server_lock ();

        add_initial_locales ();
        
	CORBA_exception_init ((ev = &real_ev));


        src_dir = build_src_dir ();
        matecomponent_object_directory_init (threaded_poa, src_dir->str, ev);
        g_string_free (src_dir, TRUE);

        od = matecomponent_object_directory_get ();
	event_source = matecomponent_object_directory_event_source_get();

	memset (&environment, 0, sizeof (MateComponent_ActivationEnvironment));

        matecomponent_activate (); /* activate the ORB */

        /* A non-theading poa - naming almost entirely unused */
        naming_service = impl_CosNaming_NamingContext__create (matecomponent_poa(), ev);
        if (ev->_major != CORBA_NO_EXCEPTION || naming_service == NULL)
                g_warning ("Failed to create naming service");
        CORBA_exception_init (ev);

        MateComponent_ObjectDirectory_register_new 
                (od, NAMING_CONTEXT_IID, &environment, naming_service,
                 0, "", &existing, ev);
        g_assert (ev->_major == CORBA_NO_EXCEPTION);

	if (existing != CORBA_OBJECT_NIL)
		CORBA_Object_release (existing, NULL);

        MateComponent_ObjectDirectory_register_new 
                (od, EVENT_SOURCE_IID, &environment, event_source,
                 0, "", &existing, ev);
        g_assert (ev->_major == CORBA_NO_EXCEPTION);
        
	if (existing != CORBA_OBJECT_NIL)
		CORBA_Object_release (existing, NULL);

        if (ior_fd < 0 && !server_ac && !server_reg) {
#ifdef G_OS_WIN32
                ensure_output_visible (FALSE);
#endif
                g_critical ("\n\n-- \nThe matecomponent-activation-server must be started by\n"
                            "libmatecomponent-activation, and cannot be run by itself.\n"
                            "This is due to us doing client side locking.\n-- \n");
        }

        if (server_reg) {
                g_warning ("Running in user-forked debugging mode");
                server_ac = 1;
        }
        
        /*
         *     It is no longer useful at all to be a pure
         * ObjectDirectory we have binned that mode of
         * operation, as a bad, racy and inefficient job.
         */
        g_assert (server_ac);
        
        activation_context_setup (threaded_poa, od, ev);

        dump_ior (orb, dev_null_fd, ev);

	od_finished_internal_registration (); 

#ifdef HAVE_DBUS
        connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
        if (connection == NULL) {
                g_warning ("could not associate with desktop session: %s",
                           error->message);
                g_error_free (error);
        } else {
                g_signal_connect (connection, "closed",
                                  G_CALLBACK (session_bus_closed_cb), main_loop);
                g_dbus_connection_set_exit_on_close (connection, FALSE);
        }
#endif
        if (getenv ("MATECOMPONENT_ACTIVATION_DEBUG") == NULL)
                chdir ("/");

        server_unlock ();

	g_main_loop_run (main_loop);

        server_lock ();

        nameserver_destroy (matecomponent_poa(), naming_service, ev);
        CORBA_Object_release (naming_service, ev);

        matecomponent_object_directory_shutdown (threaded_poa, ev);
        activation_context_shutdown ();

        cleanup_ior_and_lock_files ();

#ifdef HAVE_SYSLOG_H
	closelog ();
	g_free (syslog_ident);
#endif
        server_unlock ();

	return !matecomponent_debug_shutdown ();
}

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
static void
debug_queries (void)
{
	if (ac_evaluate) {
		QueryExpr *exp;
		const char *err;
		QueryContext tmpctx = { NULL, 0, CORBA_OBJECT_NIL };

		err = qexp_parse (ac_evaluate, &exp);
		if (err) {
			g_print ("Parse error: %s\n", err);
		} else {
			QueryExprConst res;

			qexp_dump (exp);
			g_print ("\n");
			g_print ("Evaluation with no server record is: ");
			res = qexp_evaluate (NULL, exp, &tmpctx);
			qexp_constant_dump (&res);
			g_print ("\n");
		}
	}
}
#endif
