/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "empty.h"
#include <matecomponent-activation/matecomponent-activation.h>

int
main (int argc, char *argv[])
{
	MateComponent_ServerInfoList *result;
	CORBA_Environment ev;
	char *query;
	char **sort_criteria;
	int i;

	CORBA_exception_init (&ev);
	matecomponent_activation_init (argc, argv);

	sort_criteria = NULL;

	if (argc > 1) {
		query = argv[1];
		if (argc > 2) {
			int i;
			int num_conditions;

			num_conditions = argc - 2;

			printf ("Number of sort criteria: %d\n",
				num_conditions);

			sort_criteria =
				g_malloc (sizeof (char *) *
					  (num_conditions + 1));

			for (i = 0; i < num_conditions; i++) {
				sort_criteria[i] = g_strdup (argv[i + 2]);
				puts (sort_criteria[i]);
			}

			sort_criteria[num_conditions] = NULL;
		}
	} else {
		query = "repo_ids.has('IDL:Empty:1.0')";
	}

	/* putenv("MateComponent_BARRIER_INIT=1"); */
	result = matecomponent_activation_query (query, sort_criteria, &ev);

	/* result = matecomponent_activation_query ("iid == 'OAFIID:Empty:19991025'", NULL, &ev); */

        if (ev._major != CORBA_NO_EXCEPTION) {
                if (ev._major == CORBA_USER_EXCEPTION) {
                        if (!strcmp (ev._id, ex_MateComponent_GeneralError)) {
                                MateComponent_GeneralError *err = CORBA_exception_value (&ev);
                                printf ("An exception '%s' occurred\n", err->description);
                        } else {
                                printf ("An unknown user exception ('%s') "
                                        "occurred\n", ev._id);
                        }
                } else if (ev._major == CORBA_SYSTEM_EXCEPTION) {
                        printf ("A system exception ('%s') occurred\n", ev._id);
                } else {
                        g_assert_not_reached ();
                }
	} else if (result == NULL) {
		puts ("NULL result failed");
	} else {
		printf ("number of results: %d\n", result->_length);

		for (i = 0; i < result->_length; i++) {
			puts ((result->_buffer[i]).iid);
		}
                CORBA_free (result);
	}

	CORBA_exception_free (&ev);

        if (matecomponent_activation_debug_shutdown ()) {
                return 0;
        } else {
                return 1;
        }
}
