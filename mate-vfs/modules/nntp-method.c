/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/* nntp-method.c - VFS module for NNTP

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

   Author: Andy Hertzfeld <andy@differnet.com> December 2001
     
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h> /* for atoi */
#include <stdio.h> /* for sscanf */
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>

#ifdef HAVE_MATECONF
#include <mateconf/mateconf-client.h>
#endif

#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-socket-buffer.h>
#include <libmatevfs/mate-vfs-inet-connection.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module-callback-module-api.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-parse-ls.h>
#include <libmatevfs/mate-vfs-utils.h>

#include "nntp-method.h"


#define MAX_RESPONSE_SIZE 4096 
#define READ_BUFFER_SIZE 16384

/* these parameters should eventually be fetched from mateconf */
#define MAX_MESSAGE_COUNT 2400
#define MIN_FILE_SIZE_THRESHOLD 4095

/* macros for the checking of NNTP response codes */
#define IS_100(X) ((X) >= 100 && (X) < 200)
#define IS_200(X) ((X) >= 200 && (X) < 300)
#define IS_300(X) ((X) >= 300 && (X) < 400)
#define IS_400(X) ((X) >= 400 && (X) < 500)
#define IS_500(X) ((X) >= 500 && (X) < 600)

static MateVFSResult do_open	         (MateVFSMethod               *method,
					  MateVFSMethodHandle         **method_handle,
					  MateVFSURI                   *uri,
					  MateVFSOpenMode               mode,
					  MateVFSContext               *context);
static gboolean       do_is_local        (MateVFSMethod               *method,
					  const MateVFSURI             *uri);
static MateVFSResult do_open_directory  (MateVFSMethod                *method,
					  MateVFSMethodHandle         **method_handle,
					  MateVFSURI                   *uri,
					  MateVFSFileInfoOptions        options,
					  MateVFSContext               *context);
static MateVFSResult do_close_directory (MateVFSMethod               *method,
					  MateVFSMethodHandle          *method_handle,
					  MateVFSContext               *context);
static MateVFSResult do_read_directory  (MateVFSMethod               *method,
		                          MateVFSMethodHandle          *method_handle,
		                          MateVFSFileInfo              *file_info,
		                          MateVFSContext               *context);

guint                 nntp_connection_uri_hash  (gconstpointer c);
int                   nntp_connection_uri_equal (gconstpointer c, gconstpointer d);
static MateVFSResult nntp_connection_acquire   (MateVFSURI *uri, 
		                                NNTPConnection **connection, 
		                                MateVFSContext *context);
static void           nntp_connection_release   (NNTPConnection *conn);

static GList* assemble_files_from_overview (NNTPConnection *conn, char *command); 

static const char *anon_user = "anonymous";
static const char *anon_pass = "nobody@gnome.org";
static const int   nntp_port = 119;


/* A GHashTable of GLists of NNTPConnections */
static GHashTable *spare_connections = NULL;
G_LOCK_DEFINE_STATIC (spare_connections);
static int total_connections = 0;
static int allocated_connections = 0;

/* single element cache of file objects for a given newsgroup */
static char* current_newsgroup_name = NULL;
static GList* current_newsgroup_files = NULL;

static MateVFSResult 
nntp_response_to_vfs_result (NNTPConnection *conn) 
{
	int response = conn->response_code;

	switch (response) {
	case 421: 
	case 426: 
	  return MATE_VFS_ERROR_CANCELLED;
	case 425:
	  return MATE_VFS_ERROR_ACCESS_DENIED;
	case 530:
	case 331:
	case 332:
	case 532:
	  return MATE_VFS_ERROR_LOGIN_FAILED;
	case 450:
	case 451:
	case 550:
	case 551:
	  return MATE_VFS_ERROR_NOT_FOUND;
	case 452:
	case 552:
	  return MATE_VFS_ERROR_NO_SPACE;
	case 553:
	  return MATE_VFS_ERROR_BAD_FILE;
	}

	/* is this the correct interpretation of this error? */
	if (IS_100 (response)) return MATE_VFS_OK;
	if (IS_200 (response)) return MATE_VFS_OK;
	/* is this the correct interpretation of this error? */
	if (IS_300 (response)) return MATE_VFS_OK;
	if (IS_400 (response)) return MATE_VFS_ERROR_GENERIC;
	if (IS_500 (response)) return MATE_VFS_ERROR_INTERNAL;

	return MATE_VFS_ERROR_GENERIC;

}

static MateVFSResult read_response_line(NNTPConnection *conn, char **line) {
	MateVFSFileSize bytes = MAX_RESPONSE_SIZE, bytes_read;
	char *ptr, *buf = g_malloc (MAX_RESPONSE_SIZE+1);
	int line_length;
	MateVFSResult result = MATE_VFS_OK;

	while (!strstr (conn->response_buffer->str, "\r\n")) {
		/* we don't have a full line. Lets read some... */
		bytes_read = 0;
		result = mate_vfs_socket_buffer_read (conn->socketbuf, buf,
						       bytes, &bytes_read, NULL);
		buf[bytes_read] = '\0';
		conn->response_buffer = g_string_append (conn->response_buffer,
							 buf);
		if (result != MATE_VFS_OK) {
		        g_warning ("Error `%s' during read\n", 
				   mate_vfs_result_to_string(result));
			g_free (buf);
			return result;
		}
	}

	g_free (buf);

	ptr = strstr (conn->response_buffer->str, "\r\n");
	line_length = ptr - conn->response_buffer->str;

	*line = g_strndup (conn->response_buffer->str, line_length);

	g_string_erase (conn->response_buffer, 0 , line_length + 2);

	return result;
}


static MateVFSResult
get_response (NNTPConnection *conn)
{
	/* all that should be pending is a response to the last command */
	MateVFSResult result;

	while (TRUE) {
		char *line = NULL;
		result = read_response_line (conn, &line);

		if (result != MATE_VFS_OK) {
			g_free (line);
			g_warning ("Error reading response line.");
			return result;
		}
		
		/* response needs to be at least: "### x"  - I think*/
		if (g_ascii_isdigit (line[0]) &&
		    g_ascii_isdigit (line[1]) &&
		    g_ascii_isdigit (line[2]) &&
		    g_ascii_isspace (line[3])) {

			conn->response_code = g_ascii_digit_value (line[0]) * 100
			                      + g_ascii_digit_value (line[1]) * 10
					      + g_ascii_digit_value (line[2]);

			if (conn->response_message) g_free (conn->response_message);
			conn->response_message = g_strdup (line+4);

			g_free (line);

			return nntp_response_to_vfs_result (conn);

		}

		/* hmm - not a valid line - lets ignore it :-) */
		g_free (line);
	}

	return MATE_VFS_OK; /* should never be reached */

}

static MateVFSResult do_control_write (NNTPConnection *conn, 
					char *command) 
{
        char *actual_command = g_strdup_printf ("%s\r\n", command);
	MateVFSFileSize bytes = strlen (actual_command), bytes_written;
	MateVFSResult result = mate_vfs_socket_buffer_write (conn->socketbuf,
							       actual_command, bytes, &bytes_written,
							       NULL);
	mate_vfs_socket_buffer_flush (conn->socketbuf, NULL);
	g_free (actual_command);

	return result;
}

static void
nntp_connection_reset_buffer (NNTPConnection *conn)
{	
	g_string_erase (conn->response_buffer, 0, strlen(conn->response_buffer->str));
}

static MateVFSResult 
do_basic_command (NNTPConnection *conn, 
		  char *command) 
{
	MateVFSResult result;

	nntp_connection_reset_buffer (conn);	
	
	result = do_control_write(conn, command);

	if (result != MATE_VFS_OK) {
		return result;
	}

	result = get_response (conn);

	return result;
}

/* the following routines manage the connection with the news server */

/* create a new connection */
static MateVFSResult 
nntp_connection_create (NNTPConnection **connptr, MateVFSURI *uri, MateVFSContext *context) 
{
	NNTPConnection *conn = g_new (NNTPConnection, 1);
	MateVFSResult result;
	char *tmpstring;
	int port = nntp_port;
	const char *user = anon_user;
	const char *pass = anon_pass;
	
	conn->uri = mate_vfs_uri_dup (uri);
	
	conn->response_buffer = g_string_new ("");
	conn->response_message = NULL;
	conn->response_code = -1;
	
	conn->anonymous = TRUE;
	
	/* allocate the read buffer */
	conn->buffer = g_malloc(READ_BUFFER_SIZE);
	conn->buffer_size = READ_BUFFER_SIZE;
	conn->amount_in_buffer = 0;
	conn->buffer_offset = 0;
	
	conn->request_in_progress = FALSE;
	
	if (mate_vfs_uri_get_host_port (uri)) {
		port = mate_vfs_uri_get_host_port (uri);
	}

	if (mate_vfs_uri_get_user_name (uri)) {
		user = mate_vfs_uri_get_user_name (uri);
		conn->anonymous = FALSE;
	}

	if (mate_vfs_uri_get_password (uri)) {
		pass = mate_vfs_uri_get_password (uri);
	}

	result = mate_vfs_inet_connection_create (&conn->inet_connection, 
						   mate_vfs_uri_get_host_name (uri), 
						   port, 
						   context ? mate_vfs_context_get_cancellation(context) : NULL);
	
	if (result != MATE_VFS_OK) {
	        g_warning ("mate_vfs_inet_connection_create (\"%s\", %d) = \"%s\"",
			   mate_vfs_uri_get_host_name (uri),
			   mate_vfs_uri_get_host_port (uri),
			   mate_vfs_result_to_string (result));
		g_string_free (conn->response_buffer, TRUE);
		g_free (conn);
		return result;
	}

	conn->socketbuf = mate_vfs_inet_connection_to_socket_buffer (conn->inet_connection);

	if (conn->socketbuf == NULL) {
		g_warning ("mate_vfs_inet_connection_get_socket_buffer () failed");
		mate_vfs_inet_connection_destroy (conn->inet_connection, NULL);
		g_string_free (conn->response_buffer, TRUE);
		g_free (conn);
		return MATE_VFS_ERROR_GENERIC;
	}

	result = get_response (conn);

	if (result != MATE_VFS_OK) { 
		g_warning ("nntp server (%s:%d) said `%d %s'", 
			   mate_vfs_uri_get_host_name (uri),
			   mate_vfs_uri_get_host_port (uri), 
			   conn->response_code, conn->response_message);
		g_string_free (conn->response_buffer, TRUE);
		g_free (conn);
		return result;
	}

	if (!conn->anonymous) {
		tmpstring = g_strdup_printf ("AUTHINFO user %s", user);
		result = do_basic_command (conn, tmpstring);
		g_free (tmpstring);

		if (IS_300 (conn->response_code)) {
			tmpstring = g_strdup_printf ("AUTHINFO pass %s", pass);
			result = do_basic_command (conn, tmpstring);
			g_free (tmpstring);
		}

		if (result != MATE_VFS_OK) {
			/* login failed */
			g_warning ("NNTP server said: \"%d %s\"\n", conn->response_code,
			   	conn->response_message);
			mate_vfs_socket_buffer_destroy (conn->socketbuf, FALSE, context ? mate_vfs_context_get_cancellation(context) : NULL);
			mate_vfs_inet_connection_destroy (conn->inet_connection, NULL);
			g_free (conn);
			return result;
		}
	}
	*connptr = conn;

	total_connections++;

	/* g_message("successfully logged into server, total connections: %d", total_connections); */
	return MATE_VFS_OK;
}

/* destroy the connection object and deallocate the associated resources */
static void
nntp_connection_destroy (NNTPConnection *conn) 
{
	MateVFSResult result;
	
	if (conn->inet_connection) { 
		result = do_basic_command(conn, "QUIT");      
		mate_vfs_inet_connection_destroy (conn->inet_connection, NULL);
	}
	
	if (conn->socketbuf) 
	        mate_vfs_socket_buffer_destroy (conn->socketbuf, FALSE, NULL);

	mate_vfs_uri_unref (conn->uri);

	if (conn->response_buffer) 
	        g_string_free(conn->response_buffer, TRUE);
	
	g_free (conn->response_message);
	g_free (conn->server_type);
	g_free (conn->buffer);
	
	g_free (conn);
	total_connections--;
}

/* hash routines for managing the pool of connections */
/* g_str_hash and g_str_equal seem to fail with null arguments */

static int 
my_str_hash (const char *c) 
{
	if (c) 
	        return g_str_hash (c);
	else return 0;
}

static int 
my_str_equal (gconstpointer c, 
	      gconstpointer d) 
{
	if ((c && !d) || (d &&!c)) 
		return FALSE;
	if (!c && !d) 
		return TRUE;
	return g_str_equal (c,d);
}

/* hash the bits of a MateVFSURI that distingush NNTP connections */
guint
nntp_connection_uri_hash (gconstpointer c) 
{
	MateVFSURI *uri = (MateVFSURI *) c;

	return my_str_hash (mate_vfs_uri_get_host_name (uri)) + 
		my_str_hash (mate_vfs_uri_get_user_name (uri)) +
		my_str_hash (mate_vfs_uri_get_password (uri)) +
		mate_vfs_uri_get_host_port (uri);
}

/* test the equality of the bits of a MateVFSURI that distingush NNTP 
 * connections
 */
int 
nntp_connection_uri_equal (gconstpointer c, 
			  gconstpointer d) 
{
	MateVFSURI *uri1 = (MateVFSURI *)c;
	MateVFSURI *uri2 = (MateVFSURI *) d;

	return my_str_equal (mate_vfs_uri_get_host_name(uri1),
			     mate_vfs_uri_get_host_name (uri2)) &&
		my_str_equal (mate_vfs_uri_get_user_name (uri1),
			      mate_vfs_uri_get_user_name (uri2)) &&
		my_str_equal (mate_vfs_uri_get_password (uri1),
			      mate_vfs_uri_get_password (uri2)) &&
		mate_vfs_uri_get_host_port (uri1) == 
		mate_vfs_uri_get_host_port (uri2);
}

static MateVFSResult 
nntp_connection_acquire (MateVFSURI *uri, NNTPConnection **connection, MateVFSContext *context) 
{
	GList *possible_connections;
	NNTPConnection *conn = NULL;
	MateVFSResult result = MATE_VFS_OK;

	G_LOCK (spare_connections);

	if (spare_connections == NULL) {
		spare_connections = g_hash_table_new (nntp_connection_uri_hash, 
						      nntp_connection_uri_equal);
	}

	possible_connections = g_hash_table_lookup (spare_connections, uri);

	if (possible_connections) {
		/* spare connection(s) found */
		conn = (NNTPConnection *) possible_connections->data;
		possible_connections = g_list_remove (possible_connections, conn);

		if(!g_hash_table_lookup (spare_connections, uri)) {
			/* uri will be used as a key in the hashtable */
			uri = mate_vfs_uri_dup (uri);
		}

		g_hash_table_insert (spare_connections, uri, possible_connections);

		/* make sure connection hasn't timed out */
		result = do_basic_command(conn, "MODE READER");
		if (result != MATE_VFS_OK) {
			nntp_connection_destroy (conn);
			result = nntp_connection_create (&conn, uri, context);
		}

	} else {
		result = nntp_connection_create (&conn, uri, context);
	}

	G_UNLOCK (spare_connections);

	*connection = conn;

	if(result == MATE_VFS_OK) allocated_connections++;
	return result;
}


static void 
nntp_connection_release (NNTPConnection *conn) 
{
	GList *possible_connections;
	MateVFSURI *uri;

	g_return_if_fail (conn);

	G_LOCK (spare_connections);
	if (spare_connections == NULL) 
		spare_connections = 
			g_hash_table_new (nntp_connection_uri_hash, 
					  nntp_connection_uri_equal);

	possible_connections = g_hash_table_lookup (spare_connections, 
						    conn->uri);
	possible_connections = g_list_append (possible_connections, conn);

	if (g_hash_table_lookup (spare_connections, conn->uri)) {
		uri = conn->uri; /* no need to duplicate uri */
	} else {
		/* uri will be used as key */
		uri = mate_vfs_uri_dup (conn->uri); 
	}
	g_hash_table_insert (spare_connections, uri, possible_connections);
	allocated_connections--;

	G_UNLOCK(spare_connections);
}

/* the following routines are the code for assembling the file list from the newsgroup headers. */

/* Here are the folder name filtering routines, which attempt to remove track numbers and other extraneous
 * info from folder names, to make folder grouping work better. First, there are some character classifying
 * helper routines
 */
static gboolean
is_number_or_space (char test_char)
{
	return g_ascii_isspace (test_char) || g_ascii_isdigit (test_char)
	       || test_char == '_' || test_char == '-' || test_char == '/';
}

static gboolean
all_numbers_or_spaces (char *left_ptr, char *right_ptr)
{
	char *current_char_ptr;
	char current_char;
	
	current_char_ptr = left_ptr;
	while (current_char_ptr < right_ptr) {
		current_char = *current_char_ptr++;
		if (!is_number_or_space(current_char)) {
			return FALSE;
		} 
	}
	return TRUE;
}

/* each of the next set of routines implement a specific filtering operation */

static void
remove_numbers_between_dashes(char *input_str)
{
	char *left_ptr, *right_ptr;
	int length_to_end, segment_size;
	
	left_ptr = strchr (input_str, '-');
	while (left_ptr != NULL) {
		right_ptr = strchr(left_ptr + 1, '-');
		if (right_ptr != NULL) {
			segment_size = right_ptr - left_ptr;
			if (all_numbers_or_spaces(left_ptr, right_ptr) && segment_size > 1) {
				length_to_end = strlen(right_ptr + 1) + 1;
					g_memmove(left_ptr, right_ptr + 1, length_to_end);
			} else {
				left_ptr = right_ptr;
			}
			
		} else {
			right_ptr = input_str + strlen(input_str) - 1;
			if (all_numbers_or_spaces(left_ptr, right_ptr)) {
				*left_ptr = '\0';	
			}
			break;
		}
	}
}


static void
remove_number_at_end (char *input_str)
{
	char *space_ptr, *next_char;
	gboolean is_digits;
	
	space_ptr = strrchr(input_str, ' ');
	next_char = space_ptr + 1;

	if (space_ptr != NULL) {
		is_digits = TRUE;
		while (*next_char != '\0') {
			if (!is_number_or_space (*next_char)) {
				is_digits = FALSE;
				break;
			}
			next_char += 1;
		}
		if (is_digits) {
			*space_ptr = '\0';
		}
	}
}

static char*
trim_nonalpha_chars (char *input_str)
{
	char *left_ptr, *right_ptr;

	/* first handle the end of the string */	
	right_ptr = input_str + strlen(input_str) - 1;
	while (!g_ascii_isalnum (*right_ptr) && right_ptr > input_str) {
		right_ptr -= 1;
	}
	
	right_ptr += 1;
	*right_ptr = '\0';

	/* now handle the beginning */
	left_ptr = input_str;
	while (*left_ptr && !g_ascii_isalnum(*left_ptr)) {
		left_ptr++;
	}
	return left_ptr;
}

static void
remove_of_expressions (char *input_str)
{
	char *left_ptr, *right_ptr;
	int length_to_end;
	gboolean found_number;
	
	left_ptr = strstr (input_str, "of");
	if (left_ptr == NULL) {
		left_ptr = strstr (input_str, "OF");
	}
	if (left_ptr == NULL) {
		left_ptr = strstr (input_str, "/");
	}
	
	if (left_ptr != NULL) {
		found_number = FALSE;
		right_ptr = left_ptr + 2;
		left_ptr = left_ptr - 1;
		while (is_number_or_space(*left_ptr) && left_ptr >= input_str) {
			found_number = found_number || g_ascii_isdigit(*left_ptr);
			left_ptr -= 1;
		}
		while (is_number_or_space(*right_ptr)) {
			found_number = found_number || g_ascii_isdigit(*right_ptr);
			right_ptr += 1;
		}
		
		if (found_number) {		
			length_to_end = strlen(right_ptr);
			if (length_to_end > 0) {
				g_memmove(left_ptr + 1, right_ptr, length_to_end + 1);
			} else {
				left_ptr += 1;
				*left_ptr = '\0';
			}
		}
	}
}

/* here is the main folder name filtering routine, which returns a new string containing the filtered
 * version of the passed-in folder name
 */
static char *
filter_folder_name(char *folder_name)
{
	char *temp_str, *save_str, *result_str;
	char *colon_ptr, *left_ptr, *right_ptr;
	int length_to_end;
	
	temp_str = g_strdup(folder_name);
	save_str = temp_str;
	temp_str = g_strstrip(temp_str);
	
	/* if there is a colon, strip everything before it */
	colon_ptr = strchr(temp_str, ':');
	if (colon_ptr != NULL) {
		temp_str = colon_ptr + 1;
	}
	
	/* remove everything in brackets */
	left_ptr = strrchr(temp_str, '[');
	if (left_ptr != NULL) {
		right_ptr = strchr(left_ptr, ']');
		if (right_ptr != NULL && right_ptr > left_ptr) {
			length_to_end = strlen(right_ptr + 1) + 1;
			g_memmove(left_ptr, right_ptr + 1, length_to_end);
		}	
	}
	
	/* eliminate "n of m" expressions */
	remove_of_expressions(temp_str);
		
	/* remove numbers at end of the string */
	remove_number_at_end(temp_str);
	
	/* remove numbers between dashes */
	remove_numbers_between_dashes(temp_str);
	
	/* trim leading and trailing non-alphanumeric characters */
	temp_str = trim_nonalpha_chars(temp_str);
	
	/* truncate if necessary */
	if (strlen(temp_str) > 30) {
		left_ptr = temp_str + 29;
		while (g_ascii_isalpha(*left_ptr)) {
			left_ptr += 1;
		}
		
		*left_ptr = '\0';
	}		
	
	/* return the result */
	result_str = g_strdup(temp_str);
	g_free(save_str);
	return result_str;
}

/* parse dates by allowing mate-vfs to do most of the work */

static void
remove_commas(char *source_str)
{
	char *ptr;
	
	ptr = source_str;
	while (*ptr != '\0') {
		if (*ptr == ',') {
			g_memmove(ptr, ptr+1, strlen(ptr));
		} else {
			ptr += 1;
		}
	}
}

static gboolean
is_number(char *input_str) {
	char *cur_char;
	
	cur_char = input_str;
	while (*cur_char) {
		if (!g_ascii_isdigit(*cur_char)) {
			return FALSE;
		}
		cur_char += 1;
	}
	return TRUE;
}

static void
parse_date_string (const char* date_string, time_t *t)
{
	struct stat s;
	char *filename, *linkname;
	char *temp_str, *mapped_date;
	gboolean result;
	char **date_parts;
	int offset;
	
	filename = NULL;
	linkname = NULL;
	
	/* remove commas from day, and flip the day and month */
   	date_parts = g_strsplit(date_string, " ", 0);
	
	if (is_number(date_parts[0])) {
		offset = 0;
	} else {
		offset = 1;
		remove_commas(date_parts[0]);
	}
	
	temp_str = date_parts[offset];
	date_parts[offset] = date_parts[offset + 1];
	date_parts[offset + 1] = temp_str;
	mapped_date = g_strjoinv(" ", date_parts);
	
	temp_str = g_strdup_printf("-rw-rw-rw- 1 500 500 %s x", mapped_date);
	g_strfreev(date_parts);
	g_free (mapped_date);
	
	result = mate_vfs_parse_ls_lga (temp_str, &s, &filename, &linkname);
	if (!result) {
		g_message("error parsing %s, %s", date_string, temp_str); 
	}
	
	if (filename != NULL) {
		g_free (filename);
	}
	if (linkname != NULL) {
		g_free (linkname);
	}
	
	g_free(temp_str);
	*t = s.st_mtime;
}

/* parse_header takes a message header an extracts the subject and id, and then
 * parses the subject to get the file name and fragment sequence number
 */
static gboolean
parse_header(const char* header_str, char** filename, char** folder_name, char** message_id,
		int* message_size, int* part_number, int* total_parts, time_t *mod_date)
{
	char* temp_str, *file_start;
	char* left_paren, *right_paren, *slash_pos;
 	int slash_bump;
	
	char **header_parts;
  	gboolean has_count;
	
	*part_number = -1;
	*total_parts = -1;
	*message_size = 0;
	
	*filename = NULL;
	*folder_name = NULL;
	*message_id = NULL;
	slash_pos = NULL;
		
    	header_parts = g_strsplit(header_str, "\t", 0);
	temp_str = g_strdup(header_parts[1]);
	
	*message_id = g_strdup(header_parts[4]);
 
 	if (header_parts[6] != NULL) {
		*message_size = atoi(header_parts[6]);
	}
 
 	parse_date_string(header_parts[3], mod_date);
		
    	g_strfreev(header_parts);
	
	/* find the parentheses and extra the part number and total part count */	
	
	has_count = FALSE;
	left_paren = strchr(temp_str,'(');
	right_paren = strchr(temp_str, ')');
	
	/* if we could't find parentheses, try braces */
	if (left_paren == NULL) {
		left_paren = strchr(temp_str,'[');
		right_paren = strchr(temp_str, ']');
	}
	
	while (!has_count && left_paren != NULL && right_paren != NULL) {
		slash_pos = strchr(left_paren, '/');
		slash_bump = 1;
		
		if (slash_pos == NULL) {
			slash_pos = strchr(left_paren, '-');
		}
		if (slash_pos == NULL) {
			slash_pos = strstr(left_paren, " of ");
			slash_bump = 4;
		}
		
		if (slash_pos != NULL) {
			has_count = TRUE;
			*slash_pos = '\0';
			*right_paren = '\0';

			*part_number = atoi (left_paren + 1);
			*total_parts = atoi (slash_pos + slash_bump);
		} else {
			left_paren = strchr(right_paren + 1, '(');
			right_paren = strchr(right_paren + 1, ')');
		}
	}
	
	if (!has_count) {	
		*part_number = 1;
		*total_parts = 1;
	}
	
	
	/* isolate the filename and copy it to the result */
	if (has_count) {
		*left_paren = '\0';
		file_start = strrchr (temp_str, '-');
	
		if (file_start == NULL) {
			g_free (*message_id);
			g_free (temp_str);
			return FALSE;
		}
	
		*filename = g_strdup (g_strstrip (file_start + 1));
		*file_start = '\0';
		*folder_name = filter_folder_name (temp_str);
	} else
		{
		*filename = g_strdup (temp_str);
	}
	
	g_free(temp_str);
	return TRUE;
}


/* create a new file fragment structure */
static nntp_fragment*
nntp_fragment_new(int fragment_number, char* fragment_id, int fragment_size)
{
	nntp_fragment* new_fragment = (nntp_fragment*) g_malloc (sizeof (nntp_fragment));
	new_fragment->fragment_number = fragment_number;
	new_fragment->fragment_id = g_strdup (fragment_id);
	new_fragment->fragment_size = fragment_size;
	new_fragment->bytes_read = 0;
	
	return new_fragment;
}

/* deallocate the passed in file fragment */
static void
nntp_fragment_destroy(nntp_fragment* fragment)
{
	g_free(fragment->fragment_id);
	g_free(fragment);
}

/* utility routine to map slashes to dashes in the passed in string so we never return
 * a filename with slashes
 */
static char *
map_slashes(char *str)
{
	char *current_ptr;
	
	current_ptr = str;
	while (*current_ptr != '\0') {
		if (*current_ptr == '/') {
			*current_ptr = '-';
		}
		current_ptr += 1;
	}
	return str;
}

/* allocate a file object */
static nntp_file*
nntp_file_new(char* file_name, char *folder_name, int total_parts)
{
	
	nntp_file* new_file = (nntp_file*) g_malloc (sizeof (nntp_file));

	if (strlen(map_slashes(file_name)) < 1) {
		file_name = "(Empty)";
	}
	
	new_file->file_name = map_slashes(g_strdup(file_name));
	new_file->folder_name = g_strdup(folder_name);
	
	new_file->file_type = NULL;
	new_file->part_list = NULL;
	
	new_file->file_size = 0;
	new_file->total_parts = total_parts;
	new_file->is_directory = FALSE;
	
	return new_file;
}

/* deallocate a file object */
static void
nntp_file_destroy (nntp_file* file)
{
	GList *current_part;
	
	g_free(file->file_name);
	g_free(file->folder_name);
	
	current_part = file->part_list;
	while (current_part != NULL) {
		if (file->is_directory) {
			nntp_file_destroy((nntp_file*) current_part->data);
		} else {
			nntp_fragment_destroy((nntp_fragment*) current_part->data);
		}
		current_part = current_part->next;
	}
	
	g_list_free(file->part_list);
	g_free(file);
}


/* look up a fragment in a file */
static nntp_fragment*
nntp_file_look_up_fragment (nntp_file *file, int fragment_number)
{
	nntp_fragment *fragment;
	GList *current_fragment = file->part_list;
	while (current_fragment != NULL) {
		fragment = (nntp_fragment*) current_fragment->data;
		if (fragment->fragment_number == fragment_number) {
			return fragment;
		}
		current_fragment = current_fragment->next;
	}
	return NULL;	
}

/* compute the total size of a file by adding up the size of its fragments, then scale down by 3/4 to account for
 * the uu or base64 encoding
 */
static int
nntp_file_get_total_size (nntp_file *file)
{
	nntp_fragment *fragment;
	int total_size;
	GList *current_fragment;
	
	total_size = 0;
	current_fragment = file->part_list;	
	while (current_fragment != NULL) {
		fragment = (nntp_fragment*) current_fragment->data;
		total_size += fragment->fragment_size - 800; /* subtract out average header size */
		
		current_fragment = current_fragment->next;
	}
	
	return 3 * total_size / 4;	
}

/* add a fragment to a file */
static void
nntp_file_add_part (nntp_file *file, int fragment_number, char* fragment_id, int fragment_size)
{
	/* make sure we don't already have this fragment */
	/* note: this isn't good for threads, where more than one message has the same subject line,
	 * so we really should collect them instead of discarding all but the first */
	 
	if (nntp_file_look_up_fragment (file, fragment_number) == NULL) {
		nntp_fragment* fragment = nntp_fragment_new (fragment_number, fragment_id, fragment_size);
		file->part_list = g_list_append (file->part_list, fragment);
	}
}


/* look up a file object in the file list from its name; avoid lookup if same as last time (soon) */
static nntp_file*
look_up_file (GList *file_list, char *filename, gboolean is_directory)
{
	nntp_file* file;
	GList* current_file = file_list;
	while (current_file != NULL) {
		file = (nntp_file*) current_file->data;
		if (g_ascii_strcasecmp( file->file_name, filename) == 0 && 
		    file->is_directory == is_directory) {
			return file;
		}
		current_file = current_file->next;
	}
	return NULL;
}

/* start reading the article associated with the passed in fragment */
static MateVFSResult
start_loading_article (NNTPConnection *conn, nntp_fragment *fragment)
{
	MateVFSResult result;
	char *command_str;
	char *line = NULL;
	
	/* issue the command to fetch the article */
	command_str = g_strdup_printf("BODY %s", fragment->fragment_id);
	result = do_control_write(conn, command_str);	
	g_free(command_str);
	
	if (result != MATE_VFS_OK) {
		return result;
	}
	
	/* read the response command, and check that it's the right number (eventually) */
	result = read_response_line (conn, &line);	
	g_free(line);
	if (result != MATE_VFS_OK) {
		return result;
	}
	
	conn->request_in_progress = TRUE;
	return MATE_VFS_OK;
}

/* utility routine to uudecode the passed in text.  Translate every four 6-bit bytes to 3 8 bit ones */
static int
uu_decode_text(char *text_buffer, int text_len)
{
	int input_index, output_index;
	int byte_0, byte_1, byte_2, byte_3;
	
	input_index = 1; /* skip length byte */
	output_index = 0;
	while (input_index < text_len) {
		byte_0 = text_buffer[input_index] - 32;
		byte_1 = text_buffer[input_index + 1] - 32;
		byte_2 = text_buffer[input_index + 2] - 32;
		byte_3 = text_buffer[input_index + 3] - 32;

		text_buffer[output_index] = ((byte_0 << 2) & 252) | ((byte_1 >> 4) & 3);		
		text_buffer[output_index + 1] = ((byte_1 << 4) & 240) | ((byte_2 >> 2) & 15);		
		text_buffer[output_index + 2] = ((byte_2 << 6) & 192) | (byte_3 & 63);		
	
		input_index += 4;
		output_index += 3;
	}
	return output_index;
}

/* utility to decode the passed in text using base 64 */

static int
base_64_map(char input_char)
{
	if (input_char >= 'A' && input_char <= 'Z') {
		return input_char - 'A';
	}
	
	if (input_char >= 'a' && input_char <= 'z') {
		return 26 + input_char - 'a';
	}
	
	if (input_char >= '0' && input_char <= '9') {
		return 52 + input_char - '0';
	}
	
	if (input_char == '+') {
		return 62;
	}
	
	if (input_char == '/') {
		return 63;
	}
	
	if (input_char == '=') {
		return 0;
	}
	
	/* not a valid character, so return -1 */
	return -1;	
}

static int
base_64_decode_text(char* text_buffer, int text_len)
{
	int input_index, output_index;
	int byte_0, byte_1, byte_2, byte_3;
	
	input_index = 1; /* skip length byte */
	output_index = 0;
	while (input_index < text_len) {
		byte_0 = base_64_map(text_buffer[input_index]);
		byte_1 = base_64_map(text_buffer[input_index + 1]);
		byte_2 = base_64_map(text_buffer[input_index + 2]);
		byte_3 = base_64_map(text_buffer[input_index + 3]);

		/* if we hit a return, we're done */
		if (text_buffer[input_index] < 32) {
			return output_index;
		}
		
		/* if there are any unmappable characters, skip this line */
		if (byte_0 < 0 || byte_1 < 0 || byte_2 < 0 || byte_3 < 0) {
			return 0;
		}
		
		/* shift the bits into place and output them */
		text_buffer[output_index] = ((byte_0 << 2) & 252) | ((byte_1 >> 4) & 3);		
		text_buffer[output_index + 1] = ((byte_1 << 4) & 240) | ((byte_2 >> 2) & 15);		
		text_buffer[output_index + 2] = ((byte_2 << 6) & 192) | (byte_3 & 63);		
	
		input_index += 4;
		output_index += 3;
	}
	return output_index;
}

/* utility that returns true if all the characters in the passed in line are valide for uudecoding */
static gboolean
line_in_decode_range(char* input_line)
{
	char *line_ptr;
	char current_char;
	
	line_ptr = input_line;
	while (*line_ptr != '\0') {
		current_char = *line_ptr++;
		if (current_char < 32 || current_char > 95) {
			return FALSE;	
		}
	}
	return TRUE;
}

/* fill the buffer from the passed in article fragment.  If the first line flag is set,
 * test to see if uudecoding is required.
 */
static MateVFSResult
load_from_article (NNTPConnection *conn, nntp_fragment *fragment, gboolean first_line_flag)
{
	MateVFSResult result;
	char *line = NULL;
	char *dest_ptr;
	int buffer_offset;
	int line_len;
			
	/* loop, loading the article into the buffer */
	buffer_offset = 0;
	
	while (buffer_offset < conn->buffer_size - 1024) {
		result = read_response_line (conn, &line);	
		if (first_line_flag && !conn->uu_decode_mode && !conn->base_64_decode_mode) {
			/* FIXME: should apply additional checks here for the permission flags, etc */			
			if (strncmp(line, "begin ", 6) == 0) {
				conn->uu_decode_mode = TRUE;
				g_free (line);
				buffer_offset = 0;
				continue;
			} else if (strncmp(line, "Content-Transfer-Encoding: base64", 33) == 0) {
				conn->base_64_decode_mode = TRUE;
				g_free (line);
				buffer_offset = 0;
				continue;
			} else if (line[0] == 'M' && strlen(line) == 61 && line_in_decode_range(line)) {
				conn->uu_decode_mode = TRUE;
				buffer_offset = 0;
			}

		}
		
		if (line[0] != '.' && line[1] != '\r') {
			line_len = strlen(line);
			if (buffer_offset + line_len > conn->buffer_size) {
				g_message("Error! exceeded buffer! %d", buffer_offset + line_len);
				line_len = conn->buffer_size - buffer_offset;
			}
			dest_ptr = (char*) conn->buffer + buffer_offset;
			g_memmove(dest_ptr, line, line_len);
			if (conn->uu_decode_mode) {
				line_len = uu_decode_text(dest_ptr, line_len);
				buffer_offset += line_len;
				fragment->bytes_read += line_len;
			} else if (conn->base_64_decode_mode) {
				line_len = base_64_decode_text(dest_ptr, line_len);
				buffer_offset += line_len;
				fragment->bytes_read += line_len;
			} else {
				buffer_offset += line_len + 1;
				dest_ptr += line_len;
				*dest_ptr = '\n';
				fragment->bytes_read += line_len + 1;
			}
		} else {
			g_free(line);
			conn->request_in_progress = FALSE;
			break;
		}
		g_free(line);
	}
	conn->amount_in_buffer = buffer_offset;
	conn->buffer_offset = 0;
	
	return MATE_VFS_OK;
}

/* load data from the current file fragment, advancing to a new one if necessary */
static MateVFSResult
load_file_fragment(NNTPConnection *connection)
{
	nntp_fragment *fragment;
	MateVFSResult result;
	gboolean first_line_flag;
	
	first_line_flag = FALSE;
	if (!connection->request_in_progress) {
		if (connection->current_fragment == NULL) {
			connection->current_fragment = connection->current_file->part_list;
			first_line_flag = TRUE;
		} else {
			connection->current_fragment = connection->current_fragment->next;
			if (connection->current_fragment == NULL) {
				connection->eof_flag = TRUE;
				return MATE_VFS_ERROR_EOF;
			}
		}
		fragment = (nntp_fragment *) connection->current_fragment->data;	
		result = start_loading_article(connection, fragment);
	}
	
	if (connection->current_fragment == NULL) {
		connection->eof_flag = TRUE;
		return MATE_VFS_ERROR_EOF;
	}
	
	fragment = (nntp_fragment *) connection->current_fragment->data;	
	result = load_from_article(connection, fragment, first_line_flag);
	return result;
}

/* calculate the size of the data remaining in the buffer */
static int
bytes_in_buffer(NNTPConnection *connection)
{
	return connection->amount_in_buffer - connection->buffer_offset;
}

/* utility routine to move bytes from the fragment to the application */
static int
copy_bytes_from_buffer(NNTPConnection *connection,
			gpointer destination_buffer,
			int bytes_to_read,
			MateVFSFileSize *bytes_read)
{
	int size_to_move;
	
	size_to_move = bytes_in_buffer(connection);
	if (size_to_move == 0) {
		return 0;
	}
	
	if (bytes_to_read < size_to_move) {
		size_to_move = bytes_to_read;
	}
	/* move the bytes from the buffer */
	g_memmove(destination_buffer, ((char*) connection->buffer) + connection->buffer_offset, size_to_move);
	
	/* update the counts */
	connection->buffer_offset += size_to_move;
	*bytes_read += size_to_move;
	return size_to_move;
}

/* read a byte range from the active connection */
static MateVFSResult
nntp_file_read(NNTPConnection *connection,
		gpointer buffer,
		MateVFSFileSize num_bytes,
		MateVFSFileSize *bytes_read) 
{	
	int bytes_to_read;
	MateVFSResult result;
	
	/* loop, loading fragments and copying data until the request is fulfilled or
	 * we're out of fragments
	 */
	bytes_to_read = num_bytes;
	*bytes_read = 0;
	while (bytes_to_read > 0) {
		bytes_to_read -= copy_bytes_from_buffer(connection, ((char*) buffer) + *bytes_read, bytes_to_read, bytes_read);
		if (bytes_to_read > bytes_in_buffer(connection)) {
			if (connection->eof_flag) {
				/* don't return EOF here as it will cause the last part to be discarded */
				return MATE_VFS_OK;
			}
			result = load_file_fragment(connection);
		}
	}	
	return MATE_VFS_OK;
}

/* deallocate the passed in file list */
static void
free_nntp_file_list (GList *file_list)
{
	GList* current_file;
	if (file_list == NULL) {
		return;
	}
	
	current_file = file_list;
	while (current_file != NULL) {
		nntp_file_destroy ((nntp_file*) current_file->data);
		current_file = current_file->next;
	}
	g_list_free (file_list);
}

/* utility to obtain authorization info from mate-vfs */
static void
get_auth_info (NNTPConnection *conn, char** user, char** password)
{
	MateVFSResult res;
	MateVFSModuleCallbackAuthenticationIn in_args;
	MateVFSModuleCallbackAuthenticationOut out_args;
	
	*user = NULL;
	*password = NULL;
	
	memset (&in_args, 0, sizeof (in_args));			
	memset (&out_args, 0, sizeof (out_args));
	
	in_args.uri = mate_vfs_uri_to_string(conn->uri, 0);			
	res = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_AUTHENTICATION,
						&in_args, sizeof (in_args), 
							&out_args, sizeof (out_args));	
	g_free(in_args.uri);
						
	*user = out_args.username;
	*password = out_args.password;
}

/* key routine to load the newsgroup overview and return a list of nntp_file objects.
 * It maintains a cache to avoid reloading.
 *
 * FIXME: for now, we use a single element cache, but eventually we want to cache multiple newsgroups
 * also, we eventually want to reload it if enough time has passed
 */
static MateVFSResult
get_files_from_newsgroup (NNTPConnection *conn, const char* newsgroup_name, GList** result_file_list)
{
	MateVFSResult result;
	char *group_command, *tmpstring;
	int first_message, last_message, total_messages;
	char *command_str;
	GList *file_list;
	gchar *user, *pass;
	
	/* see if we can load it from the cache */	
	if (current_newsgroup_name != NULL && g_ascii_strcasecmp (newsgroup_name, current_newsgroup_name) == 0) {
		*result_file_list = current_newsgroup_files;
		return MATE_VFS_OK;
	}
	
	*result_file_list = NULL;
	
	/* we don't have it in the cache, so load it from the server */	
	if (current_newsgroup_name != NULL) {
		free_nntp_file_list (current_newsgroup_files);
		g_free (current_newsgroup_name);
		current_newsgroup_name = NULL;
	}
		
	group_command = g_strdup_printf ("GROUP %s", newsgroup_name);
	
	result = do_basic_command (conn, group_command);
	g_free (group_command);

	if (result != MATE_VFS_OK || conn->response_code != 211) {
		/* if we're anonymous, prompt for a password and try that */
		if (conn->anonymous) {
			get_auth_info(conn, &user, &pass);
			
			if (user != NULL) {
				conn->anonymous = FALSE;				
				tmpstring = g_strdup_printf ("AUTHINFO user %s", user);
				result = do_basic_command (conn, tmpstring);
				g_free (tmpstring);

				if (IS_300 (conn->response_code)) {
					tmpstring = g_strdup_printf ("AUTHINFO pass %s", pass);
					result = do_basic_command (conn, tmpstring);
					g_free (tmpstring);
			
					group_command = g_strdup_printf ("GROUP %s", newsgroup_name);
					result = do_basic_command (conn, group_command);
					g_free (group_command);		

				}
			}
			g_free (user);
			g_free (pass);
		}
		
		if (result != MATE_VFS_OK || conn->response_code != 211) {
			g_message ("couldnt set group to %s, code %d", newsgroup_name, conn->response_code);
				return MATE_VFS_ERROR_NOT_FOUND; /* could differentiate error better */
		}
	}
	
	sscanf (conn->response_message, "%d %d %d", &total_messages, &first_message, &last_message);

	/* read in the header and build the file list for the connection */
	if ((last_message - first_message) > MAX_MESSAGE_COUNT) {
		first_message = last_message - MAX_MESSAGE_COUNT;
	}
	
	command_str = g_strdup_printf ("XOVER %d-%d", first_message, last_message);
	file_list = assemble_files_from_overview (conn, command_str);
	g_free (command_str);
	
	current_newsgroup_name = g_strdup (newsgroup_name);
	current_newsgroup_files = file_list;
	
	*result_file_list = file_list;
	return MATE_VFS_OK;
}

/* utility to strip leading and trailing slashes from a string. It frees the input string and
 * returns a new one
 */
static char*
strip_slashes (char* source_string)
{
	char *temp_str, *result_str;
	int last_offset;
	
	temp_str = source_string;
	if (temp_str[0] == '/') {
		temp_str += 1;	
	}	
	last_offset = strlen(temp_str) - 1;
	if (temp_str[last_offset] == '/') {
		temp_str[last_offset] = '\0';		
	}

	result_str = g_strdup(temp_str);
	g_free(source_string);
	return result_str;
}

/* parse the uri to extract the newsgroup name and the file name */
static void
extract_newsgroup_and_filename(MateVFSURI *uri, char** newsgroup, char **directory, char **filename)
{
	char *dirname, *slash_pos;
	
	*filename = mate_vfs_unescape_string(mate_vfs_uri_extract_short_name (uri), "");
	*directory = NULL;
	 
	dirname = strip_slashes(mate_vfs_uri_extract_dirname(uri));
	
	*newsgroup = mate_vfs_unescape_string (dirname, "");
	slash_pos = strchr (*newsgroup, '/');
	if (slash_pos != NULL) {
		*slash_pos = '\0';
		*directory = g_strdup (slash_pos + 1);
	}
	g_free (dirname);
}

/* fetch the nntp_file object from the passed in uri */
static nntp_file*
nntp_file_from_uri (NNTPConnection *conn, MateVFSURI *uri)
{
	MateVFSResult result;
	char *newsgroup_name, *file_name, *directory_name;
	nntp_file *file;
	GList *file_list;
	
	/* extract the newsgroup name */
	extract_newsgroup_and_filename(uri, &newsgroup_name, &directory_name, &file_name);
	
	/* load the (cached) file list */
	result = get_files_from_newsgroup (conn, newsgroup_name, &file_list);	
	if (file_list == NULL) {
		file = NULL;
	} else {
		/* parse the uri into the newsgroup and file */
		if (directory_name != NULL) {
			file = look_up_file (file_list, directory_name, TRUE);
			if (file != NULL) {
				file = look_up_file (file->part_list, file_name, FALSE);
			}
		} else {
			file = look_up_file (file_list, file_name, FALSE);	
		}
	}
	
	g_free(newsgroup_name);
	g_free(file_name);
	g_free(directory_name);
	
	return file;
}

/* determine if a file has all of it's fragments */
static gboolean
has_all_fragments (nntp_file *file)
{
	return g_list_length (file->part_list) >= file->total_parts;
}

/* add a file fragment to the file list, creating a new file object if necessary */
static GList *add_file_fragment(GList* file_list, char* filename, char* folder_name, char* fragment_id, int fragment_size,
				int part_number, int part_total, time_t mod_date)
{
	nntp_file *base_file;

	/* don't use part 0 */
	if (part_number == 0) {
		return file_list;
	}
	
	/* get the file object associated with the filename if any */
	base_file = look_up_file(file_list, filename, FALSE);
	if (base_file == NULL) {
		base_file = nntp_file_new(filename, folder_name, part_total);
		base_file->mod_date = mod_date;
		file_list = g_list_append(file_list, base_file);
	}
	
	/* add the fragment to the file */
	nntp_file_add_part(base_file, part_number, fragment_id, fragment_size);
	return file_list;
}

/* remove any partial files from the file list */
static GList *remove_partial_files (GList *file_list)
{
	nntp_file* file;
	
	GList* next_file;
	GList* current_file = file_list;

	while (current_file != NULL) {
		next_file = current_file->next;
		file = (nntp_file*) current_file->data;
		if (!has_all_fragments (file)) {
			file_list = g_list_remove_link (file_list, current_file);
			nntp_file_destroy (file);
		}
		current_file = next_file;
	}	
	return file_list;
}


/* loop through the file list to update the size fields */
static void
update_file_sizes(GList *file_list)
{
	nntp_file* file;	
	GList* current_file = file_list;
		
	while (current_file != NULL) {
		file = (nntp_file*) current_file->data;
		file->file_size = nntp_file_get_total_size(file);
		current_file = current_file->next;
	}	
}

/* the following set of routines are used to group the files into psuedo-directories by using
 * a hash table containing lists of nntp_file objects.
 */

/* add the passed in file's foldername to the hash table */
static void
add_file_to_folder (GHashTable *folders, nntp_file *file)
{
	GList *folder_contents;
	
	if (file->folder_name == NULL) {
		return;
	}
	
	folder_contents = g_hash_table_lookup (folders, file->folder_name);
	if (folder_contents != NULL) {
		folder_contents = g_list_append (folder_contents, file);
	} else {
		folder_contents = g_list_append (NULL, file);
		g_hash_table_insert (folders, g_strdup(file->folder_name), folder_contents);
	}
}

/* remove non-singleton files that are contained in folders from the file list */
static void
remove_file_from_list (gpointer key, gpointer value, gpointer callback_data)
{
	GList *element_list;
	GList** file_list_ptr;
		
	file_list_ptr = (GList**) callback_data;
	element_list = (GList*) value;

	/* if there is more than one element in the list, remove all of them from the file list */	
	if (element_list != NULL && g_list_length(element_list) > 1) {
		while (element_list != NULL) {
			*file_list_ptr = g_list_remove(*file_list_ptr, element_list->data);
			element_list = element_list->next;
		}
	}	
}

static void
remove_contained_files(GHashTable *folders, GList** file_list_ptr)
{
	/* iterate through the passed in hash table to find the files to remove */
	g_hash_table_foreach (folders, remove_file_from_list, file_list_ptr);	
}

/* utility routine to calculate a folder's mod-date by inspecting the dates of it's constituents */
static void
calculate_folder_mod_date (nntp_file *folder)
{
	time_t latest_mod_date;
	GList *current_file;
	nntp_file* current_file_data;
	
	latest_mod_date = 0;
	current_file = folder->part_list;
	while (current_file != NULL) {
		current_file_data = (nntp_file*) current_file->data;
		if (current_file_data->mod_date > latest_mod_date) {
			latest_mod_date = current_file_data->mod_date;
		}
		current_file = current_file->next;
	}
	folder->mod_date = latest_mod_date;
}

/* generate folder objects from the entries in the hash table */
static void
generate_folder_from_element (gpointer key, gpointer value, gpointer callback_data)
{
	GList *element_list;
	int number_of_elements;
	nntp_file *new_folder;
	char* key_as_string;
	GList** file_list_ptr;
		
	file_list_ptr = (GList**) callback_data;
	element_list = (GList*) value;
	key_as_string = (char*) key;
	
	number_of_elements = g_list_length(element_list);
	if (number_of_elements > 1) {
		if (strlen(key_as_string) == 0) {
			key_as_string = "Unknown Title";
		}
		
		new_folder = nntp_file_new (key_as_string, NULL, number_of_elements);
		new_folder->is_directory = TRUE;
		new_folder->file_type = g_strdup ("x-directory/normal");
		new_folder->part_list = g_list_copy (element_list);
		
		calculate_folder_mod_date (new_folder);
		
		*file_list_ptr = g_list_append (*file_list_ptr, new_folder);
	}
}

static void
generate_folders(GHashTable *folders, GList** file_list_ptr)
{
	/* iterate through the passed in hash table to generate a folder for each non-singleton entry */
	g_hash_table_foreach (folders, generate_folder_from_element, file_list_ptr);
}

/* helper routine to deallocate hash table elements */
static gboolean
destroy_element (gpointer key, gpointer value, gpointer data)
{
	g_free(key);
        g_list_free((GList*) value);
	return TRUE;
}

/* deallocate the data structures associate with the folder hash table */
static void
destroy_folder_hash(GHashTable *folders)
{
	/* iterate through the hash table to destroy the lists it contains, before destroying
	 *the table itself
	 */
	g_hash_table_foreach_remove (folders, destroy_element, NULL);
	g_hash_table_destroy(folders);
}

/* walk through the file list and try to generate folders to group files with similar subjects */
static GList*
assemble_folders (GList *file_list)
{
	GList *current_item;
	nntp_file *current_file;
	GHashTable *folders;
	GList* file_list_ptr;

	/* need indirection for file list head */
	file_list_ptr = file_list;
	
	/* make a pass through the files grouping the ones with matching foldernames.  Use a hash
	 * table for fast lookups
	 */
	folders = g_hash_table_new (g_str_hash, g_str_equal);
	current_item = file_list_ptr;
	
	while (current_item != NULL) {
		current_file = (nntp_file*) current_item->data;
		if (current_file->folder_name != NULL) {
			add_file_to_folder (folders, current_file);	
		}
		current_item = current_item->next;	
	}

	/* make a pass through the folder list to remove contained files from the file list */
	remove_contained_files (folders, &file_list_ptr);
	
	/* generate folders from the hash table (not singletons) */
	generate_folders (folders, &file_list_ptr);
	
	/* deallocate resources now that we're done */
	destroy_folder_hash (folders);

	return file_list_ptr;
}

/* this is the main loop for assembling the files.  It issues the passed in
 * overview command and then reads the headers one at a time, processing the
 * fragments into a list of files
 */
static GList* 
assemble_files_from_overview (NNTPConnection *conn, char *command) 
{
	MateVFSResult result;
	char *line = NULL;
	int message_size;
	int part_number, total_parts;
	char* filename, *folder_name, *message_id;
	GList *file_list;
	time_t mod_date;
	
	file_list = NULL;
	
	/* issue the overview command to the server */
	result = do_control_write (conn, command);	
	
	if (result != MATE_VFS_OK) {
		return file_list;
	}
	
	/* read the response command, and check that it's the right number (eventually) */
	result = read_response_line (conn, &line);	
	g_free(line);

	if (result != MATE_VFS_OK) {
		return file_list;
	}

	while (TRUE) {
		result = read_response_line (conn, &line);	
		if (line[0] != '.' && line[1] != '\r') {
			if (parse_header (line, &filename, &folder_name, &message_id, &message_size, &part_number, &total_parts, &mod_date)) {
				file_list = add_file_fragment (file_list, filename, folder_name, message_id, message_size,
						part_number, total_parts, mod_date);
				g_free (filename);
				g_free (folder_name);
				g_free (message_id);
			}		
		} else {
			break;
		}
		g_free(line);
	}
	
	file_list = remove_partial_files (file_list);
	update_file_sizes (file_list);
	file_list = assemble_folders (file_list);
	
	return file_list;
}

/* newsgroup files aren't local */
gboolean 
do_is_local (MateVFSMethod *method, 
	     const MateVFSURI *uri)
{
	return FALSE;
}

static void
prepare_to_read_file (NNTPConnection *conn, nntp_file *file)
{
	conn->current_file = file;
	conn->current_fragment = NULL;
	conn->buffer_offset = 0;
	conn->amount_in_buffer = 0;
	conn->eof_flag = FALSE;
	conn->uu_decode_mode = FALSE;
	conn->base_64_decode_mode = FALSE;
	
	nntp_connection_reset_buffer (conn);
}

static MateVFSResult 
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context) 
{
	MateVFSResult result;
	NNTPConnection *conn;
	nntp_file *file;
	const char* basename;

	/* we can save allocating a connection to search for the .directory file */
	basename = mate_vfs_uri_extract_short_name(uri);
	if (strcmp(basename, ".directory") == 0) {
		return MATE_VFS_ERROR_NOT_FOUND;
	}
		
	result = nntp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK) 
		return result;
		
	/* make sure we have the file */
	file = nntp_file_from_uri(conn, uri);
	if (file == NULL) {		
		nntp_connection_release (conn);
		return MATE_VFS_ERROR_NOT_FOUND;
	} else {
		prepare_to_read_file (conn, file);		
	}
	
	if (result == MATE_VFS_OK) {
		*method_handle = (MateVFSMethodHandle *) conn;
	} else {
		*method_handle = NULL;
		nntp_connection_release (conn);
	}
	return result;
}

static MateVFSResult 
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context) 
{
	NNTPConnection *conn = (NNTPConnection *) method_handle;

	nntp_connection_release (conn);

	return MATE_VFS_OK;
}

static MateVFSResult 
do_read (MateVFSMethod *method, 
	 MateVFSMethodHandle *method_handle, 
	 gpointer buffer,
	 MateVFSFileSize num_bytes, 
	 MateVFSFileSize *bytes_read, 
	 MateVFSContext *context) 
{
	NNTPConnection *conn = (NNTPConnection * )method_handle;	
	return nntp_file_read(conn, buffer, num_bytes, bytes_read);
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context) 
{
	const char* host_name;
	const char* temp_str, *first_slash;
	
	MateVFSURI *parent = mate_vfs_uri_get_parent (uri);
	MateVFSResult result;

	host_name = mate_vfs_uri_get_host_name(uri);
	if (host_name == NULL) {
		return MATE_VFS_ERROR_INVALID_HOST_NAME;
	}

	/* if it's the top level newsgroup, treat it like a directory */	
	temp_str = mate_vfs_uri_get_path(uri);
	first_slash = strchr(temp_str + 1, '/');	
	if (parent == NULL || first_slash == NULL) {
		/* this is a request for info about the root directory */
		file_info->name = g_strdup("/");
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->permissions = MATE_VFS_PERM_USER_READ | MATE_VFS_PERM_GROUP_READ |
						MATE_VFS_PERM_OTHER_READ; 
		file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		
		return MATE_VFS_OK;
	} else {
		MateVFSMethodHandle *method_handle;
		char *name;
		
		result = do_open_directory (method, &method_handle, parent,
					    options, context);
		mate_vfs_uri_unref (parent);

		if (result != MATE_VFS_OK) {
			return result;
		}

		name = mate_vfs_uri_extract_short_name (uri);		
		while (result == MATE_VFS_OK) {
			result = do_read_directory (method, method_handle, 
						    file_info, context);
			if (result == MATE_VFS_OK) {
				if (file_info->name && !strcmp (file_info->name,
								name)) {
					g_free (name);
					do_close_directory(
							method, method_handle,
						       	context);
					return MATE_VFS_OK;
				}

				mate_vfs_file_info_clear (file_info);
			}
		}
		do_close_directory(method, method_handle, context);
	}

	return MATE_VFS_ERROR_NOT_FOUND;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	return do_get_file_info (method,
				 ((NNTPConnection *)method_handle)->uri, 
				 file_info, options, context);
}

static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	char *newsgroup_name;
	char *directory_name, *mapped_dirname;
	
	NNTPConnection *conn;
	MateVFSResult result;
	nntp_file *file;
	GList *file_list;
	
	newsgroup_name = mate_vfs_uri_extract_dirname (uri);
	directory_name = g_strdup (mate_vfs_uri_extract_short_name (uri));
	
	if (strcmp (newsgroup_name, "/") == 0 || strlen(newsgroup_name) == 0) {
		g_free (newsgroup_name);
		newsgroup_name = directory_name;
		directory_name = NULL;
	}

	if (newsgroup_name == NULL) {
		g_free(directory_name);
		return MATE_VFS_ERROR_NOT_FOUND;
	}

	newsgroup_name = strip_slashes (newsgroup_name);
	result = nntp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK) {
		g_free (newsgroup_name);
		g_free (directory_name);
		return result;
	}

	/* get the files from the newsgroup */
	result = get_files_from_newsgroup (conn, newsgroup_name, &file_list);
	if (result != MATE_VFS_OK) {
		g_free (newsgroup_name);
		g_free (directory_name);
		nntp_connection_release(conn);
		return result;
	}
		
	if (directory_name == NULL) {
		conn->next_file = file_list;
	} else {
		if (file_list == NULL) {
			file = NULL;
		} else {
			mapped_dirname = mate_vfs_unescape_string (directory_name, "");
			file = look_up_file (file_list, mapped_dirname, TRUE);
			g_free (mapped_dirname);
		}
		
		if (file == NULL) {
			g_message ("couldnt find file %s", directory_name);
			return MATE_VFS_ERROR_NOT_FOUND;
		}
		if (file->is_directory) {
			conn->next_file = file->part_list;
		} else {
			conn->next_file = NULL;
		}
	}
		
	if (result != MATE_VFS_OK) {
		g_message ("couldnt set group!");
		nntp_connection_release (conn);
		
		g_free (newsgroup_name);
		g_free (directory_name);
		return result;
	}
	*method_handle = (MateVFSMethodHandle *) conn;

	g_free (newsgroup_name);
	g_free (directory_name);

	return result;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context) 
{
	NNTPConnection *conn = (NNTPConnection *) method_handle;

	nntp_connection_release (conn);

	return MATE_VFS_OK;
}


static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	NNTPConnection *conn = (NNTPConnection *) method_handle;
	nntp_file *file_data;
	const char* mime_string;
	
	if (!conn->next_file)
		return MATE_VFS_ERROR_EOF;
	
	/* fill out the information for the current file, and bump the pointer */
	mate_vfs_file_info_clear(file_info);
	
	/* implement the size threshold by skipping files smaller than the threshold */
	file_data = (nntp_file*) conn->next_file->data;
	while (file_data->file_size < MIN_FILE_SIZE_THRESHOLD && !file_data->is_directory) {
		conn->next_file = conn->next_file->next;
		if (conn->next_file == NULL) {
			return MATE_VFS_ERROR_EOF;
		} else {
			file_data = (nntp_file*) conn->next_file->data;
		}
	}
	
	file_info->name = g_strdup(file_data->file_name);
	
	file_info->permissions = MATE_VFS_PERM_USER_READ | MATE_VFS_PERM_GROUP_READ |
				 MATE_VFS_PERM_USER_WRITE |MATE_VFS_PERM_OTHER_READ;
				 
	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS |
			MATE_VFS_FILE_INFO_FIELDS_MTIME;
	
	/* handle the case when it's a directory */
	if (file_data->is_directory) {
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->mtime = file_data->mod_date;
		file_info->permissions = MATE_VFS_PERM_USER_READ | MATE_VFS_PERM_GROUP_READ |
						MATE_VFS_PERM_USER_WRITE |
						MATE_VFS_PERM_OTHER_READ | MATE_VFS_PERM_USER_EXEC |
						MATE_VFS_PERM_GROUP_EXEC | MATE_VFS_PERM_OTHER_EXEC;			
	} else {
	
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;

		file_info->mtime = file_data->mod_date;
		
		/* figure out the mime type from the extension; if it's unrecognized, use "text/plain"
	 	* instead of "application/octet-stream"
	 	*/
		mime_string = mate_vfs_mime_type_from_name(file_data->file_name);
		if (strcmp(mime_string, "application/octet-stream") == 0) {
			file_info->mime_type = g_strdup("text/plain");	
		} else {
			file_info->mime_type = g_strdup(mime_string);	
		}
	
		file_info->size = file_data->file_size;	
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
	}
	
	conn->next_file = conn->next_file->next;
	return MATE_VFS_OK;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
      MateVFSURI *a,
      MateVFSURI *b,
      gboolean *same_fs_return,
      MateVFSContext *context)
{
	*same_fs_return = nntp_connection_uri_equal (a,b);
	return MATE_VFS_OK;
}

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	NULL, /* create */
	do_close,
	do_read,
	NULL, /* write */
	NULL, /* seek */
	NULL, /* tell */
	NULL, /* truncate */
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	NULL, /* make_directory */
	NULL, /* remove directory */
	NULL, /* move */
	NULL, /* unlink */
	do_check_same_fs,
	NULL, /* set_file_info */
	NULL, /* truncate */
	NULL, /* find_directory */
	NULL  /* create_symbolic_link */
};

MateVFSMethod *
vfs_module_init (const char *method_name, 
		 const char *args)
{
#ifdef HAVE_MATECONF
	char *argv[] = {"vfs-nntp-method"};
	int argc = 1;

	/* Ensure MateConf is initialized.  If more modules start to rely on
	 * MateConf, then this should probably be moved into a more 
	 * central location
	 */

	if (!mateconf_is_initialized ()) {
		/* auto-initializes OAF if necessary */
		mateconf_init (argc, argv, NULL);
	}
#endif
	
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
}
