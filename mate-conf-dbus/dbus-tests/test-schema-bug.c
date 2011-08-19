/* gcc -Wall -g -o mateconf mateconf.c `pkg-config --libs --cflags mateconf-2.0`
 * run-standalone.sh ./mateconf
 */

#include <string.h>
#include <stdlib.h>
#include <mateconf/mateconf-client.h>

#define KEY "/foo-schemas/bar/string"

int
main (int argc, char **argv)
{
	MateConfClient *client;
	MateConfValue  *gval;
	GError      *err = NULL;
	gchar       *val;
	
	g_type_init ();
	
	system ("mateconftool-2 --install-schema-file test-schemas.schemas");
        
	client = mateconf_client_get_default ();
	
	mateconf_client_unset (client, KEY, &err);
	if (err) {
		goto error;
        }

	val = mateconf_client_get_string (client, KEY, &err);
	if (err) {
		goto error;
	}
	
	g_print ("Value after reset is: '%s'\n", val);

	if (!val || strcmp (val, "baz") != 0) {
		g_error ("Failed");
	}
	
	mateconf_client_set_string (client, KEY, "another string", &err);
	if (err) {
		goto error;
	}
	
	val = mateconf_client_get_string (client, KEY, &err);
	if (err) {
		goto error;
	}
	
	g_print ("Value after set is: '%s'\n", val);

	if (!val || strcmp (val, "another string") != 0) {
		g_error ("Failed");
	}

	gval = mateconf_client_get_default_from_schema (client, KEY, &err);
	if (err) {
		goto error;
	}
	
	if (gval) {
		val = (gchar *) mateconf_value_get_string (gval);
		g_print ("Default value: '%s'\n", val);

		if (!val || strcmp (val, "baz") != 0) {
			g_error ("Failed");
		}
	} else {
		g_print ("No default value\n");
		g_error ("Failed");
	}

error:
	if (err) {
		g_critical ("Got error: %s", err->message);
		g_error_free (err);
		return 1;
	}

	g_print ("OK\n");
	
	return 0;
}

