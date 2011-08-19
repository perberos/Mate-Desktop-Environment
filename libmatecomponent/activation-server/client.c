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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <matecomponent-activation/matecomponent-activation.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <matecomponent-activation/MateComponent_ActivationContext.h>

static gchar *acior = NULL, *specs = NULL, *add_path = NULL, *remove_path = NULL, *registerior = NULL, *registeriid = NULL;
static gboolean do_query;
static CORBA_ORB orb;
static CORBA_Environment ev;

static const GOptionEntry options[] = {

	{"ac-ior", '\0', 0, G_OPTION_ARG_STRING, &acior,
	 "IOR of ActivationContext to use", "IOR"},
	{"do-query", 'q', 0, G_OPTION_ARG_NONE, &do_query,
	 "Run a query instead of activating", "QUERY"},
	{"spec", 's', 0, G_OPTION_ARG_STRING, &specs,
	 "Specification string for object to activate", "SPEC"},
	{"add-path", '\0', 0, G_OPTION_ARG_FILENAME, &add_path,
	 "Specification string for search path to be added in runtime", "PATH"},
	{"remove-path", '\0', 0, G_OPTION_ARG_FILENAME, &remove_path,
	 "Specification string for search path to be removed in runtime", "PATH"},
	{"register-ior", '\0', 0, G_OPTION_ARG_STRING, &registerior,
	 "IOR of the server to be registered", "IOR"},
	{"register-iid", '\0', 0, G_OPTION_ARG_STRING, &registeriid,
         "IID of the server to be registered", "IID"},
	{NULL}
};

static void
od_dump_list (MateComponent_ServerInfoList * list)
{
	int i, j, k;

	for (i = 0; i < list->_length; i++) {
		g_print ("IID %s, type %s, location %s\n",
			 list->_buffer[i].iid,
			 list->_buffer[i].server_type,
			 list->_buffer[i].location_info);
		for (j = 0; j < list->_buffer[i].props._length; j++) {
			MateComponent_ActivationProperty *prop =
				&(list->_buffer[i].props._buffer[j]);
			g_print ("    %s = ", prop->name);
			switch (prop->v._d) {
			case MateComponent_ACTIVATION_P_STRING:
				g_print ("\"%s\"\n", prop->v._u.value_string);
				break;
			case MateComponent_ACTIVATION_P_NUMBER:
				g_print ("%f\n", prop->v._u.value_number);
				break;
			case MateComponent_ACTIVATION_P_BOOLEAN:
				g_print ("%s\n",
					 prop->v.
					 _u.value_boolean ? "TRUE" : "FALSE");
				break;
			case MateComponent_ACTIVATION_P_STRINGV:
				g_print ("[");
				for (k = 0;
				     k < prop->v._u.value_stringv._length;
				     k++) {
					g_print ("\"%s\"",
						 prop->v._u.
						 value_stringv._buffer[k]);
					if (k <
					    (prop->v._u.
					     value_stringv._length - 1))
						g_print (", ");
				}
				g_print ("]\n");
				break;
			}
		}
	}
}

static void
add_load_path (void)
{
	MateComponent_DynamicPathLoadResult  res;

	res = matecomponent_activation_dynamic_add_path (add_path, &ev);
 
	switch (res) {
	case MateComponent_DYNAMIC_LOAD_SUCCESS:
		g_print ("Doing dynamic path(%s) adding successfully\n", add_path);
		break;
	case MateComponent_DYNAMIC_LOAD_ERROR:
		g_print ("Doing dynamic path(%s) adding unsuccessfully\n", add_path);
		break;
	case MateComponent_DYNAMIC_LOAD_ALREADY_LISTED:
		g_print ("The path(%s) already been listed\n", add_path);
		break;
        default:
		g_print ("Unknown error return (%d)\n", res);
                break;
	}
}

static void
remove_load_path (void)
{
	MateComponent_DynamicPathLoadResult  res;

	res = matecomponent_activation_dynamic_remove_path (remove_path, &ev);

	switch (res) {
	case MateComponent_DYNAMIC_LOAD_SUCCESS:
		g_print ("Doing dynamic path(%s) removing successfully\n", remove_path);
		break;
	case MateComponent_DYNAMIC_LOAD_ERROR:
		g_print ("Doing dynamic path(%s) removing unsuccessfully\n", remove_path);
		break;
	case MateComponent_DYNAMIC_LOAD_NOT_LISTED:
		g_print ("The path(%s) wasn't listed\n", remove_path);
		break;
        default:
		g_print ("Unknown error return (%d)\n", res);
                break;
	}
}

static int
register_activate_server(void)
{
	MateComponent_RegistrationResult res;
	CORBA_Object r_obj = CORBA_OBJECT_NIL;

	if (registerior) {
		r_obj = CORBA_ORB_string_to_object (orb, registerior, &ev);
		if (ev._major != CORBA_NO_EXCEPTION)
			return 1;
	}

	if (r_obj) {
		res = matecomponent_activation_active_server_register(registeriid, r_obj);
		if (res == MateComponent_ACTIVATION_REG_SUCCESS || res == MateComponent_ACTIVATION_REG_ALREADY_ACTIVE)
			return 0;
	}

	return 1;
}

static void
do_query_server_info(void)
{
	MateComponent_ActivationContext ac;
	MateComponent_ServerInfoList *slist;
	MateComponent_StringList reqs = { 0 };

	if (acior) {
                ac = CORBA_ORB_string_to_object (orb, acior, &ev);
                if (ev._major != CORBA_NO_EXCEPTION)
                        g_print ("Error doing string_to_object(%s)\n", acior);
        } else
                ac = matecomponent_activation_activation_context_get ();

	slist = MateComponent_ActivationContext_query (
                                        ac, specs, &reqs,
                                        matecomponent_activation_context_get (), &ev);
	switch (ev._major) {
        case CORBA_NO_EXCEPTION:
		od_dump_list (slist);
		CORBA_free (slist);
		break;
	case CORBA_USER_EXCEPTION:
		{
			char *id;
			id = CORBA_exception_id (&ev);
			g_print ("User exception \"%s\" resulted from query\n", id);
			if (!strcmp (id, "IDL:MateComponent/ActivationContext/ParseFailed:1.0")) {
				MateComponent_Activation_ParseFailed
						* exdata = CORBA_exception_value (&ev);
				if (exdata)
					g_print ("Description: %s\n", exdata->description);
			}
		}
		break;
	case CORBA_SYSTEM_EXCEPTION:
		{
			char *id;
			id = CORBA_exception_id (&ev);
			g_print ("System exception \"%s\" resulted from query\n", id);	
        	}
		break;
        }	
	return;	
}

static int
do_activating(void)
{
	MateComponent_ActivationEnvironment environment;
	MateComponent_ActivationResult *a_res;
	MateComponent_ActivationContext ac;	
	MateComponent_StringList reqs = { 0 };
	char *resior;
	int res = 1;

	if (acior) {
                ac = CORBA_ORB_string_to_object (orb, acior, &ev);
                if (ev._major != CORBA_NO_EXCEPTION)
        	return 1;
	} else
                ac = matecomponent_activation_activation_context_get ();

	memset (&environment, 0, sizeof (MateComponent_ActivationEnvironment));
                                                                                                                             
	a_res = MateComponent_ActivationContext_activateMatchingFull (
 				ac, specs, &reqs, &environment, 0,
                                matecomponent_activation_client_get (),
				matecomponent_activation_context_get (), &ev);
	switch (ev._major) {
	case CORBA_NO_EXCEPTION:
		g_print ("Activation ID \"%s\" ", a_res->aid);
		switch (a_res->res._d) {
		case MateComponent_ACTIVATION_RESULT_OBJECT:
			g_print ("RESULT_OBJECT\n");
			resior = CORBA_ORB_object_to_string (orb,
							     a_res->
							     res._u.res_object,
							     &ev);
			g_print ("%s\n", resior);
			break;
		case MateComponent_ACTIVATION_RESULT_SHLIB:
			g_print ("RESULT_SHLIB\n");
      			break;
		case MateComponent_ACTIVATION_RESULT_NONE:
			g_print ("RESULT_NONE\n");
			break;
		}
		res = 0;	
		break;
	case CORBA_USER_EXCEPTION:
		{
			char *id;
			id = CORBA_exception_id (&ev);
			g_print ("User exception \"%s\" resulted from query\n",
				 id);
			if (!strcmp (id,"IDL:MateComponent/ActivationContext/ParseFailed:1.0")) {
				MateComponent_Activation_ParseFailed
					* exdata = CORBA_exception_value (&ev);
                                if (exdata)
					g_print ("Description: %s\n",
						 exdata->description);
			} else if (!strcmp (id,"IDL:MateComponent/GeneralError:1.0")) {
				MateComponent_GeneralError *exdata;
                                                                                                                             
				exdata = CORBA_exception_value (&ev);
                                                                                                                             
				if (exdata)
					g_print ("Description: %s\n",
						 exdata->description);
			}
			res = 1;
		}
		break;
	case CORBA_SYSTEM_EXCEPTION:
		{
			char *id;
			id = CORBA_exception_id (&ev);
			g_print ("System exception \"%s\" resulted from query\n",
				 id);
			res = 1;
		}
		break;
	}
	return res;
}

int
main (int argc, char *argv[])
{
	GOptionContext *ctx;
	GError *error = NULL;
	gboolean do_usage_exit = FALSE;
	int res = 0;

	CORBA_exception_init (&ev);

	g_set_prgname ("activation-client");
	ctx = g_option_context_new (NULL);
	g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
	if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		do_usage_exit = TRUE;
	}
	g_option_context_free (ctx);

	if (!do_usage_exit && !specs && !add_path && !remove_path && !(registerior && registeriid)) {
		g_printerr ("You must specify an operation to perform.\n");
		do_usage_exit = TRUE;
	}

	if (do_usage_exit) {
		g_printerr ("Run '%s --help' to see a full list of available command line options.\n", g_get_prgname ());
		exit (1);
	}

	orb = matecomponent_activation_init (argc, argv);

	if (specs) {
		g_print ("Query spec is \"%s\"\n", specs);
		if (do_query)
			do_query_server_info();
		else
			res = do_activating();
	} 

	if (add_path && !res)
		add_load_path();

	if (remove_path && !res)
		remove_load_path();

	if (registerior && registeriid && !res)
		res = register_activate_server();

	CORBA_exception_free (&ev);

        return res;
}
