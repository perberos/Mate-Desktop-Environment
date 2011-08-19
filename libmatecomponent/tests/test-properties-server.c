#include <config.h>
#include <stdio.h>
#include <string.h>
#include <glib-object.h>
#include <libmatecomponent.h>

static CORBA_ORB	    orb;
static MateComponentPropertyBag  *pb;
static CORBA_Environment   ev;

enum {
       PROP_BOOLEAN_TEST,
       PROP_INTEGER_TEST,
       PROP_LONG_TEST,
       PROP_FLOAT_TEST,
       PROP_DOUBLE_TEST,
       PROP_STRING_TEST
};

typedef struct {
	gint      i;
	glong     l;
	gboolean  b;
	gfloat    f;
	gdouble   d;
	char     *s;
} PropData;

static void
set_prop (MateComponentPropertyBag *bag,
	  const MateComponentArg   *arg,
	  guint              arg_id,
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	PropData *pd = user_data;

	switch (arg_id) {
	case PROP_BOOLEAN_TEST:
		pd->b = MATECOMPONENT_ARG_GET_BOOLEAN (arg);
		break;

	case PROP_INTEGER_TEST:
		pd->i = MATECOMPONENT_ARG_GET_INT (arg);
		break;

	case PROP_LONG_TEST:
		pd->l = MATECOMPONENT_ARG_GET_LONG (arg);
		break;

	case PROP_FLOAT_TEST:
		pd->f = MATECOMPONENT_ARG_GET_FLOAT (arg);
		break;

	case PROP_DOUBLE_TEST:
		pd->d = MATECOMPONENT_ARG_GET_DOUBLE (arg);
		break;

	case PROP_STRING_TEST:
		pd->s = MATECOMPONENT_ARG_GET_STRING (arg);
		break;

	default:
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
	};
}

static void
get_prop (MateComponentPropertyBag *bag,
	  MateComponentArg         *arg,
	  guint              arg_id,
	  CORBA_Environment *ev,
	  gpointer           user_data)
{
	PropData *pd = user_data;

	switch (arg_id) {
	case PROP_BOOLEAN_TEST:
		MATECOMPONENT_ARG_SET_BOOLEAN (arg, pd->b);
		break;

	case PROP_INTEGER_TEST:
		MATECOMPONENT_ARG_SET_INT (arg, pd->i);
		break;

	case PROP_LONG_TEST:
		MATECOMPONENT_ARG_SET_LONG (arg, pd->l);
		break;

	case PROP_FLOAT_TEST:
		MATECOMPONENT_ARG_SET_FLOAT (arg, pd->f);
		break;

	case PROP_DOUBLE_TEST:
		MATECOMPONENT_ARG_SET_DOUBLE (arg, pd->d);
		break;

	case PROP_STRING_TEST:
		MATECOMPONENT_ARG_SET_STRING (arg, pd->s);
		break;

	default:
		matecomponent_exception_set (ev, ex_MateComponent_PropertyBag_NotFound);
	};
}

static char *
simple_prop_to_string (MateComponentArg *arg)
{
	static char s [1024];

	if (!arg) {
		g_snprintf (s, sizeof (s), "NULL");
		return g_strdup (s);
	}
	
	g_assert (arg->_type != NULL);
	
	switch (arg->_type->kind) {

	case CORBA_tk_boolean:
		g_snprintf (s, sizeof (s), "boolean: %s",
			    MATECOMPONENT_ARG_GET_BOOLEAN (arg) ? "True" : "False");
		break;

	case CORBA_tk_long:
		g_snprintf (s, sizeof (s), "integer: %d",
			    MATECOMPONENT_ARG_GET_INT (arg));
		break;

	case CORBA_tk_float:
		g_snprintf (s, sizeof (s), "float: %f",
			    MATECOMPONENT_ARG_GET_FLOAT (arg));
		break;

	case CORBA_tk_double:
		g_snprintf (s, sizeof (s), "double: %g",
			    MATECOMPONENT_ARG_GET_DOUBLE (arg));
		break;

	case CORBA_tk_string: {
		g_snprintf (s, sizeof (s), "string: '%s'",
			    MATECOMPONENT_ARG_GET_STRING (arg));
		break;
	}

	default:
		g_error ("Unhandled type: %u", arg->_type->kind);
		break;
	}

	return g_strdup (s);
}

static void
create_bag (void)
{
	PropData   *pd = g_new0 (PropData, 1);
	MateComponentArg  *def;
	char       *dstr;
	CORBA_char *ior;
	FILE       *iorfile;

	pd->i = 987654321;
	pd->l = 123456789;
	pd->b = TRUE;
	pd->f = 2.71828182845;
	pd->d = 3.14159265358;
	pd->s = "Hello world";

	/* Create the property bag. */
	pb = matecomponent_property_bag_new (get_prop, set_prop, pd);

	dstr = "Testing 1 2 3";

	/* Add properties */
	matecomponent_property_bag_add (pb, "int-test", PROP_INTEGER_TEST,
				 MATECOMPONENT_ARG_INT, NULL, dstr, 0);

	def = matecomponent_arg_new (MATECOMPONENT_ARG_STRING);
	MATECOMPONENT_ARG_SET_STRING (def, "a default string");

	matecomponent_property_bag_add (pb, "string-test", PROP_STRING_TEST,
				 MATECOMPONENT_ARG_STRING, def, dstr, 
				 MateComponent_PROPERTY_READABLE |
				 MateComponent_PROPERTY_WRITEABLE);

	matecomponent_property_bag_add (pb, "long-test", PROP_LONG_TEST,
				 MATECOMPONENT_ARG_LONG, NULL, dstr, 0);

	matecomponent_property_bag_add (pb, "boolean-test", PROP_BOOLEAN_TEST,
				 MATECOMPONENT_ARG_BOOLEAN, NULL, dstr, 0);

	def = matecomponent_arg_new (MATECOMPONENT_ARG_FLOAT);
	MATECOMPONENT_ARG_SET_FLOAT (def, 0.13579);
	matecomponent_property_bag_add (pb, "float-test", PROP_FLOAT_TEST,
				 MATECOMPONENT_ARG_FLOAT, def, dstr, 0);

	matecomponent_property_bag_add (pb, "double-test", PROP_DOUBLE_TEST,
				 MATECOMPONENT_ARG_DOUBLE, NULL, dstr, 0);

	iorfile = fopen ("iorfile", "wb");

	/* Print out the IOR for this object. */
	ior = CORBA_ORB_object_to_string (orb, MATECOMPONENT_OBJREF (pb), &ev);

	/* So we can tee the output to compare */
	fwrite (ior, strlen (ior), 1, iorfile);
	fclose (iorfile);

	CORBA_free (ior);
}

static void
print_props (void)
{
	GList *props;
	GList *l;

	/* This is a private function; we're just using it for testing. */
	props = matecomponent_property_bag_get_prop_list (pb);

	for (l = props; l != NULL; l = l->next) {
		MateComponentArg *arg;
		MateComponentProperty *prop = l->data;
		char           *s1, *s2;

		arg = matecomponent_pbclient_get_value (MATECOMPONENT_OBJREF(pb), prop->name,
						NULL, NULL);

		s1  = simple_prop_to_string (arg);

		matecomponent_arg_release (arg);

		s2  = simple_prop_to_string (prop->default_value);

		g_print ("Prop %12s [%2u] %s %s %s %s %s %s\n",
			 prop->name, prop->type->kind,
			 s1, s2,
			 prop->doctitle,
			 prop->flags & MateComponent_PROPERTY_READABLE        ? "Readable"         : "NotReadable",
			 prop->flags & MateComponent_PROPERTY_WRITEABLE       ? "Writeable"        : "NotWriteable",
			 prop->flags & MateComponent_PROPERTY_NO_LISTENING    ? "NoListening" : "Listeners-enabled");

		g_free (s1);
		g_free (s2);

	}

	g_list_free (props);
}

static void
quit_main (GObject *object)
{
	matecomponent_main_quit ();
}

int
main (int argc, char **argv)
{
	MateComponentObject *context;

        g_thread_init (NULL);

	if (!matecomponent_init (&argc, argv))
		g_error ("Could not initialize MateComponent");

	orb = matecomponent_orb ();

	create_bag ();

	print_props ();

	/* FIXME: this is unusual, with a factory you normally
	 * want to use matecomponent_running_context_auto_exit_unref */
	context = matecomponent_context_running_get ();
	g_signal_connect_data (
		G_OBJECT (context), "last_unref",
		G_CALLBACK (quit_main), NULL, NULL, 0);
	matecomponent_object_unref (context);

	matecomponent_main ();

	return matecomponent_debug_shutdown ();
}
