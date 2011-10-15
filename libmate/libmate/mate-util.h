/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Copyright (C) 1999, 2000 Red Hat, Inc.
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
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
  @NOTATION@
 */

#ifndef __MATE_UTIL_H__
#define __MATE_UTIL_H__

#include <stdlib.h>
#include <glib.h>
#include <libmate/mate-init.h>
#include <libmate/mate-program.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Return pointer to the character after the last .,
   or "" if no dot. */
const char* g_extension_pointer(const char* path);


/* pass in a string, and it will add the users home dir ie,
 * pass in .mate/bookmarks.html and it will return
 * /home/imain/.mate2/bookmarks.html
 *
 * Remember to g_free() returned value! */
#define mate_util_prepend_user_home(x) (g_build_filename(g_get_home_dir(), (x), NULL))

/* very similar to above, but adds $HOME/.mate2/ to beginning
 * This is meant to be the most useful version.
 */
#define mate_util_home_file(afile) (g_build_filename(g_get_home_dir(), MATE_DOT_MATE, (afile), NULL))

/* Find the name of the user's shell.  */
char* mate_util_user_shell(void);

#ifndef MATE_DISABLE_DEPRECATED

/* Portable versions of setenv/unsetenv */

/* Note: setenv will leak on some systems (those without setenv) so
 * do NOT use inside a loop.  Semantics are the same as those in glibc */
int mate_setenv(const char* name, const char* value, gboolean overwrite);
void mate_unsetenv(const char* name);
void mate_clearenv(void);

/* Some deprecated functions macroed to their new equivalents */

#define g_file_exists(filename) g_file_test((filename), G_FILE_TEST_EXISTS)
#define g_unix_error_string(error_num) g_strerror((error_num))
#define mate_util_user_home() g_get_home_dir()
#define g_copy_vector(vec) g_strdupv((vec))
#define g_concat_dir_and_file(dir,file)	g_build_filename((dir), (file), NULL)

#define mate_is_program_in_path(program)	g_find_program_in_path((program))

#define mate_libdir_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_LIBDIR, (f), TRUE, NULL))
#define mate_datadir_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_DATADIR, (f), TRUE, NULL))
#define mate_sound_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_SOUND, (f), TRUE, NULL))
#define mate_pixmap_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_PIXMAP, (f), TRUE, NULL))
#define mate_config_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_CONFIG, (f), TRUE, NULL))

#define mate_unconditional_libdir_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_LIBDIR, (f), FALSE, NULL))
#define mate_unconditional_datadir_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_DATADIR, (f), FALSE, NULL))
#define mate_unconditional_sound_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_SOUND, (f), FALSE, NULL))
#define mate_unconditional_pixmap_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_PIXMAP, (f), FALSE, NULL))
#define mate_unconditional_config_file(f) (mate_program_locate_file(NULL, MATE_FILE_DOMAIN_CONFIG, (f), FALSE, NULL))

#endif /* MATE_DISABLE_DEPRECATED */


#ifdef __cplusplus
}
#endif

#endif
