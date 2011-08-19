#ifndef PROCMAN_SELINUX_H_20050525
#define PROCMAN_SELINUX_H_20050525

#include <glib.h>

#include "procman.h"

void
get_process_selinux_context (ProcInfo *info);

gboolean
can_show_security_context_column (void) G_GNUC_CONST;

#endif /* PROCMAN_SELINUX_H_20050525 */
