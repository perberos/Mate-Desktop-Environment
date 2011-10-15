/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* http-neon-method.c - The HTTP method implementation for the MATE Virtual 
   File System using the neon http/webdav library.

   Copyright (C) 2004 Christian Kellner <gicmo@gnome.org>
   
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

*/

#include <config.h>

#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-mime-sniff-buffer.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>
#include <libmatevfs/mate-vfs-module-callback-module-api.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-ssl.h>
#include <libmatevfs/mate-vfs-private-utils.h>

#include <glib.h>

/* neon header files */
#include <ne_session.h>
#include <ne_request.h>
#include <ne_auth.h>
#include <ne_basic.h>
#include <ne_string.h>
#include <ne_uri.h>
#include <ne_socket.h>
#include <ne_locks.h>
#include <ne_alloc.h>
#include <ne_utils.h>
#include <ne_dates.h>
#include <ne_207.h>
#include <ne_props.h>
#include <ne_redirect.h>
#include <ne_matevfs.h>

/* for getenv */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "http-proxy.h"

#define DEFAULT_USER_AGENT         "mate-vfs/" VERSION

/* Custom User-Agent environment variable */
#define CUSTOM_USER_AGENT_VARIABLE "MATE_VFS_HTTP_USER_AGENT"

/* Standard HTTP[S] port.  */
#define DEFAULT_HTTP_PORT 	80
#define DEFAULT_HTTPS_PORT 	443

#define HTTP_DIR_MIME_TYPE      "x-directory/webdav"

/* ************************************************************************** */
/* module interfaces */
MateVFSMethod *   vfs_module_init      (const char     *method_name,
					 const char     *args);

void               vfs_module_shutdown  (MateVFSMethod *method);

/* ************************************************************************** */
/* DEBUGING stuff */

#ifdef DEBUG_HTTP_ENABLE

void http_debug_printf(const char *func, const char *fmt, ...) G_GNUC_PRINTF (2,3);


#define DEBUG_HTTP_3(fmt, ...) http_debug_printf (__PRETTY_FUNCTION__, fmt, ##__VA_ARGS__) 
#define DEBUG_HTTP_2(fmt, ...) http_debug_printf (__PRETTY_FUNCTION__, fmt, ##__VA_ARGS__) 
#define DEBUG_HTTP(fmt, ...) http_debug_printf (__PRETTY_FUNCTION__, fmt, ##__VA_ARGS__)
#define DEBUG_HTTP_FUNC(_enter) http_debug_printf (_enter ? "+++" : "---", "%s",__PRETTY_FUNCTION__)


#define DEBUG_HTTP_CONTEXT(c) http_debug_printf (__PRETTY_FUNCTION__,       \
						  "[Context] Session :%p, path: %s, dav_mode %d", \
						  c->session, c->path, c->dav_mode)

void
http_debug_printf (const char *func, const char *fmt, ...)
{
	va_list args;
	gchar * out;

	g_assert (fmt);

	va_start (args, fmt);

	out = g_strdup_vprintf (fmt, args);

	g_print ("HTTP:[%p] %s {in %s}\n", g_thread_self (), out, func);

	g_free (out);
	va_end (args);
}
#else /* DEBUG_HTTP_ENABLE */
#define DEBUG_HTTP_3(fmt, ...)	
#define DEBUG_HTTP_2(fmt, ...)
#define DEBUG_HTTP(fmt, ...)
#define DEBUG_HTTP_CONTEXT(c)
#define DEBUG_HTTP_FUNC(_enter)
#endif

/* ************************************************************************** */
/* Http status responses and result mapping */
typedef enum {
	
	/* Successful 2xx.  */
	HTTP_STATUS_OK				= 200,
	HTTP_STATUS_CREATED			= 201,
	HTTP_STATUS_ACCEPTED			= 202,
	HTTP_STATUS_NON_AUTHORITATIVE		= 203,
	HTTP_STATUS_NO_CONTENT			= 204,
	HTTP_STATUS_RESET_CONTENT		= 205,
	HTTP_STATUS_PARTIAL_CONTENTS		= 206,

	/* Redirection 3xx.  */
	HTTP_STATUS_MULTIPLE_CHOICES		= 300,
	HTTP_STATUS_MOVED_PERMANENTLY		= 301,
	HTTP_STATUS_MOVED_TEMPORARILY		= 302,
	HTTP_STATUS_SEE_OTHER			= 303,
	HTTP_STATUS_NOT_MODIFIED		= 304,
	HTTP_STATUS_USE_PROXY			= 305,

	/* Client error 4xx.  */
	HTTP_STATUS_BAD_REQUEST			= 400,
	HTTP_STATUS_UNAUTHORIZED		= 401,
	HTTP_STATUS_PAYMENT_REQUIRED		= 402,
	HTTP_STATUS_FORBIDDEN			= 403,
	HTTP_STATUS_NOT_FOUND			= 404,
	HTTP_STATUS_METHOD_NOT_ALLOWED		= 405,
	HTTP_STATUS_NOT_ACCEPTABLE		= 406,
	HTTP_STATUS_PROXY_AUTH_REQUIRED		= 407,
	HTTP_STATUS_REQUEST_TIMEOUT		= 408,
	HTTP_STATUS_CONFLICT			= 409,
	HTTP_STATUS_GONE			= 410,
	HTTP_STATUS_LENGTH_REQUIRED		= 411,
	HTTP_STATUS_PRECONDITION_FAILED		= 412,
	HTTP_STATUS_REQENTITY_TOO_LARGE 	= 413,
	HTTP_STATUS_REQURI_TOO_LARGE		= 414,
	HTTP_STATUS_UNSUPPORTED_MEDIA		= 415,
	HTTP_STATUS_LOCKED			= 423,

	/* Server errors 5xx.  */
	HTTP_STATUS_INTERNAL			= 500,
	HTTP_STATUS_NOT_IMPLEMENTED		= 501,
	HTTP_STATUS_BAD_GATEWAY			= 502,
	HTTP_STATUS_UNAVAILABLE			= 503,
	HTTP_STATUS_GATEWAY_TIMEOUT		= 504,
	HTTP_STATUS_UNSUPPORTED_VERSION		= 505,
	HTTP_STATUS_INSUFFICIENT_STORAGE 	= 507
	
} HttpStatus;


static MateVFSResult
resolve_result (int result, ne_request *request)
{
	const ne_status *status = ne_get_status (request);
	
	switch (result) {
		
	case NE_ERROR:

		if (ne_matevfs_last_error (request) != MATE_VFS_OK)	{
			DEBUG_HTTP ("gle: %s", mate_vfs_result_to_string 
				    (ne_matevfs_last_error (request)));
			return ne_matevfs_last_error (request);
		}

	case NE_OK:
		break; 
		
	case NE_AUTH:
	case NE_PROXYAUTH:
		return MATE_VFS_ERROR_ACCESS_DENIED;
	
	case NE_REDIRECT:		
	case NE_RETRY:
		g_assert_not_reached ();
		break;
		
	case NE_LOOKUP:
		return MATE_VFS_ERROR_HOST_NOT_FOUND;
		
	case NE_TIMEOUT:
		return MATE_VFS_ERROR_TIMEOUT;
	
	case NE_CONNECT:
		if (ne_matevfs_last_error (request) != MATE_VFS_OK)	{
			DEBUG_HTTP ("gle: %s", mate_vfs_result_to_string 
				    (ne_matevfs_last_error (request)));
			return ne_matevfs_last_error (request);
		}	
	case NE_FAILED:
	default:
		return MATE_VFS_ERROR_GENERIC;
	}	
	
	if (status->klass == 2)
		return (MATE_VFS_OK);
	
	/* mostly taken from the old http-method.c source code */
	switch (status->code) {
	
		/* If this mapping is not true it should be handled 
			inside the specific function */
	case HTTP_STATUS_PRECONDITION_FAILED:
		return MATE_VFS_ERROR_FILE_EXISTS;
		
	case HTTP_STATUS_UNAUTHORIZED:
	case HTTP_STATUS_FORBIDDEN:
	case HTTP_STATUS_PROXY_AUTH_REQUIRED:
		return MATE_VFS_ERROR_ACCESS_DENIED;
		
	case HTTP_STATUS_NOT_FOUND:
		return MATE_VFS_ERROR_NOT_FOUND;
		
	case HTTP_STATUS_METHOD_NOT_ALLOWED:
	case HTTP_STATUS_BAD_REQUEST:
	case HTTP_STATUS_NOT_IMPLEMENTED:
	case HTTP_STATUS_UNSUPPORTED_VERSION:
		return MATE_VFS_ERROR_NOT_SUPPORTED;
		
	case HTTP_STATUS_CONFLICT:
		return MATE_VFS_ERROR_NOT_FOUND;
	
	case HTTP_STATUS_LOCKED:
		return MATE_VFS_ERROR_LOCKED;
	
	case HTTP_STATUS_INSUFFICIENT_STORAGE:
		return MATE_VFS_ERROR_NO_SPACE;
		

	default:
		return MATE_VFS_ERROR_GENERIC;
	}	
	
	
	return MATE_VFS_ERROR_GENERIC;
}

#define IS_REDIRECT(_X) ((_X) == 301 || (_X) == 302 || \
                         (_X) == 303 || (_X) == 307)

#define IS_AUTH_REQ(_X) ((_X) == 401 || (_X) == 407)

#define IS_AUTH_FAILED(_X) ((_X) == 401 || (_X) == 407 || (_X) == 403)


/* ************************************************************************** */
/* Utility functions */

typedef enum {
	
	DAV_CLASS_NOT_SET = -1,
	NO_DAV            =  0,
	DAV_CLASS_1       =  1,
	DAV_CLASS_2       =  2

} DavClass;

typedef struct {
	
	char      *scheme;
	gboolean   ssl;
	char      *alias;
	gboolean   is_dav;
	   
} MethodSchemes;

static const MethodSchemes supported_schemes[] = {

	{"http",  FALSE, "http",  FALSE},
	{"dav",   FALSE, "http",  TRUE},
	{"https", TRUE,  "https", FALSE},
	{"davs",  TRUE,  "https", TRUE},
	{NULL,    FALSE,          NULL}	   
};

static char *
resolve_alias (const char *scheme) 
{
	const MethodSchemes *iter;
	
	if (scheme == NULL)
		return NULL;
	
	for (iter = supported_schemes; iter->scheme; iter++) {
		if (! g_ascii_strcasecmp (iter->scheme, scheme))
			break;
	}
	
	if (iter == NULL)
		return NULL;
	
	return iter->alias;
}

static gboolean
scheme_is_dav (MateVFSURI *uri)
{
	const char *scheme;

	scheme = mate_vfs_uri_get_scheme (uri);
	
	if (scheme == NULL)
		return FALSE;

	if (! g_ascii_strcasecmp (scheme, "dav") ||
	    ! g_ascii_strcasecmp (scheme, "davs"))
		return TRUE;

	return FALSE;
}

static guint
http_session_uri_hash (gconstpointer v)
{
	MateVFSURI *uri;
	guint hash;
	
	uri = (MateVFSURI *) v;
	
	hash = g_str_hash (mate_vfs_uri_get_host_name (uri));
	hash += g_str_hash (resolve_alias (mate_vfs_uri_get_scheme (uri)));
	hash += mate_vfs_uri_get_host_port (uri);
	
	if (mate_vfs_uri_get_user_name (uri))
		hash += g_str_hash (mate_vfs_uri_get_user_name (uri));
	
	return hash;
}

#if 0
static gboolean
g_str_equal_allow_nulls (const char *s1, const char *s2)
{
	return g_str_equal (s1 == NULL ? "" : s1, s2 == NULL ? "" : s2);
}


/* Allow all certificates. */
static int ignore_verify (void *ud, int fs, const ne_ssl_certificate *cert)
{
	return 0;
}
#endif

static gboolean   
http_session_uri_equal (gconstpointer v, gconstpointer v2)
{
	MateVFSURI *a, *b;
	char *sa, *sb;
	
	a = (MateVFSURI *) v;
	b = (MateVFSURI *) v2;
	
	sa = resolve_alias (mate_vfs_uri_get_scheme (a));
	sb = resolve_alias (mate_vfs_uri_get_scheme (b)); 
	
	return (g_str_equal (sa, sb) &&
		g_str_equal (mate_vfs_uri_get_host_name (a),
			     mate_vfs_uri_get_host_name (b)) &&
		mate_vfs_uri_get_host_port (a) ==
		mate_vfs_uri_get_host_port (b));

}

/* ************************************************************************** */
/* Authentication */

/* Review: tune these values ? */
#define AC_CLEANUP  ((60 * 1000))
#define AI_LIFETIME AC_CLEANUP / 1000

G_LOCK_DEFINE (auth_cache);
static GHashTable *auth_cache_basic;
static GHashTable *auth_cache_proxy;
static guint       cleanup_id;
static guint       module_refcount = 0;

typedef struct {
	
	enum {
		AUTH_BASIC,
		AUTH_PROXY
	} type;
	
	MateVFSURI *uri;
	
	gboolean updated;
	gboolean save;
	
	char *username;
	char *password;
	char *realm;
	char *keyring;	
	
	GTimeVal last_used;
	
} HttpAuthInfo;



static HttpAuthInfo *
http_auth_info_new (int type, MateVFSURI *uri, char *username, char *password)
{
	HttpAuthInfo *ai;
	
	ai = g_new0 (HttpAuthInfo, 1);
	
	ai->type     = type;
	ai->uri      = mate_vfs_uri_ref (uri);
	ai->updated  = FALSE;
	ai->save	 = FALSE;
	
	if (username)
		ai->username = g_strdup (username);
	
	if (password)
		ai->password = g_strdup (password);
	
	return ai;
}

static void 
http_auth_info_free (HttpAuthInfo *info)
{
	if (info->username)
		g_free (info->username);
	
	if (info->password)
		g_free (info->password);
	
	if (info->realm)
		g_free (info->realm);
	
	if (info->keyring)
		g_free (info->keyring);
	
	if (info->uri)
		mate_vfs_uri_unref (info->uri);
	
	g_free (info);
}


static void
http_auth_info_copy (HttpAuthInfo *dest, HttpAuthInfo *src)
{
	dest->type = src->type;
	
	if (dest->uri)
		mate_vfs_uri_unref (dest->uri);
	
	if (src->uri) 
		dest->uri = mate_vfs_uri_ref (src->uri);
	else
		dest->uri = NULL;
	
	
	if (dest->username)
		g_free (dest->username);
	
	if (src->username)
		dest->username = g_strdup (src->username);
	else 
		dest->username = NULL;
	
	
	if (dest->password)
		g_free (dest->password);
	
	if (src->password)
		dest->password = g_strdup (src->password);
	else
		dest->password = NULL;
	
	dest->save = src->save;
	dest->updated = src->updated;
}

static gboolean
http_auth_cache_info_check (gpointer key, gpointer value, gpointer data)
{
	HttpAuthInfo *info = value;
	gboolean     *restart_timeout = data;
	GTimeVal      now;

	g_get_current_time (&now);

	if (now.tv_sec > info->last_used.tv_sec + AI_LIFETIME) {
		DEBUG_HTTP ("[AuthCache] Removing auth entry!");
		return TRUE;
	}

	*restart_timeout = TRUE;
	
	return FALSE;
}


static gboolean
http_auth_cache_cleanup (gpointer *data)
{
	gboolean restart_timeout;

	DEBUG_HTTP ("[AuthCache] Cleanup!");

	restart_timeout = FALSE;

	G_LOCK (auth_cache);
	g_hash_table_foreach_remove (auth_cache_proxy, 
				     http_auth_cache_info_check, 
				     &restart_timeout);
	
	
	g_hash_table_foreach_remove (auth_cache_basic, 
				     http_auth_cache_info_check, 
				     &restart_timeout);

	if (restart_timeout == FALSE)
		cleanup_id = 0;

	G_UNLOCK (auth_cache);
		
	return restart_timeout;
}


static void
http_auth_cache_init (void)
{
	auth_cache_proxy = g_hash_table_new_full (http_session_uri_hash, 
						  http_session_uri_equal,
						  NULL,
						  (GDestroyNotify) http_auth_info_free);
	
	
	auth_cache_basic = g_hash_table_new_full (http_session_uri_hash,
						  http_session_uri_equal,
						  NULL,
						  (GDestroyNotify) 
						  http_auth_info_free);
}


static void
http_auth_cache_shutdown (void)
{
	g_hash_table_destroy (auth_cache_proxy);
	
	g_hash_table_destroy (auth_cache_basic);
}


static gboolean
query_cache_for_authentication (HttpAuthInfo *info)
{
	HttpAuthInfo *aic = NULL;
	gboolean res = FALSE;

	G_LOCK (auth_cache);

	if (info->type == AUTH_BASIC) {
		DEBUG_HTTP ("[Auth] Basic queried");
		aic = g_hash_table_lookup (auth_cache_basic, info->uri);
		
	} else {
		DEBUG_HTTP ("[Auth] proxy queried");
		aic = g_hash_table_lookup (auth_cache_proxy, info->uri);
		
	}

		
	if (aic != NULL) {
		http_auth_info_copy (info, aic);
		g_get_current_time (&(info->last_used));
		res = TRUE;
		DEBUG_HTTP ("[Auth] hit!");
	}

	G_UNLOCK (auth_cache);
		
	return res;
}


static void
store_auth_info_in_cache (HttpAuthInfo *info)
{
	HttpAuthInfo *copy;
	
	copy = g_new0 (HttpAuthInfo, 1);
	http_auth_info_copy (copy, info);
	copy->updated = FALSE;

	g_get_current_time (&(copy->last_used));

	G_LOCK (auth_cache);

	if (info->type == AUTH_BASIC) {
	
		DEBUG_HTTP ("[Auth] Basic stored [%s/%s]", info->username, info->password);
		
		g_hash_table_replace (auth_cache_basic, copy->uri, copy);
	
		
	} else {
		
		DEBUG_HTTP ("[Auth] PROXY stored [%s/%s]", info->username, info->password);
		
		g_hash_table_replace (auth_cache_proxy, copy->uri, copy);
	}

	if (cleanup_id == 0) {
		cleanup_id = g_timeout_add (AC_CLEANUP, (GSourceFunc) 
					    http_auth_cache_cleanup, NULL);
	}

	G_UNLOCK (auth_cache);
	
	
}


static gboolean
query_keyring_for_authentication (HttpAuthInfo *info)
{
	MateVFSModuleCallbackFillAuthenticationIn in_args;
	MateVFSModuleCallbackFillAuthenticationOut out_args;
	gboolean ret;
	
	memset (&in_args,  0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));	
	
	DEBUG_HTTP ("[Auth] Query Keyring!");
	
	in_args.uri = mate_vfs_uri_to_string (info->uri, MATE_VFS_URI_HIDE_NONE);
	
	in_args.protocol = "http";
	in_args.authtype = (info->type == AUTH_BASIC ? "basic" : "proxy");
	in_args.object	 = info->realm;
	in_args.server   = (char *) mate_vfs_uri_get_host_name (info->uri);
	in_args.port     = mate_vfs_uri_get_host_port (info->uri);
	in_args.username = info->username;

	ret = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
						&in_args, sizeof (in_args), 
						&out_args, sizeof (out_args));
		
	g_free (in_args.uri);							
		
	if (!ret) return FALSE;
	
	if ((ret = out_args.valid) == TRUE) {
		
		if (info->username)
			g_free (info->username);
		
		if (info->password)
			g_free (info->password);
		
		info->username = out_args.username;
		info->password = out_args.password;
		
		info->updated = TRUE;
		info->save = FALSE;
		DEBUG_HTTP ("[Auth] Keyring hit!");
		
	} else {
		
		g_free (out_args.username);
		g_free (out_args.password);
	}
	
	g_free (out_args.domain);
	
	return ret;
}

static gboolean
query_user_for_authentication (HttpAuthInfo *info, int attempt)
{
	MateVFSModuleCallbackFullAuthenticationIn  in_args;
	MateVFSModuleCallbackFullAuthenticationOut out_args;
	MateVFSToplevelURI 	*toplevel_uri;
	gboolean 		 ret;
	
	DEBUG_HTTP ("[Auth] Query User!");
	
	memset (&in_args,  0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));
	
	toplevel_uri = mate_vfs_uri_get_toplevel (info->uri);
	in_args.uri  = mate_vfs_uri_to_string (info->uri, MATE_VFS_URI_HIDE_NONE);
	
	in_args.protocol     = "http";
	in_args.authtype     = info->type == AUTH_BASIC ? "basic" : "proxy";
	in_args.object	     = (char *) info->realm;
	in_args.server       = (char *) toplevel_uri->host_name;
	in_args.port         = toplevel_uri->host_port;
	in_args.username     = info->username;
	in_args.default_user = in_args.username;

		
	in_args.flags = MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD
		| MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED;
	
	
	if (attempt > 0) {
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED;
	}
	
	if (info->username == NULL)
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME;
	
	
	ret = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
						&in_args, sizeof (in_args), 
						&out_args, sizeof (out_args));
	
	g_free (in_args.uri);
	g_free (out_args.domain);
	
	if (!ret) {
		ret = FALSE;
		goto error;
	}
		
	if ((ret = !out_args.abort_auth)) {
		info->keyring = out_args.keyring;
		
		
		if (info->username)
			g_free (info->username);
		
		if (info->password)
			g_free (info->password);
		
		info->username = out_args.username;
		info->password = out_args.password;
		
		ret = TRUE;
		
		if (out_args.save_password) {
			info->save    = TRUE;
			info->keyring = out_args.keyring;
		} else {
			g_free (out_args.keyring);
		}
		
		/* Review: should we always store auth info in local cache
		   even if it is stored in keyring ? */
		info->updated = TRUE;
		return TRUE;
	}
	
 error:	
	g_free (out_args.username);
	g_free (out_args.password);
	g_free (out_args.keyring);
	
	return ret;
}


static int
neon_session_supply_auth (void 		*userdata, 
			  const char    *realm, 
			  int 		 attempt,
			  char          *username, 
			  char 		*password)
{
	HttpAuthInfo *info;
	
	info = (HttpAuthInfo *) userdata;
	
	/* use information in uri if possible */
	if (attempt == 0 && info->username && info->password) {
		
		g_strlcpy (username, info->username, NE_ABUFSIZ);
		g_strlcpy (password, info->password, NE_ABUFSIZ);
		
		DEBUG_HTTP ("[Auth] Using auth info from uri!");
		return 0;
	}
	
	if (attempt == 0 && query_cache_for_authentication (info)) {
		
		g_strlcpy (username, info->username, NE_ABUFSIZ);
		g_strlcpy (password, info->password, NE_ABUFSIZ);
		
		DEBUG_HTTP ("[Auth]  Cache hit!");
		
		return 0;
	}

	if (info->realm == NULL)
		info->realm = g_strdup (realm);
	
	if (attempt == 0 && query_keyring_for_authentication (info)) {
		
		g_strlcpy (username, info->username, NE_ABUFSIZ);
		g_strlcpy (password, info->password, NE_ABUFSIZ);
		
		DEBUG_HTTP ("[Auth] keyring hit!");
		
		return 0;
	}
	
	if (query_user_for_authentication (info, attempt)) {
		
		g_strlcpy (username, info->username, NE_ABUFSIZ);
		g_strlcpy (password, info->password, NE_ABUFSIZ);
		
		DEBUG_HTTP ("[Auth] Trying user info!");
		
		return 0;
	}
	
	DEBUG_HTTP ("[Auth] FAILED!");
	
	return 1;
}


static int
neon_session_save_auth (ne_request 	  *req, 
		   	void		  *userdata,
		   	const ne_status   *status)
{
	MateVFSModuleCallbackSaveAuthenticationIn  in_args;
	MateVFSModuleCallbackSaveAuthenticationOut out_args;
	gboolean ret;
	HttpAuthInfo *info;
	
	info = (HttpAuthInfo *) userdata;
	ret  = FALSE;


	if (info->updated != TRUE) {
		DEBUG_HTTP ("[Auth] Auth info not updated");
		return 0;
	}
	
	info->updated = FALSE;
	
	if (status && IS_AUTH_FAILED (status->code)) {
		return 0;
	}
	
	store_auth_info_in_cache (info);
	
	if (info->save != TRUE) {
		return 0;
	}
	
	DEBUG_HTTP ("[Auth] Storing auth in keyring!");
	
	memset (&in_args,  0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));
	
	in_args.keyring  = info->keyring;
	in_args.uri 	 = mate_vfs_uri_to_string (info->uri, MATE_VFS_URI_HIDE_NONE);
	in_args.server 	 = (char *) mate_vfs_uri_get_host_name (info->uri);
	in_args.port 	 = mate_vfs_uri_get_host_port (info->uri);
	in_args.username = info->username;
	in_args.password = info->password;
	in_args.protocol = "http";
	in_args.authtype = info->type == AUTH_BASIC ? "basic" : "proxy";
	in_args.object 	 = info->realm;
	
	mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					  &in_args, sizeof (in_args), 
					  &out_args, sizeof (out_args)); 
	
	
	info->save = FALSE;
	return 0;
}


/* ************************************************************************** */
/* Header parsing utilities */

static gboolean
header_value_to_number (const char *header_value, gulong *number)
{
	gulong result;
	const char *p = header_value;

	if (header_value == NULL) {
		return FALSE;
	}
	
	for (result = 0; g_ascii_isdigit (*p); p++)
		result = 10 * result + (*p - '0');
	
	if (*p) return FALSE;

	*number = result;

	return TRUE;
}


static char *
strip_semicolon (const char *value)
{
	char *p = strchr (value, ';');

	if (p != NULL) {
		return g_strndup (value, p - value);
	}
	else {
		return g_strdup (value);
	}
}


static DavClass 
parse_dav_header (const char *value)
{
	char *tokens = ne_strdup(value), *pnt = tokens;
        DavClass dav_class = DAV_CLASS_NOT_SET;
    
	DEBUG_HTTP_3 ("parsing dav: %s", tokens); 

	do {
		char *tok = ne_qtoken (&pnt, ',',  "\"'");
		if (!tok) break;
	
		tok = ne_shave(tok, " \r\t");

		if (strcmp (tok, "1") == 0) {
			dav_class = 1;
		} else if (strcmp(tok, "2") == 0) {
			dav_class = 1;
		} 
		
		DEBUG_HTTP_3 ("DAV Level: %d", dav_class);
		
	} while (pnt != NULL);
    
	ne_free (tokens);
	return dav_class;
}

typedef enum {
	ALLOW_NOT_SET   = 0,
	ALLOW_OPTIONS   = (1 << 0),
	ALLOW_GET       = (1 << 1),
	ALLOW_HEAD      = (1 << 2),
	ALLOW_POST      = (1 << 3),
	ALLOW_PUT	= (1 << 4),
	ALLOW_DELETE    = (1 << 5),
	ALLOW_TRACE     = (1 << 6),
	ALLOW_PROPFIND  = (1 << 7),
	ALLOW_PROPPATCH = (1 << 8),
	ALLOW_MKCOL	= (1 << 9),
	ALLOW_COPY	= (1 << 10),
	ALLOW_MOVE	= (1 << 11),
	ALLOW_LOCK	= (1 << 12),
	ALLOW_UNLOCK	= (1 << 13)
	
} HttpMethods;

static struct HttpMethod {
	char           *string;
	HttpMethods  type;
	
} const http_all_methods [] = {
	{"OPTIONS", ALLOW_OPTIONS},
	{"GET", ALLOW_GET},
	{"HEAD", ALLOW_HEAD},
	{"POST", ALLOW_POST},
	{"PUT", ALLOW_PUT},
	{"DELETE", ALLOW_DELETE},
	{"TRACE", ALLOW_TRACE},
	{"PROPFIND", ALLOW_PROPFIND},
	{"PROPPATCH", ALLOW_PROPPATCH},
	{"MKCOL", ALLOW_MKCOL},
	{"COPY", ALLOW_COPY},
	{"MOVE", ALLOW_MOVE},
	{"LOCK", ALLOW_LOCK},
	{"UNLOCK", ALLOW_UNLOCK},
	{NULL, ALLOW_NOT_SET}
};

static GHashTable *http_methods = NULL;

static void
quick_allow_lookup_init (void)
{
	const struct HttpMethod *iter;
	
	http_methods = g_hash_table_new (g_str_hash, g_str_equal);
	
	for (iter = http_all_methods; iter->string; iter++) {
		g_hash_table_insert (http_methods, iter->string, (struct HttpMethod *)iter);
	}
}

static void
quit_allow_lookup_destroy (void)
{
	g_hash_table_destroy (http_methods);
}


static HttpMethods
parse_allow_header (const char *value)
{
	char *tokens = ne_strdup (value), *pnt = tokens;
	HttpMethods methods;
	struct HttpMethod *method;
    
	DEBUG_HTTP_3 ("parsing allow: %s", pnt);
	methods = ALLOW_NOT_SET;

	do {
		char *tok = ne_qtoken (&pnt, ',',  "\"'");
		if (!tok) break;
	
		tok = ne_shave(tok, " \r\t");

		method = g_hash_table_lookup (http_methods, tok);
		
		if (method == NULL)
			continue;
		
		DEBUG_HTTP_3 ("setting %s to yes", method->string);
		
		methods |= method->type;
		
	} while (pnt != NULL);
    
	ne_free (tokens);
	return methods;
}

/* function for setting the etag value
   we dont use it at the moment
static void 
set_etag (void **etag, const char *value)
{		
	if (value != NULL)
		*etag = g_strdup (value);
	else
		*etag = NULL;
}
*/

static void
std_headers_to_file_info (ne_request *req, MateVFSFileInfo *info)
{
	const char *value;
	time_t	    time;
	gboolean    result;
	gulong      size;
	
	value = ne_get_response_header (req, "Last-Modified");

	if (value != NULL && mate_vfs_atotm (value, &time)) {
		
		info->mtime = time;
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MTIME;
	}

	
	value  = ne_get_response_header (req, "Content-Length");
	result = header_value_to_number (value, &size);
	
	if (result == TRUE) {
	
		info->size = size;
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
	}
	
	
	value  = ne_get_response_header (req, "Content-Type");

	if (value != NULL) {
		g_free (info->mime_type);

		info->mime_type = strip_semicolon (value);
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	}

	value = ne_get_response_header (req, "Date");

	if (value != NULL && mate_vfs_atotm (value, &time)) {
		info->atime = time;
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ATIME;
	}
}

/* ************************************************************************** */
/* Propfind request handlers */
static const ne_propname file_info_props[] = {
	{"DAV:", "getlastmodified"},
	{"DAV:", "creationdate"},
	{"DAV:", "resourcetype"},
	{"DAV:", "getcontenttype"},
	{"DAV:", "getcontentlength"},
	/*      {"DAV:", "getetag"}, */
	/*	{"http://apache.org/dav/props/", "executable"}, */
	{NULL}
};

typedef struct {

	const char *path;
	MateVFSFileInfo *target;
	gboolean include_target;
	
	GList *children;
	
	char *etag;
	
} PropfindContext;


static void
propfind_context_init (PropfindContext *pfctx)
{
	pfctx->path = NULL;
	pfctx->target = NULL;
	pfctx->include_target = TRUE;
	pfctx->children = NULL;
	pfctx->etag = NULL;
	
}

static void
propfind_context_clear (PropfindContext *pfctx)
{
	GList *l;
	
	if (pfctx->target != NULL) {
		mate_vfs_file_info_unref (pfctx->target);
		pfctx->target = NULL;
	}
		
	if (pfctx->children) {
		l = mate_vfs_file_info_list_unref (pfctx->children);
		g_list_free (l);
		pfctx->children = NULL;
	}
	
	if (pfctx->etag) {
		g_free (pfctx->etag);
		pfctx->etag = NULL;
	}
}


static void 
propfind_result (void *userdata, const char *href, const ne_prop_result_set *set)
{
	MateVFSFileInfo *info;
	PropfindContext *ctx;
	const char *value;
	time_t	time;
	gulong  size;
	ne_uri  uri;
	char *unesc_path, *unesc_ctx_path;

	ctx = (PropfindContext *) userdata;

	if (ne_uri_parse (href, &uri))
		return;

	if (uri.path == NULL) {
		ne_uri_free (&uri);
		return;
	}

	DEBUG_HTTP_2 ("href: %s", href);

	info = mate_vfs_file_info_new ();
	unesc_path = ne_path_unescape (uri.path);
	info->name = g_path_get_basename (unesc_path);

	unesc_ctx_path = ne_path_unescape (ctx->path);
	DEBUG_HTTP_2 ("Comparing: \n\t[%s] \n\t[%s]", ctx->path, uri.path);
	if (ne_path_compare (unesc_ctx_path, unesc_path) == 0) {
		DEBUG_HTTP_3 ("target");
		ctx->target = info;

		/* Set the etag on target */
		/* We are not requesting the etag information at 
		 * the moment so don't even check for it 
		value = ne_propset_value (set, &file_info_props[5]);

		if (value != NULL)
			ctx->etag = g_strdup (value);

		*/
	} else {
		ctx->children = g_list_append (ctx->children, info);
	}

	NE_FREE (unesc_ctx_path);
	NE_FREE (unesc_path);
	ne_uri_free (&uri);

	/* getlastmodified */
	value = ne_propset_value (set, &file_info_props[0]);

	if (value && mate_vfs_atotm (value, &time)) {
		info->mtime = time;
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MTIME;
	}

	/* creationdate */
	value = ne_propset_value (set, &file_info_props[1]);

	if (value && mate_vfs_atotm (value, &time)) {
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_CTIME;
	}

	/* resourctype */
	value = ne_propset_value (set, &file_info_props[2]);

	info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;

	if (value != NULL && strstr (value, "collection")) {
		info->mime_type = g_strdup (HTTP_DIR_MIME_TYPE);
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		info->type = MATE_VFS_FILE_TYPE_DIRECTORY;

		return;
	} else {
		info->type = MATE_VFS_FILE_TYPE_REGULAR;
	}

	/*** The following properties only apply to files ***/

	/* getcontenttype */

	/* We only evaluate the getcontenttype filed if it hasn't already
	 * been set (i.e. to HTTP_DIR_MIME_TYPE) so we can indicate the
	 * directory mime-type */
	if (! (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE)) {
		value = ne_propset_value (set, &file_info_props[3]);
	} else {
		value = NULL;
	}

	if (! value) {
		value = mate_vfs_mime_type_from_name (info->name);
	}

	if (value) {
		info->mime_type = g_strdup (value);
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	}

	/* getcontentlength */

	value = ne_propset_value (set, &file_info_props[4]);

	if (value && header_value_to_number (value, &size)) {
		info->size = size;
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
	}
}


/* ************************************************************************** */
/* neon session pool and related helper functions */

G_LOCK_DEFINE (nst_lock);
static GHashTable *neon_session_table;
static guint nst_tid;

typedef struct {
	
	MateVFSURI *uri;
	
	GList *unused_sessions;

	GTimeVal last_used;
	
} NeonSessionPool;

static void
neon_session_pool_destroy (NeonSessionPool *pool)
{
	GList *iter;
	
	
	for (iter = pool->unused_sessions; iter; iter = iter->next) {
		ne_session *session = iter->data;
		
		ne_session_destroy (session);
	}
	
	g_list_free (pool->unused_sessions);
	
	mate_vfs_uri_unref (pool->uri);
	g_free (pool);
}


static void
neon_session_pool_init (void)
{
	neon_session_table = g_hash_table_new_full (http_session_uri_hash, 
						    http_session_uri_equal, NULL,
						    (GDestroyNotify) neon_session_pool_destroy);	
}


static void
neon_session_pool_shutdown (void)
{
	g_hash_table_destroy (neon_session_table);
}


static gboolean
neon_session_pool_check (gpointer key, gpointer value, gpointer data)
{
	NeonSessionPool *pool = value;
	gboolean        *restart_timeout = data;
	GTimeVal         now;

	g_get_current_time (&now);

	if (now.tv_sec > pool->last_used.tv_sec + AI_LIFETIME) {
		DEBUG_HTTP ("[Sesssion Pool] Removing!");
		return TRUE;
	}

	*restart_timeout = TRUE;
	
	return FALSE;
}


static gboolean
neon_session_pool_cleanup (gpointer *data)
{
	gboolean restart_timeout;

	restart_timeout = FALSE;

	DEBUG_HTTP ("[Session Pool] Cleanup!");
	
	G_LOCK (nst_lock);
	g_hash_table_foreach_remove (neon_session_table, 
				     neon_session_pool_check,
				     &restart_timeout);
	

	if (restart_timeout == FALSE)
		nst_tid = 0;

	G_UNLOCK (nst_lock);
		
	return restart_timeout;
}


static void
neon_session_pool_insert (MateVFSURI *uri, ne_session *session)
{
	NeonSessionPool *pool;

	G_LOCK (nst_lock);
		
	pool = g_hash_table_lookup (neon_session_table, uri);
	
	if (pool == NULL) {
		pool = g_new0 (NeonSessionPool, 1);
		pool->uri = mate_vfs_uri_ref (uri);
		g_hash_table_insert (neon_session_table, uri, pool);
	}
	
	/* Make sure we forget the auth info so multiple users for
	 * the same uri doesnt confict. We have our own cache to
	 * compensate this */
	ne_forget_auth (session);	
	
	pool->unused_sessions = g_list_append (pool->unused_sessions, 
					       session);

	DEBUG_HTTP ("[Session Pool] Storing [%p] int [%p] (%d)", session, pool,
		    g_list_length (pool->unused_sessions));

	if (nst_tid == 0) {
		nst_tid = g_timeout_add (AC_CLEANUP, 
					 (GSourceFunc) neon_session_pool_cleanup, 
					 NULL);
	}

	G_UNLOCK (nst_lock);
}

static ne_session *
neon_session_pool_lookup (MateVFSURI *uri)
{
	NeonSessionPool *pool;
	ne_session *session;

	G_LOCK (nst_lock);
		
	DEBUG_HTTP ("[Session Pool] Searching (%d)", 
		    g_hash_table_size (neon_session_table));
	
	session = NULL;
	pool = g_hash_table_lookup (neon_session_table, uri);
	
	/* search in session pool */
	if (pool != NULL && pool->unused_sessions) {

		session = pool->unused_sessions->data;
		pool->unused_sessions = g_list_remove (pool->unused_sessions,
						       session);
		DEBUG_HTTP ("[Session Pool] Found [%p] in [%p] (%d)", session, pool,
			    g_list_length (pool->unused_sessions));
		
		g_get_current_time (&(pool->last_used));
	}
	
	G_UNLOCK (nst_lock);
	return session;
}

/* ************************************************************************** */
/* Additional Headers */
static int 
neon_return_headers (ne_request *req, void *userdata, const ne_status *status)
{	
	MateVFSModuleCallbackReceivedHeadersIn in_args;
	MateVFSModuleCallbackReceivedHeadersOut out_args;
	GList       *headers;
	void        *hiter;
	MateVFSURI *uri;
	ne_session  *session;
	const char  *hkey, *hval;

	DEBUG_HTTP_FUNC (1);
	
	session = ne_get_session (req);
	
	if (ne_get_request_private (req, "Headers Returned")) {
		return 0;
	}
	
	headers = NULL;
	hiter = NULL;

	while ((hiter = ne_response_header_iterate (req,
						    hiter,
						    &hkey,
						    &hval))) {
		char *header;

		if (hkey == NULL || hval == NULL) {
			continue;
		}
		
		header = g_strdup_printf ("%s: %s", hkey, hval);
		headers = g_list_prepend (headers, header);
	}

	if (headers == NULL) {
		return 0;
	}
	
	uri = ne_get_session_private (session, "MateVFSURI");

	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));
	
	in_args.uri = uri;
	in_args.headers = headers;

	mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_HTTP_RECEIVED_HEADERS,
					&in_args, sizeof (in_args),
					&out_args, sizeof (out_args));
	
	g_list_foreach (headers, (GFunc) g_free, NULL);
	g_list_free (headers);
	
	ne_set_request_private (req, "Headers Returned", "TRUE");
	
	DEBUG_HTTP_FUNC (0);
	
	return 0;
}

static void 
neon_setup_headers (ne_request *req, void *userdata, ne_buffer *header)
{
	MateVFSModuleCallbackAdditionalHeadersIn in_args;
	MateVFSModuleCallbackAdditionalHeadersOut out_args;
	GList *iter;
	gboolean ret;
	MateVFSURI *uri;
	ne_session *session;

	DEBUG_HTTP_FUNC (1);
	
	session = ne_get_session (req);
	uri = ne_get_session_private (session, "MateVFSURI");

	memset (&in_args, 0, sizeof (in_args));
	memset (&out_args, 0, sizeof (out_args));

	in_args.uri = uri;

	ret = mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_HTTP_SEND_ADDITIONAL_HEADERS,
						&in_args, sizeof (in_args),
						&out_args, sizeof (out_args));
	
	for (iter = out_args.headers; iter; iter = iter->next) {
		
		if (ret) {
			ne_buffer_zappend (header, iter->data);
			DEBUG_HTTP_3 ("Adding header %s,", (char *) iter->data);
		}		
		
		g_free (iter->data);
	}
	
	g_list_free (out_args.headers);
	
	DEBUG_HTTP_FUNC (0);
}

/* ************************************************************************** */
/* Http context */
typedef struct {
	
	MateVFSURI *uri;
	char        *path;
	
	const char *scheme;
	gboolean    ssl;
	
	DavClass    dav_class;
	HttpMethods methods;
	
	ne_session *session;

	gboolean    dav_mode;
	gboolean    redirected;
	guint       redir_count;
	
} HttpContext;


static MateVFSResult
http_acquire_connection (HttpContext *context)
{
	MateVFSToplevelURI *top_uri;
	char *user_agent;	
	HttpAuthInfo *basic_auth;
	HttpProxyInfo proxy;
	ne_session *session;
	

	if (context->ssl == TRUE &&
	    ! ne_has_support (NE_FEATURE_SSL)) {
		DEBUG_HTTP ("SSL not supported!");
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	top_uri = mate_vfs_uri_get_toplevel (context->uri);

	if (top_uri == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}
		
	session = neon_session_pool_lookup (context->uri);
	
	if (session != NULL) {
			
		ne_set_session_private (session, "MateVFSURI", context->uri);
	
		context->session = session;
		return MATE_VFS_OK;
	}
		
	
	DEBUG_HTTP ("[Session] new connection %s, %d", top_uri->host_name, top_uri->host_port);
	/* no free connection, create one */
	session = ne_session_create (context->scheme, top_uri->host_name, 
				     top_uri->host_port);
	
	if (session == NULL) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	ne_set_read_timeout (session, 30);

	user_agent = getenv (CUSTOM_USER_AGENT_VARIABLE);
	   
	if (user_agent == NULL) {
		user_agent = DEFAULT_USER_AGENT;
	}
	   
	ne_set_useragent (session, user_agent);
	
	basic_auth = http_auth_info_new (AUTH_BASIC, context->uri, 
					 top_uri->user_name, 
					 top_uri->password);
	
	/* Authentication */
	ne_set_server_auth (session, neon_session_supply_auth, basic_auth);
	ne_hook_post_send (session, neon_session_save_auth, basic_auth);
	ne_hook_destroy_session (session, (ne_destroy_sess_fn)
				 http_auth_info_free, basic_auth);

	ne_redirect_register (session);

	/* Headers stuff */
	ne_set_session_private (session, "MateVFSURI", context->uri);
	ne_hook_pre_send (session, neon_setup_headers, NULL);
	ne_hook_post_send (session, neon_return_headers, NULL);

	if (proxy_for_uri (top_uri, &proxy)) {
		HttpAuthInfo *proxy_auth;
		
		DEBUG_HTTP ("using proxy");
		ne_session_proxy (session, proxy.host, proxy.port);
		
		proxy_auth = http_auth_info_new (AUTH_PROXY, context->uri, 
						 proxy.username,
						 proxy.password);
		
		ne_set_proxy_auth (session, neon_session_supply_auth, 
				   proxy_auth);
		
		ne_hook_post_send (session, neon_session_save_auth, 
				   proxy_auth);
		
		ne_hook_destroy_session (session, (ne_destroy_sess_fn)
					 http_auth_info_free, proxy_auth);

		g_free (proxy.host);
	}
	
	context->session = session;
	
	return MATE_VFS_OK;
}



static void
http_context_free (HttpContext *context)
{
	if (context->session) {
		neon_session_pool_insert (context->uri, context->session);
		context->session = NULL;
	}

	g_free (context->path);
	
	mate_vfs_uri_unref (context->uri);
	
	g_free (context);
}


static void
http_context_set_uri (HttpContext *context, MateVFSURI *uri)
{
	char *uri_string;
	
	if (context->uri) {
		mate_vfs_uri_unref (context->uri);
	}
		
	if (context->path != NULL) {
		g_free (context->path);
	}
		
	context->uri = mate_vfs_uri_dup (uri);
	context->scheme = resolve_alias (mate_vfs_uri_get_scheme (uri));
	
	if (mate_vfs_uri_get_host_port (context->uri) == 0) {
		if (g_str_equal (context->scheme, "https")) {
			mate_vfs_uri_set_host_port (context->uri,
						     DEFAULT_HTTPS_PORT);
			context->ssl = TRUE;
		} else { 
			mate_vfs_uri_set_host_port (context->uri,
						     DEFAULT_HTTP_PORT);
			context->ssl = FALSE;
		}
	}
	
	uri_string = mate_vfs_uri_to_string (context->uri, MATE_VFS_URI_HIDE_USER_NAME
					      | MATE_VFS_URI_HIDE_PASSWORD
					      | MATE_VFS_URI_HIDE_HOST_NAME
					      | MATE_VFS_URI_HIDE_HOST_PORT
					      | MATE_VFS_URI_HIDE_TOPLEVEL_METHOD
					      | MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER);
	if (uri_string[0] == '\0') {
		g_free (uri_string);
		uri_string = g_strdup ("/");
	}
	
	context->path      = uri_string;
	context->methods   = 0;
	context->dav_class = DAV_CLASS_NOT_SET;
	
}


static MateVFSResult
http_context_open (MateVFSURI *uri, HttpContext **context)
{
	HttpContext *ctx;
	MateVFSResult result;
	
	if (mate_vfs_uri_get_host_name (uri) == NULL) 
		return MATE_VFS_ERROR_INVALID_URI;
	
	ctx = g_new0 (HttpContext, 1);
	
	http_context_set_uri (ctx, uri);
	
	if (ctx->scheme == NULL) {
		http_context_free (ctx);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	result = http_acquire_connection (ctx);
	
	if (result != MATE_VFS_OK) {
		*context = NULL;
		http_context_free (ctx);
		return result;
	}

	/* initialize the missing bits */
	ctx->dav_mode  = scheme_is_dav (uri);
	ctx->dav_class = DAV_CLASS_NOT_SET;
	ctx->methods   = ALLOW_NOT_SET;
	
	*context = ctx;

	return MATE_VFS_OK;
}

static gboolean
http_context_host_matches (HttpContext *context, const char *glob)
{
        MateVFSToplevelURI *uri;
        GPatternSpec *spec;
        gboolean res;

        uri = mate_vfs_uri_get_toplevel (context->uri);

        if (uri == NULL ||  uri->host_name == NULL || glob == NULL)
                return FALSE;

        spec = g_pattern_spec_new (glob);
        res = g_pattern_match_string (spec, uri->host_name);
        g_pattern_spec_free (spec);

        return res;
}

static MateVFSResult
http_follow_redirect (HttpContext *context)
{
	MateVFSResult result;
	MateVFSURI   *new_uri;
	const ne_uri  *redir_uri;
	char          *redir_texturi;
	
	context->redirected = TRUE;
	context->redir_count++;
	
	if (context->redir_count > 7)
		return MATE_VFS_ERROR_TOO_MANY_LINKS;
	
	DEBUG_HTTP ("[Redirec] Following redir [count: %d]", context->redir_count);
	redir_uri = ne_redirect_location (context->session);
	redir_texturi = ne_uri_unparse (redir_uri);
	new_uri = mate_vfs_uri_new (redir_texturi);
	
	NE_FREE (redir_texturi);
	
	/* see if redirect is to another host:port pair so we need a new
	 * connection/session */
	if (! http_session_uri_equal (context->uri, new_uri)) {
		
		/* release connection */
		neon_session_pool_insert (context->uri, context->session);
		context->session = NULL;
		
		http_context_set_uri (context, new_uri);
		result = http_acquire_connection (context);
		
	} else {
		
		http_context_set_uri (context, new_uri);
		ne_set_session_private (context->session, "MateVFSURI", context->uri);
		result = MATE_VFS_OK;
	}
	
	/* uri got dupped by http_context_set_uri () */
	mate_vfs_uri_unref (new_uri);
	
	DEBUG_HTTP ("[Redirec] Redirect result: %s", 
		    mate_vfs_result_to_string (result));
	return result;
}

/* ************************************************************************** */
/* Http operations */

static MateVFSResult
http_get_file_info (HttpContext *context, MateVFSFileInfo *info)
{
	MateVFSResult result;
	PropfindContext pfctx;
	ne_propfind_handler *pfh;
	ne_request *req;
	int res;

	DEBUG_HTTP_CONTEXT (context);
	
	/* no dav server */
	if (context->dav_mode == FALSE || context->dav_class == NO_DAV) 
		goto head_start;
 	
	propfind_context_init (&pfctx);
	
 propfind_start:	
	pfctx.path = context->path;
	pfctx.include_target = TRUE;

	pfh = ne_propfind_create (context->session, context->path, 0);
	res = ne_propfind_named (pfh, file_info_props, propfind_result, &pfctx);
	
	if (res == NE_REDIRECT) {
		ne_propfind_destroy (pfh);
		
		result = http_follow_redirect (context);
		
		if (result == MATE_VFS_OK) {
			goto propfind_start;
		} else {
			return result;
		}
	}

	req = ne_propfind_get_request (pfh);
	result = resolve_result (res, req);
	DEBUG_HTTP ("%d, %d", res, ne_get_status (req)->code);
	ne_propfind_destroy (pfh);

	/* Let's be very cautious here! If the server doesn't respond with a
	   207 here just fall back to HEAD because some server server (eg. gws)
	   close the connection on an unknown command, most (stupid) php scripts
	   will treat PROPFIND as GET and some servers may deny us PROFIND but
	   allow us HEAD. 
	   The only exception is if we receive a 404 */

	if (res == NE_OK) {
		gboolean have_result = FALSE;
		const ne_status *status = ne_get_status (req);

		if (status->code == 207) {

			have_result = TRUE;

			if (pfctx.target != NULL) {
				mate_vfs_file_info_copy (info, pfctx.target);
			} else {
				result = MATE_VFS_ERROR_NOT_FOUND;
			}

		} else if (status->code == 404) {
			have_result = TRUE;
			result = MATE_VFS_ERROR_NOT_FOUND;
		}

		if (have_result == TRUE) {
			propfind_context_clear (&pfctx);
			return result;
		}
	}

	propfind_context_clear (&pfctx);
	DEBUG_HTTP ("!! Fallthrough to head (%s)", mate_vfs_result_to_string (result));

 head_start:
	req  = ne_request_create (context->session, "HEAD", context->path);

	res = ne_request_dispatch (req);

	if (res == NE_REDIRECT) {
		ne_request_destroy (req);
		req = NULL;

		result = http_follow_redirect (context);		
	
		if (result == MATE_VFS_OK) {
			goto head_start;
		} else {
			return result;
		}
	}


	result = resolve_result (res, req);	
	
	if (result == MATE_VFS_OK) {
		const char *name;	
		
		name = mate_vfs_uri_get_path (context->uri);
	
		mate_vfs_file_info_clear (info);
		
		info->name  = g_path_get_basename (name);
		info->type  = MATE_VFS_FILE_TYPE_REGULAR;
		info->flags = MATE_VFS_FILE_FLAGS_NONE;
		
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;

		std_headers_to_file_info (req, info);
		
		/* work-around for broken icecast server */
		if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
		    && ! g_ascii_strcasecmp (info->mime_type, "audio/mpeg")) {
			ne_close_connection (ne_get_session (req));
		}
		
	}
	
	ne_request_destroy (req);
	
	return result;
}

static void assure_trailing_slash (HttpContext *context)
{
	char *tofree;

	if (! ne_path_has_trailing_slash (context->path)) {
		tofree = context->path;
		context->path = g_strconcat (tofree, "/", NULL);
		g_free (tofree);
	}
}

static MateVFSResult
http_list_directory (HttpContext *context, PropfindContext *pfctx)
{
	MateVFSResult result;
	ne_propfind_handler *pfh;
	ne_request *req;
	int res;

	DEBUG_HTTP_CONTEXT (context);
	
	propfind_context_init (pfctx);
	pfctx->path = context->path;
	
 propfind_start:	
	pfctx->include_target = TRUE;
	
	pfh = ne_propfind_create (context->session, context->path, 1);
	res = ne_propfind_named (pfh, file_info_props, propfind_result, pfctx);
	
	if (res == NE_REDIRECT) {
		
		ne_propfind_destroy (pfh);
		
		result = http_follow_redirect (context);
		
		if (result == MATE_VFS_OK) {
			goto propfind_start;
		} else {
			return result;
		}
	}
	
	req 	= ne_propfind_get_request (pfh);
	result 	= resolve_result (res, req);
	
	ne_propfind_destroy (pfh);

	if (result == MATE_VFS_OK && pfctx->target == NULL) {
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	
	return result;
}


static MateVFSResult
http_options (HttpContext *hctx)
{
	MateVFSResult  result;
	ne_request     *req;
	int		res;
	
 options_start:	
	req = ne_request_create (hctx->session, "OPTIONS", hctx->path);
	res = ne_request_dispatch (req);
	
	if (res == NE_REDIRECT) {
		ne_request_destroy (req);
		result = http_follow_redirect (hctx);
		
		if (result == MATE_VFS_OK) {
			goto options_start;
		} else {
			return result;
		}
	}

	result = resolve_result (res, req);
		
	if (result == MATE_VFS_OK) {
		const char *value;

		value = ne_get_response_header (req, "DAV");

		if (value != NULL) {
			hctx->dav_class = parse_dav_header (value);
		}
	
		value = ne_get_response_header (req, "Allow");

		if (value != NULL) {
			hctx->methods = parse_allow_header (value);
		}
	}

	ne_request_destroy (req);

	return result;
}


static void 
end_response (void *userdata, void *response,
   	      const ne_status *status, const char *description)
{
	guint *error = userdata;
	
	if (status && status->klass != 2) {
		*error = status->klass;
	}

}

static int
dav_request (ne_request *req, gboolean allow_redirect)
{
	ne_207_parser   *p207;
	ne_xml_parser   *p;
	guint            error;
	int 	         res;
	ne_status *status;

	p    = ne_xml_create();
	p207 = ne_207_create (p, &error);
    
	error = 0;
	
	ne_207_set_response_handlers (p207, NULL /*start response */, end_response);
	ne_207_set_propstat_handlers (p207, NULL, NULL /* end_propstat */);
    
	ne_add_response_body_reader (req, ne_accept_207, ne_xml_parse_v, p);
	
	res = ne_request_dispatch (req);
	status = (ne_status *) ne_get_status (req);
    
	if (status->code == 207) {
		if (ne_xml_failed (p))
			res = NE_ERROR;  
		if (error != 0) {
			status->code = error;
			status->klass = error % 100;
		}
	} else if (status->klass != 2 &&
		   (!allow_redirect || res != NE_REDIRECT)) {
		return NE_ERROR;
	}
	
	return res;
}
 


/* ************************************************************************** */
/* File operations */


typedef struct {
	
	HttpContext        *context;
	MateVFSOpenMode    mode;
	MateVFSFileInfo   *info;
	MateVFSFileOffset  offset;
	char               *etag;
	
	/* Whether we do/can ranged gets or not */
	gboolean can_range;
	gboolean use_range;
	
	union {
	
		ne_request *read;
		GByteArray *write;
		
	} transfer;
	
	enum {
		TRANSFER_IDLE,
		TRANSFER_READ,
		TRANSFER_WRITE,
		TRANSFER_ERROR
		
	} transfer_state;
	
	MateVFSResult last_error;
	
} HttpFileHandle;

static MateVFSResult
http_file_handle_new (MateVFSURI *uri, 
		      HttpFileHandle **handle_out, 
		      MateVFSOpenMode mode)
{
	HttpFileHandle *handle;
	HttpContext *hctx;
	MateVFSResult result;
	
	result = http_context_open (uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;
	
	handle = g_new0 (HttpFileHandle, 1);
	
	handle->context = hctx;
	handle->mode = mode;
	handle->transfer_state = TRANSFER_IDLE;

	handle->info = mate_vfs_file_info_new ();
	
	*handle_out = handle;
	return MATE_VFS_OK;
}

static void
http_transfer_abort (HttpFileHandle *handle)
{
	
	if (handle->transfer_state == TRANSFER_READ) {
	
		ne_end_request (handle->transfer.read);
		
		/* We need a new connection :( */
		ne_close_connection(handle->context->session);
		ne_request_destroy (handle->transfer.read);
		
		handle->transfer_state = TRANSFER_IDLE;
		handle->transfer.read = NULL;
		
	} else if (handle->transfer_state == TRANSFER_WRITE) {
		
		g_byte_array_free (handle->transfer.write, TRUE);
	}
}


#define is_transfering(_t) (_t == TRANSFER_READ || _t == TRANSFER_WRITE)

static void
http_file_handle_destroy (HttpFileHandle *handle)
{
	
	if (is_transfering (handle->transfer_state))
		http_transfer_abort (handle);
	
	if (handle->context) {
		http_context_free (handle->context);
	}
	
	mate_vfs_file_info_unref (handle->info);
	
	g_free (handle);
}

static MateVFSResult
http_transfer_start_write (HttpFileHandle *handle)
{
	MateVFSResult  result;
	HttpContext    *hctx;

	hctx = handle->context;
	
	/* Check to see if we have any file information. If not fetch it!*/
	if (handle->info->valid_fields == 0) {
		result = http_get_file_info (hctx, handle->info);

		if (result != MATE_VFS_OK) {
			return result;
		}
	}
	
	if (handle->info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
		result = MATE_VFS_ERROR_IS_DIRECTORY;
		return result;
	}
	
			
	/* write seeks only for zero length files (eg newly created ones) */
	if (handle->mode & MATE_VFS_OPEN_RANDOM && 
		!((handle->info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SIZE) ||
			handle->info->size != 0)) {
		result = MATE_VFS_ERROR_NOT_SUPPORTED;
		return result;		
	}
	
	handle->transfer.write = g_byte_array_new ();
	handle->transfer_state = TRANSFER_WRITE;

	return MATE_VFS_OK;
}


static MateVFSResult
http_transfer_start_read (HttpFileHandle *handle)
{
	MateVFSResult result;
	HttpContext *hctx;
	ne_request  *req;
	int 	     res;
	const ne_status *status;
	
	if (handle->transfer_state == TRANSFER_READ)
		return MATE_VFS_OK;
	
	hctx = handle->context;
	
get_start:	
	req = ne_request_create (hctx->session, "GET", hctx->path);
	
	if (handle->use_range) {
		
		handle->can_range = TRUE;
		
		ne_print_request_header (req, "Range",
					 "bytes=%"MATE_VFS_OFFSET_FORMAT_STR"-",
					 handle->offset);

		ne_add_request_header (req, "Accept-Ranges", "bytes");
	}

get_retry:

	res    = ne_begin_request (req);
	result = resolve_result (res, req);
	status = ne_get_status (req);
	
	DEBUG_HTTP ("[GET] %s, %d, %d", mate_vfs_result_to_string (result), res, status->code);
	
	if (res != NE_OK && res != NE_REDIRECT) {
		handle->transfer_state = TRANSFER_ERROR;
		handle->last_error = result;
		ne_request_destroy (req);
		return result;
	}
	
	if (IS_REDIRECT (status->code) || IS_AUTH_REQ (status->code)) {
		/* We are not interested in the body */
		res = ne_discard_response (req);	
		
		if (res < 0) {
			handle->transfer_state = TRANSFER_ERROR;
			result = MATE_VFS_ERROR_IO;
			handle->last_error = result;
			ne_request_destroy (req);
			return result;
		}
		
		res = ne_end_request (req);

		if (res == NE_RETRY) {
			goto get_retry;
		}
		
		ne_request_destroy (req);
		req = NULL;
		
		if (res == NE_REDIRECT) {
			result = http_follow_redirect (hctx);
	
			if (result == MATE_VFS_OK)
				goto get_start;
		}
	} 
	
	if (result == MATE_VFS_OK) { 
		/* 2xx .. success */
	
		std_headers_to_file_info (req, handle->info);
		
		if (handle->use_range && status->code != 206) {
			DEBUG_HTTP ("[GET] {ranged} disabled");
			handle->can_range = FALSE;
		}
	
		/* If we are in a GET we invoke the callback of received headers 
		   right before reading the data because we might be in a 
		   stream and we wanna have the headers callbac invoked ASAP
		*/
		neon_return_headers (req, NULL, status);

		handle->transfer_state = TRANSFER_READ;
		handle->transfer.read = req;
	} 

	return result;
}

static MateVFSResult
http_transfer_start (HttpFileHandle *handle)
{
	if (handle->mode & MATE_VFS_OPEN_READ) {
		return http_transfer_start_read (handle);
	} else {
		return http_transfer_start_write (handle);
	}

	return MATE_VFS_ERROR_INTERNAL;
}

/* TRUE means we can range, ie server is sane, FALSE means FAIL */
static gboolean
i_can_haz_range_cause_serverz_not_br0ken (HttpContext *hctx)
{
        gboolean br0ken = FALSE;

        if (http_context_host_matches (hctx, "*youtube.*")) {
                DEBUG_HTTP ("Youtube detected! Cannot use ranged gets");
                br0ken = TRUE;
        }

        return br0ken == FALSE;
}

/* ************************************************************************** */

static MateVFSResult
do_open (MateVFSMethod 	*method,
	 MateVFSMethodHandle  **method_handle,
	 MateVFSURI 		*uri,
	 MateVFSOpenMode 	 mode,
	 MateVFSContext 	*context)
{
	HttpContext *hctx;
	HttpFileHandle *handle;
	MateVFSResult result;
	
	DEBUG_HTTP_FUNC (1);
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);
	
	if (mode & MATE_VFS_OPEN_READ) {
		if (mode & MATE_VFS_OPEN_WRITE)
			return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	} else {
		if (!(mode & MATE_VFS_OPEN_WRITE))
			return MATE_VFS_ERROR_INVALID_OPEN_MODE;
		
	}
	
	result = http_file_handle_new (uri, &handle, mode);
	
	if (result != MATE_VFS_OK)
		return result;
	
	hctx = handle->context;

	if (mode & MATE_VFS_OPEN_WRITE) {
		if ((result = http_options (hctx)) != MATE_VFS_OK) {
			http_file_handle_destroy (handle);
			return result;
		}

		/*  OPTIONS worked fine! Now are we allowed to put ?

	   		Note: It is not asured that the PUT is in the OPTIONS 
			response even if we would be allowed to do a PUT but 
			we do not have any other options to the tell. If I 
			looked correctly caja does a DELETE and do_create 
			anyway.
		*/

		if (!(hctx->methods & ALLOW_PUT)) {
			http_file_handle_destroy (handle);
			result = MATE_VFS_ERROR_READ_ONLY;
			return result;
		} 
	} else {
		handle->use_range = i_can_haz_range_cause_serverz_not_br0ken (hctx);
                DEBUG_HTTP ("Use range: %s\n", handle->use_range ? "on" : "off");
	}

	result = http_transfer_start (handle);	

	if (result != MATE_VFS_OK) {
		http_file_handle_destroy (handle);
		handle = NULL;
	}
	
	*method_handle = (MateVFSMethodHandle *) handle;
	
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult
do_create (MateVFSMethod	 *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI 		 *uri,
	   MateVFSOpenMode 	  mode,
	   gboolean 		  exclusive,
	   guint 		  perm,
	   MateVFSContext 	 *context)
{
	HttpFileHandle *handle;
	HttpContext    *hctx;
	MateVFSResult result;
	int res;
	ne_request *req;
	
	DEBUG_HTTP_FUNC (1);
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);

	if ((mode & MATE_VFS_OPEN_READ))
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	
	result = http_file_handle_new (uri, &handle, mode);

	if (result != MATE_VFS_OK)
		return result;

	hctx = handle->context;	

put_start:
	req = ne_request_create (hctx->session, "PUT", hctx->path);

	/* Apache seems to not handle "If-None-Match: *" correctly 
	   so we don't use them */
#if 0	
	if (exclusive == TRUE)	
		ne_add_request_header (req, "If-None-Match", "*");
#endif
	
	if (exclusive == TRUE) {
		result = http_get_file_info (hctx, handle->info);
		
		if (result != MATE_VFS_ERROR_NOT_FOUND) {
			http_file_handle_destroy (handle);
			ne_request_destroy (req);
			return MATE_VFS_ERROR_FILE_EXISTS;
		}
	}
	
	ne_set_request_body_buffer (req, NULL, 0);
	
	res = ne_request_dispatch (req);

	if (res == NE_REDIRECT) {
		ne_request_destroy (req);
		req = NULL;
		
		result = http_follow_redirect (hctx);
		
		if (result == MATE_VFS_OK) {
			goto put_start;
		} else {
			return result;
		}
	}

	result 	= resolve_result (res, req);
	ne_request_destroy (req);
	
	if (result == MATE_VFS_OK && mode != MATE_VFS_OPEN_NONE) {

		handle->info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
		handle->info->size = 0;
		handle->info->type = MATE_VFS_FILE_TYPE_REGULAR;
		handle->info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;

		result = http_transfer_start (handle);
	}

	if (result != MATE_VFS_OK) {
		http_file_handle_destroy (handle);
		handle = NULL;
	} 

	*method_handle = (MateVFSMethodHandle *) handle;	

	DEBUG_HTTP_FUNC (0);
	return result;
	
}

static MateVFSResult
do_close (MateVFSMethod 	*method,
	  MateVFSMethodHandle  *method_handle,
	  MateVFSContext 	*context)
{
	MateVFSResult result;
	HttpFileHandle *handle;
	HttpContext    *ctx;
	int res;

	DEBUG_HTTP_FUNC (1);
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	ctx = handle->context;
	result = MATE_VFS_OK;
	
	if (handle->transfer_state == TRANSFER_WRITE &&
	    handle->transfer.write->len != 0) {
		ne_request    		*req;
		guint8			*data;
		guint			 len;
		
		req = ne_request_create (ctx->session, "PUT", ctx->path);

		data = handle->transfer.write->data;
		len = handle->transfer.write->len;
		   
		DEBUG_HTTP ("[PUT] Filesize: %d", len);
		
		ne_set_request_body_buffer (req, (char *)data, len);
		res = ne_request_dispatch (req);

		result = resolve_result (res, req);
		DEBUG_HTTP ("[PUT] returned: %d, %d (%s)", res, 
				ne_get_status (req)->code, 
				mate_vfs_result_to_string (result));
		ne_request_destroy (req);
	}
	
	
	http_file_handle_destroy (handle);
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult
do_read (MateVFSMethod 	*method,
	 MateVFSMethodHandle 	*method_handle,
	 gpointer 		 buffer,
	 MateVFSFileSize 	 num_bytes,
	 MateVFSFileSize 	*bytes_read,
	 MateVFSContext 	*context)
{
	MateVFSResult result;
	HttpFileHandle *handle;
	ssize_t n;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	result = MATE_VFS_OK;
	
	if (handle->mode & MATE_VFS_OPEN_WRITE)
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	
	if (handle->transfer_state == TRANSFER_ERROR) {
		return handle->last_error;
	}
	
	if (handle->transfer_state == TRANSFER_IDLE) {
		result = http_transfer_start (handle);
		
		if (result != MATE_VFS_OK)
			return result;
	}
	
	n = ne_read_response_block (handle->transfer.read, buffer, num_bytes);

	if (n < 1) {
				
		if (n == 0) {
			ne_end_request (handle->transfer.read);
			
			result = MATE_VFS_ERROR_EOF;
			handle->transfer_state = TRANSFER_IDLE;
		} else {
			result = MATE_VFS_ERROR_IO;
			handle->transfer_state = TRANSFER_ERROR;
		}
		
		ne_request_destroy (handle->transfer.read);
		handle->transfer.read = NULL;
		handle->last_error = result;
		handle->offset = 0;
		*bytes_read = 0;
		
		DEBUG_HTTP ("[read] error during read: %s", 
				mate_vfs_result_to_string (result));

		return result;
	}
	
		
	/* cast is valid because n must be greater than 0 */
	*bytes_read = n;

	DEBUG_HTTP ("[read] bytes read %" MATE_VFS_SIZE_FORMAT_STR,
	            *bytes_read);

	handle->offset += *bytes_read;
	return result;
}


static MateVFSResult
do_write (MateVFSMethod 	*method,
	  MateVFSMethodHandle  *method_handle,
	  gconstpointer 	 buffer,
	  MateVFSFileSize 	 num_bytes,
	  MateVFSFileSize 	*bytes_written,
	  MateVFSContext 	*context)
{
	HttpFileHandle *handle;
	GByteArray     *ba;
	const guint8   *pos;
	gint		over_len;
	int		i;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	
	if (handle->mode & MATE_VFS_OPEN_READ)
		return MATE_VFS_ERROR_READ_ONLY;
	
	if (num_bytes == 0)
		return MATE_VFS_OK;
	
	ba = handle->transfer.write;
	
	while (ba->len < handle->offset) {
		guint8 null = 0;
		ba = g_byte_array_append (ba, &null, 1);
		DEBUG_HTTP ("[write] zero fill");
	}
	
	pos = buffer;
	over_len = MIN (ba->len - handle->offset, num_bytes);
	DEBUG_HTTP ("over_len %d", over_len); 
	
	for (i = over_len; i > 0; i--) {
		ba->data[handle->offset] = *pos;
		pos++;
		handle->offset++;
		DEBUG_HTTP ("%s, %" MATE_VFS_SIZE_FORMAT_STR
		            ", %" MATE_VFS_OFFSET_FORMAT_STR,
		            (char *) pos, num_bytes, handle->offset);
	}
	
	DEBUG_HTTP ("[write] %" MATE_VFS_OFFSET_FORMAT_STR
	            ", %d, %" MATE_VFS_SIZE_FORMAT_STR,
	            handle->offset, ba->len, num_bytes);
	ba = g_byte_array_append (ba, pos, num_bytes - over_len);
	
	handle->offset += num_bytes;
	
	if (bytes_written != NULL)
		*bytes_written = num_bytes;

	handle->transfer.write = ba;

	/* FIXME: set file size info here ?? */
	
	return MATE_VFS_OK;
}


static MateVFSResult
do_seek (MateVFSMethod	      *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition  whence,
	 MateVFSFileOffset    offset,
	 MateVFSContext      *context)
{
	MateVFSResult           result;
	HttpFileHandle 		*handle;
	MateVFSFileOffset  	 new_position;

	DEBUG_HTTP_FUNC (1);
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	
	if (handle->mode & MATE_VFS_OPEN_READ && handle->can_range != TRUE) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
		
	result = MATE_VFS_OK;
	
	switch (whence) {
	
	case MATE_VFS_SEEK_START:
		
		new_position = offset;
		break;
	
	case MATE_VFS_SEEK_END:
	
		/* We need to know the file size for that */
		if (!(handle->info->valid_fields & 
		      MATE_VFS_FILE_INFO_FIELDS_SIZE)) {
			DEBUG_HTTP ("[seek] do not know file-size not seeking ...");
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		}
		
		new_position = handle->info->size + offset;
		break;
		
	case MATE_VFS_SEEK_CURRENT:
		
		new_position = handle->offset + offset;
		break;
	
	default:
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	
	}
	
	/* FIXME: is this error correct ? */
	if (new_position < 0) {
		DEBUG_HTTP ("seeking to %" MATE_VFS_OFFSET_FORMAT_STR,
		            new_position);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	/* if we shall seek to where we already are just
	 * return OK */
	if (handle->offset == new_position) {
		return MATE_VFS_OK;
	}
	
	handle->offset = new_position;
	
	if (handle->transfer_state == TRANSFER_READ) {
		http_transfer_abort (handle);
	}
	
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult
do_tell (MateVFSMethod       *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize   *offset_return)
{
	HttpFileHandle *handle;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	
	*offset_return = handle->offset;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	HttpFileHandle *handle;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	
	if (handle->mode & MATE_VFS_OPEN_READ)
		return MATE_VFS_ERROR_READ_ONLY;
	
	
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_open_directory (MateVFSMethod 	    *method,
		   MateVFSMethodHandle    **method_handle,
		   MateVFSURI 		    *uri,
		   MateVFSFileInfoOptions   options,
		   MateVFSContext 	    *context) 
{
	MateVFSResult  result;
	HttpContext    *hctx;
	PropfindContext *pfctx;

	DEBUG_HTTP_FUNC (1);

	if (scheme_is_dav (uri) == FALSE) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	result = http_context_open  (uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;

	assure_trailing_slash (hctx);
	result = http_options (hctx);

	if (result != MATE_VFS_OK || hctx->dav_class == NO_DAV) {
		http_context_free (hctx);
		
		if (result != MATE_VFS_OK)
			return result;
		
		return MATE_VFS_ERROR_NOT_A_DIRECTORY;
	}
		
	pfctx = g_new0 (PropfindContext, 1);
	
	result = http_list_directory (hctx, pfctx);
	
	http_context_free (hctx);
	
	if (result == MATE_VFS_ERROR_NOT_SUPPORTED ||
	    (result == MATE_VFS_OK && pfctx->target->type 
	     != MATE_VFS_FILE_TYPE_DIRECTORY)) {
		
		result = MATE_VFS_ERROR_NOT_A_DIRECTORY;
	}
	
	if (result != MATE_VFS_OK) {
		propfind_context_clear (pfctx);
		g_free (pfctx);
	} else {
		*method_handle 	= (MateVFSMethodHandle  *) pfctx;
	}
	
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult
do_close_directory (MateVFSMethod 	 *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext 	 *context) 
{
	PropfindContext *pfctx;
	
	DEBUG_HTTP_FUNC (1);
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);

	pfctx = (PropfindContext *) method_handle;
	
	propfind_context_clear (pfctx);
	g_free (pfctx);
	
	DEBUG_HTTP_FUNC (0);
	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod 	*method,
       		   MateVFSMethodHandle *method_handle,
       		   MateVFSFileInfo 	*file_info,
		   MateVFSContext 	*context)
{
	PropfindContext *pfctx;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);

	pfctx = (PropfindContext *) method_handle;

	if (pfctx->children) {
		MateVFSFileInfo *info = pfctx->children->data;
		
		mate_vfs_file_info_copy (file_info, info); 
		pfctx->children = g_list_remove (pfctx->children, info);
		mate_vfs_file_info_unref (info);
	
		return MATE_VFS_OK;
	}
	
	return MATE_VFS_ERROR_EOF;
}

static MateVFSResult
do_get_file_info (MateVFSMethod 		*method,
		  MateVFSURI 			*uri,
		  MateVFSFileInfo 		*file_info,
		  MateVFSFileInfoOptions 	 options,
		  MateVFSContext 		*context)
{
	MateVFSResult  result;
	HttpContext    *hctx;

	DEBUG_HTTP_FUNC (1);

	result = http_context_open (uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;
		
	result = http_get_file_info (hctx, file_info);
	
	http_context_free (hctx);
	
	DEBUG_HTTP_FUNC (0);	
	return result;
}


static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod 		*method,
			      MateVFSMethodHandle 	*method_handle,
			      MateVFSFileInfo 		*file_info,
			      MateVFSFileInfoOptions    options,
			      MateVFSContext 		*context)
{
	HttpFileHandle *handle;
	MateVFSResult  result;
	
	DEBUG_HTTP_FUNC (1);
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	
	handle = (HttpFileHandle *) method_handle;
	
	if (handle->transfer_state == TRANSFER_READ) {
		mate_vfs_file_info_copy (file_info, handle->info);
		return MATE_VFS_OK;
	}
	
	result = http_get_file_info (handle->context, handle->info);

	if (result == MATE_VFS_OK) {
		mate_vfs_file_info_copy (file_info, handle->info);
	}

	/* Review: updated the file size in info to the length of the write
	   buffer. ? */
	
	return result;

}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return FALSE;
}

static MateVFSResult
do_make_directory (MateVFSMethod  *method, 
		   MateVFSURI     *uri,
                   guint 	    perm, 
		   MateVFSContext *context) 
{
	MateVFSResult    result;
	HttpContext      *hctx;
	MateVFSURI      *uri_parent;
	ne_request       *req;
	int 		  res;
	
	DEBUG_HTTP_FUNC (1);

	if (scheme_is_dav (uri) == FALSE) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
		
	uri_parent = mate_vfs_uri_get_parent (uri);
	
	result = http_context_open (uri_parent, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;
	
	result = http_options (hctx);
	
	if (result != MATE_VFS_OK || (hctx->dav_class == NO_DAV)) {

		if (result == MATE_VFS_OK)
			result = MATE_VFS_ERROR_NOT_SUPPORTED;
		
		goto out;
	}
	
	http_context_set_uri (hctx, uri);

 mkcol_start:
	req = ne_request_create (hctx->session, "MKCOL", hctx->path);
	res = ne_request_dispatch (req);

	if (res == NE_REDIRECT) {
		result = http_follow_redirect (hctx);
		
		if (result == MATE_VFS_OK)
			ne_request_destroy (req);
			req = NULL;
			goto mkcol_start;
	} else if (res == NE_OK) {
		const ne_status *status = ne_get_status (req);
		if (status->code == 409)
			result = MATE_VFS_ERROR_NOT_FOUND;
		else if (status->code == 405) 
			result = MATE_VFS_ERROR_FILE_EXISTS;
	} else {
		result = resolve_result (res, req);
	}
	
	ne_request_destroy (req);
	
 out:	
	mate_vfs_uri_unref (uri_parent);
	http_context_free (hctx);
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult 
do_remove_directory (MateVFSMethod  *method, 
		     MateVFSURI     *uri, 
		     MateVFSContext *context) 
{
	MateVFSResult  result;
	HttpContext    *hctx;
	PropfindContext  pfctx;
	ne_request     *req;
	int  		res;
	
	DEBUG_HTTP_FUNC (1);

	if (scheme_is_dav (uri) == FALSE) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	result = http_context_open  (uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;

	assure_trailing_slash (hctx);
	
	propfind_context_init (&pfctx);
	result = http_list_directory (hctx, &pfctx);
	
	if (result != MATE_VFS_OK) {
		goto out;	
	}
	
	if (pfctx.target->type != MATE_VFS_FILE_TYPE_DIRECTORY) {
		result = MATE_VFS_ERROR_NOT_A_DIRECTORY;
		goto out;
	}
		
	if (pfctx.children != NULL) {
		result = MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY;
	}
	
	req = ne_request_create (hctx->session, "DELETE", hctx->path);
	res = dav_request (req, FALSE);
	
	result = resolve_result (res, req);
	ne_request_destroy (req);
	
 out:	
	propfind_context_clear (&pfctx);
	http_context_free (hctx);
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSURI *
resolve_schema_alias (MateVFSURI *uri)
{
	MateVFSURI *resolved;

	resolved = mate_vfs_uri_dup (uri);
	g_free (resolved->method_string);
	resolved->method_string = g_strdup (resolve_alias (mate_vfs_uri_get_scheme (uri)));
	return resolved;
}


static MateVFSResult
do_move (MateVFSMethod  *method,
	 MateVFSURI 	 *old_uri,
	 MateVFSURI 	 *new_uri,
	 gboolean         force_replace,
	 MateVFSContext *context)
{
	MateVFSResult result;
	HttpContext *hctx;
	char	    *dest;
	ne_request  *req;
	int 	     res;
	MateVFSURI *dest_uri;
	
	DEBUG_HTTP_FUNC (1);

	if (scheme_is_dav (old_uri) == FALSE) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	if (! http_session_uri_equal (old_uri, new_uri)) {
		return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	}	

	result = http_context_open (old_uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;

	dest_uri = resolve_schema_alias (new_uri);
	dest = mate_vfs_uri_to_string (dest_uri, MATE_VFS_URI_HIDE_USER_NAME | 
					MATE_VFS_URI_HIDE_PASSWORD);
	mate_vfs_uri_unref (dest_uri);
	

 head_start:
	req = ne_request_create (hctx->session, "MOVE", hctx->path);
	
	ne_add_request_header (req, "Destination", dest);
    	ne_add_request_header (req, "Overwrite", force_replace ? "T" : "F");
	
	
	res = dav_request (req, TRUE);
	if (res == NE_REDIRECT) {
		result = http_follow_redirect (hctx);

		if (result == MATE_VFS_OK) {
			ne_request_destroy (req);
			req = NULL;
			goto head_start;
		}
	} else {
		result = resolve_result (res, req);
	}

	ne_request_destroy (req);
	http_context_free (hctx);
	
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult 
do_unlink (MateVFSMethod  *method,
	   MateVFSURI 	   *uri,
	   MateVFSContext *context)
{
	MateVFSResult    result;
	HttpContext      *hctx;
	MateVFSFileInfo *file_info;
	ne_request       *req;
	int 		  res;
	
	DEBUG_HTTP_FUNC (1);
	result = http_context_open (uri, &hctx);
	
	if (result != MATE_VFS_OK)
		return result;
	
	file_info = mate_vfs_file_info_new ();
	result = http_get_file_info (hctx, file_info);
	
	if (result != MATE_VFS_OK) {
		goto out;
	}
	
	if (file_info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
		result = MATE_VFS_ERROR_IS_DIRECTORY;
		goto out;
	}
	
	req = ne_request_create (hctx->session, "DELETE", hctx->path);
	res = dav_request (req, FALSE);

	result = resolve_result (res, req);
	ne_request_destroy (req);
	
 out:	
	http_context_free (hctx);
	mate_vfs_file_info_unref (file_info);
	DEBUG_HTTP_FUNC (0);
	return result;
}

static MateVFSResult 
do_check_same_fs (MateVFSMethod  *method,
		  MateVFSURI 	  *a,
		  MateVFSURI 	  *b,
		  gboolean 	  *same_fs_return,
		  MateVFSContext *context)
{
	
	*same_fs_return = http_session_uri_equal (a, b);
	
	return MATE_VFS_OK;	
}

static MateVFSResult
do_set_file_info (MateVFSMethod 		*method,
		  MateVFSURI 			*uri,
		  const MateVFSFileInfo 	*info,
		  MateVFSSetFileInfoMask 	 mask,
		  MateVFSContext 		*context)
{
	MateVFSURI *parent_uri, *new_uri;
	MateVFSResult result;
	
	DEBUG_HTTP_FUNC (1);
	
	if ((mask & ~(MATE_VFS_SET_FILE_INFO_NAME)) != 0) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	parent_uri = mate_vfs_uri_get_parent (uri);
	
	if (parent_uri == NULL) {
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	
	new_uri = mate_vfs_uri_append_file_name (parent_uri, info->name);
	mate_vfs_uri_unref (parent_uri);
	
	result = do_move (method, uri, new_uri, FALSE, context);
	
	mate_vfs_uri_unref (new_uri);
	
	DEBUG_HTTP_FUNC (0);
	
	return result;
}

static MateVFSResult
do_truncate (MateVFSMethod *method,
	     MateVFSURI *uri,
	     MateVFSFileSize where,
	     MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_find_directory (MateVFSMethod *method,
		   MateVFSURI *near_uri,
		   MateVFSFindDirectoryKind kind,
		   MateVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod *method,
			 MateVFSURI *uri,
			 const char *target_reference,
			 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_add (MateVFSMethod *method,
		MateVFSMethodHandle **method_handle_return,
		MateVFSURI *uri,
		MateVFSMonitorType monitor_type)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_file_control (MateVFSMethod *method,
		 MateVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

/* ************************************************************************** */

static MateVFSMethod http_method = {
	sizeof (MateVFSMethod),
	do_open, 
	do_create,
	do_close,
	do_read, 
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle, 
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
	do_truncate, 
	do_find_directory, 
	do_create_symbolic_link,  
	do_monitor_add,  
	do_monitor_cancel,  
	do_file_control 	
};


MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	if (module_refcount++ == 0) {
		proxy_init ();
		/* ne_debug_init (stdout, 0xfffe); */
		neon_session_pool_init ();
		http_auth_cache_init ();
		quick_allow_lookup_init ();
	}

	return &http_method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	if (--module_refcount == 0) {
		quit_allow_lookup_destroy ();
		http_auth_cache_shutdown ();
		neon_session_pool_shutdown ();
		proxy_shutdown ();
	}
}
