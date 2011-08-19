#include <config.h>

#include "procman.h"
#include "procman_gksu.h"
#include "util.h"

static gboolean (*gksu_run)(const char *, GError **);


static void load_gksu(void)
{
	static gboolean init;

	if (init)
		return;

	init = TRUE;

	load_symbols("libgksu2.so",
		     "gksu_run", &gksu_run,
		     NULL);
}





gboolean procman_gksu_create_root_password_dialog(const char *command)
{
	GError *e = NULL;

	/* Returns FALSE or TRUE on success, depends on version ... */
	gksu_run(command, &e);

	if (e) {
		g_critical("Could not run gksu_run(\"%s\") : %s\n",
			   command, e->message);
		g_error_free(e);
		return FALSE;
	}

	g_message("gksu_run did fine\n");
	return TRUE;
}



gboolean
procman_has_gksu(void)
{
	load_gksu();
	return gksu_run != NULL;
}

