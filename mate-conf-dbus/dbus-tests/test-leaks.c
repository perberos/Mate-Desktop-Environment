/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

#include <mateconf/mateconf-client.h>

#define KEY1 "/foo/bar/sliff"
#define KEY2 "/foo/bar/sloff"

static gboolean stress_test = FALSE;
static gboolean notify_test = FALSE;

static GOptionEntry entries[] = {
	{ "stress-test", 's', 0, G_OPTION_ARG_NONE, 
	  &stress_test, "Stress test mateconf with gets and sets", NULL },
	{ "notify-test", 'n', 0, G_OPTION_ARG_NONE, 
	  &notify_test, "Test notifications", NULL },
	{ NULL }
};

static GMainLoop   *main_loop = NULL;
static MateConfClient *client = NULL;

static gboolean
stress_timeout_func (gpointer data)
{
	static gboolean  on = FALSE;
	static gint      count = 1;
	gchar           *str1, *str2;

	/* Get current value */
	str1 = mateconf_client_get_string (client, KEY1, NULL);
	str2 = mateconf_client_get_string (client, KEY2, NULL);

	g_print ("Getting value, key1:'%s', key2:'%s' (%3.3d of 200)\n", 
		 str1, str2, count);

	g_free (str1);
	g_free (str2);

	/* Set new value */
	str1 = g_strdup_printf ("%d", on ? 1 : 0);
	str2 = g_strdup_printf ("%d", on ? 0 : 1);

	mateconf_client_set_string (client, KEY1, str1, NULL);
	mateconf_client_set_string (client, KEY2, str2, NULL);

	g_free (str1);
	g_free (str2);

	/* Switch static variables */
	on = !on;

	if (++count > 200) {
		g_main_loop_quit (main_loop);
		return FALSE;
	}

	return TRUE;
}

static gboolean
notify_timeout_func (gpointer data)
{
	static gint  i = 1;
	static gint  j = -1;
	gchar       *str1, *str2;

	str1 = g_strdup_printf ("test %3.3d", i);
	str2 = g_strdup_printf ("test %3.3d", j);

	g_print ("Setting value, key1:'%s', key2:'%s'\n", str1, str2);
	
	mateconf_client_set_string (client, KEY1, str1, NULL);
	mateconf_client_set_string (client, KEY2, str2, NULL);

	g_free (str1);
	g_free (str2);
	
	i++;
	j--;

	return TRUE;
}

static void
notify_func (MateConfClient *client,
	     guint        cnxn_id,
	     MateConfEntry  *entry,
	     gpointer     user_data)
{
	static gint  count = 0;
	const gchar *key;
	gchar       *str;

	key = mateconf_entry_get_key (entry);
	str = mateconf_client_get_string (client, key, NULL);

	g_print ("Notification of key:'%s' value:'%s'\n", 
		 key, str);

	g_free (str);

	if (++count >= 200) {
		g_main_loop_quit (main_loop);
	}
}
	
int
main (int argc, char **argv)
{
	GOptionContext *context;
	gint            success = EXIT_SUCCESS;

	g_type_init ();

	context = g_option_context_new ("- test for leaks");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);
	
	client = mateconf_client_get_default ();
	
	mateconf_client_add_dir (client,
			      KEY1,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE,
			      NULL);
	mateconf_client_add_dir (client,
			      KEY2,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE,
			      NULL);
	
	main_loop = g_main_loop_new (NULL, FALSE);

	if (stress_test) {
		g_timeout_add (5, stress_timeout_func, NULL);
		g_main_loop_run (main_loop);
	} else if (notify_test) {
		mateconf_client_notify_add (client,
					 KEY1,
					 notify_func,
					 NULL,
					 NULL,
					 NULL);
		mateconf_client_notify_add (client,
					 KEY2,
					 notify_func,
					 NULL,
					 NULL,
					 NULL);
		
		g_timeout_add (25, notify_timeout_func, NULL);
		g_main_loop_run (main_loop);
	} else {
		g_printerr ("Use --help for usage\n");
		success = EXIT_FAILURE;
	}

	g_main_loop_unref (main_loop);

	return success;
}
