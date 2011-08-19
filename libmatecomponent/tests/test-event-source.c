#include <config.h>
#include <stdio.h>
#include <string.h>
#include <libmatecomponent.h>

static int idle_id, em_count, ev_count;

#define CHECK_RESULT(ev, evc, emc) {g_assert (!MATECOMPONENT_EX (ev)); g_assert (evc == ev_count); g_assert (emc == em_count);}

static void
event_cb (MateComponentListener    *listener,
	  const char        *event_name, 
	  const CORBA_any   *any,
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	if (!strcmp (event_name, user_data))
		em_count++;
	ev_count++;

	printf ("Got Event %s %s %d %d\n",
		event_name, (char *)user_data, 
		ev_count, em_count);
}

static void
run_tests (void)
{
	MateComponentEventSource *es;
	CORBA_Environment ev;
	CORBA_any *value;
	char *mask;

	CORBA_exception_init (&ev);

	g_source_remove (idle_id);

	value = matecomponent_arg_new (MATECOMPONENT_ARG_LONG);

	es = matecomponent_event_source_new ();
	g_assert (es != NULL);
	
	mask = "a/test";
	matecomponent_event_source_client_add_listener (MATECOMPONENT_OBJREF (es), event_cb, mask, &ev, mask);

	mask = "=a/test";
	matecomponent_event_source_client_add_listener (MATECOMPONENT_OBJREF (es), event_cb, mask, &ev, mask);
	
	matecomponent_event_source_notify_listeners (es, "a/test", value, &ev); 
	CHECK_RESULT (&ev, 2, 1);

	matecomponent_event_source_notify_listeners (es, "a/test/xyz", value, &ev); 
	CHECK_RESULT (&ev, 3, 1);

	matecomponent_event_source_notify_listeners (es, "a/tes", value, &ev); 
	CHECK_RESULT (&ev, 3, 1);

	matecomponent_event_source_notify_listeners (es, "test", value, &ev); 
	CHECK_RESULT (&ev, 3, 1);

	matecomponent_event_source_notify_listeners (es, "a/test", value, &ev); 
	CHECK_RESULT (&ev, 5, 2);
	
	matecomponent_event_source_notify_listeners (es, "a/test:", value, &ev); 
	CHECK_RESULT (&ev, 6, 2);

	matecomponent_event_source_notify_listeners (es, "a/test:xyz", value, &ev); 
	CHECK_RESULT (&ev, 7, 2);

	matecomponent_event_source_notify_listeners (es, "a/", value, &ev); 
	CHECK_RESULT (&ev, 7, 2);

	matecomponent_event_source_notify_listeners (es, "a/test1:xyz", value, &ev); 
	CHECK_RESULT (&ev, 8, 2);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (es));

	matecomponent_main_quit ();
}

int
main (int argc, char **argv)
{
        g_thread_init (NULL);

	if (!matecomponent_init (&argc, argv))
		g_error ("Cannot init matecomponent");
	
	idle_id = g_idle_add ((GSourceFunc) run_tests, NULL);

	matecomponent_main ();

	return matecomponent_debug_shutdown ();
}
