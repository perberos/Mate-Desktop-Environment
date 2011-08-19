/* Random utility functions for menu code */

/*
 * Copyright (C) 2003 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MENU_UTIL_H__
#define __MENU_UTIL_H__

#include <glib.h>

#include "menu-layout.h"

G_BEGIN_DECLS

#ifdef G_ENABLE_DEBUG

void menu_verbose (const char *format, ...) G_GNUC_PRINTF (1, 2);

void menu_debug_print_layout (MenuLayoutNode *node,
                              gboolean        onelevel);

#else /* !defined(G_ENABLE_DEBUG) */

#ifdef G_HAVE_ISO_VARARGS
#define menu_verbose(...)
#elif defined(G_HAVE_GNUC_VARARGS)
#define menu_verbose(format...)
#else
#error "Cannot disable verbose mode due to lack of varargs macros"
#endif

#define menu_debug_print_layout(n,o)

#endif /* G_ENABLE_DEBUG */

G_END_DECLS

#endif /* __MENU_UTIL_H__ */
