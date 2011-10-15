/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-parse-ls.h - Routines for parsing output from the `ls' command.

   Copyright (C) 1999 Free Software Foundation

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
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: 1995 Miguel de Icaza
   1995 Jakub Jelinek
   1998 Pavel Machek
   1999 Cleanup by Ettore Perazzoli

   finduid, findgid are from GNU tar.  */

#ifndef MATE_VFS_PARSE_LS_H
#define MATE_VFS_PARSE_LS_H

#include <sys/stat.h>

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat;

int mate_vfs_parse_ls_lga (const char *p, struct stat *s,
			    char **filename, char **linkname);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_PARSE_LS_H */
