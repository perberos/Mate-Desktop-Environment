/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <libmatecomponent.h>
#include <string.h>
#include "matecomponent/matecomponent-marshal.h"

#define TEST_MESSAGE    "test-message"
#define CLOSURE_MESSAGE "closure-message"

static gboolean
quit_after_timeout (gpointer data)
{
	matecomponent_main_quit ();
	return FALSE;
}

static GValue *
message_quit_cb (MateComponentAppClient *app_client, const gchar *message, GValueArray *args)
{
	g_timeout_add (1000, quit_after_timeout, NULL);
	return NULL;
}

static GValue *
message_cb (MateComponentAppClient *app_client, const gchar *message, GValueArray *args)
{
	GValue *retval;

	g_return_val_if_fail (strcmp (message, TEST_MESSAGE) == 0, NULL);
	g_return_val_if_fail (args->n_values == 2, NULL);
	g_return_val_if_fail (G_VALUE_HOLDS_DOUBLE (&args->values[0]), NULL);
	g_return_val_if_fail (G_VALUE_HOLDS_STRING (&args->values[1]), NULL);

	g_message ("message_cb: %s(%f, \"%s\")", message,
		   g_value_get_double (&args->values[0]),
		   g_value_get_string (&args->values[1]));

	retval = g_new0 (GValue, 1);
	g_value_init (retval, G_TYPE_DOUBLE);
	g_value_set_double (retval, 2 * g_value_get_double (&args->values[0]));
	return retval;
}

static void
test_app_hook (MateComponentApplication *app, gpointer data)
{
	g_message ("App '%s' created; data == %p", app->name, data);
}

static gint
new_instance_cb (MateComponentApplication *app, gint argc, char *argv[])
{
	int i;

	g_message ("new-instance received. argc = %i; argv follows:", argc);
	for (i = 0; i < argc; ++i)
		g_message ("argv[%i] = \"%s\"", i, argv[i]);
	g_message ("new-instance: returning argc (%i)", argc);
	return argc;
}


static gdouble
closure_message_cb (MateComponentApplication *app, gint arg_1, gdouble arg_2, gpointer data2)
{
	g_message("closure_message_cb: %p, %i, %f, %p",
		  app, arg_1, arg_2, data2);
	return arg_1 * arg_2;
}


int
main (int argc, char *argv [])
{
	MateComponentApplication         *app;
	gchar                     *serverinfo;
	MateComponent_RegistrationResult  reg_res;
	MateComponentAppClient           *client;
	double                     msg_arg = 3.141592654;
	GClosure                  *closure;
	gchar const               *envp[] = { "LANG", NULL };

	if (matecomponent_init (&argc, argv) == FALSE)
		g_error ("Can not matecomponent_init");
	matecomponent_activate ();

	matecomponent_application_add_hook (test_app_hook, (gpointer) 0xdeadbeef);

	app = matecomponent_application_new ("Libmatecomponent-Test-Uniqapp");

	closure = g_cclosure_new (G_CALLBACK (closure_message_cb),
				  (gpointer) 0xdeadbeef,  NULL);
	g_closure_set_marshal (closure, matecomponent_marshal_DOUBLE__LONG_DOUBLE);
	matecomponent_application_register_message (app, CLOSURE_MESSAGE,
					     "This is a test message",
					     closure,
					     G_TYPE_DOUBLE, G_TYPE_LONG,
					     G_TYPE_DOUBLE, G_TYPE_NONE);
	serverinfo = matecomponent_application_create_serverinfo (app, envp);
	reg_res = matecomponent_application_register_unique (app, serverinfo, &client);
	g_free (serverinfo);

	switch (reg_res)
	{

	case MateComponent_ACTIVATION_REG_ALREADY_ACTIVE: {
		MateComponentAppClientMsgDesc const *msgdescs;
		GValue                       *retval;
		int                           i;

		g_message ("I am an application client.");
		matecomponent_object_unref (MATECOMPONENT_OBJECT (app));
		app = NULL;

		msgdescs = matecomponent_app_client_msg_list (client);
		g_assert (msgdescs);

		for (i = 0; msgdescs[i].name; ++i)
			g_message ("Application supports message '%s'", msgdescs[i].name);

		g_message ("Sending message string '%s' with argument %f",
			   TEST_MESSAGE, msg_arg);
		retval = matecomponent_app_client_msg_send (client, TEST_MESSAGE, NULL,
						     G_TYPE_DOUBLE, msg_arg,
						     G_TYPE_STRING, "this is a string",
						     G_TYPE_NONE);
		g_message ("Return value: %f", g_value_get_double (retval));
		if (retval) {
			g_value_unset (retval);
			g_free (retval);
		}

		g_message ("Sending message string '%s' with arguments %i and %f",
			   CLOSURE_MESSAGE, 10, 3.141592654);
		retval = matecomponent_app_client_msg_send (client, CLOSURE_MESSAGE,
						     NULL,
						     G_TYPE_LONG, 10,
						     G_TYPE_DOUBLE, 3.141592654,
						     G_TYPE_NONE);
		g_message ("Return value: %f", g_value_get_double (retval));
		if (retval) {
			g_value_unset (retval);
			g_free (retval);
		}

		g_message ("Sending new-instance, with argc/argv");
		i = matecomponent_app_client_new_instance (client, argc, argv, NULL);
		g_message ("new-instance returned %i", i);

		g_message ("Asking the server to quit");
		retval = matecomponent_app_client_msg_send (client, "quit", NULL, G_TYPE_NONE);
		if (retval) {
			g_value_unset (retval);
			g_free (retval);
		}

		g_object_unref (client);
		return matecomponent_debug_shutdown ();
	}
	case MateComponent_ACTIVATION_REG_SUCCESS:
		g_message ("I am an application server");
		g_signal_connect (app, "message::test-message",
				  G_CALLBACK (message_cb), NULL);
		g_signal_connect (app, "new-instance",
				  G_CALLBACK (new_instance_cb), NULL);
		matecomponent_application_new_instance (app, argc, argv);
		g_signal_connect (app, "message::quit",
				  G_CALLBACK (message_quit_cb), NULL);
		break;

	case MateComponent_ACTIVATION_REG_ERROR:
	default:
		g_error("matecomponent activation error when registering unique application");
	}

	matecomponent_main ();

	if (app) matecomponent_object_unref (MATECOMPONENT_OBJECT (app));

	return matecomponent_debug_shutdown ();
}
