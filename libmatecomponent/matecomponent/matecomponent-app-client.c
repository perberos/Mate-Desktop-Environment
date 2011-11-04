  /* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <string.h>
#include <gobject/gvaluecollector.h>
#include "matecomponent-app-client.h"
#include "matecomponent-exception.h"
#include "matecomponent-types.h"


static void matecomponent_app_client_class_init    (MateComponentAppClientClass *klass);
static void matecomponent_app_client_init          (MateComponentAppClient      *app_client);
static void matecomponent_app_client_free_msgdescs (MateComponentAppClient      *self);

static gpointer parent_class;

GType
matecomponent_app_client_get_type (void)
{
	static GType app_client_type = 0;

	if (!app_client_type)
	{
		const GTypeInfo app_client_info =
			{
				sizeof (MateComponentAppClientClass),
				NULL,		/* base_init */
				NULL,		/* base_finalize */
				(GClassInitFunc) matecomponent_app_client_class_init,
				NULL,		/* class_finalize */
				NULL,		/* class_data */
				sizeof (MateComponentAppClient),
				0,		/* n_preallocs */
				(GInstanceInitFunc) matecomponent_app_client_init
			};

		app_client_type = g_type_register_static
			(G_TYPE_OBJECT, "MateComponentAppClient", &app_client_info, 0);
	}

	return app_client_type;
}


static void
matecomponent_app_client_finalize (GObject *object)
{
	MateComponentAppClient *self = MATECOMPONENT_APP_CLIENT (object);

	if (self->msgdescs)
		matecomponent_app_client_free_msgdescs (self);

	if (self->app_server != CORBA_OBJECT_NIL) {
		matecomponent_object_release_unref (self->app_server, NULL);
		self->app_server = CORBA_OBJECT_NIL;
	}

	if (G_OBJECT_CLASS(parent_class)->finalize)
		G_OBJECT_CLASS(parent_class)->finalize (object);
}


static void
matecomponent_app_client_class_init (MateComponentAppClientClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	object_class->finalize = matecomponent_app_client_finalize;
}


static void
matecomponent_app_client_init (MateComponentAppClient *app_client)
{

}


/**
 * matecomponent_app_client_new:

 * @app_server: object reference to a MateComponent::Application; this
 * function takes ownership of this reference (use
 * matecomponent_object_dup_ref() if you want to keep your own reference.)
 *
 * Create an application client object connected to the remote (or
 * local) MateComponent::Application object.
 *
 * <warning>Applications should not use this function. See
 * matecomponent_application_register_unique().</warning>
 *
 * Return value: a #MateComponentAppClient object.
 **/
MateComponentAppClient *
matecomponent_app_client_new (MateComponent_Application app_server)
{
	MateComponentAppClient *app_client;

	app_client = g_object_new (MATECOMPONENT_TYPE_APP_CLIENT, NULL);
	app_client->app_server = app_server;
	return app_client;
}


/**
 * matecomponent_app_client_msg_send_argv:
 * @app_client: client
 * @message: message name
 * @argv: %NULL-terminated vector of pointers to GValue, the arguments
 * to pass with the message.
 *
 * Like matecomponent_app_client_msg_send(), except that it receives a single
 * argument vector instead of a variable number of arguments.
 *
 * Return value: the message return value
 **/
GValue *
matecomponent_app_client_msg_send_argv (MateComponentAppClient   *app_client,
				 const char        *message,
				 const GValue      *argv[],
				 CORBA_Environment *opt_env)
{
	CORBA_any                  *ret;
	MateComponent_Application_ArgList *args;
	GValue                     *rv;
	CORBA_Environment           ev1, *ev;
	int                         i, argv_len;

	g_return_val_if_fail (app_client, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_APP_CLIENT (app_client), NULL);

	for (argv_len = -1; argv[++argv_len];);

	args = MateComponent_Application_ArgList__alloc ();
	args->_length = argv_len;
	args->_buffer = MateComponent_Application_ArgList_allocbuf (argv_len);
	for (i = 0; i < argv_len; ++i) {
		if (!matecomponent_arg_from_gvalue_alloc (&args->_buffer[i], argv[i])) {
			g_warning ("Failed to convert type '%s' to CORBA::any",
				   g_type_name (G_VALUE_TYPE (argv[i])));
			args->_buffer[i]._type = TC_void;
		}
	}
	CORBA_sequence_set_release (args, CORBA_TRUE);

	if (opt_env)
		ev = opt_env;
	else {
		CORBA_exception_init (&ev1);
		ev = &ev1;
	}
	ret = MateComponent_Application_message (app_client->app_server, message, args, ev);
	CORBA_free (args);
	if (ev->_major != CORBA_NO_EXCEPTION) {
		if (!opt_env) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("error while sending message to application server: %s",
				   text);
			g_free (text);
			CORBA_exception_free (&ev1);
		}
		return NULL;
	}
	if (!opt_env)
		CORBA_exception_free (&ev1);

	if (ret->_type != TC_void) {
		rv = g_new0 (GValue, 1);
		matecomponent_arg_to_gvalue_alloc (ret, rv);
	} else
		rv = NULL;
	CORBA_free (ret);
	return rv;
}


/**
 * matecomponent_app_client_msg_send_valist:
 * @app_client: client
 * @message: message name
 * @opt_env: optional corba environment
 * @first_arg_type: first message parameter
 * @var_args: remaining parameters
 *
 * See matecomponent_app_client_msg_send().
 *
 * Return value: return value
 **/
GValue *
matecomponent_app_client_msg_send_valist (MateComponentAppClient   *app_client,
				   const char        *message,
				   CORBA_Environment *opt_env,
				   GType              first_arg_type,
				   va_list            var_args)
{
	GValue                     *value, *rv;
	GPtrArray                  *argv;
	GType                       type;
	gchar                      *err;
	int                         i;
	gboolean                    first_arg = TRUE;

	g_return_val_if_fail (app_client, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_APP_CLIENT (app_client), NULL);

	argv = g_ptr_array_new ();
	while ((type = (first_arg? first_arg_type :
			va_arg (var_args, GType))) != G_TYPE_NONE)
	{
		first_arg = FALSE;
		value = g_new0 (GValue, 1);
		g_value_init (value, type);
		G_VALUE_COLLECT(value, var_args, 0, &err);
		if (err) g_error("error collecting value: %s", err);
		g_ptr_array_add (argv, value);
	}
	g_ptr_array_add (argv, NULL);

	rv = matecomponent_app_client_msg_send_argv (app_client, message,
					      (const GValue **) argv->pdata,
					      opt_env);

	for (i = 0; i < argv->len - 1; ++i) {
		value = g_ptr_array_index (argv, i);
		g_value_unset (value);
		g_free (value);
	}
	g_ptr_array_free (argv, TRUE);

	return rv;
}

/**
 * matecomponent_app_client_msg_send:
 * @app_client: the client interface associated with the application
 * to which we wish to send a message
 * @message: message name
 * @...: arguments
 *
 * Send a message to the application server. Takes a variable length
 * argument list of GType, value pairs, terminated with
 * %G_TYPE_NONE. Values are direct C values, not GValues! Example:
 * <informalexample><programlisting>
 * GValue *retval;
 * retval = matecomponent_app_client_msg_send (app_client, "openURL",
 *                                      G_TYPE_STRING, "http://www.gnome.org",
 *                                      G_TYPE_BOOLEAN, TRUE,
 *                                      G_TYPE_NONE);
 * </programlisting></informalexample>
 *
 * Return value: a GValue containing the value returned from the
 * aplication server.
 **/
GValue *
matecomponent_app_client_msg_send (MateComponentAppClient   *app_client,
			    const char        *message,
			    CORBA_Environment *opt_env,
			    GType              first_arg_type,
			    ...)
{
	GValue  *rv;
	va_list  var_args;

	va_start (var_args, first_arg_type);
	rv = matecomponent_app_client_msg_send_valist (app_client, message, opt_env,
						first_arg_type, var_args);
	va_end (var_args);
	return rv;
}


static inline GType
_typecode_to_gtype (CORBA_TypeCode tc)
{
	static GHashTable *hash = NULL;

	if (!hash) {
		hash = g_hash_table_new (g_direct_hash, g_direct_equal);
#define mapping(gtype, corba_type)\
		g_hash_table_insert (hash, corba_type, GUINT_TO_POINTER (gtype));

		mapping (G_TYPE_NONE,    TC_void);
		mapping (G_TYPE_BOOLEAN, TC_CORBA_boolean);
		mapping (G_TYPE_LONG,    TC_CORBA_long);
		mapping (G_TYPE_ULONG,   TC_CORBA_unsigned_long);
		mapping (G_TYPE_FLOAT,   TC_CORBA_float);
		mapping (G_TYPE_DOUBLE,  TC_CORBA_double);
		mapping (G_TYPE_STRING,  TC_CORBA_string);

		mapping (MATECOMPONENT_TYPE_CORBA_ANY,  TC_CORBA_any);
#undef mapping
	}
	return GPOINTER_TO_UINT (g_hash_table_lookup (hash, tc));
}

static void
matecomponent_app_client_free_msgdescs (MateComponentAppClient *self)
{
	int i;

	for (i = 0; self->msgdescs[i].name; ++i) {
		g_free (self->msgdescs[i].name);
		g_free (self->msgdescs[i].types);
	}
	g_free (self->msgdescs);
	self->msgdescs = NULL;
}

static void
matecomponent_app_client_get_msgdescs (MateComponentAppClient *self)
{
	MateComponent_Application_MessageList  *msglist;
	CORBA_Environment                ev;
	int                              i, j;

	g_return_if_fail (!self->msgdescs);
	CORBA_exception_init (&ev);
	msglist = MateComponent_Application_listMessages (self->app_server, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		char *text = matecomponent_exception_get_text (&ev);
		g_warning ("MateComponent::Application::listMessages: %s", text);
		g_free (text);
		CORBA_exception_free (&ev);
		return;
	}
	CORBA_exception_free (&ev);
	g_return_if_fail (msglist);
	self->msgdescs = g_new (MateComponentAppClientMsgDesc, msglist->_length + 1);
	for (i = 0; i < msglist->_length; ++i) {
		self->msgdescs[i].name = g_strdup (msglist->_buffer[i].name);
		self->msgdescs[i].return_type =
			_typecode_to_gtype (msglist->_buffer[i].return_type);
		self->msgdescs[i].types = g_new (GType, msglist->_buffer[i].types._length + 1);
		for (j = 0; j < msglist->_buffer[i].types._length; ++j)
			self->msgdescs[i].types[j] =
				_typecode_to_gtype (msglist->_buffer[i].types._buffer[j]);
		self->msgdescs[i].types[j] = G_TYPE_NONE;
		self->msgdescs[i].description = g_strdup (msglist->_buffer[i].description);
	}
	self->msgdescs[i].name = NULL;
	self->msgdescs[i].return_type = G_TYPE_NONE;
	self->msgdescs[i].types = NULL;
	CORBA_free (msglist);
}


/**
 * matecomponent_app_client_msg_list:
 *
 * Obtain a list of messages supported by the application server.
 *
 * Return value: a NULL terminated vector of strings; use g_strfreev()
 * to free it.
 **/
MateComponentAppClientMsgDesc const *
matecomponent_app_client_msg_list (MateComponentAppClient *app_client)
{

	g_return_val_if_fail (MATECOMPONENT_IS_APP_CLIENT (app_client), NULL);

	if (!app_client->msgdescs)
		matecomponent_app_client_get_msgdescs (app_client);
	return app_client->msgdescs;
}


/**
 * matecomponent_app_client_new_instance:
 * @app_client: a #MateComponentAppClient
 * @argc: length of @argv
 * @argv: array of command-line arguments
 * @opt_env: a #CORBA_Environment, or %NULL.
 *
 * Ask the application server to emit a "new-instance" signal
 * containing the specified string vector.
 *
 * Return value: the message return value
 **/
gint
matecomponent_app_client_new_instance (MateComponentAppClient   *app_client,
				int                argc,
				char              *argv[],
				CORBA_Environment *opt_env)
{
	CORBA_sequence_CORBA_string *corba_argv;
	int                          i;
	gint                         rv;
	CORBA_Environment            *ev, ev1;

	corba_argv = CORBA_sequence_CORBA_string__alloc();
	corba_argv->_buffer  = CORBA_sequence_CORBA_string_allocbuf (argc);
	corba_argv->_length  = argc;
	corba_argv->_maximum = argc;
	for (i = 0; i < argc; ++i)
		corba_argv->_buffer[i] = CORBA_string_dup (argv[i]);

	if (opt_env)
		ev = opt_env;
	else {
		CORBA_exception_init (&ev1);
		ev = &ev1;
	}
	rv = MateComponent_Application_newInstance (app_client->app_server, corba_argv, ev);
	CORBA_free (corba_argv);
	if (!opt_env) {
		if (ev->_major != CORBA_NO_EXCEPTION) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("newInstance failed: %s", text);
			g_free (text);
		}
		CORBA_exception_free (&ev1);
	}
	return rv;
}

