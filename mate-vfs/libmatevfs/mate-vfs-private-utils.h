/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-private-utils.h - Private utility functions for the MATE Virtual
   File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef MATE_VFS_PRIVATE_UTILS_H
#define MATE_VFS_PRIVATE_UTILS_H

/* You should not use calls in here outside MateVFS. The APIs in here may
 * break even when the MateVFS APIs are otherwise frozen.
 */

#include <libmatevfs/mate-vfs-cancellation.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-uri.h>

#ifdef G_OS_WIN32
#include <io.h>

#undef MATE_VFS_DATADIR
extern char *_mate_vfs_datadir;
#define MATE_VFS_DATADIR _mate_vfs_datadir

#undef MATE_VFS_LIBDIR
extern char *_mate_vfs_libdir;
#define MATE_VFS_LIBDIR _mate_vfs_libdir

#undef MATE_VFS_PREFIX
extern char *_mate_vfs_prefix;
#define MATE_VFS_PREFIX _mate_vfs_prefix

#undef MATE_VFS_LOCALEDIR
extern char *_mate_vfs_localedir;
#define MATE_VFS_LOCALEDIR _mate_vfs_localedir

#undef MATE_VFS_SYSCONFDIR
extern char *_mate_vfs_sysconfdir;
#define MATE_VFS_SYSCONFDIR _mate_vfs_sysconfdir

#endif

#ifdef __cplusplus
extern "C" {
#endif

gboolean	 _mate_vfs_have_ipv6			 (void);

gchar   	*_mate_vfs_canonicalize_pathname         (char *path);
MateVFSResult   mate_vfs_remove_optional_escapes 	 (char *escaped_uri);

MateVFSResult	mate_vfs_create_temp 	(const gchar *prefix,
					 gchar **name_return,
					 MateVFSHandle **handle_return);
gboolean	mate_vfs_atotm		(const gchar *time_string,
					 time_t *value_return);

MateVFSURI    *mate_vfs_uri_new_private (const gchar *text_uri,
					   gboolean allow_unknown_method,
					   gboolean allow_unsafe_method,
					   gboolean allow_translate);


gboolean	_mate_vfs_istr_has_prefix (const char *haystack,
					   const char *needle);
gboolean	_mate_vfs_istr_has_suffix (const char *haystack,
					   const char *needle);

MateVFSResult _mate_vfs_uri_resolve_all_symlinks_uri (MateVFSURI *uri,
							MateVFSURI **result_uri);
MateVFSResult  _mate_vfs_uri_resolve_all_symlinks (const char *text_uri,
						     char **resolved_text_uri);
char *          mate_vfs_resolve_symlink          (const char *path,
						    const char *symlink);


gboolean  _mate_vfs_uri_is_in_subdir (MateVFSURI *uri, MateVFSURI *dir);


MateVFSResult _mate_vfs_url_show_using_handler_with_env (const char   *url,
                                                           char        **envp);
gboolean       _mate_vfs_use_handler_for_scheme          (const char   *scheme);

gboolean       _mate_vfs_prepend_terminal_to_vector      (int          *argc,
							   char       ***argv);

gboolean       _mate_vfs_socket_set_blocking             (int       sock_fd,
		                                           gboolean  blocking);

int	       _mate_vfs_pipe				  (int       *fds);
gboolean       _mate_vfs_pipe_set_blocking               (int        pipe_fd,
							   gboolean   blocking);

#ifdef G_OS_WIN32
void	       _mate_vfs_map_winsock_error_to_errno	  (void);
const char    *_mate_vfs_winsock_strerror		  (int		 error);

#define _MATE_VFS_SOCKET_IS_ERROR(s) ((s) == SOCKET_ERROR)
#define _MATE_VFS_SOCKET_CLOSE(s) closesocket(s)
#define _MATE_VFS_SOCKET_READ(s,b,n) recv(s,b,n,0)
#define _MATE_VFS_SOCKET_WRITE(s,b,n) send(s,b,n,0)

#else

#define _MATE_VFS_SOCKET_IS_ERROR(s) ((s) < 0)
#define _MATE_VFS_SOCKET_CLOSE(s) close(s)
#define _MATE_VFS_SOCKET_READ(s,b,n) read(s,b,n)
#define _MATE_VFS_SOCKET_WRITE(s,b,n) write(s,b,n)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _MATE_VFS_PRIVATE_UTILS_H */
