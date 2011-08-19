#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libmatecomponent.h>

#define NUM_THREADS 8
#define NUM_GETS    8

#define PROP_IN_MAIN   1
#define PROP_NON_MAIN  2

static GThread *main_thread = NULL;

static void
get_fn (MateComponentPropertyBag *bag,
	MateComponentArg         *arg,
	guint              arg_id,
	CORBA_Environment *ev,
	gpointer           user_data)
{
	fprintf (stderr, "Check property %u\n", arg_id);
	if (arg_id == PROP_IN_MAIN)
		g_assert (g_thread_self () == main_thread);
	else
		g_assert (g_thread_self () != main_thread);
	MATECOMPONENT_ARG_SET_BOOLEAN (arg, TRUE);
}

typedef struct {
	const char *prop;
	gboolean    value;
	MateComponent_PropertyBag pb;
} TestClosure;

static void
test_prop (TestClosure *tc, CORBA_Environment *ev)
{
	g_assert (matecomponent_pbclient_get_boolean (tc->pb, tc->prop, ev) == tc->value);
	g_assert (!MATECOMPONENT_EX (ev));
}

static long running_threads;
G_LOCK_DEFINE_STATIC (running_threads);

static gpointer
test_thread (gpointer data)
{
	int i;
	CORBA_Environment ev[1];
	TestClosure *tc = data;

	CORBA_exception_init (ev);

	for (i = 0; i < NUM_GETS; i++)
		test_prop (tc, ev);

	G_LOCK (running_threads);
	running_threads--;
	G_UNLOCK (running_threads);

	CORBA_exception_free (ev);

	return data;
}

static gboolean
wakeup_fn (gpointer data)
{
	return TRUE;
}

static void
test_threads (TestClosure *tc)
{
	int i;
	guint wakeup;
	GThread *threads [NUM_THREADS];

	running_threads = NUM_THREADS;

	for (i = 0; i < NUM_THREADS; i++)
		threads [i] = g_thread_create (test_thread, tc, TRUE, NULL);
	
	wakeup = g_timeout_add (100, wakeup_fn, NULL);

	while (1) {
		G_LOCK (running_threads);
		if (running_threads == 0) {
			G_UNLOCK (running_threads);
			break;
		}
		G_UNLOCK (running_threads);
		g_main_context_iteration (NULL, TRUE);
	}

	g_source_remove (wakeup);
	
	for (i = 0; i < NUM_THREADS; i++) {
		if (!(g_thread_join (threads [i]) == tc))
			g_error ("Wierd thread join problem '%d'", i);
	}
}

int
main (int argc, char *argv [])
{
	CORBA_Environment  ev[1];
	MateComponentPropertyBag *pb;
	PortableServer_POA poa;
	TestClosure        tc;

	free (malloc (8));

	CORBA_exception_init (ev);

	if (matecomponent_init (&argc, argv) == FALSE)
		g_error ("Can not matecomponent_init");
	matecomponent_activate ();

	main_thread = g_thread_self ();

	{
		poa = matecomponent_poa_get_threaded (MATECORBA_THREAD_HINT_PER_REQUEST);
		pb = g_object_new (MATECOMPONENT_TYPE_PROPERTY_BAG,
				   "poa", poa, NULL);
		matecomponent_property_bag_construct
			(pb, g_cclosure_new (G_CALLBACK (get_fn), NULL, NULL), NULL,
			 matecomponent_event_source_new ());
		matecomponent_property_bag_add (pb, "non_main", PROP_NON_MAIN, TC_CORBA_boolean,
					 NULL, "non_main", MATECOMPONENT_PROPERTY_READABLE);
		tc.prop  = "non_main";
		tc.value = TRUE;
		tc.pb    = MATECOMPONENT_OBJREF (pb);

		test_threads (&tc);
		matecomponent_object_unref (pb);
	}	

	{
		pb = matecomponent_property_bag_new (get_fn, NULL, NULL);
		matecomponent_property_bag_add (pb, "in_main", PROP_IN_MAIN, TC_CORBA_boolean,
					 NULL, "in_main", MATECOMPONENT_PROPERTY_READABLE);
		
		tc.prop  = "in_main";
		tc.value = TRUE;
		tc.pb    = MATECOMPONENT_OBJREF (pb);
		
		test_prop (&tc, ev);
		test_threads (&tc);

		matecomponent_object_unref (pb);
	}

	CORBA_exception_free (ev);

	return matecomponent_debug_shutdown ();
}
