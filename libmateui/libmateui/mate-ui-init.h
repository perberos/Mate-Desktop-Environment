/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef MATE_INIT_H
#define MATE_INIT_H

#include <libmate/mate-program.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBMATEUI_PARAM_CRASH_DIALOG	"show-crash-dialog"
#define LIBMATEUI_PARAM_DISPLAY	"display"
#define LIBMATEUI_PARAM_DEFAULT_ICON	"default-icon"

#define LIBMATEUI_MODULE libmateui_module_info_get()
const MateModuleInfo * libmateui_module_info_get (void) G_GNUC_CONST;
#define MATE_GTK_MODULE mate_gtk_module_info_get()
const MateModuleInfo * mate_gtk_module_info_get (void) G_GNUC_CONST;


#ifndef MATE_DISABLE_DEPRECATED
/* use #mate_program_init with the LIBMATEUI_MODULE */
int mate_init_with_popt_table (const char *app_id,
				const char *app_version,
				int argc, char **argv,
				const struct poptOption *options,
				int flags,
				poptContext *return_ctx);
#define mate_init(app_id,app_version,argc,argv) \
	mate_init_with_popt_table (app_id, app_version, \
				    argc, argv, NULL, 0, NULL)
#endif


#ifdef __cplusplus
}
#endif

#endif
