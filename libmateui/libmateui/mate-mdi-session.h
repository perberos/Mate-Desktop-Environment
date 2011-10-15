/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * mate-mdi-session.h - session managament functions
 * written by Martin Baulig <martin@home-of-linux.org>
 */

#ifndef __MATE_MDI_SESSION_H__
#define __MATE_MDI_SESSION_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <string.h>

#include "libmateui/mate-mdi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This function should parse the config string and return a newly
 * created MateMDIChild. */
typedef MateMDIChild *(*MateMDIChildCreator) (const gchar *);

/* mate_mdi_restore_state(): call this with the MateMDI object, the
 * config section name and the function used to recreate the MateMDIChildren
 * from their config strings. */
gboolean	mate_mdi_restore_state	(MateMDI *mdi, const gchar *section,
					 MateMDIChildCreator create_child_func);

/* mate_mdi_save_state (): call this with the MateMDI object as the
 * first and the config section name as the second argument. */
void		mate_mdi_save_state	(MateMDI *mdi, const gchar *section);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif
