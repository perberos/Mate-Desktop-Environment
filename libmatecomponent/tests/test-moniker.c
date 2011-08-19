#include <config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libmatecomponent.h>

static void
check_string (const char *prefix, const char *escaped, const char *unescaped)
{
	MateComponentMoniker *moniker;
	const char    *const_str;
	char          *str;
	char          *s, *name;
	CORBA_Environment ev;
	CORBA_long        equal;

	moniker = matecomponent_moniker_construct (
		g_object_new (matecomponent_moniker_get_type (), NULL), prefix);
	
	name = g_strconcat (prefix, escaped, NULL);
	matecomponent_moniker_set_name (moniker, name);

	const_str = matecomponent_moniker_get_name (moniker);
	fprintf (stderr, "'%s' == '%s'\n", unescaped, const_str);
	g_assert (!strcmp (const_str, unescaped));

	CORBA_exception_init (&ev);
	equal = MateComponent_Moniker_equal (MATECOMPONENT_OBJREF (moniker), name, &ev);
	g_assert (!MATECOMPONENT_EX (&ev));
	g_assert (equal);
	CORBA_exception_free (&ev);

	s = g_strconcat (prefix, escaped, NULL);
	str = matecomponent_moniker_get_name_escaped (moniker);
	fprintf (stderr, "'%s' == '%s'\n", str, s);
	g_assert (!strcmp (str, s));
	g_free (str);
	g_free (s);

	g_assert (matecomponent_moniker_client_equal (
		MATECOMPONENT_OBJREF (moniker), name, NULL));

	matecomponent_object_unref (MATECOMPONENT_OBJECT (moniker));

	g_free (name);
}

static void
check_parse_name (const char *name, const char *res, int plen)
{
	const char *mname;
	int l;

	mname = matecomponent_moniker_util_parse_name (name, &l);

	fprintf (stderr, "result %s %s %d\n", name, mname, l);

	g_assert (!strcmp (res, mname));
	g_assert (plen == l);
}

static void
test_real_monikers (void)
{
	CORBA_Environment *ev, real_ev;
	MateComponent_Unknown     object;

	CORBA_exception_init ((ev = &real_ev));

	/* Try an impossible moniker resolve */
	object = matecomponent_get_object ("OAFIID:MateComponent_Moniker_Oaf",
				    "IDL:MateComponent/PropertyBag:1.0", ev);
	g_assert (object == CORBA_OBJECT_NIL);
	if (MATECOMPONENT_EX (ev))
		printf ("%s\n", matecomponent_exception_get_text (ev));

	CORBA_exception_free (ev);
}

int
main (int argc, char *argv [])
{
        g_thread_init (NULL);

	free (malloc (8));

	if (!matecomponent_init (NULL, NULL))
		g_error ("Can not matecomponent_init");

	matecomponent_activate ();

	check_string ("a:", "\\\\", "\\");

	check_string ("a:", "\\#", "#");

	check_string ("prefix:", "\\!", "!");

	check_string ("a:",
		      "1\\!\\#\\!\\!\\#\\\\",
		      "1!#!!#\\");

	check_parse_name ("#b:", "b:", 0);

	check_parse_name ("a:#b:", "b:", 2);

	check_parse_name ("a:!b:", "!b:", 2);

	check_parse_name ("a:3456789#b:", "b:", 9);

	check_parse_name ("a:\\##b:", "b:", 4);

	check_parse_name ("a:\\#c:", "a:\\#c:", 0);

	check_parse_name ("a:\\\\##c:", "c:", 5);

	check_parse_name ("a:\\\\#b:#c:", "c:", 7);

	check_parse_name ("a:\\\\#b:\\#c:", "b:\\#c:", 4);

	check_parse_name ("a:\\\\\\#b:\\#c:", "a:\\\\\\#b:\\#c:", 0);

	test_real_monikers ();

	return matecomponent_debug_shutdown ();
}
