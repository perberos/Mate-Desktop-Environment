/* nntp-method.h - VFS modules for NNTP

    Copyright (C) 2001 Andy Hertzfeld

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

   based on Ian McKellar's (yakk@yakk.net) ftp method for mate-vfs
   
   presents a high level, file-oriented view of a newsgroup, integrating file fragments
   and organizing them in folders

   Author: Andy Hertzfeld <andy@differnet.com>
 
 */

#ifndef NNTP_METHOD_H
#define NNTP_METHOD_H

#include <libmatevfs/mate-vfs-module.h>

typedef struct
{
	int fragment_number;
	char* fragment_id;
	int fragment_size;
	int bytes_read;
} nntp_fragment;

typedef struct
{
  	char* file_name;
	char* folder_name;
	char* file_type;
	int file_size;
	gboolean is_directory;
	time_t mod_date;
	
	int total_parts;
	GList *part_list;
 } nntp_file;

typedef struct {
	MateVFSMethodHandle method_handle;
	MateVFSInetConnection *inet_connection;
	MateVFSSocketBuffer *socketbuf;
	MateVFSURI *uri;
	GString *response_buffer;
	gchar *response_message;
	gint response_code;
	enum {
		NNTP_NOTHING,
		NNTP_READ,
		NNTP_WRITE,
		NNTP_READDIR
	} operation;
		
	gchar *server_type; /* the response from TYPE */
	gboolean anonymous;
	
	GList *next_file;
	nntp_file *current_file;
	GList *current_fragment;
	
	gpointer buffer;
	int buffer_size;
	int amount_in_buffer;
	int buffer_offset;
	
	gboolean request_in_progress;
	gboolean eof_flag;
	gboolean uu_decode_mode;
	gboolean base_64_decode_mode;
} NNTPConnection;


#endif /* NNTP_METHOD_H */
