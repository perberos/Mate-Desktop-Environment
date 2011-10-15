/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/* ftp-method.c - VFS modules for FTP

   Copyright (C) 2000 Ian McKellar, Eazel Inc.

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

   Author: Ian McKellar <yakk@yakk.net> */

/* see RFC 959 for protocol details */

#include <config.h>

/* Keep <sys/types.h> above any network includes for FreeBSD. */
#include <sys/types.h>

/* Keep <netinet/in.h> above <arpa/inet.h> for FreeBSD. */
#include <netinet/in.h>

#include <sys/time.h>
#include <arpa/inet.h>

#include <mateconf/mateconf-client.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-inet-connection.h>
#include <libmatevfs/mate-vfs-socket-buffer.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>
#include <libmatevfs/mate-vfs-module-callback-module-api.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-parse-ls.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <stdio.h> /* for sscanf */
#include <stdlib.h> /* for atoi */
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <time.h>

#ifdef HAVE_GSSAPI_H
#include <gssapi.h>
#endif

#ifdef HAVE_GSSAPI_GSSAPI_H
#include <gssapi/gssapi.h>
#endif

#ifdef HAVE_GSSAPI_GSSAPI_GENERIC_H
#include <gssapi/gssapi_generic.h>
#endif

#define PROT_C 1 /* in the clear */
#define PROT_S 2 /* safe */
#define PROT_P 3 /* private */

#define REAP_TIMEOUT (15 * 1000) /* milliseconds */
#define CONNECTION_CACHE_MIN_LIFETIME (30 * 1000) /* milliseconds */
#define DIRLIST_CACHE_TIMEOUT 30 /* seconds */

/* maximum size of response we're expecting to get */
#define MAX_RESPONSE_SIZE 4096 

/* macros for the checking of FTP response codes */
#define IS_100(X) ((X) >= 100 && (X) < 200)
#define IS_200(X) ((X) >= 200 && (X) < 300)
#define IS_300(X) ((X) >= 300 && (X) < 400)
#define IS_400(X) ((X) >= 400 && (X) < 500)
#define IS_500(X) ((X) >= 500 && (X) < 600)

typedef struct {
	char *ip; /* Saved ip to get the same resolv each time */
	gchar *server_type; /* the response from TYPE */
	char *user;
	char *password;

	time_t last_use;
	
	GList *spare_connections;
	int num_connections;
	int num_monitors;
	
	GHashTable *cached_dirlists; /* path -> FtpCachedDirlist */
} FtpConnectionPool;

typedef struct {
	char *dirlist;
	time_t read_time;
} FtpCachedDirlist;

typedef struct {
	MateVFSMethodHandle method_handle;
	MateVFSSocketBuffer *socket_buf;
	MateVFSURI *uri;
	gchar *cwd;
	GString *response_buffer;
	gchar *response_message;
	gint response_code;
	MateVFSSocketBuffer *data_socketbuf;
	guint32 my_ip;
	MateVFSFileOffset offset;
	enum {
		FTP_NOTHING,
		FTP_READ,
		FTP_WRITE,
		FTP_READDIR
	} operation;
	gchar *server_type; /* the response from TYPE */
	MateVFSResult fivefifty; /* the result to return for an FTP 550 */

	const char *list_cmd; /* the command to be used for LIST */

#ifdef HAVE_GSSAPI
	gboolean use_gssapi;
	gss_ctx_id_t gcontext;
	int clevel;
#endif
	
	FtpConnectionPool *pool;
} FtpConnection;

typedef struct {
	MateVFSURI *uri;
	gchar *dirlist;
	gchar *dirlistptr;
	gchar *server_type; /* the response from TYPE */
	MateVFSFileInfoOptions file_info_options;
} FtpDirHandle;

/* Gconf key's "use_http_proxy" is for FTP as well, it should be renamed */
static const char USE_PROXY_KEY[] = "/system/http_proxy/use_http_proxy";

static const char PROXY_FTP_HOST_KEY[] = "/system/proxy/ftp_host";
static const char PROXY_FTP_PORT_KEY[] = "/system/proxy/ftp_port";


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

guint                 ftp_connection_uri_hash  (gconstpointer c);
gint                  ftp_connection_uri_equal (gconstpointer c, gconstpointer d);
static MateVFSResult ftp_connection_acquire   (MateVFSURI *uri, 
		                                FtpConnection **connection, 
		                                MateVFSContext *context);
static void           ftp_connection_release   (FtpConnection *conn,
						gboolean error_release);

static MateVFSResult get_list_command (FtpConnection   *conn,
					MateVFSContext *context);

static const int   control_port = 21;

/* FTP Proxy */
static gchar *proxy_host = NULL;
static int proxy_port = 0;

/* A GHashTable of FtpConnectionPool */

static GHashTable *connection_pools = NULL;
G_LOCK_DEFINE_STATIC (connection_pools);
static gint connection_pool_timeout = 0;
static gint total_connections = 0;
static gint allocated_connections = 0;

#if ENABLE_FTP_DEBUG

#define ftp_debug(c,g) FTP_DEBUG((c),(g),__FILE__, __LINE__, __PRETTY_FUNCTION__)
static void 
FTP_DEBUG (FtpConnection *conn, 
	   gchar *text, 
	   gchar *file, 
	   gint line, 
	   gchar *func) 
{
	if (conn) {
		g_print ("[%p] %s:%d (%s) [ftp conn=%p]\n %s\n",
			 g_thread_self(),
			 file, line, 
			 func, conn, text);
	} else {
		g_print ("[%p] %s:%d (%s) [ftp]\n %s\n",
			 g_thread_self(), file, line, func, text);
	}

	g_free (text);
}

#else 

#define ftp_debug(c,g)

#endif

static MateVFSCancellation *
get_cancellation (MateVFSContext *context)
{
	if (context) {
		return mate_vfs_context_get_cancellation (context);
	}
	return NULL;
}

static MateVFSResult 
ftp_response_to_vfs_result (FtpConnection *conn) 
{
	gint response = conn->response_code;

	switch (response) {
	case 421: 
	case 426: 
	  return MATE_VFS_ERROR_CANCELLED;
	case 425:
		/*FIXME this looks like a bad mapping.  
		 * 425 is "could not open data connection" which
		 * probably doesn't have anything to do with file permissions
		 */ 
	  return MATE_VFS_ERROR_ACCESS_DENIED;
	case 530:
	case 331:
	case 332:
	case 532:
	  return MATE_VFS_ERROR_LOGIN_FAILED;
	case 450:
	case 451:
	case 551:
	  return MATE_VFS_ERROR_NOT_FOUND;
	case 504:
	  return MATE_VFS_ERROR_BAD_PARAMETERS;
	case 550:
	  return conn->fivefifty;
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

#ifdef HAVE_GSSAPI
static const char *radixN =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char pad = '=';

static char *
radix_decode (const unsigned char *inbuf,
	      int *len)
{
	int i, D = 0;
	char *p;
	unsigned char c = 0;
	GString *s;

	s = g_string_new (NULL);
	
	for (i=0; inbuf[i] && inbuf[i] != pad; i++) {
		if ((p = strchr(radixN, inbuf[i])) == NULL) {
			g_string_free (s, TRUE);
			return NULL;
		}
		D = p - radixN;
		switch (i&3) {
		case 0:
			c = D<<2;
			break;
		case 1:
			g_string_append_c (s, c | D>>4);
			c = (D&15)<<4;
			break;
		case 2:
			g_string_append_c (s, c | D>>2);
			c = (D&3)<<6;
			break;
		case 3:
			g_string_append_c (s, c | D);
		}
	}
	switch (i&3) {
	case 1:
		g_string_free (s, TRUE);
		return NULL;
	case 2:
		if (D&15) {
			g_string_free (s, TRUE);
			return NULL;
		}
		if (strcmp((char *)&inbuf[i], "==")) {
			g_string_free (s, TRUE);
			return NULL;
		}
		break;
	case 3:
		if (D&3) {
			g_string_free (s, TRUE);
			return NULL;
		}
		if (strcmp((char *)&inbuf[i], "=")) {
			g_string_free (s, TRUE);
			return NULL;
		}
	}
	*len = s->len;
	return g_string_free (s, FALSE);
}

static char *
radix_encode (unsigned char inbuf[],
	      int len)
{
	int i = 0;
	unsigned char c = 0;
	GString *s;

	s = g_string_new (NULL);
	
	for (i=0; i < len; i++)
		switch (i%3) {
		case 0:
			g_string_append_c (s, radixN[inbuf[i]>>2]);
			c = (inbuf[i]&3)<<4;
			break;
		case 1:
			g_string_append_c (s, radixN[c|inbuf[i]>>4]);
			c = (inbuf[i]&15)<<2;
			break;
		case 2:
			g_string_append_c (s, radixN[c|inbuf[i]>>6]);
			g_string_append_c (s, radixN[inbuf[i]&63]);
			c = 0;
		}
	if (i%3)
		g_string_append_c (s, radixN[c]);
	switch (i%3) {
	case 1:
		g_string_append_c (s, pad);
	case 2:
		g_string_append_c (s, pad);
	}
	g_string_append_c (s, '\0');
	return g_string_free (s, FALSE);
}
#endif /* HAVE_GSSAPI */


static MateVFSResult
read_response_line (FtpConnection *conn, gchar **line,
		    MateVFSCancellation *cancellation)
{
	MateVFSFileSize bytes = MAX_RESPONSE_SIZE, bytes_read;
	gchar *ptr, *buf = g_malloc (MAX_RESPONSE_SIZE+1);
	gint line_length;
	MateVFSResult result = MATE_VFS_OK;

	while (!strstr (conn->response_buffer->str, "\r\n")) {
		/* we don't have a full line. Lets read some... */
		/*ftp_debug (conn,g_strdup_printf ("response `%s' is incomplete", conn->response_buffer->str));*/
		bytes_read = 0;
		result = mate_vfs_socket_buffer_read (conn->socket_buf, buf,
						       bytes, &bytes_read, cancellation);
		buf[bytes_read] = '\0';
		/*ftp_debug (conn,g_strdup_printf ("read `%s'", buf));*/
		conn->response_buffer = g_string_append (conn->response_buffer,
							 buf);
		if (result != MATE_VFS_OK) {
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
get_response (FtpConnection *conn, MateVFSCancellation *cancellation)
{
	/* all that should be pending is a response to the last command */
	MateVFSResult result;

	/*ftp_debug (conn,g_strdup_printf ("get_response(%p)",  conn));*/

	while (TRUE) {
		gchar *line = NULL;
		result = read_response_line (conn, &line, cancellation);

		if (result != MATE_VFS_OK) {
			g_free (line);
			return result;
		}

#ifdef HAVE_GSSAPI
		if (conn->use_gssapi) {
			/* This is a GSSAPI-protected response message */
			int conf_state, decoded_response_len;
			char *decoded_response;
			gss_buffer_desc encrypted_buf, decrypted_buf;
			gss_qop_t maj_stat, min_stat;

			conf_state = (line[0] == '6' && line[1] == '3' &&  line[2] == '1');
			
			/* BASE64-decode the response */
			decoded_response = radix_decode ((guchar *)line + 4, &decoded_response_len);
			g_free (line);
			if (decoded_response == NULL) {
				return MATE_VFS_ERROR_GENERIC;
			}
			
			/* Unseal the GSSAPI-protected response */
			encrypted_buf.value = decoded_response;
			encrypted_buf.length = decoded_response_len;
			maj_stat = gss_unseal (&min_stat, conn->gcontext, &
					       encrypted_buf, &decrypted_buf,
					       &conf_state, NULL);
			g_free (decoded_response);
			
			if (maj_stat != GSS_S_COMPLETE) {
				g_warning ("failed unsealing reply");
				return MATE_VFS_ERROR_GENERIC;
			}

			line = g_strdup_printf ("%s\r\n", (char *)decrypted_buf.value);
			gss_release_buffer (&min_stat, &decrypted_buf);
		}
#endif /* HAVE_GSSAPI */
		
		/* response needs to be at least: "### x"  - I think*/
		if (g_ascii_isdigit (line[0]) &&
		    g_ascii_isdigit (line[1]) &&
		    g_ascii_isdigit (line[2]) &&
		    g_ascii_isspace (line[3])) {

			conn->response_code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');

			if (conn->response_message) g_free (conn->response_message);
			conn->response_message = g_strdup (line+4);

			ftp_debug (conn,g_strdup_printf ("got response %d (%s)", 
							 conn->response_code, conn->response_message));

			g_free (line);

			return ftp_response_to_vfs_result (conn);

		}

		/* hmm - not a valid line - lets ignore it :-) */
		g_free (line);

	}

	return MATE_VFS_OK; /* should never be reached */

}

static MateVFSResult do_control_write (FtpConnection *conn, 
					gchar *command,
					MateVFSCancellation *cancellation) 
{
        gchar *actual_command;
	MateVFSFileSize bytes, bytes_written;
	MateVFSResult result;


	actual_command = g_strdup_printf ("%s\r\n", command);

#ifdef HAVE_GSSAPI
	if (conn->use_gssapi) {
		gss_buffer_desc in_buf, out_buf;
		gss_qop_t maj_stat, min_stat;
		int conf_state;
		char *base64;
		
		/* The session is protected with GSSAPI, so the command
		   must be sealed properly */
		in_buf.value = actual_command;
		in_buf.length = strlen (actual_command) + 1;
		maj_stat = gss_seal (&min_stat,
				     conn->gcontext, (conn->clevel == PROT_P),
				     GSS_C_QOP_DEFAULT,
				     &in_buf, &conf_state, &out_buf);
		g_free (actual_command);
		
		if (maj_stat != GSS_S_COMPLETE) {
			g_warning ("Error sealing the command %s", actual_command);
			return MATE_VFS_ERROR_GENERIC;
		} else if ((conn->clevel == PROT_P) && !conf_state) {
			g_warning ("GSSAPI didn't encrypt the message");
			return MATE_VFS_ERROR_GENERIC;
		}
		
		/* Encode the command in BASE64 */
		base64 = radix_encode (out_buf.value, out_buf.length);
		gss_release_buffer (&min_stat, &out_buf);
		
		actual_command = g_strdup_printf ("%s %s\r\n", (conn->clevel == PROT_P) ? "ENC" : "MIC", base64);
		g_free (base64);
	}
#endif /* HAVE_GSSAPI */
	
	bytes = strlen (actual_command);
	result = mate_vfs_socket_buffer_write (conn->socket_buf,
						actual_command, bytes,
						&bytes_written, cancellation);
#if 1
	ftp_debug (conn, g_strdup_printf ("sent \"%s\\r\\n\"", command));
#endif
	mate_vfs_socket_buffer_flush (conn->socket_buf, cancellation);

	if(result != MATE_VFS_OK) {
		g_free (actual_command);
		return result;
	}

	if(bytes != bytes_written) {
		g_free (actual_command);
		return result;
	}

	g_free (actual_command);

	return result;
}

static MateVFSResult 
do_basic_command (FtpConnection *conn, 
		  gchar *command,
		  MateVFSCancellation *cancellation) 
{
	MateVFSResult result = do_control_write (conn, command, cancellation);

	if (result != MATE_VFS_OK) {
		return result;
	}

	result = get_response (conn, cancellation);

	return result;
}

static MateVFSResult 
do_path_command (FtpConnection *conn, 
		 gchar *command,
		 MateVFSURI *uri,
		 MateVFSCancellation *cancellation) 
{
	char *unescaped;
	gchar *basename,
	      *path,
	      *actual_command,
	      *cwd_command;
	MateVFSResult result;
	int end;

	/* Execute a CD and then a command using the basename rather
	 * than the full path. Useful for some (buggy?) systems. */
	unescaped = mate_vfs_unescape_string (uri->text, G_DIR_SEPARATOR_S);
	if (unescaped == NULL || unescaped[0] == '\0') {
		g_free (unescaped);
		unescaped = g_strdup ("/");
	}

	/* Remove trailing slashes */
	end = strlen(unescaped) - 1;
	if (end > 0 && unescaped[end] == '/') {
		unescaped[end] = 0;
	}
	
	basename = g_path_get_basename (unescaped);
	path = g_path_get_dirname (unescaped);
	g_free (unescaped);

	cwd_command = g_strconcat ("CWD ", path, NULL);
	g_free (path);
	result = do_basic_command (conn, cwd_command, cancellation);
	g_free (cwd_command);
	if (result != MATE_VFS_OK) {
		g_free (basename);
		return result;
	}

	actual_command = g_strconcat (command, " ", basename, NULL);
	g_free (basename);

	result = do_basic_command (conn, actual_command, cancellation);
	g_free (actual_command);
	return result;
}

static MateVFSResult 
do_path_command_completely (gchar *command, 
                            MateVFSURI *uri, 
			    MateVFSContext *context,
			    MateVFSResult fivefifty) 
{
	FtpConnection *conn;
	MateVFSResult result;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);
	result = ftp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK) {
		return result;
	}

	conn->fivefifty = fivefifty;
	result = do_path_command (conn, command, uri, cancellation);
	ftp_connection_release (conn, result != MATE_VFS_OK);

	return result;
}

static MateVFSResult
do_transfer_command (FtpConnection *conn, gchar *command, MateVFSContext *context) 
{
	char *host = NULL;
	gint port;
	MateVFSResult result;
	MateVFSInetConnection *data_connection;
	MateVFSSocket *socket;
	MateVFSCancellation *cancellation;
	struct sockaddr_in my_ip;
	socklen_t my_ip_len;
	
	cancellation = get_cancellation (context);

	/* Image mode (binary to the uninitiated) */
	result = do_basic_command (conn, "TYPE I", cancellation);

	if (result != MATE_VFS_OK) {
		return result;
	}

	/* FIXME bugzilla.eazel.com 1464: implement non-PASV mode */

	/* send PASV */
	result = do_basic_command (conn, "PASV", cancellation);
	
	if (result != MATE_VFS_OK) {
		return result;
	}

	/* parse response */
	{
	        gint a1, a2, a3, a4, p1, p2;
		gchar *ptr, *response = g_strdup (conn->response_message);
		ptr = strchr (response, '(');
		if (!ptr ||
		    (sscanf (ptr+1,"%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, 
			     &a4, &p1, &p2) != 6)) {
			g_free (response);
			return MATE_VFS_ERROR_CORRUPTED_DATA;
		}

		host = g_strdup_printf ("%d.%d.%d.%d", a1, a2, a3, a4);
		port = p1*256 + p2;

		g_free (response);

	}

	/* connect */
	result = mate_vfs_inet_connection_create (&data_connection,
						   host, 
						   port,
						   cancellation);

	g_free (host);
	if (result != MATE_VFS_OK) {
		return result;
	}
	
	my_ip_len = sizeof (my_ip);
	if (getsockname( mate_vfs_inet_connection_get_fd (data_connection),
			 (struct sockaddr*)&my_ip, &my_ip_len) == 0) {
		conn->my_ip = my_ip.sin_addr.s_addr;
	}

	socket = mate_vfs_inet_connection_to_socket (data_connection);
	conn->data_socketbuf = mate_vfs_socket_buffer_new (socket);
	if (conn->offset) {
		gchar *tmpstring;
		tmpstring = g_strdup_printf ("REST %"MATE_VFS_OFFSET_FORMAT_STR, conn->offset);
		result = do_basic_command (conn, tmpstring, cancellation);
		g_free (tmpstring);

		if (result != MATE_VFS_OK) {
			mate_vfs_socket_buffer_destroy (conn->data_socketbuf, TRUE, cancellation);
			conn->data_socketbuf = NULL;
			return result;
		}
	}

	result = do_control_write (conn, command, cancellation);

	if (result != MATE_VFS_OK) {
		mate_vfs_socket_buffer_destroy (conn->data_socketbuf, TRUE, cancellation);
		conn->data_socketbuf = NULL;
		return result;
	}

	result = get_response (conn, cancellation);

	if (result != MATE_VFS_OK) {
		mate_vfs_socket_buffer_destroy (conn->data_socketbuf, TRUE, cancellation);
		conn->data_socketbuf = NULL;
		return result;
	}

	return result;
}

static MateVFSResult
do_path_transfer_command (FtpConnection *conn, gchar *command, MateVFSURI *uri, MateVFSContext *context) 
{
	char *unescaped;
	gchar *basename,
	      *path,
	      *cwd_command,
	      *actual_command;
	MateVFSResult result;
	int end;

	unescaped = mate_vfs_unescape_string (uri->text, G_DIR_SEPARATOR_S);
	if (unescaped == NULL || unescaped[0] == '\0') {
		g_free (unescaped);
		unescaped = g_strdup ("/");
	}

	/* Remove trailing slashes */
	end = strlen(unescaped) - 1;
	if (end > 0 && unescaped[end] == '/') {
		unescaped[end] = 0;
	}

	basename = g_path_get_basename (unescaped);
	path = g_path_get_dirname (unescaped);
	g_free (unescaped);

	cwd_command = g_strconcat ("CWD ", path, NULL);
	g_free (path);
	result = do_basic_command (conn, cwd_command,
				   get_cancellation (context));
	g_free (cwd_command);
	if (result != MATE_VFS_OK) {
		g_free (basename);
		return result;
	}

	actual_command = g_strconcat (command, " ", basename, NULL);
	g_free (basename);

	result = do_transfer_command (conn, actual_command, context);
	g_free (actual_command);
	return result;
}


static MateVFSResult 
end_transfer (FtpConnection *conn,
	      MateVFSCancellation *cancellation) 
{
	MateVFSResult result;

	/*ftp_debug (conn, g_strdup ("end_transfer()"));*/

	if (conn->data_socketbuf) {
		mate_vfs_socket_buffer_flush (conn->data_socketbuf, cancellation);
		mate_vfs_socket_buffer_destroy (conn->data_socketbuf, TRUE, cancellation);
		conn->data_socketbuf = NULL;
	}

	result = get_response (conn, cancellation);

	return result;

}

static void
save_authn_info (MateVFSURI *uri,
		 char *user,
		 char *pass,
		 char *keyring)
{
	MateVFSModuleCallbackSaveAuthenticationIn in_args;
	MateVFSModuleCallbackSaveAuthenticationOut out_args;
	
        memset (&in_args, 0, sizeof (in_args));
        memset (&out_args, 0, sizeof (out_args));
	
        in_args.keyring = keyring;

	in_args.uri = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
        in_args.server = (char *)mate_vfs_uri_get_host_name (uri);
        in_args.port = mate_vfs_uri_get_host_port (uri);
        in_args.username = user;
        in_args.password = pass;
        in_args.protocol = "ftp";
	
        mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					  &in_args, sizeof (in_args),
					  &out_args, sizeof (out_args));
	
        g_free (in_args.uri);
}

/*
 * Returns FALSE if callback was not handled (*aborted
 *   will be FALSE, user will be "anonymous")
 *
 * Returns TRUE if callback invocation succeeded, *aborted
 *   will be set to TRUE only if the user didn't cancel
 */
static gboolean
query_user_for_authn_info (MateVFSURI *uri, 
			   char **user, char **pass, char **keyring,
			   gboolean *save, gboolean *aborted,
			   gboolean no_username)
{
	MateVFSModuleCallbackFullAuthenticationIn in_args;
        MateVFSModuleCallbackFullAuthenticationOut out_args;
	gboolean ret;

	ret = *aborted = FALSE;
	
	memset (&in_args, 0, sizeof (in_args));
        memset (&out_args, 0, sizeof (out_args));
	
	in_args.uri = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
        in_args.server = (char *)mate_vfs_uri_get_host_name (uri);
        in_args.port = mate_vfs_uri_get_host_port (uri);
        if (*user != NULL && *user[0] != 0) {
                in_args.username = *user;
        }
	
	in_args.protocol = "ftp";
	
	in_args.flags =	MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED | 
		MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD; 
        if (no_username) {
                in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME;
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_ANON_SUPPORTED;
        }
	
	in_args.default_user = in_args.username;
	
	ret = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
                                                &in_args, sizeof (in_args),
                                                &out_args, sizeof (out_args));

	if (!ret) {
		/* No callback, try anon login */
		*user = g_strdup ("anonymous");
		*pass = g_strdup ("nobody@gnome.org");
                goto error;
        }

	*aborted = out_args.abort_auth;

	if (out_args.abort_auth) {
                goto error;
        }
	
	g_free (*user);
	g_free (*pass);
	if (out_args.out_flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_OUT_ANON_SELECTED) {
		*user = g_strdup ("anonymous");
		*pass = g_strdup ("nobody@gnome.org");
	}
	else {
		*user = g_strdup (out_args.username);
		*pass = g_strdup (out_args.password);
	}
	
	*save = FALSE;
	if (out_args.save_password) {
		*save = TRUE;
		g_free (*keyring);
		*keyring = g_strdup (out_args.keyring);
	}
	
 error:
      	g_free (in_args.uri);
       	g_free (in_args.object);
       	g_free (out_args.username);
       	g_free (out_args.domain);
       	g_free (out_args.password);
       	g_free (out_args.keyring);

	return ret;
} 

static gboolean
query_keyring_for_authn_info (MateVFSURI *uri,
			      char **user, char **pass)
{
	MateVFSModuleCallbackFillAuthenticationIn in_args;
        MateVFSModuleCallbackFillAuthenticationOut out_args;
        gboolean ret;
	
        ret = FALSE;
	
        memset (&in_args, 0, sizeof (in_args));
        memset (&out_args, 0, sizeof (out_args));
	
        in_args.uri = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
        in_args.server = (char *)mate_vfs_uri_get_host_name (uri);
        in_args.port = mate_vfs_uri_get_host_port (uri);
        if (*user != NULL && *user[0] != 0) {
                in_args.username = *user;
        }
        in_args.protocol = "ftp";
	
        ret = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
                                                &in_args, sizeof (in_args),
                                                &out_args, sizeof (out_args));
	
	if (!ret) {
                goto error;
        }
	
        ret = out_args.valid;
	
        if (!ret) {
                goto error;
        }
	
	g_free (*user);
	g_free (*pass);
        *user = g_strdup (out_args.username);
        *pass = g_strdup (out_args.password);
	
 error:
        g_free (in_args.uri);
        g_free (in_args.object);
        g_free (out_args.username);
        g_free (out_args.domain);
        g_free (out_args.password);
	
        return ret;
}

#ifdef HAVE_GSSAPI
static MateVFSResult
ftp_kerberos_login (FtpConnection *conn, 
		    const char *user,
		    char *saved_ip,
		    MateVFSCancellation *cancellation)

{
	char *tmpstring;
	struct gss_channel_bindings_struct chan;
	gss_buffer_desc send_tok, recv_tok, *token_ptr;
	gss_qop_t maj_stat, min_stat;
	gss_name_t target_name;
	struct in_addr my_ip;
	char *decoded_token;
	char *encoded_token;
	int len;
	MateVFSResult result;

	result = do_basic_command (conn, "AUTH GSSAPI", cancellation);
	if (result != MATE_VFS_OK) {
		return result;
	}
	
	if (conn->response_code != 334) {
		return MATE_VFS_ERROR_LOGIN_FAILED;
	}

	if (inet_aton (saved_ip, &my_ip) == 0) {
		return MATE_VFS_ERROR_GENERIC;
	}

	chan.initiator_addrtype = GSS_C_AF_INET; /* OM_uint32  */
	chan.initiator_address.length = 4;
	chan.initiator_address.value = &my_ip.s_addr;
	chan.acceptor_addrtype = GSS_C_AF_INET; /* OM_uint32 */
	chan.acceptor_address.length = 4;
	chan.acceptor_address.value = &conn->my_ip;
	chan.application_data.length = 0;
	chan.application_data.value = 0;

	send_tok.value = g_strdup_printf ("host@%s", saved_ip);
	send_tok.length = strlen (send_tok.value) + 1;
	maj_stat = gss_import_name (&min_stat, &send_tok, GSS_C_NT_HOSTBASED_SERVICE,
				    &target_name);
	g_free (send_tok.value);
	if (maj_stat != GSS_S_COMPLETE) {
		g_warning ("gss_import_name failed");
		return MATE_VFS_ERROR_GENERIC;
	}

	token_ptr = GSS_C_NO_BUFFER;
	conn->gcontext = GSS_C_NO_CONTEXT;
	decoded_token = NULL;
	do {
		maj_stat = gss_init_sec_context (&min_stat,
						 GSS_C_NO_CREDENTIAL, &conn->gcontext,
						 target_name, GSS_C_NO_OID,
						 GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
						 0, &chan, token_ptr, NULL, &send_tok,
						 NULL, NULL);
		g_free (decoded_token);
		if (maj_stat != GSS_S_CONTINUE_NEEDED && maj_stat != GSS_S_COMPLETE) {
			gss_release_name (&min_stat, &target_name);
			return MATE_VFS_ERROR_GENERIC;
		} 

		if (send_tok.length != 0) {
			/* Encode the GSSAPI-generated token which will be sent
			   to the remote server in order to perform authentication. */
			encoded_token = radix_encode (send_tok.value, send_tok.length);
			
			/* Sent the BASE64-encoded, GSSAPI-generated authentication
			   token to the remote server via the ADAT command. */
			tmpstring = g_strdup_printf ("ADAT %s", encoded_token);
			g_free (encoded_token);
			result = do_basic_command (conn, tmpstring, cancellation);
			g_free (tmpstring);
			if (result != MATE_VFS_OK) {
				gss_release_name (&min_stat, &target_name);
				gss_release_buffer (&min_stat, &send_tok);
				return MATE_VFS_ERROR_GENERIC;
			}
		       
			if (conn->response_code != 235) {
				/* If the remote server does not support the ADAT command,
				   use plain-text login. */
				gss_release_name (&min_stat, &target_name);
				gss_release_buffer (&min_stat, &send_tok);
				return MATE_VFS_ERROR_LOGIN_FAILED;
			}
			
			decoded_token = radix_decode ((guchar *)conn->response_message + 5,
						      &len);
			if (decoded_token == NULL) {
				gss_release_name (&min_stat, &target_name);
				gss_release_buffer (&min_stat, &send_tok);
				return MATE_VFS_ERROR_GENERIC;
			}
			
			token_ptr = &recv_tok;
			recv_tok.value = decoded_token;
			recv_tok.length = len;
			/* decoded_token is freed in the next iteration */
		}
	} while (maj_stat == GSS_S_CONTINUE_NEEDED);
	
	gss_release_buffer (&min_stat, &send_tok);
	gss_release_name (&min_stat, &target_name);
	conn->use_gssapi = TRUE;
	conn->clevel = PROT_S;
	
	tmpstring = g_strdup_printf ("USER %s", g_get_user_name ());
	result = do_basic_command (conn, tmpstring, cancellation);
	g_free (tmpstring);

	if (IS_500 (conn->response_code)) {
		conn->use_gssapi = FALSE;
		return MATE_VFS_ERROR_GENERIC;
	}
	return MATE_VFS_OK;
}
#endif /* HAVE_GSSAPI */

static MateVFSResult
ftp_login (FtpConnection *conn, 
	   const char *user, const char *password,
	   MateVFSCancellation *cancellation)
{
	gchar *tmpstring;
	MateVFSResult result;
	
	if (!proxy_host) {
		tmpstring = g_strdup_printf ("USER %s", user);
	} else {
		/* Using FTP proxy */
		tmpstring = g_strdup_printf ("USER %s@%s",
					     user,
					     mate_vfs_uri_get_host_name (conn->uri));
	}

	result = do_basic_command (conn, tmpstring, cancellation);
	g_free (tmpstring);

	if (IS_300 (conn->response_code)) {
		tmpstring = g_strdup_printf ("PASS %s", password);
		result = do_basic_command (conn, tmpstring, cancellation);
		g_free (tmpstring);
	}

	return result;
}

static MateVFSResult
try_connection (MateVFSURI *uri,
		char **saved_ip,
		FtpConnection *conn,
		MateVFSCancellation *cancellation)
{
	MateVFSInetConnection* inet_connection;
	MateVFSResult result;
	gint port = control_port;
	const char *host;
	
	if (proxy_host) {
		port = proxy_port;
	} else if (mate_vfs_uri_get_host_port (uri)) {
                port = mate_vfs_uri_get_host_port (uri);
        }


	if (*saved_ip != NULL) {
		host = *saved_ip;
	} else {
		if (!proxy_host) {
			host = mate_vfs_uri_get_host_name (uri);
		} else  {
			/* Use FTP proxy */
			host = proxy_host;
		}
	}

	if (host == NULL) {
		return MATE_VFS_ERROR_INVALID_HOST_NAME;
	}
	
	result = mate_vfs_inet_connection_create (&inet_connection,
                                                   host,
                                                   port,
                                                   cancellation);
	
        if (result != MATE_VFS_OK) {
                return result;
        }

	if (*saved_ip == NULL) {
		*saved_ip = mate_vfs_inet_connection_get_ip (inet_connection);
	}
	
        conn->socket_buf = mate_vfs_inet_connection_to_socket_buffer (inet_connection);
	
        if (conn->socket_buf == NULL) {
                mate_vfs_inet_connection_destroy (inet_connection, NULL);
                return MATE_VFS_ERROR_GENERIC;
        }
        conn->offset = 0;
	
        result = get_response (conn, cancellation);
	
	return result;	
}

static MateVFSResult
try_kerberos (MateVFSURI *uri,
	      char **saved_ip,
	      FtpConnection *conn,
	      const char *user,
	      gboolean *connection_failed,
	      MateVFSCancellation *cancellation)
{
	MateVFSResult result;

	/* Even without kerberos we do this login so we
	   can see if the connection fails. The connection will
	   be reused if we succeed */
	*connection_failed = FALSE;
	if (conn->socket_buf == NULL) {
		result = try_connection (uri,
					 saved_ip,
					 conn,
					 cancellation);
		
		if (result != MATE_VFS_OK) {
			*connection_failed = TRUE;
			return result;
		}
	}
	
#ifdef HAVE_GSSAPI
        result = ftp_kerberos_login (conn, user, *saved_ip, cancellation);
	
        if (result != MATE_VFS_OK) {
                /* if login failed, don't free socket buf.
		   Its reused for normal login */
		if (result != MATE_VFS_ERROR_LOGIN_FAILED) {
			mate_vfs_socket_buffer_destroy (conn->socket_buf, TRUE, cancellation);
			conn->socket_buf = NULL;
		}
                return result;
        }
	
	return result;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
}

static MateVFSResult
try_login (MateVFSURI *uri,
	   char **saved_ip,
	   FtpConnection *conn,
	   gchar *user, 
	   gchar *pass,
	   MateVFSCancellation *cancellation)
{
	MateVFSResult result;

	if (conn->socket_buf == NULL) {
		result = try_connection (uri,
					 saved_ip,
					 conn,
					 cancellation);
		
		if (result != MATE_VFS_OK) {
			return result;
		}
	}
	
        result = ftp_login (conn, user, pass, cancellation);
	
        if (result != MATE_VFS_OK) {
                /* login failed */
                mate_vfs_socket_buffer_destroy (conn->socket_buf, TRUE, cancellation);
		conn->socket_buf = NULL;
                return result;
        }
	
	return result;	
}

static void toggle_winnt_into_unix_mode(FtpConnection *conn, 
					MateVFSCancellation *cancellation)
{
	do_basic_command (conn, "SITE DIRSTYLE", cancellation);

	if (conn->response_message != NULL
	 && strstr(conn->response_message, "is on") != NULL)
	{
		/*
		 * msdos directory mode was already off, but
		 * we just toggled it on.  toggle it back off again
		 */

		do_basic_command(conn, "SITE DIRSTYLE", cancellation);
	}

	/* server should now be in unix directory style */
}

static MateVFSResult 
ftp_connection_create (FtpConnectionPool *pool,
		       FtpConnection **connptr,
		       MateVFSURI *uri,
		       MateVFSContext *context) 
{
	FtpConnection *conn;
	MateVFSResult result;
	gchar *user;
	gchar *pass;
	MateVFSCancellation *cancellation;
	gchar *keyring = NULL;
        gboolean save_authn = FALSE;
	gboolean uri_has_username;
	gboolean got_connection;
	gboolean ret;
	gboolean connection_failed;
	gboolean aborted = FALSE;
	
	cancellation = get_cancellation (context);
	
	conn = g_new0 (FtpConnection, 1);
	conn->pool = pool;
	conn->uri = mate_vfs_uri_dup (uri);
	conn->response_buffer = g_string_new ("");
	conn->response_code = -1;
	conn->fivefifty = MATE_VFS_ERROR_NOT_FOUND;
	
	user = NULL;
	pass = NULL;

	result = try_kerberos (uri, &pool->ip, conn,
			       mate_vfs_uri_get_user_name (uri),
			       &connection_failed,
			       cancellation);
	if (connection_failed) {
		mate_vfs_uri_unref (conn->uri);
		g_string_free (conn->response_buffer, TRUE);
		g_free (conn);
		g_free (user);
		g_free (pass);
		return result;
	} else if (result == MATE_VFS_OK) {
		/* Logged in successfully using kerberos */
	} else if (pool->user != NULL &&
		   pool->password != NULL) {
		result = try_login (uri, &pool->ip,
				    conn, pool->user, pool->password, cancellation);
		if (result != MATE_VFS_OK) {
                	mate_vfs_uri_unref (conn->uri);
                	g_string_free (conn->response_buffer, TRUE);
                	g_free (conn);
			g_free (user);
			g_free (pass);
                	return result;
		}
	} else if (mate_vfs_uri_get_user_name (uri) == NULL ||
		   /* If name set, do this if no password or anon ftp */
		   (strcmp (mate_vfs_uri_get_user_name (uri), "anonymous") != 0 &&
		    mate_vfs_uri_get_password (uri) == NULL)) {
		got_connection = FALSE;
		uri_has_username = FALSE;
		if (mate_vfs_uri_get_user_name (uri) != NULL) {
			user = g_strdup (mate_vfs_uri_get_user_name (uri));
			uri_has_username = TRUE;
		}
		pool->num_connections++;
		G_UNLOCK (connection_pools);
		ret = query_keyring_for_authn_info (uri, &user, &pass);
		G_LOCK (connection_pools);
		pool->num_connections--;
		if (ret) {
                	result = try_login (uri,  &pool->ip, conn,
					    user, pass, cancellation);
			g_free (user);
			g_free (pass);
			user = NULL;
			pass = NULL;
                       	if (result == MATE_VFS_OK) {
				got_connection = TRUE;
			} else if (uri_has_username) {
				user = g_strdup (mate_vfs_uri_get_user_name (uri));
			}
                }
		if (!got_connection) {
			while (TRUE) {
				pool->num_connections++;
				G_UNLOCK (connection_pools);
				ret = query_user_for_authn_info (uri, &user, &pass, &keyring, &save_authn, 
								 &aborted, !uri_has_username); 
				G_LOCK (connection_pools);
				pool->num_connections--;
				if (aborted) {
					mate_vfs_uri_unref (conn->uri);
					g_string_free (conn->response_buffer, TRUE);
					g_free (conn);
					g_free (user);
					g_free (pass);
					g_free (keyring);

					return MATE_VFS_ERROR_CANCELLED;
				}
				g_string_free (conn->response_buffer, TRUE);
                        	conn->response_buffer = g_string_new ("");
				conn->response_code = -1;
				result = try_login (uri,  &pool->ip, conn,
						    user, pass, cancellation);
				if (result == MATE_VFS_OK) {
					break;
				}
				if (result != MATE_VFS_ERROR_LOGIN_FAILED ||
				    !ret /* if callback was not handled, and anonymous
					    login failed, don't run into endless loop */) {
					mate_vfs_uri_unref (conn->uri);
					g_string_free (conn->response_buffer, TRUE);
					g_free (conn);
					g_free (user);
					g_free (pass);
					g_free (keyring);
					return result;
				}
                        }
               	}
       	} else {
               	user = g_strdup (mate_vfs_uri_get_user_name (uri));
               	pass = g_strdup (mate_vfs_uri_get_password (uri));
		if (pass == NULL) {
			pass = g_strdup ("nobody@gnome.org");
		}
		
		result = try_login (uri, &pool->ip, conn,
				    user, pass, cancellation);
		if (result != MATE_VFS_OK) {
                	mate_vfs_uri_unref (conn->uri);
                	g_string_free (conn->response_buffer, TRUE);
                	g_free (conn);
			g_free (user);
			g_free (pass);
                	return result;
		}
        }
	
	/* okay, we should be connected now */
	
        if (save_authn) {
		pool->num_connections++;
		G_UNLOCK (connection_pools);
               	save_authn_info (uri, user, pass, keyring);
		G_LOCK (connection_pools);
		pool->num_connections--;
		g_free (keyring);
        }

	if (pool->user == NULL) {
		pool->user = user;
		pool->password = pass;
	} else {
		g_free (user);
		g_free (pass);
	}

	/* Image mode (binary to the uninitiated) */

	do_basic_command (conn, "TYPE I", cancellation);

	/* Get the system type */

	if (pool->server_type == NULL) {
		do_basic_command (conn, "SYST", cancellation);
		pool->server_type = g_strdup (conn->response_message);

	}

	/* if this is a windows server, toggle it into NT mode */

	if (strncmp (pool->server_type, "Windows_NT", 10) == 0) {
		toggle_winnt_into_unix_mode(conn, cancellation);
	}

	conn->server_type = g_strdup (pool->server_type);

	*connptr = conn;

	ftp_debug (conn, g_strdup ("created"));

	total_connections++;

	pool->num_connections++;
	return MATE_VFS_OK;
}

/* Call with lock held */
static void
ftp_connection_destroy (FtpConnection *conn,
			MateVFSCancellation *cancellation) 
{
	if (conn->pool != NULL)
		conn->pool->num_connections--;
	
	if (conn->socket_buf) 
	        mate_vfs_socket_buffer_destroy (conn->socket_buf, TRUE, cancellation);

	mate_vfs_uri_unref (conn->uri);
	g_free (conn->cwd);

	if (conn->response_buffer) 
	        g_string_free(conn->response_buffer, TRUE);
	g_free (conn->response_message);
	g_free (conn->server_type);

	if (conn->data_socketbuf) 
	        mate_vfs_socket_buffer_destroy (conn->data_socketbuf, TRUE, cancellation);

#ifdef HAVE_GSSAPI
	if (conn->gcontext != GSS_C_NO_CONTEXT) {
		gss_qop_t maj_stat, min_stat;
		gss_buffer_desc output_tok;
		
		maj_stat = gss_delete_sec_context (&min_stat,
						   &conn->gcontext,
						   &output_tok);
		
		if (maj_stat == GSS_S_COMPLETE) {
			gss_release_buffer (&min_stat, &output_tok);
		}
		conn->gcontext = GSS_C_NO_CONTEXT;
	}
#endif
	
	g_free (conn);
	total_connections--;
}

/* g_str_hash and g_str_equal don't take null arguments */

static guint 
my_str_hash (const char *c) 
{
	if (c) 
	        return g_str_hash (c);
	return 0;
}

static gboolean
my_str_equal (const char *c, 
	      const char *d) 
{
	if ((c && !d) || (d &&!c)) 
		return FALSE;
	if (!c && !d) 
		return TRUE;
	return strcmp (c,d) == 0;
}

/* hash the bits of a MateVFSURI that distingush FTP connections */
guint
ftp_connection_uri_hash (gconstpointer c) 
{
	MateVFSURI *uri = (MateVFSURI *) c;

	return my_str_hash (mate_vfs_uri_get_host_name (uri)) + 
		my_str_hash (mate_vfs_uri_get_user_name (uri)) +
		my_str_hash (mate_vfs_uri_get_password (uri)) +
		mate_vfs_uri_get_host_port (uri);
}

/* test the equality of the bits of a MateVFSURI that distingush FTP 
 * connections */
gint 
ftp_connection_uri_equal (gconstpointer c, 
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

static void
ftp_cached_dirlist_free (FtpCachedDirlist *dirlist)
{
	g_free (dirlist->dirlist);
	g_free (dirlist);
}

/* Call with lock held */
static FtpConnectionPool *
ftp_connection_pool_lookup (MateVFSURI *uri)
{
	FtpConnectionPool *pool;
	pool = g_hash_table_lookup (connection_pools, uri);
	
	if (pool == NULL) {
		pool = g_new0 (FtpConnectionPool, 1);

		pool->cached_dirlists = g_hash_table_new_full (g_str_hash, 
							       g_str_equal,
							       g_free,
							       (GDestroyNotify)ftp_cached_dirlist_free);

		
		g_hash_table_insert (connection_pools, mate_vfs_uri_dup (uri), pool);
	}
	return pool;
}

static void
invalidate_dirlist_cache (MateVFSURI *uri)
{
	FtpConnectionPool *pool;
	
	G_LOCK (connection_pools);
	pool = ftp_connection_pool_lookup (uri);
	g_hash_table_remove (pool->cached_dirlists,
			     uri->text != NULL ? uri->text : "/");
	G_UNLOCK (connection_pools);
}

static void
invalidate_parent_dirlist_cache (MateVFSURI *uri)
{
	MateVFSURI *parent;

	parent = mate_vfs_uri_get_parent (uri);
	invalidate_dirlist_cache (parent);
	mate_vfs_uri_unref (parent);
}

static MateVFSResult 
ftp_connection_acquire (MateVFSURI *uri,
			FtpConnection **connection,
			MateVFSContext *context) 
{
	FtpConnection *conn = NULL;
	MateVFSResult result = MATE_VFS_OK;
	MateVFSCancellation *cancellation;
	FtpConnectionPool *pool;
	struct timeval tv;
	
	cancellation = get_cancellation (context);
	G_LOCK (connection_pools);

	pool = ftp_connection_pool_lookup (uri);
	
	if (pool->spare_connections) {
		/* spare connection(s) found */
		conn = (FtpConnection *) pool->spare_connections->data;
		
		/* update the uri as it may be different */
		if (conn->uri) {
			mate_vfs_uri_unref (conn->uri);
		}
		conn->uri = mate_vfs_uri_dup (uri);
		pool->spare_connections = g_list_remove (pool->spare_connections, 
							 conn);

		/* Reset offset */
                conn->offset = 0;

		/* make sure connection hasn't timed out */
		result = do_basic_command (conn, "PWD", cancellation);
		if (result != MATE_VFS_OK) {
			ftp_connection_destroy (conn, cancellation);
			result = ftp_connection_create (pool, &conn, uri, context);
		}
	} else {
		result = ftp_connection_create (pool, &conn, uri, context);
	}

	gettimeofday (&tv, NULL);
	pool->last_use = tv.tv_sec;
	
	G_UNLOCK (connection_pools);

	*connection = conn;

	if (result == MATE_VFS_OK) {
		allocated_connections++;
	}

	return result;
}

static void
ftp_connection_pool_free (FtpConnectionPool *pool)
{
	g_assert (pool->num_connections == 0);
	g_assert (pool->num_monitors == 0);
	g_assert (pool->spare_connections == NULL);
	g_free (pool->ip);
	g_free (pool->user);
	g_free (pool->password);
	g_free (pool->server_type);
	g_hash_table_destroy (pool->cached_dirlists);
	g_free (pool);
}

/* Call with lock held */
static gboolean
ftp_connection_pool_reap (gpointer  key,
			  gpointer  value,
			  gpointer  user_data)
{
	FtpConnectionPool *pool;
	gboolean *continue_timeout;
	struct timeval tv;
	GList *l;

	pool = (FtpConnectionPool *)value;
	continue_timeout = (gboolean *)user_data;
	
	/* Never reap spare connections if used recently */
	
	gettimeofday (&tv, NULL);
	if (tv.tv_sec >= pool->last_use &&
	    tv.tv_sec <= pool->last_use + CONNECTION_CACHE_MIN_LIFETIME) {
		if (pool->spare_connections != NULL) {
			*continue_timeout = TRUE;
		}

		if (pool->num_connections == 0 &&
		    pool->num_monitors <= 0) {
			*continue_timeout = TRUE;
		}
		
		return FALSE;
	}
	
	for (l = pool->spare_connections; l != NULL; l = l->next) {
		ftp_connection_destroy (l->data, NULL);
	}
	g_list_free (pool->spare_connections);
	pool->spare_connections = NULL;

	if (pool->num_connections == 0 &&
	    pool->num_monitors <= 0) {
		mate_vfs_uri_unref (key);
		ftp_connection_pool_free (pool);
		return TRUE;
	}

	return FALSE;
}


static gboolean
ftp_connection_pools_reap (gpointer data)
{
	gboolean continue_timeout;

	G_LOCK (connection_pools);

	continue_timeout = FALSE;
	g_hash_table_foreach_remove (connection_pools,
				     ftp_connection_pool_reap,
				     &continue_timeout);

	
	if (!continue_timeout)
		connection_pool_timeout = 0;
	
	G_UNLOCK (connection_pools);
	
	return continue_timeout;
}


static void 
ftp_connection_release (FtpConnection *conn,
			gboolean error_release) 
{
	FtpConnectionPool *pool;
	
	g_return_if_fail (conn);
	
	/* reset the 550 result */
	conn->fivefifty = MATE_VFS_ERROR_NOT_FOUND;

	G_LOCK (connection_pools);
	pool = conn->pool;

	if (error_release) {
		ftp_connection_destroy (conn, NULL);
	} else {
		pool->spare_connections = g_list_prepend (pool->spare_connections, 
							  conn);
	}

	allocated_connections--;

	if (connection_pool_timeout == 0) {
		connection_pool_timeout = g_timeout_add (REAP_TIMEOUT, ftp_connection_pools_reap, NULL);
	}
	
	G_UNLOCK(connection_pools);
}

gboolean 
do_is_local (MateVFSMethod *method, 
	     const MateVFSURI *uri)
{
	return FALSE;
}


static MateVFSResult 
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context) 
{
	MateVFSResult result;
	FtpConnection *conn;

	if ((mode & MATE_VFS_OPEN_READ) && (mode & MATE_VFS_OPEN_WRITE)) {
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	if (!(mode & MATE_VFS_OPEN_READ) && !(mode & MATE_VFS_OPEN_WRITE)) {
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	result = ftp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK) 
		return result;

	if (mode & MATE_VFS_OPEN_READ) {
		conn->operation = FTP_READ;
		result = do_path_transfer_command (conn, "RETR", uri, context);
	} else if (mode & MATE_VFS_OPEN_WRITE) {

		invalidate_parent_dirlist_cache (uri);
		
		conn->operation = FTP_WRITE;
		conn->fivefifty = MATE_VFS_ERROR_ACCESS_DENIED;
		result = do_path_transfer_command (conn, "STOR", uri, context);
		conn->fivefifty = MATE_VFS_ERROR_NOT_FOUND;
	}
	if (result == MATE_VFS_OK) {
		*method_handle = (MateVFSMethodHandle *) conn;
	} else {
		*method_handle = NULL;
		ftp_connection_release (conn, TRUE);
	}
	return result;
}

static MateVFSResult
do_create (MateVFSMethod *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI *uri,
	   MateVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   MateVFSContext *context)
{
	MateVFSResult result;
	FtpConnection *conn;
	gchar *chmod_command;

	if ((mode & MATE_VFS_OPEN_READ) && (mode & MATE_VFS_OPEN_WRITE)) {
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	if (!(mode & MATE_VFS_OPEN_READ) && !(mode & MATE_VFS_OPEN_WRITE)) {
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	result = ftp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK)
		return result;

	/* Exclusive set: we have to fail if the file exists */
	if (exclusive) {
		conn->operation = FTP_READ;
		result = do_path_transfer_command (conn, "RETR", uri, context);

		/* File exists or an error occurred */
		if (result != MATE_VFS_ERROR_NOT_FOUND) {
			ftp_connection_release (conn, TRUE);

			/* Return the error if there is one, otherwise the file exists */
			return (result == MATE_VFS_OK) ? MATE_VFS_ERROR_FILE_EXISTS
		                                        : result;
		}
	}

	result = do_open(method, method_handle, uri, mode, context);
	if (result != MATE_VFS_OK) {
		ftp_connection_release (conn, TRUE);
		return result;
	}

	/* This is a not-standard command and FTP server may not support it,
	 * so eventual errors will be ignored */
	chmod_command = g_strdup_printf("SITE CHMOD %o", perm);
	do_path_command (conn, chmod_command, uri, get_cancellation (context));

	g_free (chmod_command);
	ftp_connection_release (conn, TRUE);

	return result;
}

static MateVFSResult 
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context) 
{
	FtpConnection *conn = (FtpConnection *) method_handle;
	MateVFSResult result;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);
	result = end_transfer (conn, cancellation);
	ftp_connection_release (conn, result != MATE_VFS_OK);

	return result;
}

static MateVFSResult 
do_read (MateVFSMethod *method, 
	 MateVFSMethodHandle *method_handle, 
	 gpointer buffer,
	 MateVFSFileSize num_bytes, 
	 MateVFSFileSize *bytes_read, 
	 MateVFSContext *context) 
{
	FtpConnection *conn = (FtpConnection * )method_handle;
	MateVFSResult result;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);
	result = mate_vfs_socket_buffer_read (conn->data_socketbuf, buffer, num_bytes, bytes_read, cancellation);

 	if (*bytes_read == 0) {
 		result = MATE_VFS_ERROR_EOF;
 	}

	if (result == MATE_VFS_OK) {
		conn->offset += *bytes_read;
	}

 	return result;
}

static MateVFSResult 
do_write (MateVFSMethod *method, 
	  MateVFSMethodHandle *method_handle, 
	  gconstpointer buffer, 
	  MateVFSFileSize num_bytes, 
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context) 
{
	FtpConnection *conn = (FtpConnection *) method_handle;
	MateVFSResult result;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);

	if (conn->operation != FTP_WRITE) 
		return MATE_VFS_ERROR_NOT_PERMITTED;
	
	result = mate_vfs_socket_buffer_write (conn->data_socketbuf, buffer, 
						num_bytes, 
						bytes_written, cancellation);

	if (result == MATE_VFS_OK) {
		conn->offset += *bytes_written;
	}

	return result;
}

static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FtpConnection *conn = (FtpConnection * )method_handle;
	MateVFSResult result;
	MateVFSFileOffset real_offset;
	MateVFSFileOffset orig_offset;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);
	
	/* Calculate real offset */
	switch (whence) {
		case MATE_VFS_SEEK_START:
			real_offset = offset;
			break;
		case MATE_VFS_SEEK_CURRENT:
			real_offset = conn->offset + offset;
			break;
		case MATE_VFS_SEEK_END:
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		default:
			return MATE_VFS_ERROR_GENERIC;
	}

	/* We need to stop the data transfer first */
	result = end_transfer (conn, cancellation);

	/* Do not check for result != MATE_VFS_OK because we know the error:
	 * 426 Failure writing network stream.
	 */

	/* Save the original offset */
	orig_offset = conn->offset;

	/* Try to restart the transfer now with the new offset */
	conn->offset = real_offset;
	switch (conn->operation) {
		case FTP_READ:
			result = do_path_transfer_command (conn, "RETR", conn->uri, context);
			break;
		case FTP_WRITE:
			result = do_path_transfer_command (conn, "STOR", conn->uri, context);
			break;
		default:
			return MATE_VFS_ERROR_GENERIC;
	}

	/* Restore the original offset if there is any error */
	if (result != MATE_VFS_OK)
		conn->offset = orig_offset;

	return result;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FtpConnection *conn = (FtpConnection * )method_handle;

	*offset_return = conn->offset;

	return MATE_VFS_OK;
}

/* parse one directory listing from the string pointed to by ls. Parse
 * only one line from that string. Fill in the appropriate fields of file_info.
 * return TRUE if a directory entry was found, FALSE otherwise
 */

/**
 * return TRUE if entry found, FALSE otherwise
 */
static gboolean 
netware_ls_to_file_info (gchar *ls, MateVFSFileInfo *file_info, 
			 MateVFSFileInfoOptions options) 
{
	const char *mime_type;

	/* check parameters */
	g_return_val_if_fail (file_info != NULL, FALSE);

	/* start by knowing nothing */
	file_info->valid_fields = 0;

	/* If line starts with "total" then we should skip it */
	if (strncmp (ls, "total", 5) == 0) {
		return FALSE;
	}

	/* First char is 'd' for directory, '-' for regular file */
	file_info->type = MATE_VFS_FILE_TYPE_UNKNOWN;
	if (strlen (ls) >= 1) {
		if (ls[0] == 'd') {
			file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		} else if (ls[0] == '-') {
			file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		} else {
			g_warning ("netware_ls_to_file_info: unknown file type '%c'", ls[0]);
		}
	}
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;
    
	/* Next is a listing of Netware permissions */
	/* ignored */

	/* Following the permissions is the "owner/creator" of the file */
	/* file info structure requires a UID, which of course is not available */
	/* ignored */

	/* following type, permissions, and owner is the size, right justified up
	 * to but not including column 50. */
	/* if we start at column 35, that allows 15 chars for size--that should be
	 * enough :) */
	if (strlen (ls) > 35) {
		file_info->size = strtol (ls + 35, NULL, 0);
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
	}

	/* columns 51-63 contain modification date of file/directory */
	file_info->mtime = 0;
	if (strlen (ls) >= 51) {
		char *mtime_str = g_strndup (ls + 51, 12);
		GDate *mtime_date;

		/* mtime_str is one of two formats...
		 *  1)  "mmm dd hh:mm"        (24hr time)
		 *  2)  "mmm dd  yyyy"
		 */
		mtime_date = g_date_new ();
		if (index (mtime_str, ':') != NULL) {
			/* separate time */
			char *date_str = g_strndup (mtime_str, 6);
			g_date_set_parse (mtime_date, date_str);
			g_free (date_str);
		} else {
			g_date_set_parse (mtime_date, mtime_str);
		}

		if (!g_date_valid (mtime_date)) {
			g_warning ("netware_ls_to_file_info: cannot parse date '%s'",
			           mtime_str);
		}
		else {
			struct tm mtime_parts;
			g_date_to_struct_tm (mtime_date, &mtime_parts);
			mtime_parts.tm_hour = 0;
			mtime_parts.tm_min = 0;
			mtime_parts.tm_sec = 0;
			mtime_parts.tm_isdst = -1;
			if (index (mtime_str, ':')) {
				/* get the time */
				int h, mn;
				if (sscanf (mtime_str + 7, "%2d:%2d", &h, &mn) == 2) {
					mtime_parts.tm_hour = h;
					mtime_parts.tm_min = mn;
				} else {
					g_warning ("netware_ls_to_file_info: invalid time '%s'",
					           mtime_str + 7);
				}
			}
			file_info->mtime = mktime (&mtime_parts);
			file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MTIME;
		}

		g_date_free (mtime_date);
		g_free (mtime_str);
	}

	/* just in case client doesn't check valid_fields */
	file_info->atime = file_info->mtime;
	file_info->ctime = file_info->mtime;

	/* finally, the file/directory name (column 64) */
	if (strlen (ls) >= 64) {
		int i;
		i = strcspn (ls + 64, "\r\n");
		file_info->name = g_strndup (ls + 64, i);
	} else {
		file_info->name = NULL;
	}

	/* mime type */
	if (file_info->type == MATE_VFS_FILE_TYPE_REGULAR) {
		mime_type = mate_vfs_mime_type_from_name_or_default (
		                 file_info->name,
		                 MATE_VFS_MIME_TYPE_UNKNOWN);
	} else {
		mime_type = mate_vfs_mime_type_from_mode (S_IFDIR);
	}
	file_info->mime_type = g_strdup (mime_type);
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;

	/* fill in other structures with meaningful data, even though
	 * it may not be valid */
	file_info->permissions = MATE_VFS_PERM_USER_ALL |
	                         MATE_VFS_PERM_GROUP_ALL |
	                         MATE_VFS_PERM_OTHER_ALL;
	file_info->flags = MATE_VFS_FILE_FLAGS_NONE;

	return TRUE;
}

static gboolean 
unix_ls_to_file_info (gchar *ls, MateVFSFileInfo *file_info, 
		      MateVFSFileInfoOptions options) 
{
	struct stat s;
	gchar *filename = NULL, *linkname = NULL;
	const char *mime_type;

	mate_vfs_parse_ls_lga (ls, &s, &filename, &linkname);

	if (filename) {

		mate_vfs_stat_to_file_info (file_info, &s);
		
		/* FIXME: This is a hack, but we can't change
		   the above API until after Mate 1.4.  Ideally, we
		   would give the stat_to_file_info function this
		   information.  Also, there may be more fields here that are not 
		   valid that we haven't dealt with.  */
		file_info->valid_fields &= ~(MATE_VFS_FILE_INFO_FIELDS_DEVICE
					     | MATE_VFS_FILE_INFO_FIELDS_INODE
					     | MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE);

		/* We want large reads on FTP */
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
		file_info->io_block_size = 32*1024;

		file_info->name = g_path_get_basename (filename);

		if(*(file_info->name) == '\0') {
			g_free (file_info->name);
			file_info->name = g_strdup ("/");
		}

		if(linkname) {
			file_info->symlink_name = linkname;
			file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME | MATE_VFS_FILE_INFO_FIELDS_FLAGS;
			file_info->flags |= MATE_VFS_FILE_FLAGS_SYMLINK;
		}

		if (file_info->type == MATE_VFS_FILE_TYPE_REGULAR) {
			mime_type = mate_vfs_mime_type_from_name_or_default (file_info->name, MATE_VFS_MIME_TYPE_UNKNOWN);
		} else {
			mime_type = mate_vfs_mime_type_from_mode_or_default (s.st_mode, MATE_VFS_MIME_TYPE_UNKNOWN);
		}
		/*ftp_debug (conn, g_strdup_printf ("mimetype = %s", mime_type));*/
		file_info->mime_type = g_strdup (mime_type);
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;

		/*ftp_debug (conn, g_strdup_printf ("info got name `%s'", file_info->name));*/

		g_free (filename);

		return TRUE;
	} else {
		return FALSE;
	}
}


#if 0
static MateVFSResult
internal_get_file_info (MateVFSMethod *method,
			MateVFSURI *uri,
			MateVFSFileInfo *file_info,
			MateVFSFileInfoOptions options,
			MateVFSContext *context) 
{
	FtpConnection *conn;
	/* FIXME bugzilla.eazel.com 1463 */
	MateVFSResult result;
	MateVFSFileSize num_bytes = 1024, bytes_read;
	gchar buffer[num_bytes+1];

	result = ftp_connection_acquire(uri, &conn);
	if (result != MATE_VFS_OK) {
		return result;
	}

	if(strstr(conn->server_type,"MACOS")) {
		/* don't ask for symlinks from MacOS servers */
		do_path_transfer_command (conn, "LIST -ld", uri, context);
	} else {
		do_path_transfer_command (conn, "LIST -ldL", uri, context);
	}

	result = mate_vfs_socket_buffer_read (conn->data_socketbuf, buffer, 
					       num_bytes, &bytes_read);

	if (result != MATE_VFS_OK) {
		/*ftp_debug (conn, g_strdup ("mate_vfs_socket_buffer_read failed"));*/
		ftp_connection_release (conn, TRUE);
		return result;
	}

	result = end_transfer (conn);

	/* FIXME bugzilla.eazel.com 2793: check return? */

	ftp_connection_release (conn, TRUE);

	if (result != MATE_VFS_OK) {
		/*ftp_debug (conn,g_strdup ("LIST for get_file_info failed."));*/
		return result;
	}

	if (bytes_read>0) {

		buffer[bytes_read] = '\0';
		file_info->valid_fields = 0; /* make sure valid_fields is 0 */

		if (ls_to_file_info (buffer, file_info)) {
			return MATE_VFS_OK;
		}

	}
	
	return MATE_VFS_ERROR_NOT_FOUND;

}
#endif

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context) 
{
	MateVFSURI *parent = mate_vfs_uri_get_parent (uri);
	MateVFSResult result;

	if (parent == NULL) {
		FtpConnectionPool *pool;
		FtpConnection *conn;
		gboolean was_cached;
		
		/* this is a request for info about the root directory */

		G_LOCK (connection_pools);
		pool = ftp_connection_pool_lookup (uri);
		was_cached = pool->server_type != NULL;
		G_UNLOCK (connection_pools);

		if (was_cached) {
			result = MATE_VFS_OK;
		} else {
			/* is the host there? */
			result = ftp_connection_acquire (uri, &conn, context);
			
			if (result != MATE_VFS_OK) {
				/* doesn't look like it */
				return result;
			}
			
			ftp_connection_release (conn, FALSE);
		}

		file_info->name = g_strdup ("/");
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	} else {
		MateVFSMethodHandle *method_handle;
		gchar *name;


	       	name = mate_vfs_uri_extract_short_name (uri);
		if (name == NULL) {
			mate_vfs_uri_unref (parent);
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		}

		result = do_open_directory (method, &method_handle, parent,
					    options, context);

		mate_vfs_uri_unref (parent);

		if (result != MATE_VFS_OK) {
			g_free (name);
			return result;
		}

		while (1) {
			mate_vfs_file_info_clear (file_info);
			result = do_read_directory (method, method_handle, 
						    file_info, context);
			if (result != MATE_VFS_OK) {
				result = MATE_VFS_ERROR_NOT_FOUND;
				break;
			}
			if (file_info->name != NULL
			    && strcmp (file_info->name, name) == 0) {
				break;
			}
		}
		g_free (name);
		do_close_directory (method, method_handle, context);

		/* maybe it was a hidden directory not included in ls output
		 * (IIS virtual directory), so try to CWD to it. */
		if (result == MATE_VFS_ERROR_NOT_FOUND) {
			FtpConnection *connection;

			if (ftp_connection_acquire (uri, &connection, context) == MATE_VFS_OK) {
				result = do_path_command (connection, "CWD", uri, get_cancellation (context));
				ftp_connection_release (connection, FALSE);
			}

			if (result == MATE_VFS_OK) {
				char *unescaped;
				char *basename;

				unescaped = mate_vfs_unescape_string (uri->text, G_DIR_SEPARATOR_S);
				basename = g_path_get_basename (unescaped);
				g_free (unescaped);

				if (basename != NULL) {
					file_info->name = basename;
					file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
					file_info->mime_type = g_strdup ("x-directory/normal");
					file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
						MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
				} else {
					result = MATE_VFS_ERROR_NOT_FOUND;
				}
			} else {
				result = MATE_VFS_ERROR_NOT_FOUND;
			}
		}
	}

	return result;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	return do_get_file_info (method,
				 ((FtpConnection *)method_handle)->uri, 
				 file_info, options, context);
}

/* Tries out all suitable LIST parameter combinations until one succeeds.
 * If there is an unexpected error, it is returned and has to be handled
 * by the caller. Note that a successful LIST command also means that the
 * list is actually requested, which also has to be handled by the caller.
 * */
static MateVFSResult
get_list_command (FtpConnection   *conn,
		  MateVFSContext *context)
{
	MateVFSResult result;

	static const char * const osx_candidates[] = { "LIST -a", NULL };
	static const char * const non_osx_candidates[] = { "LIST -aL", "LIST -a", "LIST -L", NULL };

	const char * const *candidates;

	if (strstr(conn->server_type, "MACOS")) {
		candidates = osx_candidates;
	} else {
		candidates = non_osx_candidates;
	}

	do {
		result = do_transfer_command (conn, (char *) *candidates, context);
	}
	while (*++candidates != NULL &&
	       result == MATE_VFS_ERROR_BAD_PARAMETERS);

	if (result == MATE_VFS_OK) {
		/* we found the best suitable candidate for this server. */
		g_assert (candidates != NULL);

		conn->list_cmd = *candidates;
	} else {
		/* all commands failed, or an unexpected error occured,
		 * just use "LIST". */

		result = do_transfer_command (conn, "LIST", context);
		conn->list_cmd = "LIST";
	}

	return result;
}

static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
#define BUFFER_SIZE 1024
	FtpConnection *conn;
	MateVFSResult result;
	MateVFSFileSize num_bytes = BUFFER_SIZE, bytes_read;
	gchar buffer[BUFFER_SIZE+1];
	GString *dirlist = g_string_new ("");
	char *dirlist_str;
	MateVFSCancellation *cancellation;
	FtpDirHandle *handle;
	char *server_type;
	FtpConnectionPool *pool;
	FtpCachedDirlist *cached_dirlist;
	struct timeval tv;

	dirlist_str = NULL;
	server_type = NULL;
	cancellation = get_cancellation (context);

	
	G_LOCK (connection_pools);
	pool = ftp_connection_pool_lookup (uri);
	cached_dirlist = g_hash_table_lookup (pool->cached_dirlists,
					      uri->text != NULL ? uri->text : "/");
	if (cached_dirlist != NULL) {
		gettimeofday (&tv, NULL);

		if (tv.tv_sec >= cached_dirlist->read_time &&
		    tv.tv_sec <= (cached_dirlist->read_time + DIRLIST_CACHE_TIMEOUT)) {
			dirlist_str = g_strdup (cached_dirlist->dirlist);
			server_type = g_strdup (pool->server_type);
		}
	}
	G_UNLOCK (connection_pools);
	if (dirlist_str != NULL) {
		result = MATE_VFS_OK;
		goto has_cache;
	}
	
	result = ftp_connection_acquire (uri, &conn, context);
	if (result != MATE_VFS_OK) {
		g_string_free (dirlist, TRUE);
		return result;
	}

	/* LIST does not return an error if called on a file, but CWD
	 * should. This allows us to have proper mate-vfs semantics.
	 * does the cwd break other things though?  ie, are
	 * connections stateless?
	 */
	conn->fivefifty = MATE_VFS_ERROR_NOT_A_DIRECTORY;
	result = do_path_command (conn, "CWD", uri, cancellation);
	if (result != MATE_VFS_OK) {
		ftp_connection_release (conn, TRUE);
		g_string_free (dirlist, TRUE);
		return result;
	}

	if (conn->list_cmd != NULL) {
		result = do_transfer_command (conn, (char *) conn->list_cmd, context);
	} else {
		result = get_list_command (conn, context);
	}

	if (result != MATE_VFS_OK) {
		ftp_connection_release (conn, TRUE);
		g_string_free (dirlist, TRUE);
		return result;
	}

	while (result == MATE_VFS_OK) {
		result = mate_vfs_socket_buffer_read (conn->data_socketbuf, buffer, 
						       num_bytes, &bytes_read,
						       cancellation);
		if (result == MATE_VFS_OK && bytes_read > 0) {
			buffer[bytes_read] = '\0';
			dirlist = g_string_append (dirlist, buffer);
		} else {
			break;
		}
	} 

	result = end_transfer (conn, cancellation);

	dirlist_str = g_string_free (dirlist, FALSE);
	server_type = g_strdup (conn->server_type);
	ftp_connection_release (conn, FALSE);

	if (result != MATE_VFS_OK) {
		g_free (server_type);
		g_free (dirlist_str);
		return result;
	}
	
	G_LOCK (connection_pools);
	
	pool = ftp_connection_pool_lookup (uri);

	cached_dirlist = g_new0 (FtpCachedDirlist, 1);
	cached_dirlist->dirlist = g_strdup (dirlist_str);
	gettimeofday (&tv, NULL);
	cached_dirlist->read_time = tv.tv_sec;
	
	g_hash_table_replace (pool->cached_dirlists,
			      g_strdup (uri->text != NULL ? uri->text : "/"),
			      cached_dirlist);
	
	G_UNLOCK (connection_pools);

	
 has_cache:
	
	handle = g_new0 (FtpDirHandle, 1);
	
	handle->dirlist = dirlist_str;
	handle->dirlistptr = handle->dirlist;
	handle->file_info_options = options;
	handle->uri = mate_vfs_uri_dup (uri);
	handle->server_type = server_type;
	
	*method_handle = (MateVFSMethodHandle *)handle;

	return result;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context) 
{
	FtpDirHandle *handle = (FtpDirHandle *) method_handle;

	mate_vfs_uri_unref (handle->uri);
	g_free (handle->dirlist);
	g_free (handle->server_type);
	g_free (handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	FtpDirHandle *handle = (FtpDirHandle *) method_handle;


	if (!handle->dirlistptr || *(handle->dirlistptr) == '\0') {
		return MATE_VFS_ERROR_EOF;
	}

	while (TRUE) {
		gboolean success;
                
		if (strncmp (handle->server_type, "NETWARE", 7) == 0) {
			success = netware_ls_to_file_info (handle->dirlistptr, file_info,
			                                   handle->file_info_options);
		}
		else {
			success = unix_ls_to_file_info (handle->dirlistptr, file_info,
			                                handle->file_info_options);
		}

		/* permissions aren't valid */
		file_info->valid_fields &= ~MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;

		if (handle->file_info_options & MATE_VFS_FILE_INFO_FOLLOW_LINKS &&
		    file_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
			MateVFSURI *uri;
			MateVFSFileInfo *symlink_info;
			MateVFSResult res;
			int n_symlinks;

			uri = mate_vfs_uri_append_file_name (handle->uri, file_info->name);
			symlink_info = mate_vfs_file_info_dup (file_info);
			n_symlinks = 0;
			
			do {
				MateVFSURI *link_uri;
				char *symlink_name;

				if (n_symlinks > 8) {
					res = MATE_VFS_ERROR_TOO_MANY_LINKS;
					break;
				}

				if (symlink_info->symlink_name == NULL) {
					res = MATE_VFS_ERROR_BAD_PARAMETERS;
					break;
				}

				symlink_name = mate_vfs_escape_path_string (symlink_info->symlink_name);
				mate_vfs_file_info_clear (symlink_info);
				
				link_uri = mate_vfs_uri_resolve_relative (uri, symlink_name);
				g_free (symlink_name);

				if (strcmp (mate_vfs_uri_get_host_name (uri),
					    mate_vfs_uri_get_host_name (link_uri)) != 0) {
					/* Links must be in the same host */
					res = MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
					break;
				}
				
				res = do_get_file_info (method,
							link_uri,
							symlink_info,
							handle->file_info_options & ~MATE_VFS_FILE_INFO_FOLLOW_LINKS,
							context);

				mate_vfs_uri_unref (uri);
				uri = link_uri;
				
				if (res != MATE_VFS_OK) {
					break;
				}
				n_symlinks++;
			} while (symlink_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK);

			if (res == MATE_VFS_OK) {
				char *real_name;
				
				real_name = g_strdup (file_info->name);
				
				mate_vfs_file_info_clear (file_info);
				mate_vfs_file_info_copy (file_info, symlink_info);

				MATE_VFS_FILE_INFO_SET_SYMLINK (file_info, TRUE);
				file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME | MATE_VFS_FILE_INFO_FIELDS_FLAGS;
				file_info->symlink_name = mate_vfs_unescape_string (uri->text != NULL ? uri->text : "/", "/");
				
				g_free (file_info->name);
				file_info->name = real_name;
			}
			mate_vfs_uri_unref (uri);
			mate_vfs_file_info_unref (symlink_info);
				
		}

		if (*(handle->dirlistptr) == '\0') {
			return MATE_VFS_ERROR_EOF;
		}

		/* go till we find \r\n */
		while (handle->dirlistptr &&
		       *handle->dirlistptr && 
		       *handle->dirlistptr != '\r' && 
		       *handle->dirlistptr != '\n') {
			handle->dirlistptr++;
		}
		/* go past \r\n */
		while (handle->dirlistptr && g_ascii_isspace (*handle->dirlistptr)) {
			handle->dirlistptr++;
		}

		if (success)
			break;
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
		  MateVFSURI *a,
		  MateVFSURI *b,
		  gboolean *same_fs_return,
		  MateVFSContext *context)
{
	*same_fs_return = ftp_connection_uri_equal (a,b);
	return MATE_VFS_OK;
}

static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	MateVFSResult result;
	gchar *chmod_command;

	result = do_path_command_completely ("CWD", uri, context,
			/* If the directory doesn't exist @result will be !MATE_VFS_OK,
			* in this case we don't return and proceed with MKD */
			!MATE_VFS_OK);

	if (result == MATE_VFS_OK) {
		return MATE_VFS_ERROR_FILE_EXISTS;
	}

	result = do_path_command_completely ("MKD", uri, context, 
		MATE_VFS_ERROR_ACCESS_DENIED);

	if (result == MATE_VFS_OK) {
		invalidate_parent_dirlist_cache (uri);
		/* try to set the permissions */
		/* this is a non-standard extension, so we'll just do our
		 * best. We can ignore error codes. */
		chmod_command = g_strdup_printf("SITE CHMOD %o", perm);
		do_path_command_completely (chmod_command, uri, context,
			MATE_VFS_ERROR_ACCESS_DENIED);
		g_free(chmod_command);
	} else if (result != MATE_VFS_ERROR_CANCELLED &&
		   mate_vfs_uri_exists (uri)) {
		result = MATE_VFS_ERROR_FILE_EXISTS;
	}

	return result;
}


static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	invalidate_parent_dirlist_cache (uri);
	return do_path_command_completely ("RMD", uri, context, 
		MATE_VFS_ERROR_ACCESS_DENIED);
}


static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	MateVFSResult result;
	MateVFSFileInfo *p_file_info;
	MateVFSCancellation *cancellation;
	
	cancellation = get_cancellation (context);

	if (!force_replace) {
		p_file_info = mate_vfs_file_info_new ();
		result = do_get_file_info (method, new_uri, p_file_info, MATE_VFS_FILE_INFO_DEFAULT, context);
		mate_vfs_file_info_unref (p_file_info);
		p_file_info = NULL;

		if (result == MATE_VFS_OK) {
			return MATE_VFS_ERROR_FILE_EXISTS;
		}
	}
	

	if (ftp_connection_uri_equal (old_uri, new_uri)) {
		FtpConnection *conn;
		MateVFSResult result;

		result = ftp_connection_acquire (old_uri, &conn, context);
		if (result != MATE_VFS_OK) {
			return result;
		}
		result = do_path_command (conn, "RNFR", old_uri, cancellation);
		
		if (result == MATE_VFS_OK) {
			conn->fivefifty = MATE_VFS_ERROR_ACCESS_DENIED;
			result = do_path_command (conn, "RNTO", new_uri, cancellation);
			conn->fivefifty = MATE_VFS_ERROR_NOT_FOUND;
		}

		ftp_connection_release (conn, result != MATE_VFS_OK);

		invalidate_parent_dirlist_cache (old_uri);
		invalidate_parent_dirlist_cache (new_uri);
		
		return result;
	} else {
		return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	}
}

static MateVFSResult
do_unlink (MateVFSMethod *method, MateVFSURI *uri, MateVFSContext *context)
{
	invalidate_parent_dirlist_cache (uri);
	return do_path_command_completely ("DELE", uri, context,
		MATE_VFS_ERROR_ACCESS_DENIED);
}

static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{
	MateVFSURI *parent_uri, *new_uri;
	MateVFSResult result;

	/* FIXME: For now, we only support changing the name. */
	if ((mask & ~(MATE_VFS_SET_FILE_INFO_NAME)) != 0) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	/* FIXME bugzilla.eazel.com 645: Make sure this returns an
	 * error for incoming names with "/" characters in them,
	 * instead of moving the file.
	 */

	/* Share code with do_move. */
	parent_uri = mate_vfs_uri_get_parent (uri);
	if (parent_uri == NULL) {
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	new_uri = mate_vfs_uri_append_file_name (parent_uri, info->name);
	mate_vfs_uri_unref (parent_uri);
	result = do_move (method, uri, new_uri, FALSE, context);
	mate_vfs_uri_unref (new_uri);
	return result;
}


static MateVFSResult
do_monitor_add (MateVFSMethod *method,
		MateVFSMethodHandle **method_handle_return,
		MateVFSURI *uri,
		MateVFSMonitorType monitor_type)
{
	FtpConnectionPool *pool;
	
	if (monitor_type == MATE_VFS_MONITOR_DIRECTORY) {
		G_LOCK (connection_pools);
		pool = ftp_connection_pool_lookup (uri);
		pool->num_monitors++;
		*method_handle_return = (MateVFSMethodHandle *)pool;
		G_UNLOCK (connection_pools);
		return MATE_VFS_OK;
	}
	
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle)
{
	FtpConnectionPool *pool;

	pool = (FtpConnectionPool *)method_handle;
	
	G_LOCK (connection_pools);
	pool->num_monitors--;

	if (connection_pool_timeout == 0) {
		connection_pool_timeout = g_timeout_add (REAP_TIMEOUT, ftp_connection_pools_reap, NULL);
	}
	G_UNLOCK (connection_pools);
	return MATE_VFS_OK;
}

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	NULL, /* truncate */
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
	do_set_file_info,
	NULL, /* truncate */
	NULL, /* find_directory */
	NULL, /* create_symbolic_link */
	do_monitor_add,
	do_monitor_cancel,
	NULL /* do_file_control */
};

MateVFSMethod *
vfs_module_init (const char *method_name, 
		 const char *args)
{
	MateConfClient *gclient;

	connection_pools = g_hash_table_new (ftp_connection_uri_hash, 
					     ftp_connection_uri_equal);

	gclient = mateconf_client_get_default ();
	if (gclient) {
		if (mateconf_client_get_bool (gclient, USE_PROXY_KEY, NULL)) {
			/* Using FTP proxy */
			proxy_host = mateconf_client_get_string (gclient,
							      PROXY_FTP_HOST_KEY,
							      NULL);
			/* Don't use blank hostname */
			if (proxy_host &&
			    *proxy_host == 0) {
				g_free (proxy_host);
				proxy_host = NULL;
			}
			proxy_port = mateconf_client_get_int (gclient,
							   PROXY_FTP_PORT_KEY,
							   NULL);
		} else proxy_host = NULL;
	}

	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	if (proxy_host) {
		g_free (proxy_host);
	}
}
