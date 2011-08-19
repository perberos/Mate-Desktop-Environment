#include <config.h>
#include <stdio.h>
#include <libmatecomponent.h>

static MateComponent_PropertyBag	 pb;
static CORBA_Environment	 ev;
static MateComponent_PropertyBag       pb;

static char *
simple_print_type (CORBA_TypeCode tc)
{
	static char s[1024];

	switch (tc->kind) {
	case CORBA_tk_boolean:
		g_snprintf (s, sizeof (s), "boolean");
		break;
	case CORBA_tk_short:
		g_snprintf (s, sizeof (s), "short");
		break;
	case CORBA_tk_ushort:
		g_snprintf (s, sizeof (s), "ushort");
		break;
	case CORBA_tk_long:
		g_snprintf (s, sizeof (s), "long");
		break;
	case CORBA_tk_ulong:
		g_snprintf (s, sizeof (s), "ulong");
		break;
	case CORBA_tk_float:
		g_snprintf (s, sizeof (s), "float");
		break;
	case CORBA_tk_double:
		g_snprintf (s, sizeof (s), "double");
		break;
	case CORBA_tk_string:
		g_snprintf (s, sizeof (s), "string");
		break;
	default:
		g_snprintf (s, sizeof (s), "Unknown");
		break;
	}

	return s;
}

static char *
simple_print_value (char *name, CORBA_TypeCode tc)
{
	static char s[1024];

	switch (tc->kind) {
	case CORBA_tk_boolean:
		g_snprintf (s, sizeof (s), "%s",
		    matecomponent_pbclient_get_boolean (pb, name, NULL) ?
			"True" : "False");
		break;
	case CORBA_tk_long:
		g_snprintf (s, sizeof (s), "%d",
		    matecomponent_pbclient_get_long (pb, name, NULL));
		break;
	case CORBA_tk_float:
		g_snprintf (s, sizeof (s), "%f",
		    matecomponent_pbclient_get_float (pb, name, NULL));
		break;
	case CORBA_tk_double:
		g_snprintf (s, sizeof (s), "%f",
		    matecomponent_pbclient_get_double (pb, name, NULL));
		break;
	case CORBA_tk_string:
		g_snprintf (s, sizeof (s), "%s",
		    matecomponent_pbclient_get_string (pb, name, NULL));
		break;
	default:
		g_snprintf (s, sizeof (s), "Unknown");
		break;
	}

	return s;
}


static char *
simple_print_default_value (char *name, CORBA_TypeCode tc)
{
	static char s[1024];

	switch (tc->kind) {
	case CORBA_tk_boolean:
		g_snprintf (s, sizeof (s), "%s",
		    matecomponent_pbclient_get_default_boolean (pb, name, NULL) ?
			"True" : "False");
		break;
	case CORBA_tk_long:
		g_snprintf (s, sizeof (s), "%d",
		    matecomponent_pbclient_get_default_long (pb, name, NULL));
		break;
	case CORBA_tk_float:
		g_snprintf (s, sizeof (s), "%f",
		    matecomponent_pbclient_get_default_float (pb, name, NULL));
		break;
	case CORBA_tk_double:
		g_snprintf (s, sizeof (s), "%f",
		    matecomponent_pbclient_get_default_double (pb, name, NULL));
		break;
	case CORBA_tk_string:
		g_snprintf (s, sizeof (s), "%s",
		    matecomponent_pbclient_get_default_string (pb, name, NULL));
		break;
	default:
		g_snprintf (s, sizeof (s), "Unknown");
		break;
	}

	return s;
}


static char *
simple_print_read_only (char *name)
{
	MateComponent_PropertyFlags flags;

	flags = matecomponent_pbclient_get_flags (pb, name, NULL);

	return (flags & MateComponent_PROPERTY_READABLE) ?
		"ReadOnly" : "ReadWrite";
}

static void
print_props (void)
{
	GList *props;
	GList *l;

	props = matecomponent_pbclient_get_keys (pb, NULL);

	for (l = props; l != NULL; l = l->next) {
		CORBA_TypeCode tc;
		char *name = l->data;

		tc = matecomponent_pbclient_get_type (pb, name, NULL);

		g_print ("%s [%s] %s %s %s\n",
			 name,
			 simple_print_type (tc),
			 simple_print_value (name, tc),
			 simple_print_default_value (name, tc),
			 simple_print_read_only (name));

		CORBA_Object_release ((CORBA_Object) tc, NULL);
	}

	matecomponent_pbclient_free_keys (props);
}


static guint
create_bag_client (void)
{
	print_props ();

	matecomponent_pbclient_set_boolean (pb, "boolean-test", FALSE, NULL);
	matecomponent_pbclient_set_long    (pb, "long-test", 3, NULL);
	matecomponent_pbclient_set_float   (pb, "float-test", 0.00001, NULL);
	matecomponent_pbclient_set_double  (pb, "double-test", 2.0001, NULL);
	matecomponent_pbclient_set_string  (pb, "string-test",
				     "life is a wonderful gift", NULL);

	matecomponent_pbclient_set (
		pb, NULL,
		"boolean-test", TC_CORBA_boolean, FALSE,
		"long-test", TC_CORBA_long, 3,
		"float-test", TC_CORBA_float, 0.00001,
		"double-test", TC_CORBA_double, 2.0001,
		"string-test", TC_CORBA_string, "you are a precious flower",
		NULL);

	{
		CORBA_any a;
		CORBA_boolean b = TRUE;
		a._type = TC_CORBA_boolean;
		a._value = &b;
		matecomponent_pbclient_set_value_async
			(pb, "boolean-test", &a, NULL);
	}

	matecomponent_main_quit ();

	return FALSE;
}


int
main (int argc, char **argv)
{
	CORBA_ORB orb;
	CORBA_exception_init (&ev);

        g_thread_init (NULL);

	if (!matecomponent_init (&argc, argv))
		g_error ("Could not initialize MateComponent");

	orb = matecomponent_orb ();

	{
		int   size;
		char  ior [1024];
		FILE *infile = fopen ("iorfile","rb");

		if (!infile)
			g_error ("Start the server before running the client");

		size = fread (ior,1,1024,infile);
		fclose (infile);
		ior [size] = '\0';   /* insure that string is terminated correctly */

		pb = CORBA_ORB_string_to_object (orb, ior, &ev);
		g_assert (ev._major == CORBA_NO_EXCEPTION);
	}
	
	if (pb == CORBA_OBJECT_NIL ||
	    CORBA_Object_non_existent (pb, &ev)) {
		g_error ("Could not bind to PropertyBag object");
		return 1;
	}

	g_idle_add ((GSourceFunc) create_bag_client, NULL);

	matecomponent_main ();

	matecomponent_object_release_unref (pb, NULL);

	return matecomponent_debug_shutdown ();
}
