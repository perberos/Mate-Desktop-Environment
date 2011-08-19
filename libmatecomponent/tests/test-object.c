/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmatecomponent.h>
#include <matecorba/poa/poa.h>

#define GENERAL_ERROR_MESSAGE "Hello World"

static int
ret_ex_test (CORBA_Environment *ev)
{
	MATECOMPONENT_RET_VAL_EX (ev, 1);

	return 0;
}

static void
ex_test (CORBA_Environment *ev)
{
	MATECOMPONENT_RET_EX (ev);
}

static int signal_emitted = 0;

static void
system_exception_cb (MateComponentObject      *object,
		     CORBA_Object       cobject,
		     CORBA_Environment *ev,
		     gpointer           user_data)
{
	g_assert (MATECOMPONENT_IS_OBJECT (object));
	g_assert ((MateComponentObject *)user_data == object);

	g_assert (ev != NULL);
	g_assert (ev->_major == CORBA_SYSTEM_EXCEPTION);
	g_assert (!strcmp (MATECOMPONENT_EX_REPOID (ev),
			   ex_CORBA_COMM_FAILURE));

	signal_emitted = 1;
}

int
main (int argc, char *argv [])
{
	MateComponentObject     *object;
	MateComponent_Unknown    ref;
	CORBA_Environment *ev, real_ev;

        g_thread_init (NULL);

	free (malloc (8));

	if (matecomponent_init (&argc, argv) == FALSE)
		g_error ("Can not matecomponent_init");
	matecomponent_activate ();

	ev = &real_ev;
	CORBA_exception_init (ev);

	fprintf (stderr, "Local lifecycle\n");
	{
		object = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_moniker_get_type (), NULL));

		g_assert (matecomponent_object_ref (object) == object);
		g_assert (matecomponent_object_unref (MATECOMPONENT_OBJECT (object)) == NULL);

		matecomponent_object_unref (MATECOMPONENT_OBJECT (object));
	}

	fprintf (stderr, "In-proc lifecycle\n");
	{
		object = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_moniker_get_type (), NULL));

		ref = CORBA_Object_duplicate (MATECOMPONENT_OBJREF (object), NULL);

		matecomponent_object_release_unref (ref, NULL);
	}

	fprintf (stderr, "Query interface\n");
	{
		MateComponentObject *a, *b;

		a = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_moniker_get_type (), NULL));
		b = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_stream_mem_get_type (), NULL));

		matecomponent_object_add_interface (a, b);

		fprintf (stderr, "  invalid interface\n");
		object = matecomponent_object_query_local_interface (
			a, "IDL:This/Is/Not/There:1.0");
		g_assert (object == CORBA_OBJECT_NIL);

		fprintf (stderr, "  symmetry\n");
		object = matecomponent_object_query_local_interface (
			a, "IDL:MateComponent/Stream:1.0");
		g_assert (object == b);
		matecomponent_object_unref (object);

		object = matecomponent_object_query_local_interface (
			b, "IDL:MateComponent/Stream:1.0");
		g_assert (object == b);
		matecomponent_object_unref (object);

		object = matecomponent_object_query_local_interface (
			a, "IDL:MateComponent/Moniker:1.0");
		g_assert (object == a);
		matecomponent_object_unref (object);

		object = matecomponent_object_query_local_interface (
			b, "IDL:MateComponent/Moniker:1.0");
		g_assert (object == a);
		matecomponent_object_unref (object);

		fprintf (stderr, "  remote\n");
		ref = MateComponent_Unknown_queryInterface (
			MATECOMPONENT_OBJREF (a), "IDL:Broken/1.0", ev);
		g_assert (!MATECOMPONENT_EX (ev));
		g_assert (ref == CORBA_OBJECT_NIL);

		ref = MateComponent_Unknown_queryInterface (
			MATECOMPONENT_OBJREF (a), "IDL:MateComponent/Stream:1.0", ev);
		g_assert (!MATECOMPONENT_EX (ev));
		g_assert (ref == MATECOMPONENT_OBJREF (b));
		matecomponent_object_release_unref (ref, ev);
		g_assert (!MATECOMPONENT_EX (ev));

		matecomponent_object_unref (a);
	}

	fprintf (stderr, "Environment exception checks\n");
	{
		object = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_moniker_get_type (), NULL));

		g_signal_connect (G_OBJECT (object),
				  "system_exception",
				  G_CALLBACK (system_exception_cb),
				  object);

		CORBA_exception_set_system (
			ev, ex_CORBA_COMM_FAILURE,
			CORBA_COMPLETED_MAYBE);
		g_assert (MATECOMPONENT_EX (ev));

		signal_emitted = 0;
		MATECOMPONENT_OBJECT_CHECK (
			object, MATECOMPONENT_OBJREF (object), ev);
		g_assert (signal_emitted);

		CORBA_exception_free (ev);

		matecomponent_object_unref (object);
	}

	fprintf (stderr, "Servant mapping...\n");
	{
		PortableServer_Servant servant;

		object = MATECOMPONENT_OBJECT (g_object_new (
			matecomponent_moniker_get_type (), NULL));

		servant = (PortableServer_Servant) &object->servant;

		g_assert (matecomponent_object (object) == object);
		g_assert (matecomponent_object (&object->servant) == object);
		g_assert (matecomponent_object_get_servant (object) == servant);
		g_assert (matecomponent_object_from_servant (servant) == object);
		g_assert (matecomponent_object_fast (object) == object);
		g_assert (matecomponent_object_fast (servant) == object);

		matecomponent_object_unref (object);
	}

	fprintf (stderr, "Ret-ex tests...\n");
	{
		g_assert (!ret_ex_test (ev));
		ex_test (ev);

		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_PropertyBag_NotFound, NULL);
		g_assert (ret_ex_test (ev));
		
		CORBA_exception_free (ev);
	}

	fprintf (stderr, "General error tests...\n");
	{
		matecomponent_exception_general_error_set (
			ev, NULL, "a%s exception occurred", "n exceptional");
		g_assert (MATECOMPONENT_EX (ev));
		g_assert (!strcmp (MATECOMPONENT_EX_REPOID (ev), ex_MateComponent_GeneralError));
		g_assert (!strcmp (matecomponent_exception_get_text (ev),
				   "an exceptional exception occurred"));
	}

	fprintf (stderr, "All tests passed\n");

	return matecomponent_debug_shutdown ();
}
