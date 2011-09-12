/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 *               2001 SuSE Linux AG
 * All rights reserved.
 *
 * This file is part of MATE 2.0.
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
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
  @NOTATION@
 */

#ifndef LIBMATEINIT_H
#define LIBMATEINIT_H

#include <libmate/mate-program.h>

G_BEGIN_DECLS

/* This is where the user specific files are stored under $HOME
 * (do not use these macros; use mate_user_dir_get(); it's possible
 * to override .mate2 via environment variable and this is
 * an important feature for environments that mix MATE versions)
 */
#define MATE_DOT_MATE ".mate2/"
#define MATE_DOT_MATE_PRIVATE ".mate2_private/"

#define LIBMATE_MODULE libmate_module_info_get()
const MateModuleInfo* libmate_module_info_get(void) G_GNUC_CONST;
#define MATE_MATECOMPONENT_MODULE mate_matecomponent_module_info_get()
const MateModuleInfo* mate_matecomponent_module_info_get(void) G_GNUC_CONST;

const char* mate_user_dir_get(void) G_GNUC_CONST;
const char* mate_user_private_dir_get(void) G_GNUC_CONST;
const char* mate_user_accels_dir_get(void) G_GNUC_CONST;

#ifdef G_OS_WIN32
	void mate_win32_get_prefixes(gpointer hmodule, char** full_prefix, char** cp_prefix);
#endif

G_END_DECLS

#endif /* LIBMATEINIT_H */
