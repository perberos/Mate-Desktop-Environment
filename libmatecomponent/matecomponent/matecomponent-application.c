/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include "config.h"

#include <glib/gi18n-lib.h>    

#include "matecomponent-application.h"
#include "matecomponent-app-client.h"
#include <matecomponent-exception.h>
#include "matecomponent-marshal.h"
#include "matecomponent-arg.h"
#include "matecomponent-main.h"
#include "matecomponent-types.h"
#include <string.h>


enum SIGNALS {
	MESSAGE,
	NEW_INSTANCE,
	LAST_SIGNAL
};

enum PROPERTIES {
	PROP_0,
	PROP_NAME
};

static guint signals [LAST_SIGNAL] = { 0 };

typedef struct {
	MateComponentAppHookFunc func;
	gpointer          data;
} MateComponentAppHook;


typedef struct {
	GClosure *closure;
	GType     return_type;
} MateComponentAppMessageDesc;

static GArray *matecomponent_application_hooks = NULL;

/*
 * A pointer to our parent object class
 */
static gpointer parent_class;


/* ------------ Forward function declarations ------------ */
static void     matecomponent_application_invoke_hooks (MateComponentApplication *app);


static void matecomponent_app_message_desc_free (MateComponentAppMessageDesc *msgdesc)
{
	if (msgdesc->closure)
		g_closure_unref (msgdesc->closure);
	g_free (msgdesc);
}


static gboolean
_matecomponent_application_message_accumulator(GSignalInvocationHint *ihint, 
					GValue                *return_accu, 
					const GValue          *handler_return, 
					gpointer               dummy)
{
	gboolean null_gvalue;

	null_gvalue = (G_VALUE_HOLDS (handler_return, G_TYPE_VALUE) &&
		       (g_value_peek_pointer (handler_return) == NULL));

	if (!null_gvalue) {
		g_value_copy (handler_return, return_accu);
		return FALSE;	/* stop emission */
	}
	return TRUE;		/* continue emission */
}



static void
matecomponent_application_finalize (GObject *object)
{
	MateComponentApplication *self = MATECOMPONENT_APPLICATION (object);

	if (self->message_list) {
		g_slist_foreach (self->message_list, (GFunc) CORBA_free, NULL);
		g_slist_free (self->message_list);
		self->message_list = NULL;
	}

	g_free (self->name);
	self->name = NULL;

	if (self->closure_hash) {
		g_hash_table_destroy (self->closure_hash);
		self->closure_hash = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


static CORBA_any *
impl_MateComponent_Application_message (PortableServer_Servant            servant,
				 const CORBA_char                 *msg,
				 const MateComponent_Application_ArgList *args,
				 CORBA_Environment                *ev)
{
	MateComponentApplication  *app = MATECOMPONENT_APPLICATION (matecomponent_object (servant));
	GValue             *signal_return = NULL;
	GValueArray        *signal_args;
	int                 i;
	CORBA_any          *rv;
	GValue              value;

	signal_args = g_value_array_new (args->_length);
	memset (&value, 0, sizeof (value));
	for (i = 0; i < args->_length; ++i) {
		if (matecomponent_arg_to_gvalue_alloc (&args->_buffer [i], &value)) {
			g_value_array_append (signal_args, &value);
			g_value_unset (&value);
		} else {
			g_warning ("Failed to convert type '%s' to GValue",
				   args->_buffer[i]._type->name);
		}
	}

	g_signal_emit (app, signals [MESSAGE],
		       g_quark_from_string (msg),
		       msg, signal_args, &signal_return);

	g_value_array_free (signal_args);
	rv = CORBA_any__alloc ();
	if (signal_return) {
		if (!matecomponent_arg_from_gvalue_alloc (rv, signal_return)) {
			g_warning ("Failed to convert type '%s' to CORBA::any",
				   g_type_name (G_VALUE_TYPE (signal_return)));
			rv->_type = TC_void;
		}
		g_value_unset (signal_return);
		g_free (signal_return);
	} else
		rv->_type = TC_void;

	return rv;
}

/**
 * matecomponent_application_new_instance:
 * @app: a #MateComponentApplication
 * @argc: number of elements in @argv
 * @argv: array of strings (command-line arguments)
 * 
 * Emit the "new-instance" signal of the #MateComponentApplication with the
 * given arguments.
 * 
 * Return value: signal return value
 **/
gint matecomponent_application_new_instance (MateComponentApplication *app,
				      gint               argc,
				      gchar             *argv[])
{
	gint         rv;
	gchar **new_argv = g_new (gchar *, argc + 1);

	memcpy (new_argv, argv, argc * sizeof(gchar *));
	new_argv[argc] = NULL;
	g_signal_emit (app, signals [NEW_INSTANCE], 0,
		       argc, new_argv, &rv);
	g_free (new_argv);
	return rv;
}

static GValue *
matecomponent_application_run_closures (MateComponentApplication *self,
				 const char        *name,
				 GValueArray       *args)
{
	MateComponentAppMessageDesc *desc;

	desc = g_hash_table_lookup (self->closure_hash, name);

	if (desc) {
		GValue *retval = g_new0 (GValue, 1);
		GValue *params = g_newa (GValue, args->n_values + 1);

		memset (params + 0, 0, sizeof (GValue));
		g_value_init (params + 0, G_TYPE_OBJECT);
		g_value_set_object (params + 0, self);
		memcpy (params + 1, args->values, args->n_values * sizeof (GValue));
		g_value_init (retval, desc->return_type);
		g_closure_invoke (desc->closure, retval, args->n_values + 1,
				  params, NULL /* invocation_hint */);
		g_value_unset (params + 0);
		return retval;
	}

	return NULL;
}

  /* Handle the "new-instance" standard message */
static GValue *
matecomponent_application_real_message (MateComponentApplication *app,
				 const char        *name,
				 GValueArray       *args)
{
	return matecomponent_application_run_closures (app, name, args);
}


static CORBA_long
impl_MateComponent_Application_newInstance (PortableServer_Servant           servant,
				     MateComponent_Application_argv_t const *argv,
				     CORBA_Environment               *ev)
{
	MateComponentApplication *app = MATECOMPONENT_APPLICATION (matecomponent_object (servant));
	CORBA_long         retval;

	retval = matecomponent_application_new_instance
		(app, argv->_length, argv->_buffer);
	return retval;
}

static inline void
message_desc_copy (MateComponent_Application_MessageDesc *dest,
		   MateComponent_Application_MessageDesc *src)
{
	dest->name           = CORBA_string_dup (src->name);
	dest->return_type    = src->return_type;
	dest->types._buffer  = src->types._buffer;
	dest->types._length  = src->types._length;
	dest->types._maximum = src->types._maximum;
	dest->types._release = CORBA_FALSE;
	dest->description    = CORBA_string_dup (src->description);
}


static MateComponent_Application_MessageList *
impl_MateComponent_Application_listMessages (PortableServer_Servant  servant,
				      CORBA_Environment      *ev)
{
	MateComponentApplication *app = MATECOMPONENT_APPLICATION (matecomponent_object (servant));
	int                nmessages;
	GSList            *l;
	int                i;
	MateComponent_Application_MessageList *msglist;

	nmessages = g_slist_length (app->message_list);
	msglist = MateComponent_Application_MessageList__alloc ();
	msglist->_length = msglist->_maximum = nmessages;
	msglist->_buffer = MateComponent_Application_MessageList_allocbuf (nmessages);
	for (l = app->message_list, i = 0; l; l = l->next, ++i)
		message_desc_copy (&msglist->_buffer [i],
				   (MateComponent_Application_MessageDesc *) l->data);
	CORBA_sequence_set_release (msglist, CORBA_TRUE);
	return msglist;
}

static CORBA_string
impl_MateComponent_Application_getName (PortableServer_Servant  servant,
				 CORBA_Environment      *ev)
{
	MateComponentApplication *app = MATECOMPONENT_APPLICATION (matecomponent_object (servant));
	return CORBA_string_dup (app->name);
}

static void
set_property (GObject      *g_object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	MateComponentApplication *self = (MateComponentApplication *) g_object;

	switch (prop_id) {
	case PROP_NAME:
		g_free (self->name);
		self->name = g_value_dup_string (value);
		break;
	default:
		break;
	}
}

static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	MateComponentApplication *self = (MateComponentApplication *) object;

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, self->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static GObject *
matecomponent_application_constructor (GType                  type,
				guint                  n_construct_properties,
				GObjectConstructParam *construct_properties)
{
	GObject           *object;
	MateComponentApplication *self;

	object = G_OBJECT_CLASS (parent_class)->constructor
		(type, n_construct_properties, construct_properties);
	self = MATECOMPONENT_APPLICATION (object);
	matecomponent_application_invoke_hooks (self);
	return object;
}

static void
matecomponent_application_class_init (MateComponentApplicationClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_Application__epv *epv = &klass->epv;

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize     = matecomponent_application_finalize;
	object_class->constructor  = matecomponent_application_constructor;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	signals [MESSAGE] = g_signal_new (
		"message", MATECOMPONENT_TYPE_APPLICATION,
		G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
		G_STRUCT_OFFSET (MateComponentApplicationClass, message),
		_matecomponent_application_message_accumulator, NULL,
		matecomponent_marshal_BOXED__STRING_BOXED,
		G_TYPE_VALUE, 2, /* return_type, nparams */
		G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE,
		G_TYPE_VALUE_ARRAY);

	signals [NEW_INSTANCE] = g_signal_new (
		"new-instance", MATECOMPONENT_TYPE_APPLICATION, G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (MateComponentApplicationClass, new_instance),
		NULL, NULL,	/* accumulator and accumulator data */
		matecomponent_marshal_INT__INT_BOXED,
		G_TYPE_INT, 2, /* return_type, nparams */
		G_TYPE_INT, G_TYPE_STRV);

	g_object_class_install_property
		(object_class, PROP_NAME,
		 g_param_spec_string
		 ("name", _("Name"), _("Application unique name"), NULL,
		  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	klass->message = matecomponent_application_real_message;

	epv->message      = impl_MateComponent_Application_message;
	epv->listMessages = impl_MateComponent_Application_listMessages;
	epv->newInstance  = impl_MateComponent_Application_newInstance;
	epv->getName      = impl_MateComponent_Application_getName;
}

static void
matecomponent_application_init (MateComponentApplication *self)
{
	self->closure_hash = g_hash_table_new_full
		(g_str_hash, g_str_equal,
		 g_free,(GDestroyNotify) matecomponent_app_message_desc_free);
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentApplication,
		       MateComponent_Application,
		       MATECOMPONENT_TYPE_OBJECT,
		       matecomponent_application)


/**
 * matecomponent_application_new:
 * @name: application name
 * 
 * Creates a new #MateComponentApplication object.
 * 
 * Return value: a new #MateComponentApplication
 **/
MateComponentApplication *
matecomponent_application_new (const char *name)
{
	GObject           *obj;
	MateComponentApplication *app;

	obj = g_object_new (MATECOMPONENT_TYPE_APPLICATION,
			    "poa", matecomponent_poa_get_threaded (MATECORBA_THREAD_HINT_ALL_AT_IDLE),
			    "name", name,
			    NULL);
	app = (MateComponentApplication *) obj;
	return app;
}

static inline CORBA_TypeCode
_gtype_to_typecode (GType gtype)
{
	static GHashTable *hash = NULL;

	if (!hash) {
		hash = g_hash_table_new (g_direct_hash, g_direct_equal);
#define mapping(gtype_, corba_type)\
		g_hash_table_insert (hash, GUINT_TO_POINTER (gtype_), corba_type);
		
		mapping (G_TYPE_NONE,    TC_void);
		mapping (G_TYPE_BOOLEAN, TC_CORBA_boolean);
		mapping (G_TYPE_INT,     TC_CORBA_long);
		mapping (G_TYPE_UINT,    TC_CORBA_unsigned_long);
		mapping (G_TYPE_LONG,    TC_CORBA_long);
		mapping (G_TYPE_ULONG,   TC_CORBA_unsigned_long);
		mapping (G_TYPE_FLOAT,   TC_CORBA_float);
		mapping (G_TYPE_DOUBLE,  TC_CORBA_double);
		mapping (G_TYPE_STRING,  TC_CORBA_string);
		
		mapping (MATECOMPONENT_TYPE_CORBA_ANY,  TC_CORBA_any);
#undef mapping
	}
	return (CORBA_TypeCode) g_hash_table_lookup (hash, GUINT_TO_POINTER (G_TYPE_INT));
}

/**
 * matecomponent_application_register_message_v:
 * @app: a #MateComponentApplication
 * @name: message string identifier
 * @description: a string containing a human readable description of the message
 * @opt_closure: a #GClosure that will be called for this message, or %NULL;
 * Function takes ownership of this closure.
 * @return_type: Message return #GType.
 * @arg_types: %G_TYPE_NONE -terminated vector of argument #GType's
 * 
 * See matecomponent_application_register_message().
 **/
void
matecomponent_application_register_message_v (MateComponentApplication *app,
				       const gchar       *name,
				       const gchar       *description,
				       GClosure          *opt_closure,
				       GType              return_type,
				       GType const        arg_types[])
{
	MateComponent_Application_MessageDesc *msgdesc;
	int i, arg_types_len;

	for (arg_types_len = -1; arg_types[++arg_types_len] != G_TYPE_NONE;);

	msgdesc = MateComponent_Application_MessageDesc__alloc ();

	msgdesc->return_type = _gtype_to_typecode (return_type);
	msgdesc->name        = CORBA_string_dup (name);
	msgdesc->description = CORBA_string_dup (description);

	msgdesc->types._length = msgdesc->types._maximum = arg_types_len;
	msgdesc->types._buffer =
		CORBA_sequence_CORBA_TypeCode_allocbuf (arg_types_len);

	for (i = 0; arg_types[i] != G_TYPE_NONE; ++i)
		msgdesc->types._buffer[i] = _gtype_to_typecode (arg_types[i]);

	app->message_list = g_slist_prepend (app->message_list, msgdesc);

	if (opt_closure) {
		MateComponentAppMessageDesc *desc = g_new0 (MateComponentAppMessageDesc, 1);
		g_closure_ref (opt_closure);
		g_closure_sink (opt_closure);
		desc->closure = opt_closure;
		desc->return_type = return_type;
		g_hash_table_insert (app->closure_hash, g_strdup (name), desc);
	}
}


/**
 * matecomponent_application_register_message_va:
 * @app: a #MateComponentApplication
 * @name: message string identifier
 * @description: a string containing a human readable description of the message
 * @opt_closure: a #GClosure that will be called for this message, or
 * %NULL; Function takes ownership of this closure.
 * @return_type: Message return #GType.
 * @first_arg_type: #GType of first argument of message, or %G_TYPE_NONE
 * @var_args: %G_TYPE_NONE -terminated valist of argument #GType's
 * 
 * See matecomponent_application_register_message().
 **/
void
matecomponent_application_register_message_va (MateComponentApplication *app,
					const gchar       *name,
					const gchar       *description,
					GClosure          *opt_closure,
					GType              return_type,
					GType              first_arg_type,
					va_list            var_args)
{
	GArray *arg_types;
	GType   gtype;

	arg_types = g_array_new (FALSE, FALSE, sizeof(GType));
	if (first_arg_type != G_TYPE_NONE) {
		g_array_append_val (arg_types, first_arg_type);
		while ((gtype = va_arg (var_args, GType)) != G_TYPE_NONE)
			g_array_append_val (arg_types, gtype);
	}
	gtype = G_TYPE_NONE; g_array_append_val (arg_types, gtype);

	matecomponent_application_register_message_v (app, name, description,
					       opt_closure, return_type,
					       (const GType *) arg_types->data);

	g_array_free (arg_types, TRUE);
}

/**
 * matecomponent_application_register_message:
 * @app: a #MateComponentApplication
 * @name: message string identifier
 * @description: a string containing a human readable description of the message
 * @opt_closure: a #GClosure that will be called for this message, or
 * %NULL; Function takes ownership of this closure.
 * @return_type: Message return #GType.
 * @first_arg_type: #GType of first argument of message, or %G_TYPE_NONE.
 * @...: %G_TYPE_NONE -terminated list of argument #GType's
 * 
 * Registers a new message type that the application supports.
 **/
void
matecomponent_application_register_message (MateComponentApplication *app,
				     const gchar       *name,
				     const gchar       *description,
				     GClosure          *opt_closure,
				     GType              return_type,
				     GType              first_arg_type,
				     ...)
{
	va_list var_args;

	va_start (var_args, first_arg_type);
	matecomponent_application_register_message_va (app, name, description,
						opt_closure, return_type,
						first_arg_type, var_args);
	va_end (var_args);
}


/**
 * matecomponent_application_create_serverinfo:
 * @app: a #MateComponentApplication
 * @envp: %NULL-terminated string vector, containing the enviroment
 * variables we wish to include in the server description.
 * 
 * This utility function provides a simple way to contruct a valid
 * serverinfo XML string.
 * 
 * Return value: a newly allocated string; caller must g_free() it.
 **/
gchar *
matecomponent_application_create_serverinfo (MateComponentApplication *app,
				      gchar const       *envp[])
{
	GString *description;
	int      i;
	gchar   *rv;

	description = g_string_new ("<oaf_info>\n");
	g_string_append_printf (description,
		"  <oaf_server iid=\"OAFIID:%s\" location=\"unknown\" type=\"runtime\">\n"
		"    <oaf_attribute name=\"repo_ids\" type=\"stringv\">\n"
		"       <item value=\"IDL:MateComponent/Unknown:1.0\"/>\n"
		"       <item value=\"IDL:MateComponent/Application:1.0\"/>\n"
		"    </oaf_attribute>\n"
		"    <oaf_attribute name=\"name\" type=\"string\" value=\"%s\"/>\n"
		"    <oaf_attribute name=\"description\" type=\"string\" "
		" value=\"%s application instance\"/>\n",
		app->name, app->name, app->name);

	if (envp && envp[0]) {
		g_string_append (description, "    <oaf_attribute name="
				 "\"matecomponent:environment\" type=\"stringv\">\n");
		for (i = 0; envp[i]; ++i)
			g_string_append_printf (description,
						"       <item value=\"%s\"/>\n",
						envp[i]);
		g_string_append (description, "    </oaf_attribute>");
	}
	g_string_append (description,
			 "  </oaf_server>\n"
			 "</oaf_info>");
	rv = description->str;
	g_string_free (description, FALSE);
	return rv;
}

/**
 * matecomponent_application_register_unique:
 * @app: a #MateComponentApplication instance
 * @serverinfo: the XML server
 * description. matecomponent_application_create_server_description() may be
 * used to easily create such description.
 * @client: output parameter that will contain a client object, in
 * case another instance has already running, or %NULL if we are the
 * first to register.
 * 
 * Try to register the running application, or check for an existing
 * application already registered and get a reference to it.
 * Applications already running but on different environments (as
 * defined by the matecomponent:environenment server property) than this one
 * are ignored and do not interfere.
 *
 * If the registration attempt indicates that another instance of this
 * application is already running, then the output variable
 * @client will receive a newly created #MateComponentAppClient
 * associated with the running application.  Otherwise, *@client is
 * set to %NULL.
 * 
 * Return value: the registration result.
 * %MateComponent_ACTIVATION_REG_SUCCESS means the application was registered,
 * since no other running instance was detected.  If, however, a
 * running application is detected,
 * %MateComponent_ACTIVATION_REG_ALREADY_ACTIVE is returned.
 **/
MateComponent_RegistrationResult
matecomponent_application_register_unique (MateComponentApplication  *app,
				    gchar const        *serverinfo,
				    MateComponentAppClient   **client)
{
	MateComponent_RegistrationResult  reg_res;
	gchar                     *iid;
	CORBA_Object               remote_obj = CORBA_OBJECT_NIL;
	CORBA_Environment          ev;
	int                        tries = 10;

	g_return_val_if_fail (app, MateComponent_ACTIVATION_REG_ERROR);
	g_return_val_if_fail (MATECOMPONENT_IS_APPLICATION (app), MateComponent_ACTIVATION_REG_ERROR);
	g_return_val_if_fail (serverinfo, MateComponent_ACTIVATION_REG_ERROR);
	g_return_val_if_fail (client, MateComponent_ACTIVATION_REG_ERROR);

	iid     = g_strdup_printf ("OAFIID:%s", app->name);
	*client = NULL;
	while (tries--)
	{
		reg_res = matecomponent_activation_register_active_server_ext
			(iid, matecomponent_object_corba_objref (MATECOMPONENT_OBJECT (app)), NULL,
			 MateComponent_REGISTRATION_FLAG_NO_SERVERINFO, &remote_obj,
			 serverinfo);
		if (reg_res == MateComponent_ACTIVATION_REG_SUCCESS)
			break;
		else if (reg_res == MateComponent_ACTIVATION_REG_ALREADY_ACTIVE) {
			CORBA_exception_init (&ev);
			MateComponent_Unknown_ref (remote_obj, &ev);
			if (ev._major != CORBA_NO_EXCEPTION) {
				  /* Likely cause: server has quit, leaving a
				   * stale reference.  Solution: keep trying
				   * to register as application server. */
				CORBA_exception_free (&ev);
				continue;
			}
			*client = matecomponent_app_client_new ((MateComponent_Application) remote_obj);
			break;
		}
	}
	g_free (iid);
	return reg_res;
}


/**
 * matecomponent_application_add_hook:
 * @func: hook function
 * @data: user data
 * 
 * Add a hook function to be called whenever a new #MateComponentApplication
 * instance is created.
 **/
void matecomponent_application_add_hook (MateComponentAppHookFunc func, gpointer data)
{
	MateComponentAppHook hook;

	if (matecomponent_application_hooks == NULL)
		matecomponent_application_hooks = g_array_new (FALSE, FALSE, sizeof (MateComponentAppHook));
	
	hook.func = func;
	hook.data = data;
	g_array_append_val (matecomponent_application_hooks, hook);
}


/**
 * matecomponent_application_remove_hook:
 * @func: hook function
 * @data: user data
 * 
 * Removes a hook function previously set with matecomponent_application_add_hook().
 **/
void matecomponent_application_remove_hook (MateComponentAppHookFunc func, gpointer data)
{
	MateComponentAppHook *hook;
	int            i;

	g_return_if_fail (matecomponent_application_hooks);

	for (i = 0; i < matecomponent_application_hooks->len; ++i) {
		hook = &g_array_index (matecomponent_application_hooks, MateComponentAppHook, i);
		if (hook->func == func && hook->data == data) {
			g_array_remove_index (matecomponent_application_hooks, i);
			return;
		}
	}

	g_warning ("matecomponent_application_remove_hook: "
		   "(func, data) == (%p, %p) not found.", func, data);
}


static void
matecomponent_application_invoke_hooks (MateComponentApplication *app)
{
	MateComponentAppHook *hook;
	int            i;

	if (!matecomponent_application_hooks)
		return;

	for (i = 0; i < matecomponent_application_hooks->len; ++i) {
		hook = &g_array_index (matecomponent_application_hooks, MateComponentAppHook, i);
		hook->func (app, hook->data);
	}
}

