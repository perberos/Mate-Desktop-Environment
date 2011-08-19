/* Tests for a bug that made blablabla... FIXME: summary */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <mateconf/mateconf-client.h>

#define KEY_ROOT      "/foo"
#define KEY_UNCACHED  KEY_ROOT "/baz_primitive"
#define KEY_PRIMITIVE KEY_ROOT "/bar/baz_primitive"
#define KEY_PAIR      KEY_ROOT "/bar/baz_pair"
#define KEY_LIST      KEY_ROOT "/bar/baz_list"

#define DELTA         0.0001

static MateConfClient *client;
static GMainLoop   *loop;

static gboolean
check_string (const gchar *key, const gchar *expected)
{
	gchar    *str;
	GError   *error = NULL;
	gboolean  ret;

	str = mateconf_client_get_string (client, key, &error);
	if (error) {
		g_error_free (error);
		return FALSE;
	} 

        if (!str) {
                g_warning ("String value is NULL\n");
                return FALSE;
        }
        
	ret = strcmp (str, expected) == 0;

	g_free (str);

	return ret;
}

static gboolean
check_int (const gchar *key, gint expected)
{
	gint    i;
	GError *error = NULL;

	i = mateconf_client_get_int (client, key, &error);

	if (error) {
		g_error_free (error);
		i = 0;
	}

	return i == expected;
}

static gboolean
check_float (const gchar *key, gdouble expected)
{
	gdouble  f;
	GError  *error = NULL;

	f = mateconf_client_get_float (client, key, &error);
	if (error) {
		g_error_free (error);
		f = 0;
	}

	return fabs (f - expected) < DELTA;
}

static gboolean
check_pair (const gchar *key, gint expected_i, gdouble expected_f)
{
	gint    i;
	gdouble  f;
	GError *error = NULL;

	mateconf_client_get_pair (client, key,
			       MATECONF_VALUE_INT, MATECONF_VALUE_FLOAT,
			       &i, &f, &error);
	if (error) {
		g_error_free (error);
		i = 0;
		f = 0;
	}

	return (i == expected_i && fabs (f - expected_f) < DELTA);
}

static gboolean
check_list (const gchar *key, gint expected_1, gint expected_2, gint expected_3)
{
	gint    i1, i2, i3;
	GError *error = NULL;
	GSList *list;

	list = mateconf_client_get_list (client, key, MATECONF_VALUE_INT, &error);
	if (error) {
		g_error_free (error);
		i1 = i2 = i3 = 0;
	} else {
		if (g_slist_length (list) != 3) {
			return FALSE;
		}

		/* ye ye, efficiency ;) */
		i1 = GPOINTER_TO_INT (g_slist_nth_data (list, 0));
		i2 = GPOINTER_TO_INT (g_slist_nth_data (list, 1));
		i3 = GPOINTER_TO_INT (g_slist_nth_data (list, 2));
	}

	g_slist_free (list);
	
	return (i1 == expected_1 && i2 == expected_2 && i3 == expected_3);
}

/* Checks that everything below "key" is gone. */
static gboolean
check_recursive_unset (void)
{
	GError     *error = NULL;
	gboolean    exists;
	MateConfValue *value;

	exists = mateconf_client_dir_exists (client, KEY_ROOT, &error);
	if (error) {
                g_print ("Error when checking if root exists: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	if (exists) {
                /* Note that this might exist or might not. Upstream behaves
                 * like this too for directories. The values are unset but the
                 * directory structure still exists, until the next time the
                 * database is synced to disk.
                 */
		/*g_print ("Root exists: %s\n", KEY_ROOT);
                  return FALSE;*/
	}

	exists = mateconf_client_dir_exists (client, KEY_ROOT "/bar", &error);
	if (error) {
                g_print ("Error when checking if dir exists: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	if (exists) {
                /* Same as above. */
		/*g_print ("Subdirectory exists: %s\n", KEY_ROOT "/bar");
                  return FALSE;*/
	}

	value = mateconf_client_get (client, KEY_PRIMITIVE, &error);
	if (error) {
                g_print ("Error when checking primitie key: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	if (value) {
		g_print ("Primitive key exists: %s\n", mateconf_value_to_string (value));
		mateconf_value_free (value);
		return FALSE;
	}
	
	value = mateconf_client_get (client, KEY_PAIR, &error);
	if (error) {
                g_print ("Error when checking pair key: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	if (value) {
		g_print ("Pair key exists: %s\n", mateconf_value_to_string (value));
		mateconf_value_free (value);
		return FALSE;
	}
	
	value = mateconf_client_get (client, KEY_LIST, &error);
	if (error) {
                g_print ("Error when checking list key: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	if (value) {
		g_print ("List key exists: %s\n", mateconf_value_to_string (value));
		mateconf_value_free (value);
		return FALSE;
	}
	
	return TRUE;
}

static gboolean
change_timeout_func (gpointer data)
{
	static gint  count = 0;
	gchar       *s;
	gint         i, i1, i2, i3;
	gdouble      f;
	GSList      *list;

	/* String, uncached. */
	s = g_strdup_printf ("test-%d", g_random_int_range (1, 100));
	mateconf_client_set_string (client, KEY_UNCACHED, s, NULL);
	if (!check_string (KEY_UNCACHED, s)) {
		g_print ("Uncached string FAILED\n");
		exit (1);
	}
	g_free (s);
	
	/* String. */
	s = g_strdup_printf ("test-%d", g_random_int_range (1, 100));
	mateconf_client_set_string (client, KEY_PRIMITIVE, s, NULL);
	if (!check_string (KEY_PRIMITIVE, s)) {
		g_print ("String FAILED\n");
		exit (1);
	}
	g_free (s);
	
	/* Int. */
	i = g_random_int_range (1, 100);
	mateconf_client_set_int (client, KEY_PRIMITIVE, i, NULL);
	if (!check_int (KEY_PRIMITIVE, i)) {
		g_print ("Int FAILED\n");
		exit (1);
	}
	
	/* Float. */
	f = g_random_int_range (1, 1000) / 10.0;
	mateconf_client_set_float (client, KEY_PRIMITIVE, f, NULL);
	if (!check_float (KEY_PRIMITIVE, f)) {
		g_print ("Float FAILED\n");
		exit (1);
	}
	
	/* Pair (int, float). */
	i = g_random_int_range (1, 1000);
	f = g_random_int_range (1, 10000) / 37.2;

	mateconf_client_set_pair (client, KEY_PAIR,
			       MATECONF_VALUE_INT,
			       MATECONF_VALUE_FLOAT,
			       &i, &f, NULL);
	if (!check_pair (KEY_PAIR, i, f)) {
		g_print ("Pair FAILED\n");
		exit (1);
	}

	/* List of ints. */
	i1 = g_random_int_range (1, 1000);
	i2 = g_random_int_range (1, 1000);
	i3 = g_random_int_range (1, 1000);

	list = g_slist_append (NULL, GINT_TO_POINTER (i1));
	list = g_slist_append (list, GINT_TO_POINTER (i2));
	list = g_slist_append (list, GINT_TO_POINTER (i3));
	mateconf_client_set_list (client, KEY_LIST,
			       MATECONF_VALUE_INT,
			       list, NULL);
	if (!check_list (KEY_LIST, i1, i2, i3)) {
		g_print ("List FAILED\n");
		exit (1);
	}

	g_slist_free (list);
	
	/* Unset int. */
	i = g_random_int_range (1, 100);
	mateconf_client_set_int (client, KEY_PRIMITIVE, i, NULL);
	if (!check_int (KEY_PRIMITIVE, i)) {
		g_print ("Unset FAILED\n");
		exit (1);
	}
	mateconf_client_unset (client, KEY_PRIMITIVE, NULL);
	if (!check_int (KEY_PRIMITIVE, 0)) { /* We get 0 as unset... */
		g_print ("Unset FAILED\n");
		exit (1);
	}
	
	/* Recursive unset. Unset the entire subtree and check that all the keys
	 * are non-existing.
	 */
    	g_assert (mateconf_client_recursive_unset (client, KEY_ROOT, 0, NULL) == TRUE);
	if (!check_recursive_unset ()) {
		g_print ("Recursive unset FAILED\n");
		exit (1);
	}

	g_print ("Run %d completed.\n", ++count);
	
	if (count == 3 || count == 6 || count == 9) {
		g_main_loop_quit (loop);
		return FALSE;
	}
	
	return TRUE;
}

int
main (int argc, char **argv)
{
	g_type_init ();

	client = mateconf_client_get_default ();

        /* Start out clean in case anything is left from earlier runs. */
        mateconf_client_recursive_unset (client, KEY_ROOT, 0, NULL);
                
        /* We add "/foo/bar" so that "/foo" is uncached to test both those
         * cases.
         */
	mateconf_client_add_dir (client,
			      "/foo/bar",
			      MATECONF_CLIENT_PRELOAD_RECURSIVE,
			      NULL);

	loop = g_main_loop_new (NULL, FALSE);
	
	g_idle_add (change_timeout_func, NULL);
	g_main_loop_run (loop);

	g_timeout_add (500, change_timeout_func, NULL);
	g_main_loop_run (loop);

	g_idle_add (change_timeout_func, NULL);
	g_main_loop_run (loop);

	g_object_unref (client);
	
	return 0;
}
