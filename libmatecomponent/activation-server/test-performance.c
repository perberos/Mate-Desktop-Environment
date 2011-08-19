#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "server.h"

static GTimer *timer;

#ifdef G_OS_WIN32

/* On Win32, as test-performance isn't installed, we cannot deduce the
 * "real" installation directory, and must assume that the
 * configure-time paths are valid. But running test-performance on the
 * build machine only is the intention anyway.
 */

const char *
server_win32_replace_prefix (const char *configure_time_path)
{
  return g_strdup (configure_time_path);
}

#endif

static void
test_server_info_load (void)
{
	int i;
	char *dirs [] = { SERVERINFODIR, NULL };
	MateComponent_ServerInfoList servers;
	GPtrArray *runtime_servers = g_ptr_array_new ();
	GHashTable *hash = NULL;

	fprintf (stderr, "Testing server info load ...");

	g_timer_start (timer);
	for (i = 0; i < 10; i++)
		matecomponent_server_info_load (dirs, &servers, runtime_servers, &hash,
					 matecomponent_activation_hostname_get ());

	g_hash_table_destroy (hash);

	fprintf (stderr, " %g(ms)\n",
		 g_timer_elapsed (timer, NULL) * 1000.0 / 10);
}

int
main (int argc, char *argv[])
{
	free (malloc (8));

	timer = g_timer_new ();
	g_timer_start (timer);

	add_initial_locales ();

	test_server_info_load ();

	if (g_getenv ("_MEMPROF_SOCKET")) {
		g_warning ("Waiting for memprof\n");
		g_main_context_iteration (NULL, TRUE);
	}

	return 0;
}
