/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-main.c: MateComponent Main
 *
 * Author:
 *    Miguel de Icaza  (miguel@gnu.org)
 *    Nat Friedman     (nat@nat.org)
 *    Peter Wainwright (prw@wainpr.demo.co.uk)
 *    Michael Meeks    (michael@ximian.com)
 *
 * Copyright 1999,2003 Ximian, Inc.
 */
#include <config.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-context.h>
#include <matecomponent/matecomponent-private.h>
#include <matecomponent/matecomponent-arg.h>

#ifdef G_OS_WIN32
#include "matecomponent-activation/matecomponent-activation-private.h"
#endif

#include <libintl.h>
#include <string.h>

#include <glib.h>

#ifdef HAVE_GTHREADS
GMutex                   *_matecomponent_lock;
#endif
static CORBA_ORB                 __matecomponent_orb = CORBA_OBJECT_NIL;
static PortableServer_POA        __matecomponent_poa = CORBA_OBJECT_NIL;
static PortableServer_POAManager __matecomponent_poa_manager = CORBA_OBJECT_NIL;

static guint              matecomponent_main_loop_level = 0;
static GSList *           matecomponent_main_loops = NULL;

#ifdef G_OS_WIN32

#undef MATECOMPONENT_LOCALEDIR
#define MATECOMPONENT_LOCALEDIR _matecomponent_activation_win32_get_localedir ()

#endif

/**
 * matecomponent_orb:
 *
 * Returns: The ORB used for this MateComponent application.  The ORB
 * is created in matecomponent_init().
 */
CORBA_ORB
matecomponent_orb (void)
{
	return __matecomponent_orb;
}

/**
 * matecomponent_poa:
 *
 * Returns: The POA used for this MateComponent application.  The POA
 * is created when matecomponent_init() is called.
 */
PortableServer_POA
matecomponent_poa (void)
{
	return __matecomponent_poa;
}

/**
 * matecomponent_poa_manager:
 *
 * Returns: The POA Manager used for this MateComponent application.  The POA
 * Manager is created when matecomponent_init() is called, but it is not
 * activated until matecomponent_main() is called.
 */
PortableServer_POAManager
matecomponent_poa_manager (void)
{
	return __matecomponent_poa_manager;
}

static gint matecomponent_inited = 0;

/**
 * matecomponent_is_initialized:
 * 
 *   This allows you to protect against double
 * initialization in your code.
 * 
 * Return value: whether the ORB is initialized
 **/
gboolean
matecomponent_is_initialized (void)
{
	return (matecomponent_inited > 0);
}

/**
 * matecomponent_debug_shutdown:
 * 
 *   This shuts down the ORB and any other matecomponent related
 * resources.
 * 
 * Return value: whether the shutdown was clean, a good
 * value to return from 'main'.
 **/
int
matecomponent_debug_shutdown (void)
{
	int retval = 0;

	if (matecomponent_inited == 1) {
		CORBA_Environment ev;

		matecomponent_inited--;

		CORBA_exception_init (&ev);

		matecomponent_property_bag_shutdown ();
		matecomponent_running_context_shutdown ();
		matecomponent_context_shutdown ();
		if (matecomponent_object_shutdown ())
			retval = 1;
		matecomponent_exception_shutdown ();

		if (__matecomponent_poa != CORBA_OBJECT_NIL)
			CORBA_Object_release (
				(CORBA_Object) __matecomponent_poa, &ev);
		__matecomponent_poa = CORBA_OBJECT_NIL;

		if (__matecomponent_poa_manager != CORBA_OBJECT_NIL)
			CORBA_Object_release (
				(CORBA_Object) __matecomponent_poa_manager, &ev);
		__matecomponent_poa_manager = CORBA_OBJECT_NIL;

		if (!matecomponent_activation_debug_shutdown ())
			retval = 1;

		__matecomponent_orb = CORBA_OBJECT_NIL;
	} else if (matecomponent_inited > 1) {
		matecomponent_inited--;		
	} else /* shutdown when we didn't need to error */
		retval = 1;

	return retval;
}

/**
 * matecomponent_init_full:
 * @argc: a pointer to the number of arguments
 * @argv: the array of arguments
 * @opt_orb: the ORB in which we run
 * @opt_poa: optional, a POA
 * @opt_manager: optional, a POA Manager
 *
 * Initializes the matecomponent document model.  It requires at least
 * the value for @orb.  If @poa is CORBA_OBJECT_NIL, then the
 * RootPOA will be used, in this case @manager should be CORBA_OBJECT_NIL.
 *
 * Returns %TRUE on success, or %FALSE on failure.
 */
gboolean
matecomponent_init_full (int *argc, char **argv,
		  CORBA_ORB opt_orb, PortableServer_POA opt_poa,
		  PortableServer_POAManager opt_manager)
{
	CORBA_Environment ev;

	matecomponent_activation_init (argc ? *argc : 0, argv);

	matecomponent_inited++;
	if (matecomponent_inited > 1)
		return TRUE;

	/* Init neccessary bits */
	g_type_init_with_debug_flags (0);

	matecomponent_arg_init ();

	CORBA_exception_init (&ev);

	/*
	 * Create the POA.
	 */
	if (opt_orb == CORBA_OBJECT_NIL) {
		opt_orb = matecomponent_activation_orb_get ();
		if (opt_orb == CORBA_OBJECT_NIL) {
			g_warning ("Can not resolve initial reference to ORB");
			CORBA_exception_free (&ev);
			return FALSE;
		}
	}
	
	if (opt_poa == CORBA_OBJECT_NIL) {
		opt_poa = (PortableServer_POA)
			CORBA_ORB_resolve_initial_references (
				opt_orb, "RootPOA", &ev);
		if (MATECOMPONENT_EX (&ev)) {
			g_warning ("Can not resolve initial reference to RootPOA");
			CORBA_exception_free (&ev);
			return FALSE;
		}
		
	}

	/*
	 * Create the POA Manager.
	 */
	if (opt_manager == CORBA_OBJECT_NIL) {
		opt_manager = PortableServer_POA__get_the_POAManager (opt_poa, &ev);
		if (MATECOMPONENT_EX (&ev)){
			g_warning ("Can not get the POA manager");
			CORBA_exception_free (&ev);
			return FALSE;
		}
	}

	/*
	 * Store global copies of these which can be retrieved with
	 * matecomponent_orb()/matecomponent_poa()/matecomponent_poa_manager().
	 */
	__matecomponent_orb = opt_orb;
	__matecomponent_poa = opt_poa;
	__matecomponent_poa_manager = opt_manager;

	CORBA_exception_free (&ev);

#ifdef HAVE_GTHREADS
	_matecomponent_lock = g_mutex_new ();
#endif

	matecomponent_context_init ();

	bindtextdomain (GETTEXT_PACKAGE, MATECOMPONENT_LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

	return TRUE;
}

/**
 * matecomponent_init:
 * @argc: a pointer to the number of arguments or %NULL
 * @argv: the array of arguments or %NULL
 *
 * Initializes the matecomponent component model.
 *
 * Returns %TRUE on success, or %FALSE on failure.
 */
gboolean
matecomponent_init (int *argc, char **argv)
{
	return matecomponent_init_full (
		argc, argv, NULL, NULL, NULL);
}

/**
 * matecomponent_activate:
 *
 * Activates the MateComponent POA manager registered by matecomponent_init.
 * This should be called at the end of application initialization.
 * You do not need to call this function if you use matecomponent_main().
 * 
 * Returns %TRUE on success, or %FALSE on failure.
 */
gboolean
matecomponent_activate (void)
{
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	if (!__matecomponent_poa_manager) {
		g_warning ("Tried to activate MateComponent before initializing");
		CORBA_exception_free (&ev);
		return FALSE;
	}
	PortableServer_POAManager_activate (__matecomponent_poa_manager, &ev);
	if (MATECOMPONENT_EX (&ev)){
		g_warning ("Failed to activate the MateComponent POA manager");
		CORBA_exception_free (&ev);
		return FALSE;
	}

	CORBA_exception_free (&ev);
	
	return TRUE;
}

/**
 * matecomponent_main:
 * 
 * Activates the MateComponent POA Manager and enters the main event loop.
 */
void
matecomponent_main (void)
{
	GMainLoop *loop;

	matecomponent_activate ();

	matecomponent_main_loop_level++;
  
	loop = g_main_loop_new (NULL, TRUE);
	matecomponent_main_loops = g_slist_prepend (matecomponent_main_loops, loop);

	if (g_main_loop_is_running (matecomponent_main_loops->data))
		g_main_loop_run (loop);

	matecomponent_main_loops = g_slist_remove (matecomponent_main_loops, loop);

	g_main_loop_unref (loop);

	matecomponent_main_loop_level--;
}

/**
 * matecomponent_main_level:
 *
 * Determines the number of times the matecomponent main loop has been entered (minus
 * the number of exits from the main loop).
 *
 * Returns: The number of main loops currently running (0 if no main loops are
 * running).
 */
guint
matecomponent_main_level (void)
{
	return matecomponent_main_loop_level;
}

/**
 * matecomponent_main_quit:
 * 
 * Quits the main event loop.
 */
void
matecomponent_main_quit (void)
{
	g_return_if_fail (matecomponent_main_loops != NULL);

	g_main_loop_quit (matecomponent_main_loops->data);
}

/**
 * matecomponent_poa_get_threadedv:
 * @hint: the desired thread hint
 * @args: va_args related to that hint
 * 
 * Get a predefined POA for a given threading policy/hint.  The
 * returned POA can be passed as the "poa" constructor property of a
 * #MateComponentOject.
 * 
 * Return value: the requested POA.
 **/
PortableServer_POA
matecomponent_poa_get_threadedv (MateCORBAThreadHint hint,
			  va_list         args)
{
	PortableServer_POA  poa;
	CORBA_Environment   ev[1];
	CORBA_PolicyList    policies;
	CORBA_Object        policy_vals[1];
	const char         *poa_name = NULL;

#define MAP(a,b) \
	case MATECORBA_THREAD_HINT_##a: \
		poa_name = b; \
		break

	switch (hint) {
		MAP (NONE,           "MateComponentPOAHintNone");
		MAP (PER_OBJECT,     "MateComponentPOAHintPerObject");
		MAP (PER_REQUEST,    "MateComponentPOAHintPerRequest");
		MAP (PER_POA,        "MateComponentPOAHintPerPOA");
		MAP (PER_CONNECTION, "MateComponentPOAHintPerConnection");
		MAP (ONEWAY_AT_IDLE, "MateComponentPOAHintOnewayAtIdle");
		MAP (ALL_AT_IDLE,    "MateComponentPOAHintAllAtIdle");
		MAP (ON_CONTEXT,     "MateComponentPOAHintOnContext");
#undef MAP
	default:
		g_assert_not_reached();
	}

	CORBA_exception_init (ev);

	/* (Copy-paste from MateCORBA2/test/poa/poatest-basic08.c) */

	policies._length = 1;
	policies._buffer = policy_vals;
	policies._buffer[0] = (CORBA_Object)
		PortableServer_POA_create_thread_policy
			(matecomponent_poa (),
			 PortableServer_ORB_CTRL_MODEL,
			 ev);

	poa = matecomponent_poa_new_from (__matecomponent_poa,
				   poa_name, &policies, ev);

	CORBA_Object_release (policies._buffer[0], NULL);

	if (ev->_major == CORBA_NO_EXCEPTION)
		MateCORBA_ObjectAdaptor_set_thread_hintv
			((MateCORBA_ObjectAdaptor) poa, hint, args);

	else {
		if (strcmp (CORBA_exception_id (ev),
			    ex_PortableServer_POA_AdapterAlreadyExists) == 0) {
			CORBA_exception_free (ev);
			poa = PortableServer_POA_find_POA (matecomponent_poa (),
							   poa_name,
							   CORBA_FALSE, ev);
		}
	}
	CORBA_exception_free (ev);
	if (!poa)
		g_warning ("Could not create/get poa '%s'", poa_name);

	return poa;
}

/**
 * matecomponent_poa_get_threaded:
 * @hint: the desired thread hint
 * 
 * Get a predefined POA for a given threading policy/hint.  The
 * returned POA can be passed as the "poa" constructor property of a
 * #MateComponentOject.
 * 
 * Return value: the requested POA.
 **/
PortableServer_POA
matecomponent_poa_get_threaded (MateCORBAThreadHint hint, ...)
{
	va_list args;
	PortableServer_POA poa;

	va_start (args, hint);
	poa = matecomponent_poa_get_threadedv (hint, args);
	va_end (args);

	return poa;
}

PortableServer_POA
matecomponent_poa_new_from (PortableServer_POA      tmpl,
		     const char             *name,
		     const CORBA_PolicyList *opt_policies,
		     CORBA_Environment      *opt_ev)
{
	PortableServer_POA poa;
	CORBA_Environment real_ev[1], *ev;

	if (!opt_ev)
		CORBA_exception_init ((ev = real_ev));
	else
		ev = opt_ev;

	poa = MateCORBA_POA_new_from (matecomponent_orb(),
				  tmpl, name, opt_policies, ev);

	if (!opt_ev)
		CORBA_exception_free (real_ev);

	return poa;
}
