#include <config.h>

#include <glib.h>

#include "selinux.h"
#include "procman.h"
#include "util.h"


static int (*getpidcon)(pid_t, char**);
static void (*freecon)(char*);
static int (*is_selinux_enabled)(void);

static gboolean has_selinux;

static gboolean load_selinux(void)
{
	return load_symbols("libselinux.so.1",
			    "getpidcon", &getpidcon,
			    "freecon", &freecon,
			    "is_selinux_enabled", &is_selinux_enabled,
			    NULL);
}



void
get_process_selinux_context (ProcInfo *info)
{
	char *con;

	if (has_selinux && !getpidcon (info->pid, &con)) {
		info->security_context = g_strdup (con);
		freecon (con);
	}
}



gboolean
can_show_security_context_column (void)
{
	if (!(has_selinux = load_selinux()))
		return FALSE;

	switch (is_selinux_enabled()) {
	case 1:
		/* We're running on an SELinux kernel */
		return TRUE;

	case -1:
		/* Error; hide the security context column */

	case 0:
		/* We're not running on an SELinux kernel:
		   hide the security context column */

	default:
		g_warning("SELinux was found but is not enabled.\n");
		return FALSE;
	}
}



