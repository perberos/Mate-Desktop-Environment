#include <config.h>

#include <glib.h>

#include "procman.h"
#include "procman_matesu.h"
#include "util.h"

gboolean (*matesu_exec)(const char *commandline);


static void
load_matesu(void)
{
	static gboolean init;

	if (init)
		return;

	init = TRUE;

	load_symbols("libmatesu.so.0",
		     "matesu_exec", &matesu_exec,
		     NULL);
}


gboolean
procman_matesu_create_root_password_dialog(const char *command)
{
	return matesu_exec(command);
}


gboolean
procman_has_matesu(void)
{
	load_matesu();
	return matesu_exec != NULL;
}
