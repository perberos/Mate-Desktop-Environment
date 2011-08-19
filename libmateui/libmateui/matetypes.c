#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <gtk/gtk.h>
#include <libmateui.h>

void mate_type_init (void);

void mate_type_init (void) {
	static gboolean initialized = FALSE;

	if (initialized)
		return;

#include "matetype_inits.c"

	initialized = TRUE;
}
