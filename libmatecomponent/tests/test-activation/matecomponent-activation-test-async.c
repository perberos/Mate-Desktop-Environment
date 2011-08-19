/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <matecomponent-activation/matecomponent-activation.h>

#define DEBUG_TIMEOUT 2000
#define DEBUG_TIME    1

typedef struct {
        gboolean callback_called;
        gboolean succeeded;
        GMainLoop *loop;
} callback_data_t;


static void
test_callback (CORBA_Object   activated_object, 
               const char    *error_reason, 
               gpointer       user_data)
{
        callback_data_t *data;

        data = (callback_data_t *) user_data;

        if (activated_object == CORBA_OBJECT_NIL) {
                data->succeeded = FALSE;
        } else {
                CORBA_Environment ev;

                CORBA_exception_init (&ev);
                CORBA_Object_release (activated_object, &ev);
                CORBA_exception_free (&ev);

                data->succeeded = TRUE;
        }
                
        data->callback_called = TRUE;
        g_main_loop_quit (data->loop);
}


/* returns 1 in case of success. 0 otherwise. 
   -1 if answer timeouted.... */
static int
test_activate (char *requirements)
{
        CORBA_Environment ev;
        callback_data_t data;
        GMainLoop *loop = g_main_loop_new (NULL, FALSE);
        guint timeout_id;

        CORBA_exception_init (&ev);

        data.callback_called = FALSE;
        data.succeeded = FALSE;
        data.loop = loop;

        matecomponent_activation_activate_async (requirements, NULL, 0, test_callback, &data, &ev);

        timeout_id = g_timeout_add (DEBUG_TIMEOUT, (GSourceFunc) g_main_loop_quit, loop);
        g_main_loop_run (loop);
        if (data.callback_called == FALSE)
                g_source_remove (timeout_id);
        g_main_loop_unref (loop);

        if (data.callback_called == FALSE) {
                return -1;
        }
        if (data.succeeded == TRUE) {
                return 1;
        } else {
                return 0;
        }
}

/* returns 1 in case of success. 0 otherwise. 
   -1 if answer timeouted.... */
static int
test_activate_from_id (char *aid)
{
        CORBA_Environment ev;
        callback_data_t data;
        GMainLoop *loop = g_main_loop_new (NULL, FALSE);
        guint timeout_id;

        CORBA_exception_init (&ev);

        data.callback_called = FALSE;
        data.succeeded = FALSE;
        data.loop = loop;
        matecomponent_activation_activate_from_id_async (aid, 0, test_callback, &data, &ev);

        timeout_id = g_timeout_add (DEBUG_TIMEOUT, (GSourceFunc) g_main_loop_quit, loop);
        g_main_loop_run (loop);
        if (data.callback_called == FALSE)
                g_source_remove (timeout_id);
        g_main_loop_unref (loop);

        if (data.callback_called == FALSE) {
                return -1;
        }
        if (data.succeeded == TRUE) {
                return 1;
        } else {
                return 0;
        }
}


#define TOTAL_TESTS 4
int
main (int argc, char *argv[])
{
        int test_status;
        int test_passed;

        test_passed = 0;

	matecomponent_activation_init (argc, argv);
        g_message ("testing async interfaces\n");

        g_message ("testing activate_async... ");
        /* this should fail */
        test_status = test_activate ("");
        if (test_status == FALSE) {
                test_passed++;
                g_message (" passed\n");
        } else if (test_status == TRUE
                   || test_status == -1) {
                g_message (" failed\n");
        }

        g_message ("testing activate_async... ");
        test_status = test_activate ("has (repo_ids, 'IDL:Empty:1.0')");
        if (test_status == TRUE) {
                test_passed++;
                printf (" passed\n");
        } else if (test_status == FALSE
                   || test_status == -1) {
                g_message (" failed\n");
        }

        g_message ("testing activate_from_id_async... ");
        test_status = test_activate_from_id ("");
        if (test_status == FALSE) {
                test_passed++;
                g_message (" passed\n");
        } else if (test_status == TRUE
                   || test_status == -1) {
                g_message (" failed\n");
        }

        g_message ("testing activate_from_id_async... ");
        test_status = test_activate_from_id ("OAFIID:Empty:19991025");
        if (test_status == TRUE) {
                test_passed++;
                g_message (" passed\n");
        } else if (test_status == FALSE
                   || test_status == -1) {
                g_message (" failed\n");
        }

        g_message ("Async Test Results: %d passed upon %d \n", 
                test_passed, TOTAL_TESTS);

        if (test_passed != TOTAL_TESTS) {
                return 1;
        }

        if (matecomponent_activation_debug_shutdown ()) {
                return 0;
        } else {
                return 1;
        }
}
