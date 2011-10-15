/* mate-exec.h - Execute some command.

   Copyright (C) 1998 Tom Tromey
   Copyright (C) 1999, 2000 Red Hat, Inc.
   All rights reserved.

   This file is part of the Mate Library.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */
/*
  @NOTATION@
 */

#ifndef MATE_EXEC_H
#define MATE_EXEC_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Fork and execute some program in the background.  Returns -1 on
   error.  Returns PID on success.  Should correctly report errno
   returns from a failing child invocation.  DIR is the directory in
   which to exec the child; if NULL the current directory is used.
   Searches $PATH to find the child.  */
int mate_execute_async (const char *dir, int argc, char * const argv[]);
int mate_execute_async_fds (const char *dir, int argc, char * const argv[],
			     gboolean close_fds);


/* Like mate_execute_async, but each string in ENVV is added to the
   child's environment.  If you want to set the environment exactly,
   you must set the global `environ' variable instead.  If ENVV is
   NULL, the child inherits the parent's environment.  In this case,
   the value of ENVC is ignored.  */
int mate_execute_async_with_env (const char *dir,
				  int argc, char * const argv[],
				  int envc, char * const envv[]);
int mate_execute_async_with_env_fds (const char *dir, int argc,
				      char * const argv[], int envc,
				      char * const envv[], gboolean close_fds);



/* Fork and execute commandline using the user's shell. Calls
   mate_execute_async so it does the same things and returns
   the same things. */
int mate_execute_shell (const char *dir, const char *commandline);
int mate_execute_shell_fds (const char *dir, const char *commandline,
			     gboolean close_fds);

/* prepend the terminal command to a vector */
void mate_prepend_terminal_to_vector (int *argc, char ***argv);

/* run a shell in the terminal, here commandline can be NULL
 * for just a shell, unlike in mate_execute_shell */
int mate_execute_terminal_shell (const char *dir, const char *commandline);
int mate_execute_terminal_shell_fds (const char *dir,
				      const char *commandline,
				      gboolean close_fds);

G_END_DECLS

#endif /* MATE_EXEC_H */
