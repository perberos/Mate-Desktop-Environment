/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <matecomponent-activation/matecomponent-activation.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <libmatecomponent.h>
#include "activation-server/server.h"

#include "empty.h"
#include "plugin.h"

#define TOTAL_TEST_SCORE 17

CORBA_Object name_service = CORBA_OBJECT_NIL;

static char *
matecomponent_activation_exception_id (CORBA_Environment *ev)
{
        if (ev->_major == CORBA_USER_EXCEPTION) {
                if (!strcmp (ev->_id, "IDL:MateComponent/GeneralError:1.0")) {
                        MateComponent_GeneralError *err = CORBA_exception_value (ev);
                        
                        if (!err || !err->description) {
                                return "No general exception error message";
                        } else {
                                return err->description;
                        }
                } else {
                        return ev->_id;
                }
        } else {
                return CORBA_exception_id (ev);
        }
}

static gboolean
test_matecomponent_activation_server (CORBA_Environment *ev, const char *type)
{
        CORBA_Object ns;

        ns = matecomponent_activation_name_service_get (ev);
        if (ev->_major != CORBA_NO_EXCEPTION) {
                g_warning ("Exception '%s' (%s) finding matecomponent_activation_server %s",
                           matecomponent_activation_exception_id (ev), ev->_id, type);
                return FALSE;
        }

        if (name_service != CORBA_OBJECT_NIL &&
            name_service != ns) {
                g_warning ("matecomponent_activation_server crashed %s", type);
                return FALSE;
        }

        if (name_service == CORBA_OBJECT_NIL)
                name_service = ns;
        else
                CORBA_Object_release (ns, ev);

        return TRUE;
}

static gboolean
test_object (CORBA_Object obj, CORBA_Environment *ev, const char *type)
{
	if (ev->_major != CORBA_NO_EXCEPTION) {
		g_warning ("Activation %s failed: %s\n", type,
			   matecomponent_activation_exception_id (ev));
        } else if (CORBA_Object_is_nil (obj, ev)) {
		g_warning ("Activation %s failed (returned NIL but no exception)!", type);
	} else {
                return TRUE;
        }

        if (!test_matecomponent_activation_server (ev, type)) {
                return FALSE;
        }

        return FALSE;
}

static int
test_plugin (CORBA_Object obj, CORBA_Environment *ev, const char *type)
{
	Plugin_doPluginTest (obj, ev);

	if (ev->_major != CORBA_NO_EXCEPTION) {
		g_warning ("Call failed: %s\n",
			   matecomponent_activation_exception_id (ev));
		return 0;
	} else {
		fprintf (stderr, "Test %s succeeded\n", type);
                CORBA_Object_release (obj, ev);
		return 1;
	}
}

static int
test_empty (CORBA_Object obj, CORBA_Environment *ev, const char *type)
{
        Empty_doNothing (obj, ev);

        if (ev->_major != CORBA_NO_EXCEPTION) {
                g_warning ("Call failed: %s\n",
                           matecomponent_activation_exception_id (ev));
                return 0;
        } else {
                fprintf (stderr, "Test %s succeeded\n", type);
                CORBA_Object_release (obj, ev);
                return 1;
        }
}

static int
idle_base_activation (gpointer user_data)
{
        /* This is a facile test, we always activate the
         * ActivationContext first and then get the OD from it */
        matecomponent_activation_activation_context_get ();

        return FALSE;
}

static void
race_base_init (void)
{
        g_idle_add (idle_base_activation, NULL);
        /* to race with the activation context get in the same process */
        matecomponent_activation_object_directory_get (NULL, NULL);
}

int passed = 0;
int failed = 0;
int async_done = 0;

static void
empty_activation_cb (CORBA_Object   obj,
                     const char    *error_reason, 
                     gpointer       user_data)
{
        CORBA_Environment ev;
	gboolean          ret = FALSE;
	char             *repo_id = user_data;

        CORBA_exception_init (&ev);

        if (error_reason)
                g_warning ("Async activation error activating '%s' : '%s'", repo_id, error_reason);

        else if (test_object (obj, &ev, "by async query"))
                ret = test_empty (obj, &ev, "by async query");

	if (ret) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: async activation\n", passed + failed, TOTAL_TEST_SCORE);
	} else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: async activation\n", passed + failed, TOTAL_TEST_SCORE);
	}

        CORBA_exception_free (&ev);

        async_done++;
}

static void
race_empty (CORBA_Environment *ev)
{
	matecomponent_activation_activate_async (
                "repo_ids.has('IDL:Empty2:1.0')", NULL,
                0, empty_activation_cb, "IDL:Empty2:1.0", ev);
        g_assert (ev->_major == CORBA_NO_EXCEPTION);

	matecomponent_activation_activate_async (
                "repo_ids.has('IDL:Empty:1.0')", NULL,
                0, empty_activation_cb, "IDL:Empty:1.0", ev);
        g_assert (ev->_major == CORBA_NO_EXCEPTION);

        while (async_done < 2)
                g_main_context_iteration (NULL, TRUE);
}

static void
listener_cb (MateComponentListener *listener,
             const char *event_name,
             const CORBA_any *any,
             CORBA_Environment *ev,
             gpointer user_data)
{
        g_message ("Activation callback successfully called on %s",
                   matecomponent_event_subtype (event_name));
}


int
main (int argc, char *argv[])
{
	CORBA_Object obj;
	CORBA_Environment ev;
        MateComponent_ServerInfoList *info;
        CORBA_Object ac;
        char *sort_by[4];
        char *query;
        int   i;
        GTimer *timer = g_timer_new ();
	MateComponent_EventSource event_source;
	
        g_thread_init (NULL);

	CORBA_exception_init (&ev);

        matecomponent_activation_object_directory_get (
                matecomponent_activation_username_get (),
                matecomponent_activation_hostname_get ());

	matecomponent_init (&argc, argv);
/*      putenv("MateComponent_BARRIER_INIT=1"); */


	event_source = matecomponent_activation_activate_from_id
                ("OAFIID:MateComponent_Activation_EventSource", 0, NULL, &ev);
        if (event_source != CORBA_OBJECT_NIL) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: Activation event source okay\n",
                         passed + failed, TOTAL_TEST_SCORE);
   	        matecomponent_event_source_client_add_listener (event_source, listener_cb,
                                                         "MateComponent/ObjectDirectory", &ev, NULL);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Activation event source not found\n",
                         passed + failed, TOTAL_TEST_SCORE);
	}


        race_base_init ();

        sort_by[0] = "prefer_by_list_order(iid, ["
                "'OAFIID:nautilus_file_manager_icon_view:42681b21-d5ca-4837-87d2-394d88ecc058',"
                "'OAFIID:nautilus_file_manager_list_view:521e489d-0662-4ad7-ac3a-832deabe111c',"
                "'OAFIID:nautilus_music_view:9456b5d2-60a8-407f-a56e-d561e1821391'])";
        sort_by[1] = "iid != 'OAFIID:nautilus_content_loser:95901458-c68b-43aa-aaca-870ced11062d'";
        sort_by[2] = "iid != 'OAFIID:nautilus_sample_content_view:45c746bc-7d64-4346-90d5-6410463b43ae'";
        sort_by[3] = NULL;

        query = "( (((repo_ids.has_all (['IDL:MateComponent/Control:1.0',"
                "'IDL:Nautilus/View:1.0']) OR (repo_ids.has_one "
                "(['IDL:MateComponent/Control:1.0','IDL:MateComponent/Embeddable:1.0']) AND "
                "repo_ids.has_one (['IDL:MateComponent/PersistStream:1.0', "
                "'IDL:MateComponent/ProgressiveDataSink:1.0', "
                "'IDL:MateComponent/PersistFile:1.0']))) AND (matecomponent:supported_mime_types.defined () OR "
                "matecomponent:supported_uri_schemes.defined () OR "
                "matecomponent:additional_uri_schemes.defined ()) AND "
                "(((NOT matecomponent:supported_mime_types.defined () OR "
                "matecomponent:supported_mime_types.has ('x-directory/normal') OR "
                "matecomponent:supported_mime_types.has ('x-directory/*') OR "
                "matecomponent:supported_mime_types.has ('*/*')) AND "
                "(NOT matecomponent:supported_uri_schemes.defined () OR "
                "matecomponent:supported_uri_schemes.has ('file') OR "
                "matecomponent:supported_uri_schemes.has ('*'))) OR "
                "(matecomponent:additional_uri_schemes.has ('file') OR "
                "matecomponent:additional_uri_schemes.has ('*'))) AND "
                "nautilus:view_as_name.defined ()) OR false) AND "
                "(has (['OAFIID:nautilus_file_manager_icon_view:42681b21-d5ca-4837-87d2-394d88ecc058', "
                "'OAFIID:nautilus_file_manager_list_view:521e489d-0662-4ad7-ac3a-832deabe111c'], iid)) ) AND "
                "(NOT test_only.defined() OR NOT test_only)";

        ac = matecomponent_activation_activation_context_get ();

        g_timer_start (timer);

        info = matecomponent_activation_query (query, sort_by, &ev);

        for (i = 0; i < 1000; i++) {
                MateComponent_ServerInfoList *copy;
                copy = MateComponent_ServerInfoList_duplicate (info);
                CORBA_free (copy);
        }
        g_timer_stop (timer);

        fprintf (stderr, "Time to query '%g'\n", g_timer_elapsed (timer, NULL));
        if (ev._major == CORBA_NO_EXCEPTION) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: timed query\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: timed query\n", passed + failed, TOTAL_TEST_SCORE);
                CORBA_exception_free (&ev);
	}

        /*
         *    We wait to see if the server (sever)
         * timeout is mis-behaving [ at this stage we
         * have not registered anything with the server ]
         */
        fprintf (stderr, "Waiting to see if the server erroneously quits\n");
        g_usleep (SERVER_IDLE_QUIT_TIMEOUT * 2 * 1000);
        g_assert (MateCORBA_small_get_connection_status (ac) ==
                  MATECORBA_CONNECTION_CONNECTED);

        race_empty (&ev);
	
	
	
	

	obj = matecomponent_activation_activate_from_id ("OAFIID:Empty:19991025", 0, NULL, &ev);
        if (test_object (obj, &ev, "from id") && test_empty (obj, &ev, "from id")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: IID activation\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: IID activation\n", passed + failed, TOTAL_TEST_SCORE);
	}

	obj = matecomponent_activation_activate_from_id ("OAFAID:[OAFIID:Empty:19991025]", 0, NULL, &ev);
        if (test_object (obj, &ev, "from aid") && test_empty (obj, &ev, "from aid")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: AID activation\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: AID activation\n", passed + failed, TOTAL_TEST_SCORE);
	}

	obj = matecomponent_activation_activate_from_id ("OAFAID:[OAFIID:Plugin:20010713]",  0, NULL, &ev);
	if (test_object (obj, &ev, "from aid") && test_plugin (obj, &ev, "from aid")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: plugin activation\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: plugin activation\n", passed + failed, TOTAL_TEST_SCORE);
	}

        obj = matecomponent_activation_activate_from_id ("OAFIID:Bogus:20000526", 0, NULL, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: Broken link test : %s\n",
			 passed + failed, TOTAL_TEST_SCORE, matecomponent_activation_exception_id (&ev));
                CORBA_exception_free (&ev);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Broken link test\n", passed + failed, TOTAL_TEST_SCORE);
	}

        if (test_matecomponent_activation_server (&ev, "with broken factory link")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
	}

        fprintf (stderr, "Broken exe test ");
        obj = matecomponent_activation_activate_from_id ("OAFIID:Broken:20000530", 0, NULL, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: Broken exe test : %s\n",
			 passed + failed, TOTAL_TEST_SCORE, matecomponent_activation_exception_id (&ev));
                CORBA_exception_free (&ev);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Broken exe test\n", passed + failed, TOTAL_TEST_SCORE);
	}

        if (test_matecomponent_activation_server (&ev, "with broken factory link")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
	}

        obj = matecomponent_activation_activate_from_id ("OAFIID:Circular:20000530", 0, NULL, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: Circular link test : %s\n",
			 passed + failed, TOTAL_TEST_SCORE, matecomponent_activation_exception_id (&ev));
                CORBA_exception_free (&ev);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Circular link test\n", passed + failed, TOTAL_TEST_SCORE);
	}

        if (test_matecomponent_activation_server (&ev, "with broken factory link")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
	}

        obj = matecomponent_activation_activate_from_id ("OAFIID:NotInServer:20000717", 0, NULL, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: Server that doesn't register IID test : %s\n",
			 passed + failed, TOTAL_TEST_SCORE, matecomponent_activation_exception_id (&ev));
                CORBA_exception_free (&ev);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Server that doesn't register IID test\n",
			 passed + failed, TOTAL_TEST_SCORE);
	}

        if (test_matecomponent_activation_server (&ev, "with non-registering server")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
	}

        obj = matecomponent_activation_activate_from_id ("OAFIID:BrokenNoType:20000808", 0, NULL, &ev);
        if (ev._major != CORBA_NO_EXCEPTION) {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Server with IID but no type or location : %s\n",
			 passed + failed, TOTAL_TEST_SCORE, matecomponent_activation_exception_id (&ev));
                CORBA_exception_free (&ev);
        } else if (obj) {
		failed++;
		fprintf (stderr, "FAILED %d of %d: Server with IID but no type or location\n",
			 passed + failed, TOTAL_TEST_SCORE);
        } else {
                passed++;
		fprintf (stderr, "PASSED %d of %d: Server with IID but no type or location\n",
			 passed + failed, TOTAL_TEST_SCORE);
        }

        if (test_matecomponent_activation_server (&ev, "with no-type/loc server")) {
		passed++;
		fprintf (stderr, "PASSED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
        } else {
		failed++;
		fprintf (stderr, "FAILED %d of %d: activation server okay\n", passed + failed, TOTAL_TEST_SCORE);
	}

        fprintf (stderr, "\n%d of %d tests passed (%s)\n", passed,
                 TOTAL_TEST_SCORE,
                 passed == TOTAL_TEST_SCORE? "All": "some failures");

        if (passed < (TOTAL_TEST_SCORE * 2 / 3)) {
                fprintf (stderr, "It looks like you have not installed broken.server "
                         "into ${prefix}/share/matecomponent-activation/, this must be done "
                         "by hand to avoid problems with normal operation.\n");
		fprintf (stderr, "Another possibility is that you failed to kill "
			 "matecomponent_activation_server before running make check; try running matecomponent-slay.\n");
        }

        if (name_service != CORBA_OBJECT_NIL)
                CORBA_Object_release (name_service, &ev);
        
        if (event_source != CORBA_OBJECT_NIL)
		CORBA_Object_release (event_source, &ev);

	CORBA_exception_free (&ev);

        if (passed == TOTAL_TEST_SCORE) {
                if (matecomponent_debug_shutdown ()) {
                        return 0;
                } else {
                        return 1;
                }
        } else {
                return 1;
        }
}
