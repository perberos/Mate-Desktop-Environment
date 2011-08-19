/* smb-method.h - VFS modules for SMB
 *
 * Docs:
 * http://www.ietf.org/internet-drafts/draft-crhertel-smb-url-05.txt
 * http://samba.org/doxygen/samba/group__libsmbclient.html
 * http://ubiqx.org/cifs/
 *
 *  Copyright (C) 2001,2002,2003 Red Hat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Author: Bastien Nocera <hadess@hadess.net>
 *         Alexander Larsson <alexl@redhat.com>
 * 	   Nate Nielsen <nielsen@memberwebs.com>
 */

#include <config.h>

/* libgen first for basename */
#include <libgen.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mateconf/mateconf-client.h>
#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-mime.h>

#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module-callback-module-api.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>

#include <glib/gi18n-lib.h>

#include <libsmbclient.h>

int smbc_remove_unused_server(SMBCCTX * context, SMBCSRV * srv);

typedef enum {
	SMB_URI_ERROR,
	SMB_URI_WHOLE_NETWORK,
	SMB_URI_WORKGROUP_LINK,
	SMB_URI_WORKGROUP,
	SMB_URI_SERVER_LINK,
	SMB_URI_SERVER,
	SMB_URI_SHARE,
	SMB_URI_SHARE_FILE
}  SmbUriType;

typedef struct {
	char *server_name;
	char *share_name;
	char *domain;
	char *username;
	
	SMBCSRV *server;
	time_t last_time;
} SmbServerCacheEntry;

typedef struct {
	char *username;
	char *domain;
        char *password;
        time_t stamp;
} SmbCachedUser;

static GMutex *smb_lock;

static SMBCCTX *smb_context = NULL;

static GHashTable *server_cache = NULL;

static GHashTable *user_cache = NULL;

#define SMB_BLOCK_SIZE (32*1024)

/* Reap unused server connections and user cache after 30 minutes */
#define CACHE_REAP_TIMEOUT (30 * 60)
static guint cache_reap_timeout = 0;

/* We load a default workgroup from mateconf */
#define PATH_MATECONF_MATE_VFS_SMB_WORKGROUP "/system/smb/workgroup"

/* The magic "default workgroup" hostname */
#define DEFAULT_WORKGROUP_NAME "X-MATE-DEFAULT-WORKGROUP"

/* Guest logins use: */
#define GUEST_LOGIN "guest"

/* 5 minutes before we re-read the workgroup cache again */
#define WORKGROUP_CACHE_TIMEOUT (5*60)

static GHashTable *workgroups = NULL;
static time_t workgroups_timestamp = 0;

/* Authentication ----------------------------------------------------------- */

#define SMB_AUTH_STATE_PREFILLED	0x00000010 	/* Have asked mate-auth for prefilled auth */
#define SMB_AUTH_STATE_GUEST		0x00000020 	/* Have tried 'guest' authentication */
#define SMB_AUTH_STATE_PROMPTED		0x00000040 	/* Have asked mate-auth for to prompt user */

typedef struct _SmbAuthContext {
	
	MateVFSURI *uri;		/* Uri being worked with. Does not own this URI */
	MateVFSResult res;		/* Current error code */
	
	/* Internal state */
	guint passes;			/* Number of passes through authentication code */
	guint state;			/* Various flags (above) */
	
	/* For saving passwords into mate-keyring */
	gboolean save_auth;		/* Whether to save the authentication or not */
	gchar *keyring;			/* The keyring to save passwords in */
	
	/* Used in chat between perform_authentication and auth_callback */
	gboolean auth_called;		/* Set by auth_callback */
	gboolean preset_user;		/* Set when we have a preset user from the URI */
	gchar *for_server;		/* Set by auth_callback */
	gchar *for_share;		/* Set by auth_callback */
	gchar *use_user;		/* Set by perform_authentication */
	gchar *use_domain;		/* Set by perform_authentication */
	gchar *use_password;		/* Set by perform_authentication */

	gboolean cache_added;           /* Set when cache is added to during authentication */
	gboolean cache_used;            /* Set when cache is used during authentication */

	guint prompt_flags;           	/* Various URI flags set by initial_authentication */
	
} SmbAuthContext;

static void init_authentication (SmbAuthContext *actx, MateVFSURI *uri);
static int  perform_authentication (SmbAuthContext *actx);

static SmbAuthContext *current_auth_context = NULL;

static void auth_callback (const char *server_name, const char *share_name,
		     	   char *domain, int domainmaxlen,
		     	   char *username, int unmaxlen,
		     	   char *password, int pwmaxlen);
		     	   
#if 0
#define DEBUG_SMB_ENABLE
#define DEBUG_SMB_LOCKS
#endif

#ifdef DEBUG_SMB_ENABLE
#define DEBUG_SMB(x) g_print x
#else
#define DEBUG_SMB(x) 
#endif

#ifdef DEBUG_SMB_LOCKS
#define LOCK_SMB() 	{g_mutex_lock (smb_lock); g_print ("LOCK %s\n", G_STRFUNC);}
#define UNLOCK_SMB() 	{g_print ("UNLOCK %s\n", G_STRFUNC); g_mutex_unlock (smb_lock);}
#else
#define LOCK_SMB() 	g_mutex_lock (smb_lock)
#define UNLOCK_SMB() 	g_mutex_unlock (smb_lock)
#endif

static gchar*
string_dup_nzero (const gchar *s)
{
	if (!s || !s[0])
		return NULL;
	return g_strdup (s);
}

static gchar*
string_ndup_nzero (const gchar *s, const guint n)
{
	if(!s || !s[0] || !n)
		return NULL;
	return g_strndup (s, n);
}

static const char*
string_nzero  (const gchar *s)
{
	if (!s || !s[0])
		return NULL;
	return s;
}
	     	   
static gboolean
string_compare (const char *a, const char *b)
{
	if (a != NULL && b != NULL) {
		return strcmp (a, b) == 0;
	} else {
		return a == b;
	}
}

static gchar*
string_realloc (gchar *dest, const gchar *src)
{
        if (string_compare (src, dest))
                return dest;
        g_free (dest);
        return string_dup_nzero (src);
}

static gboolean
server_equal (gconstpointer  v1,
	      gconstpointer  v2)
{
	const SmbServerCacheEntry *e1, *e2;

	e1 = v1;
	e2 = v2;

	return (string_compare (e1->server_name, e2->server_name) &&
		string_compare (e1->share_name, e2->share_name) &&
		string_compare (e1->domain, e2->domain) &&
		string_compare (e1->username, e2->username));
}

static guint
server_hash (gconstpointer  v)
{
	const SmbServerCacheEntry *e;
	guint hash;

	e = v;
	hash = 0;
	if (e->server_name != NULL) {
		hash = hash ^ g_str_hash (e->server_name);
	}
	if (e->share_name != NULL) {
		hash = hash ^ g_str_hash (e->share_name);
	}
	if (e->domain != NULL) {
		hash = hash ^ g_str_hash (e->domain);
	}
	if (e->username != NULL) {
		hash = hash ^ g_str_hash (e->username);
	}

	return hash;
}

static void
server_free (SmbServerCacheEntry *entry)
{
	g_free (entry->server_name);
	g_free (entry->share_name);
	g_free (entry->domain);
	g_free (entry->username);

	/* TODO: Should we remove the server here? */
	
	g_free (entry);
}

static void
user_free (SmbCachedUser *user)
{
        g_free (user->username);
        g_free (user->domain);
        g_free (user->password);
        g_free (user);
}

static void
add_old_servers (gpointer key,
		gpointer value,
		gpointer user_data)
{
	SmbServerCacheEntry *entry;
	GPtrArray *array;
	time_t now;

	now = time (NULL);

	entry = key;
	array = user_data;
	if (now > entry->last_time + CACHE_REAP_TIMEOUT || now < entry->last_time)
		g_ptr_array_add (array, entry->server);
}

static gboolean
reap_user (gpointer key, gpointer value, gpointer user_data)
{
        SmbCachedUser *user = (SmbCachedUser*)value;
        time_t now =  time (NULL);

        if (now > user->stamp + CACHE_REAP_TIMEOUT || now < user->stamp) 
                return TRUE;
        return FALSE;              
}

static gboolean
cache_reap_cb (void)
{
	GPtrArray *servers;
        gboolean ret;
        int size;
	int i;
        
        /* Don't deadlock here, this is callback and we're not completely
         * sure when we'll be called */
        if (!g_mutex_trylock (smb_lock))
                return TRUE;
        DEBUG_SMB(("LOCK %s\n", G_STRFUNC));

	size = g_hash_table_size (server_cache);
	servers = g_ptr_array_sized_new (size);

	/* The remove can change the hashtable, make a copy */
	g_hash_table_foreach (server_cache, add_old_servers, servers);
		
	for (i = 0; i < servers->len; i++) {
		smbc_remove_unused_server (smb_context,
					   (SMBCSRV *)g_ptr_array_index (servers, i));
	}

	g_ptr_array_free (servers, TRUE);
     
        /* Cleanup users */
        g_hash_table_foreach_remove (user_cache, reap_user, NULL);

        ret = !(g_hash_table_size (server_cache) == 0 && 
                g_hash_table_size (user_cache) == 0);
        if (!ret)
                cache_reap_timeout = 0;

        UNLOCK_SMB();

        return ret;        
}

static void
schedule_cache_reap (void)
{
	if (cache_reap_timeout == 0) {
		cache_reap_timeout = g_timeout_add (CACHE_REAP_TIMEOUT * 1000,
						    (GSourceFunc)cache_reap_cb, NULL);
	}
}

static int
add_cached_server (SMBCCTX *context, SMBCSRV *new,
		   const char *server_name, const char *share_name, 
		   const char *domain, const char *username)
{
	SmbServerCacheEntry *entry = NULL;

	DEBUG_SMB(("[auth] adding cached server: server: %s, share: %s, domain: %s, user: %s\n",
		   server_name ? server_name : "", 
		   share_name ? share_name : "",
		   domain ? domain : "",
		   username ? username : ""));
	
	schedule_cache_reap ();
	
	entry = g_new0 (SmbServerCacheEntry, 1);
	
	entry->server = new;
	
	entry->server_name = string_dup_nzero (server_name);
	entry->share_name = string_dup_nzero (share_name);
	entry->domain = string_dup_nzero (domain);
	entry->username = string_dup_nzero (username);
	entry->last_time = time (NULL);

	g_hash_table_insert (server_cache, entry, entry);
	current_auth_context->cache_added = TRUE;
	return 0;
}

static SMBCSRV *
find_cached_server (const char *server_name, const char *share_name,
                    const char *domain, const char *username)
{
	SmbServerCacheEntry entry;
	SmbServerCacheEntry *res;

	DEBUG_SMB(("find_cached_server: server: %s, share: %s, domain: %s, user: %s\n", 
		   server_name ? server_name : "", 
		   share_name ? share_name : "",
		   domain ? domain : "", 
		   username ? username : ""));

	/* "" must be treated as NULL, because add_cached_server() uses string_dup_nzero() */
	entry.server_name = (char *) string_nzero (server_name);
	entry.share_name = (char *) string_nzero (share_name);
	entry.domain = (char *) string_nzero (domain);
	entry.username = (char *) string_nzero (username);

	res = g_hash_table_lookup (server_cache, &entry);

	if (res != NULL) {
		res->last_time = time (NULL);
		return res->server;
	} 

	return NULL;
}

static SMBCSRV *
get_cached_server (SMBCCTX * context,
		   const char *server_name, const char *share_name,
		   const char *domain, const char *username)
{
	SMBCSRV *srv;
	
	srv = find_cached_server (server_name, share_name, domain, username);
	if (srv != NULL) {
		DEBUG_SMB(("got cached server: server: %s, share: %s, domain: %s, user: %s\n",
		   	   server_name ? server_name : "", 
			   share_name ? share_name : "",
			   domain ? domain : "", 
			   username ? username : ""));
		current_auth_context->cache_used = TRUE;
		return srv;
	}
	return NULL;
}

static gboolean
remove_server  (gpointer key,
		gpointer value,
		gpointer user_data)
{
	SmbServerCacheEntry *entry;
	
	entry = key;

	if (entry->server == user_data) {
		entry->server = NULL;
		return TRUE;
	} 
	return FALSE;
}

static int remove_cached_server(SMBCCTX * context, SMBCSRV * server)
{
	int removed;
	
	removed = g_hash_table_foreach_remove (server_cache, remove_server, server);

	/* return 1 if failed */
	return removed == 0;
}


static void
add_server (gpointer key,
	    gpointer value,
	    gpointer user_data)
{
	SmbServerCacheEntry *entry;
	GPtrArray *array;

	entry = key;
	array = user_data;
	g_ptr_array_add (array, entry->server);
}

static int
purge_cached(SMBCCTX * context)
{
	int size;
	GPtrArray *servers;
	gboolean could_not_purge_all;
	int i;

	size = g_hash_table_size (server_cache);
	servers = g_ptr_array_sized_new (size);

	/* The remove can change the hashtable, make a copy */
	g_hash_table_foreach (server_cache, add_server, servers);
		
	could_not_purge_all = FALSE;
	for (i = 0; i < servers->len; i++) {
		if (smbc_remove_unused_server(context,
					      (SMBCSRV *)g_ptr_array_index (servers, i))) {
			/* could not be removed */
			could_not_purge_all = TRUE;
		}
	}

	g_ptr_array_free (servers, TRUE);
	
	return could_not_purge_all;
}


static gboolean
remove_all (gpointer  key,
	    gpointer  value,
	    gpointer  user_data)
{
	return TRUE;
}

static void
update_workgroup_cache (void)
{
	SmbAuthContext actx;
	SMBCFILE *dir = NULL;
	time_t t;
	struct smbc_dirent *dirent;
	
	t = time (NULL);

	if (workgroups_timestamp != 0 &&
	    workgroups_timestamp < t &&
	    t < workgroups_timestamp + WORKGROUP_CACHE_TIMEOUT) {
		/* Up to date */
		return;
	}
	workgroups_timestamp = t;

	DEBUG_SMB(("update_workgroup_cache: enumerating workgroups\n"));
	
	g_hash_table_foreach_remove (workgroups, remove_all, NULL);
	
	LOCK_SMB();
	
	init_authentication (&actx, NULL);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		dir = smb_context->opendir (smb_context, "smb://");
		actx.res = (dir != NULL) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	if (dir != NULL) {
		while ((dirent = smb_context->readdir (smb_context, dir)) != NULL) {
			if (dirent->smbc_type == SMBC_WORKGROUP &&
			    dirent->name != NULL &&
			    strlen (dirent->name) > 0) {
				g_hash_table_insert (workgroups,
						     g_ascii_strdown (dirent->name, -1),
						     GINT_TO_POINTER (1));
			} else {
				g_warning ("non-workgroup at smb toplevel\n");
			}
		}

		smb_context->closedir (smb_context, dir);
	}
	UNLOCK_SMB();
}

static SmbUriType
smb_uri_type (MateVFSURI *uri)
{
	MateVFSToplevelURI *toplevel;
	char *first_slash;
	char *host_name;

	toplevel = (MateVFSToplevelURI *)uri;

	if (toplevel->host_name == NULL || toplevel->host_name[0] == 0) {
		/* smb:/// or smb:///foo */
		if (uri->text == NULL ||
		    uri->text[0] == 0 ||
		    strcmp (uri->text, "/") == 0) {
			return SMB_URI_WHOLE_NETWORK;
		}
		if (strchr (uri->text + 1, '/')) {
			return SMB_URI_ERROR;
		}
		return SMB_URI_WORKGROUP_LINK;
	}
	if (uri->text == NULL ||
	    uri->text[0] == 0 ||
	    strcmp (uri->text, "/") == 0) {
		/* smb://foo/ */
		update_workgroup_cache ();
		host_name = mate_vfs_unescape_string (toplevel->host_name,
		                                       G_DIR_SEPARATOR_S);
		if (!host_name)
			return SMB_URI_ERROR;
		if (!g_ascii_strcasecmp(host_name,
					DEFAULT_WORKGROUP_NAME) ||
		    g_hash_table_lookup (workgroups, host_name)) {
			g_free (host_name);
			return SMB_URI_WORKGROUP;
		} else {
			g_free (host_name);
			return SMB_URI_SERVER;
		}
	}
	first_slash = strchr (uri->text + 1, '/');
	if (first_slash == NULL) {
		/* smb://foo/bar */
		update_workgroup_cache ();
		host_name = mate_vfs_unescape_string (toplevel->host_name,
		                                       G_DIR_SEPARATOR_S);
		if (!host_name)
			return SMB_URI_ERROR;
		if (!g_ascii_strcasecmp(host_name,
					DEFAULT_WORKGROUP_NAME) ||
		    g_hash_table_lookup (workgroups, host_name)) {
			g_free (host_name);
			return SMB_URI_SERVER_LINK;
		} else {
			g_free (host_name);
			return SMB_URI_SHARE;
		}
	}
	
	return SMB_URI_SHARE_FILE;
}



static gboolean
is_hidden_entry (char *name)
{
	static const char *hidden_names[] = {
		"IPC$",
		"ADMIN$",
		"print$",
		"printer$"
	};
	
	int i;
	
 	if (name == NULL) return TRUE;
 
	for (i = 0; i < G_N_ELEMENTS (hidden_names); i++)
		if (g_ascii_strcasecmp (name, hidden_names[i]) == 0)
			return TRUE;
	
	/* Shares that end in "$" are administrative shares, and Windows hides
	 * them by default.  We have no way to say "this is a hidden file" in a
	 * MateVFSFileInfo, so we'll make them visible for now.
	 */
	
	return FALSE;
}

static gboolean
try_init (void)
{
	char *path;
	MateConfClient *gclient;
	gchar *workgroup;
	struct stat statbuf;

	LOCK_SMB();

	/* We used to create an empty ~/.smb/smb.conf to get
	 * default settings, but this breaks a lot of smb.conf
	 * configurations, so we remove this again. If you really
	 * need an empty smb.conf, put a newline in it */
	path = g_build_filename (G_DIR_SEPARATOR_S, g_get_home_dir (),
			".smb", "smb.conf", NULL);

	if (stat (path, &statbuf) == 0) {
		if (S_ISREG (statbuf.st_mode) &&
		    statbuf.st_size == 0) {
			unlink (path);
		}
	}
	g_free (path);

	smb_context = smbc_new_context ();
	if (smb_context != NULL) {
		smb_context->debug = 0;
		smb_context->callbacks.auth_fn 		    = auth_callback;
		smb_context->callbacks.add_cached_srv_fn    = add_cached_server;
		smb_context->callbacks.get_cached_srv_fn    = get_cached_server;
		smb_context->callbacks.remove_cached_srv_fn = remove_cached_server;
		smb_context->callbacks.purge_cached_fn      = purge_cached;

		gclient = mateconf_client_get_default ();
		if (gclient) {
			workgroup = mateconf_client_get_string (gclient, 
					PATH_MATECONF_MATE_VFS_SMB_WORKGROUP, NULL);

			/* libsmbclient frees this on it's own, so make sure 
			 * to use simple system malloc */
			if (workgroup && workgroup[0])
				smb_context->workgroup = strdup (workgroup);
			
			g_free (workgroup);
			g_object_unref (gclient);
		}

		if (!smbc_init_context (smb_context)) {
			smbc_free_context (smb_context, FALSE);
			smb_context = NULL;
		}

#if defined(HAVE_SAMBA_FLAGS) 
#if defined(SMB_CTX_FLAG_USE_KERBEROS) && defined(SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS)
		smb_context->flags |= SMB_CTX_FLAG_USE_KERBEROS | SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS;
#endif
#if defined(SMBCCTX_FLAG_NO_AUTO_ANONYMOUS_LOGON)
		smb_context->flags |= SMBCCTX_FLAG_NO_AUTO_ANONYMOUS_LOGON;
#endif
#endif		
	}

	server_cache = g_hash_table_new_full (server_hash, server_equal,
					      (GDestroyNotify)server_free, NULL);
	workgroups = g_hash_table_new_full (g_str_hash, g_str_equal,
					    g_free, NULL);
	user_cache = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            g_free, (GDestroyNotify)user_free);
	
	UNLOCK_SMB();

	if (smb_context == NULL) {
		g_warning ("Could not initialize samba client library\n");
		return FALSE;
	}

	return TRUE;
}


/* Authentication ----------------------------------------------------------- */

static const gchar*
get_auth_display_share (SmbAuthContext *actx)
{
        return string_compare (actx->for_share, "IPC$") ? 
                        NULL : actx->for_share;
}

static char*
get_auth_display_uri (SmbAuthContext *actx, gboolean machine)
{
        if (actx->uri != NULL && !machine)
                return mate_vfs_uri_to_string (actx->uri, 0);
        else {
                const gchar *share = get_auth_display_share (actx);
                return g_strdup_printf ("smb://%s%s%s%s", 
                                        actx->for_server ? actx->for_server : "", 
                                        actx->for_server ? "/" : "",
                                        share && !machine ? share : "",
                                        share && !machine ? "/" : "");
        }
}

static void
update_user_cache (SmbAuthContext *actx, gboolean with_share)
{
        SmbCachedUser *user;
        gchar *key;
        
        g_return_if_fail (actx->for_server != NULL);
        
        key = g_strdup_printf ("%s/%s", actx->for_server, with_share ? actx->for_share : "");
        user = (SmbCachedUser*)g_hash_table_lookup (user_cache, key);
        
        DEBUG_SMB(("[auth] Saved in cache: %s = %s:%s@%s\n", key,
		   actx->use_user ? actx->use_user : "", 
		   actx->use_domain ? actx->use_domain : "", 
		   actx->use_password ? actx->use_password : ""));
        
        if (!user) {
                user = g_new0 (SmbCachedUser, 1);
                g_hash_table_replace (user_cache, key, user);
                schedule_cache_reap ();
        } else {
                g_free (key);
        }

        user->domain = string_realloc (user->domain, actx->use_domain);
        user->username = string_realloc (user->username, actx->use_user);
        user->password = string_realloc (user->password, actx->use_password);
        user->stamp = time (NULL);
}

static gboolean
lookup_user_cache (SmbAuthContext *actx, gboolean with_share)
{
        SmbCachedUser *user;
        gchar *key;
       
        g_return_val_if_fail (actx->for_server != NULL, FALSE);
       
        key = g_strdup_printf ("%s/%s", actx->for_server, with_share ? actx->for_share : "");
        user = (SmbCachedUser*)g_hash_table_lookup (user_cache, key);
        g_free (key);
       
        if (user) {
                /* If we already have a user name or domain double check that... */
		if (!(actx->prompt_flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME) &&
		    !string_compare(user->username, actx->use_user))
			return FALSE;
		if (!(actx->prompt_flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN) &&
		    !string_compare(user->domain, actx->use_domain))
			return FALSE;

                actx->use_user = string_realloc (actx->use_user, user->username);
                actx->use_domain = string_realloc (actx->use_domain, user->domain);
                actx->use_password = string_realloc (actx->use_password, user->password);
                DEBUG_SMB(("[auth] Looked up in cache: %s:%s@%s\n", 
			   actx->use_user ? actx->use_user : "",
			   actx->use_domain ? actx->use_domain : "",
			   actx->use_password ? actx->use_password : ""));
                return TRUE;
        }
        
        return FALSE;
}      

static gboolean
initial_authentication (SmbAuthContext *actx)
{
	/* IMPORTANT: We are IN the lock at this point */
	
	MateVFSToplevelURI *toplevel_uri;
	SmbServerCacheEntry server_lookup;
	SmbServerCacheEntry *server;
        gboolean found_user = FALSE;
	char *tmp;

	DEBUG_SMB(("[auth] Initial authentication lookups\n"));

	toplevel_uri =	(MateVFSToplevelURI*)actx->uri;
	actx->prompt_flags = MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME |
		    	     MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN;
	
	/* Try parsing a user and domain out of the URI */
	if (toplevel_uri && toplevel_uri->user_name != NULL && 
  	    toplevel_uri->user_name[0] != 0) {

		tmp = strchr (toplevel_uri->user_name, ';');
		if (tmp != NULL) {
			g_free (actx->use_domain);
			actx->use_domain = string_ndup_nzero (toplevel_uri->user_name,
					    	  	      tmp - toplevel_uri->user_name);
			g_free (actx->use_user);
			actx->use_user = string_dup_nzero (tmp + 1);
			DEBUG_SMB(("[auth] User from URI: %s@%s\n", 
				   actx->use_user ? actx->use_user : "", 
				   actx->use_domain ? actx->use_domain : ""));
		} else {
			g_free (actx->use_user);
			actx->use_user = string_dup_nzero (toplevel_uri->user_name);
			g_free (actx->use_domain);
			actx->use_domain = NULL;
			DEBUG_SMB(("[auth] User from URI: %s\n", 
				   actx->use_user ? actx->use_user : ""));
		}
		
		if (actx->use_user != NULL) {
			actx->preset_user = TRUE;
			actx->prompt_flags &= ~MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME;
		}
		if (actx->use_domain != NULL) {
			actx->prompt_flags &= ~MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN;
		}
	} 

        if (lookup_user_cache (actx, TRUE) ||
            lookup_user_cache (actx, FALSE))
                found_user = TRUE;

        if (found_user || actx->preset_user) {
        	/* Lookup the server in our internal cache */
        	server_lookup.server_name = (char*)actx->for_server;
        	server_lookup.share_name = (char*)actx->for_share;
        	server_lookup.username = (char*)actx->use_user;
        	server_lookup.domain = (char*)actx->use_domain;
        		
        	server = g_hash_table_lookup (server_cache, &server_lookup);
        	if (server == NULL) {
                 
                        /* If a blank user, try looking up 'guest' */
                        if (!actx->use_user) {
                                server_lookup.username = GUEST_LOGIN;
                                server_lookup.domain = NULL;
                                server = g_hash_table_lookup (server_cache, &server_lookup);
                        }
                }
                
                /* Server is in cache already */
                if (server != NULL) {
        		DEBUG_SMB(("[auth] Using connection for '%s@%s' from our server cache\n", 
				   actx->use_user ? actx->use_user : "", 
				   actx->use_domain ? actx->use_domain : ""));
                        found_user = TRUE;
        	}
        }

	return found_user;
}

static gboolean
prefill_authentication (SmbAuthContext *actx)
{
	/* IMPORTANT: We are NOT IN the lock at this point */
	
	MateVFSModuleCallbackFillAuthenticationIn in_args;
	MateVFSModuleCallbackFillAuthenticationOut out_args;
	gboolean invoked;
	gboolean filled = FALSE;

	g_return_val_if_fail (actx != NULL, FALSE);
	g_return_val_if_fail (actx->for_server != NULL, FALSE);	

	memset (&in_args, 0, sizeof (in_args));
	in_args.uri = get_auth_display_uri (actx, FALSE);
	in_args.protocol = "smb";
	in_args.server = (char*)actx->for_server;
	in_args.object = (char*)get_auth_display_share (actx);
	in_args.username = (char*)actx->use_user;
	in_args.domain = (char*)actx->use_domain;
	in_args.port = actx->uri ? ((MateVFSToplevelURI*)actx->uri)->host_port : 0;

	memset (&out_args, 0, sizeof (out_args));

	DEBUG_SMB(("[auth] Trying to prefill credentials for: %s\n", 
		   in_args.uri ? in_args.uri : ""));

	invoked = mate_vfs_module_callback_invoke
		              (MATE_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
		              &in_args, sizeof (in_args),
		              &out_args, sizeof (out_args));

        g_free (in_args.uri);
               
        /* If that didn't work then try without the share name */
        if (!invoked || !out_args.valid) {

                g_free (out_args.username);
                g_free (out_args.domain);
                g_free (out_args.password);
      
                memset (&in_args, 0, sizeof (in_args));
                in_args.uri = get_auth_display_uri (actx, TRUE);
                in_args.protocol = "smb";
                in_args.server = (char*)actx->for_server;
                in_args.object = (char*)NULL;
                in_args.username = (char*)actx->use_user;
                in_args.domain = (char*)actx->use_domain;
                in_args.port = actx->uri ? ((MateVFSToplevelURI*)actx->uri)->host_port : 0;

                memset (&out_args, 0, sizeof (out_args));

                DEBUG_SMB(("[auth] Trying to prefill server credentials for: %s\n", 
			   in_args.uri ? in_args.uri : ""));

                invoked = mate_vfs_module_callback_invoke
                                        (MATE_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
                                        &in_args, sizeof (in_args),
                                        &out_args, sizeof (out_args));                              
        }
        
	if (invoked && out_args.valid) {
		/* When a preset user make sure we stick to this login */
		if (!actx->preset_user || string_compare (actx->use_user, out_args.username)) {
			
			g_free (actx->use_user);
			actx->use_user = string_dup_nzero (out_args.username);
			g_free (actx->use_domain);
			actx->use_domain = string_dup_nzero (out_args.domain);
			g_free (actx->use_password);
			actx->use_password = g_strdup (out_args.password);
			
			filled = TRUE;
			DEBUG_SMB(("[auth] Prefilled credentials: %s@%s:%s\n", 
				   actx->use_user ? actx->use_user : "", 
				   actx->use_domain ? actx->use_domain : "", 
				   actx->use_password ? actx->use_password : ""));
		}
	} 
	
	g_free (out_args.username);
	g_free (out_args.domain);
	g_free (out_args.password);

	return filled;
}

static gboolean
prompt_authentication (SmbAuthContext *actx,
		       gboolean       *cancelled)
{
	/* IMPORTANT: We are NOT in the lock at this point */

	MateVFSModuleCallbackFullAuthenticationIn in_args;
	MateVFSModuleCallbackFullAuthenticationOut out_args;
	gboolean invoked;
	
	g_return_val_if_fail (actx != NULL, FALSE);
	g_return_val_if_fail (actx->for_server != NULL, FALSE);
	
	memset (&in_args, 0, sizeof (in_args));
	
	in_args.flags = MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD | MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED;
	if (actx->state & SMB_AUTH_STATE_PROMPTED)
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED;
	in_args.flags |= actx->prompt_flags;

	in_args.uri = get_auth_display_uri (actx, FALSE);
	in_args.protocol = "smb";
	in_args.server = (char*)actx->for_server;
	in_args.object = (char*)get_auth_display_share (actx);
	in_args.username = (char*)actx->use_user;
	in_args.domain = (char*)actx->use_domain;
	in_args.port = actx->uri ? ((MateVFSToplevelURI*)actx->uri)->host_port : 0;

	in_args.default_user = actx->use_user;
	if (string_compare (in_args.default_user, GUEST_LOGIN))
		in_args.default_user = NULL;
	if (!in_args.default_user)
		in_args.default_user = (char*)g_get_user_name ();
	
	in_args.default_domain = actx->use_domain ? actx->use_domain : smb_context->workgroup;
	
	memset (&out_args, 0, sizeof (out_args));

	DEBUG_SMB(("[auth] Prompting credentials for: %s\n", 
		   in_args.uri ? in_args.uri : ""));

	invoked = mate_vfs_module_callback_invoke
		(MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
		 &in_args, sizeof (in_args),
		 &out_args, sizeof (out_args));

	if (invoked) {
                if (in_args.flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME) {
                     g_free (actx->use_user);
                       actx->use_user = string_dup_nzero (out_args.username);
                }
                if (in_args.flags & MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN) {
                      g_free (actx->use_domain);
                     actx->use_domain = string_dup_nzero (out_args.domain);
                }
		g_free (actx->use_password);
		actx->use_password = out_args.password ? g_strdup (out_args.password) : NULL;
		g_free (actx->keyring);
		actx->save_auth = out_args.save_password;
		actx->keyring = actx->save_auth && out_args.keyring ? g_strdup (out_args.keyring) : NULL;
		DEBUG_SMB(("[auth] Prompted credentials: %s@%s:%s\n", 
			    actx->use_user ? actx->use_user : "", 
			    actx->use_domain ? actx->use_domain : "",
			    actx->use_password ? actx->use_password : ""));
	} 

	*cancelled = out_args.abort_auth;

	actx->state |= SMB_AUTH_STATE_PROMPTED;

	g_free (out_args.username);
	g_free (out_args.domain);
	g_free (out_args.password);
	g_free (out_args.keyring);

	g_free (in_args.uri);

	return invoked && !*cancelled;
}

static void
save_authentication (SmbAuthContext *actx)
{
	MateVFSModuleCallbackSaveAuthenticationIn in_args;
	MateVFSModuleCallbackSaveAuthenticationOut out_args;
	gboolean invoked;

        /* Add to the user cache both with and without shares */
        if (actx->for_server) {
                update_user_cache (actx, TRUE);
                update_user_cache (actx, FALSE);
        }

        if (!actx->save_auth)
                return;
      
        /* Save with the domain name */
	memset (&in_args, 0, sizeof (in_args));
	in_args.uri = get_auth_display_uri (actx, TRUE);
	in_args.keyring = (char*)actx->keyring;
	in_args.protocol = "smb";
	in_args.server = (char*)actx->for_server;
	in_args.object = NULL;
	in_args.port = actx->uri ? ((MateVFSToplevelURI*)actx->uri)->host_port : 0;
	in_args.authtype = NULL;
	in_args.username = (char*)actx->use_user;
	in_args.domain = (char*)actx->use_domain;
	in_args.password = (char*)actx->use_password;

	memset (&out_args, 0, sizeof (out_args));
	DEBUG_SMB(("[auth] Saving credentials: %s = %s@%s:%s\n", 
		   in_args.uri ? in_args.uri : "", 
		   actx->use_user ? actx->use_user : "", 
		   actx->use_domain ? actx->use_domain : "",
		   actx->use_password ? actx->use_password : ""));
	DEBUG_SMB(("[auth] Keyring: %s\n", 
		   actx->keyring ? actx->keyring : ""));

	invoked = mate_vfs_module_callback_invoke
                                (MATE_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
		                &in_args, sizeof (in_args),
		                &out_args, sizeof (out_args));
        
        g_free (in_args.uri);
              
        /* Save without the domain name */
        memset (&in_args, 0, sizeof (in_args));
        in_args.uri = get_auth_display_uri (actx, FALSE);
        in_args.keyring = (char*)actx->keyring;
        in_args.protocol = "smb";
        in_args.server = (char*)actx->for_server;
        in_args.object = (char*)get_auth_display_share (actx);
        in_args.port = actx->uri ? ((MateVFSToplevelURI*)actx->uri)->host_port : 0;
        in_args.authtype = NULL;
        in_args.username = (char*)actx->use_user;
        in_args.domain = (char*)actx->use_domain;
        in_args.password = (char*)actx->use_password;
        
        memset (&out_args, 0, sizeof (out_args));
        DEBUG_SMB(("[auth] Saving credentials: %s = %s@%s:%s\n", 
		   in_args.uri ? in_args.uri : "", 
		   actx->use_user ? actx->use_user : "", 
		   actx->use_domain ? actx->use_domain : "", 
		   actx->use_password ? actx->use_password : ""));
        DEBUG_SMB(("[auth] Keyring: %s\n", 
		   actx->keyring ? actx->keyring : ""));

        invoked = mate_vfs_module_callback_invoke
                                (MATE_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
                                &in_args, sizeof (in_args),
                                &out_args, sizeof (out_args));
        
	g_free (in_args.uri);
}
	
static void
cleanup_authentication (SmbAuthContext *actx)
{
	/* IMPORTANT: We are IN the lock at this point */
	
	DEBUG_SMB(("[auth] Cleaning up Authentication\n"));
	g_return_if_fail (actx != NULL);
	
	g_free (actx->for_server);
	actx->for_server = NULL;
	
	g_free (actx->for_share);
	actx->for_share = NULL;
	
	g_free (actx->use_user);
	actx->use_user = NULL;
	
	g_free (actx->use_domain);
	actx->use_domain = NULL;
	
	g_free (actx->use_password);
	actx->use_password = NULL;
	
	g_free (actx->keyring);
	actx->keyring = NULL;
	
	g_return_if_fail (current_auth_context == actx);
	current_auth_context = NULL;
}

/* 
 * This is the workhorse of all the authentication and caching work.
 * It is called in a loop, and must be called from within the lock:
 * 
 * static MateVFSResult
 * function_xxxx (MateVFSURI* uri)
 * {
 * 	SmbAuthContext actx;
 * 
 * 	LOCK_SMB ();
 * 	init_authentication (&actx);
 * 
 * 	while (perform_authentication (&actx) > 0) {
 * 		actx.res = mate_vfs_result_from_errno_code (the_operation_here ());
 * 	}
 * 
 * 	UNLOCK_SMB();
 * 
 * 	return actx.err;
 * }
 * 
 * On different passes it performs seperate operations, such as checking
 * the cache, prompting the user for a password, saving valid passwords, 
 * cleaning up etc.... 
 * 
 * The loop must never be broken on it's own when perform_authentication
 * has not returned a 0 or negative return value.
 */

static void 
init_authentication (SmbAuthContext *actx, MateVFSURI *uri)
{
	DEBUG_SMB(("[auth] Initializing Authentication\n"));
	memset (actx, 0, sizeof(*actx));
	actx->uri = uri;
}

static int
perform_authentication (SmbAuthContext *actx)
{
	gboolean cont, auth_failed = FALSE, auth_cancelled = FALSE;
	int ret = -1;
	
	/* IMPORTANT: We are IN the lock at this point */
	DEBUG_SMB(("[auth] perform_authentication called.\n"));
	
	switch (actx->res) {
	case MATE_VFS_OK:
		auth_failed = FALSE;
		break;
		
	/* Authentication errors are special */
	case MATE_VFS_ERROR_ACCESS_DENIED:
	case MATE_VFS_ERROR_NOT_PERMITTED:
	case MATE_VFS_ERROR_LOGIN_FAILED:
		auth_failed = TRUE;
		break;
	
	/* Other errors mean we're done */
	default:
		DEBUG_SMB(("[auth] Non-authentication error. Leaving auth loop.\n"));
		cleanup_authentication (actx);
		return -1;
	}
	
	actx->passes++;

	/* First pass */
	if (actx->passes == 1) {

		DEBUG_SMB(("[auth] First authentication pass\n"));
	
		/* Our auth context is the global one for the moment */
		g_return_val_if_fail (current_auth_context == NULL, MATE_VFS_ERROR_INTERNAL);
		current_auth_context = actx;
			
		/* Continue with perform_authentication loop ... */
		ret = 1;
		
	/* Subsequent passes */
	} else {

		/* We should still be the global context at this point */
		g_return_val_if_fail (current_auth_context == actx, MATE_VFS_ERROR_INTERNAL);
		
		/* A successful operation. Done! */
		if (!auth_failed) {
			
			DEBUG_SMB(("[auth] Operation successful\n"));
			save_authentication (actx);
			ret = 0;

		/* If authentication failed, but we already have a connection 
		   ... and access failed on a file, then we return the error */
		} else if ((actx->cache_used && !actx->cache_added) && 
			(!actx->uri || smb_uri_type (actx->uri) == SMB_URI_SHARE_FILE)) {

			DEBUG_SMB(("[auth] Not reauthenticating a open connection.\n"));
			ret = -1;

		/* A failed authentication */
		} else if (actx->auth_called) {
			
			/* We need a server to perform any authentication */
			g_return_val_if_fail (actx->for_server != NULL, MATE_VFS_ERROR_INTERNAL);
			
			/* We won't be the global context for now */
			current_auth_context = NULL;
			cont = FALSE;
			
			UNLOCK_SMB();
			
				/* Do we have mate-keyring credentials for this? */
				if (!(actx->state & SMB_AUTH_STATE_PREFILLED)) {
					actx->state |= SMB_AUTH_STATE_PREFILLED;
					cont = prefill_authentication (actx);
				}

				/* Then we try a guest credentials... */
				if (!cont && !actx->preset_user && !(actx->state & SMB_AUTH_STATE_GUEST)) {
					g_free (actx->use_user);
					actx->use_user = strdup(GUEST_LOGIN);
					g_free (actx->use_domain);
					actx->use_domain = NULL;
					g_free (actx->use_password);
					actx->use_password = strdup("");
					actx->state |= SMB_AUTH_STATE_GUEST;
					cont = TRUE;
				}

				/* And as a last step, prompt */
				if (!cont)
					cont = prompt_authentication (actx, &auth_cancelled);
				
			LOCK_SMB();
			
			/* Claim the global context back */
			g_return_val_if_fail (current_auth_context == NULL, MATE_VFS_ERROR_INTERNAL);
			current_auth_context = actx;
			
			if (cont)
				ret = 1;
			else {
				ret = -1;

				if (auth_cancelled) {
					DEBUG_SMB(("[auth] Authentication cancelled by user.\n"));
					actx->res = MATE_VFS_ERROR_CANCELLED;
				} else {
					DEBUG_SMB(("[auth] Authentication failed with result %s.\n",
						  mate_vfs_result_to_string (actx->res)));
					/* Note that we leave actx->res set to whatever it was set to */
				}
			}
					
		/* Weird, don't want authentication, but failed */
		} else {
			ret = -1;
		}
	}

	if (ret <= 0)
		cleanup_authentication (actx);
	return ret;

	/* IMPORTANT: We need to still be in the lock when returning from this func */
}

static void
auth_callback (const char *server_name, const char *share_name,
	       char *domain_out, int domainmaxlen,
	       char *username_out, int unmaxlen,
	       char *password_out, int pwmaxlen)
{
	/* IMPORTANT: We are IN the global lock */
	SmbAuthContext *actx;
	SMBCSRV *server;
	
	DEBUG_SMB (("[auth] auth_callback called: server: %s share: %s\n",
		    server_name ? server_name : "", 
		    share_name ? share_name : ""));

	g_return_if_fail (current_auth_context != NULL);
	actx = current_auth_context;
	
	/* We never authenticate for the toplevel (enumerating workgroups) */
	if (!server_name || !server_name[0])
		return;

	actx->auth_called = TRUE;	
		
	/* The authentication location */
	g_free (actx->for_server);
	actx->for_server = string_dup_nzero (server_name);
	g_free (actx->for_share);
	actx->for_share = string_dup_nzero (share_name);

	/* The first pass, try the cache, fill in anything we know */
	if (actx->passes == 1)
		initial_authentication (actx);
	
	/* If we have a valid user then go for it */
	if (actx->use_user) {
		strncpy (username_out, actx->use_user, unmaxlen);
                strncpy (password_out, actx->use_password ? actx->use_password : "", pwmaxlen);
		if (actx->use_domain)
			strncpy (domain_out, actx->use_domain, domainmaxlen);
		DEBUG_SMB(("[auth] Using credentials: %s:%s@%s\n", 
			   username_out ? username_out : "", 
			   password_out ? password_out : "", 
			   domain_out ? domain_out : ""));
	
	/* We have no credentials ... */
	} else {
		g_assert (!actx->preset_user);
		
		if (actx->passes == 1)
			DEBUG_SMB(("[auth] No credentials, trying anonymous user login\n"));
		else
			DEBUG_SMB(("[auth] No credentials, returning null values\n"));
		
		strncpy (username_out, "", unmaxlen);
		strncpy (password_out, "", pwmaxlen);
	}

	/* Put in the default workgroup if none specified */
	if (domain_out[0] == 0 && smb_context->workgroup)
		strncpy (domain_out, smb_context->workgroup, domainmaxlen);

	/* 
	 * If authentication is requested a second time on a server we've 
	 * already cached, then obviously it was invalid. Remove it. Yes, 
	 * this doesn't make much sense, but such is life with libsmbclient.
	 */
	if ((actx->state & SMB_AUTH_STATE_PROMPTED) && actx->res != MATE_VFS_OK) {
		server = find_cached_server (server_name, share_name, domain_out, username_out);
		if (server) {
			DEBUG_SMB (("[auth] auth_callback. Remove the wrong server entry from server_cache.\n"));
			g_hash_table_foreach_remove (server_cache, remove_server, server);
		}
	}
}

static char *
get_workgroup_data (const char *display_name, const char *name)
{
	return g_strdup_printf ("[Desktop Entry]\n"
			"Encoding=UTF-8\n"
			"Name=%s\n"
			"Type=Link\n"
			"URL=smb://%s/\n"
			"Icon=mate-fs-network\n",
			display_name, name);
}
                                                                                
static char *
get_computer_data (const char *display_name, const char *name)
{
	return g_strdup_printf ("[Desktop Entry]\n"
			"Encoding=UTF-8\n"
			"Name=%s\n"
			"Type=Link\n"
			"URL=smb://%s/\n"
			"Icon=mate-fs-server\n",
			display_name, name);
}


static gchar *
get_base_from_uri (MateVFSURI const *uri)
{
	gchar *escaped_base, *base;

	escaped_base = mate_vfs_uri_extract_short_path_name (uri);
	base = mate_vfs_unescape_string (escaped_base, G_DIR_SEPARATOR_S);
	g_free (escaped_base);
	return base;
}



typedef struct {
	SMBCFILE *file;
	gboolean is_data;
	char *file_data;
	int fnum;
	MateVFSFileOffset offset;
	MateVFSFileOffset file_size;
} FileHandle;

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{
	SmbAuthContext actx;
	FileHandle *handle = NULL;
	char *path, *name, *unescaped_name;
	int type;
	mode_t unix_mode;
	SMBCFILE *file = NULL;
	
	DEBUG_SMB(("do_open() %s mode %d\n",
				mate_vfs_uri_to_string (uri, 0), mode));

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}
	
	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE) {
		return MATE_VFS_ERROR_IS_DIRECTORY;
	}

	if (type == SMB_URI_WORKGROUP_LINK) {
		if (mode & MATE_VFS_OPEN_WRITE) {
			return MATE_VFS_ERROR_READ_ONLY;
		}
		handle = g_new (FileHandle, 1);
		handle->is_data = TRUE;
		handle->offset = 0;
		unescaped_name = get_base_from_uri (uri);
		name = mate_vfs_uri_extract_short_path_name (uri);
		handle->file_data = get_workgroup_data (unescaped_name, name);
		handle->file_size = strlen (handle->file_data);
		g_free (unescaped_name);
		g_free (name);
		
		*method_handle = (MateVFSMethodHandle *)handle;
		
		return MATE_VFS_OK;
	}

	if (type == SMB_URI_SERVER_LINK) {
		if (mode & MATE_VFS_OPEN_WRITE) {
			return MATE_VFS_ERROR_READ_ONLY;
		}
		handle = g_new (FileHandle, 1);
		handle->is_data = TRUE;
		handle->offset = 0;
		unescaped_name = get_base_from_uri (uri);
		name = mate_vfs_uri_extract_short_path_name (uri);
		handle->file_data = get_computer_data (unescaped_name, name);
		handle->file_size = strlen (handle->file_data);
		g_free (unescaped_name);
		g_free (name);
		
		*method_handle = (MateVFSMethodHandle *)handle;
		
		return MATE_VFS_OK;
	}

	g_assert (type == SMB_URI_SHARE_FILE);

	if (mode & MATE_VFS_OPEN_READ) {
		if (mode & MATE_VFS_OPEN_WRITE)
			unix_mode = O_RDWR;
		else
			unix_mode = O_RDONLY;
	} else {
		if (mode & MATE_VFS_OPEN_WRITE)
			unix_mode = O_WRONLY;
		else
			return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}

	if ((mode & MATE_VFS_OPEN_TRUNCATE) ||
	    (!(mode & MATE_VFS_OPEN_RANDOM) && (mode & MATE_VFS_OPEN_WRITE)))
		unix_mode |= O_TRUNC;
	
	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);
	
	LOCK_SMB();
	init_authentication (&actx, uri);

	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		file = (smb_context->open) (smb_context, path, unix_mode, 0666);
		actx.res = (file != NULL) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	UNLOCK_SMB();

	g_free (path);
	
	if (file == NULL)
		return actx.res;
	
	handle = g_new (FileHandle, 1);
	handle->is_data = FALSE;
	handle->file = file;

	*method_handle = (MateVFSMethodHandle *)handle;

	return MATE_VFS_OK;
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)

{
	FileHandle *handle = (FileHandle *)method_handle;
	SmbAuthContext actx;
	MateVFSResult res;
	int r;

	DEBUG_SMB(("do_close()\n"));

	res = MATE_VFS_OK;

	if (handle->is_data) {
		g_free (handle->file_data);
	} else {
		LOCK_SMB();
		init_authentication (&actx, NULL);

		/* Important: perform_authentication leaves and re-enters the lock! */
		while (perform_authentication (&actx) > 0) {
#ifdef HAVE_SAMBA_OLD_CLOSE
			r = smb_context->close (smb_context, handle->file);
#else
			r = smb_context->close_fn (smb_context, handle->file);
#endif
			actx.res = (r >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
		}

		res = actx.res;		
		UNLOCK_SMB();
	}

	g_free (handle);
	return res;
}

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	FileHandle *handle = (FileHandle *)method_handle;
	MateVFSResult res = MATE_VFS_OK;
	SmbAuthContext actx;
	ssize_t n = 0;

	DEBUG_SMB(("do_read() %Lu bytes\n", num_bytes));

	if (handle->is_data) {
		if (handle->offset >= handle->file_size) {
			n = 0;
		} else {
			n = MIN (num_bytes, handle->file_size - handle->offset);
			memcpy (buffer, handle->file_data + handle->offset, n);
		}
	} else {
		LOCK_SMB();
		init_authentication (&actx, NULL);
		
		/* Important: perform_authentication leaves and re-enters the lock! */
		while (perform_authentication (&actx) > 0) {
			n = smb_context->read (smb_context, handle->file, buffer, MIN (USHRT_MAX, num_bytes));
			actx.res = (n >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
		}
		
		res = actx.res;
		UNLOCK_SMB();
	}

	*bytes_read = (n < 0) ? 0 : n;

	if (n == 0) 
		return MATE_VFS_ERROR_EOF;

	handle->offset += n;

	return res;
}

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)


{
	FileHandle *handle = (FileHandle *)method_handle;
	SmbAuthContext actx;
	ssize_t written = 0;

	DEBUG_SMB (("do_write() %p\n", method_handle));

	if (handle->is_data)
		return MATE_VFS_ERROR_READ_ONLY;

	LOCK_SMB();
	init_authentication (&actx, NULL);

	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		written = smb_context->write (smb_context, handle->file, (void *)buffer, num_bytes);
		actx.res = (written >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	
	UNLOCK_SMB();

	*bytes_written = (written < 0) ? 0 : written;
	return actx.res;
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
	int type;
	mode_t unix_mode;
	char *path;
	SMBCFILE *file = NULL;
	FileHandle *handle;
	SmbAuthContext actx;
	
	DEBUG_SMB (("do_create() %s mode %d\n",
				mate_vfs_uri_to_string (uri, 0), mode));

	
	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR)
		return MATE_VFS_ERROR_INVALID_URI;

	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE)
		return MATE_VFS_ERROR_IS_DIRECTORY;

	if (type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) 
		return MATE_VFS_ERROR_NOT_PERMITTED;
	
	unix_mode = O_CREAT | O_TRUNC;
	
	if (!(mode & MATE_VFS_OPEN_WRITE))
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;

	if (mode & MATE_VFS_OPEN_READ)
		unix_mode |= O_RDWR;
	else
		unix_mode |= O_WRONLY;

	if (exclusive)
		unix_mode |= O_EXCL;

	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	LOCK_SMB();
	init_authentication (&actx, uri);
	
	/* Important: perform_authentication leaves and re-enters the lock! */	
	while (perform_authentication (&actx) > 0) {
		file = (smb_context->open) (smb_context, path, unix_mode, perm);
		actx.res = (file != NULL) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	UNLOCK_SMB();

	g_free (path);

	if (file == NULL)
		return actx.res;
	
	handle = g_new (FileHandle, 1);
	handle->is_data = FALSE;
	handle->file = file;

	*method_handle = (MateVFSMethodHandle *)handle;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)

{
	struct stat st;
	char *path;
	int type, err = -1;
	const char *mime_type;
	SmbAuthContext actx;

	DEBUG_SMB (("do_get_file_info() %s\n",
				mate_vfs_uri_to_string (uri, 0)));

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}
	
	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE) {
		file_info->name = get_base_from_uri (uri);
		file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		if (type == SMB_URI_SHARE) {
			file_info->mime_type = g_strdup ("x-directory/smb-share");
		} else {
			file_info->mime_type = g_strdup ("x-directory/normal");
		}
		/* Make sure you can't write to smb:// or smb://foo. For smb://server/share we
		 * leave this empty, since accessing the data for real can cause authentication
		 * while e.g. browsing smb://server
		 */
		if (type != SMB_URI_SHARE) {
			file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
			file_info->permissions =
 				MATE_VFS_PERM_USER_READ |
 				MATE_VFS_PERM_OTHER_READ |
 				MATE_VFS_PERM_GROUP_READ;
		}
		return MATE_VFS_OK;
	}

	if (type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		file_info->name = get_base_from_uri (uri);
		file_info->valid_fields = file_info->valid_fields
			| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
			| MATE_VFS_FILE_INFO_FIELDS_TYPE
			| MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->permissions =
			MATE_VFS_PERM_USER_READ |
			MATE_VFS_PERM_OTHER_READ |
			MATE_VFS_PERM_GROUP_READ;
		return MATE_VFS_OK;
	}

	g_assert (type == SMB_URI_SHARE_FILE);
	    
	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	LOCK_SMB();
	init_authentication (&actx, uri);

	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		err = smb_context->stat (smb_context, path, &st);
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	UNLOCK_SMB();

	g_free (path);

	if (err < 0)
		return actx.res;
	
	mate_vfs_stat_to_file_info (file_info, &st);
	file_info->name = get_base_from_uri (uri);

	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
	file_info->io_block_size = SMB_BLOCK_SIZE;
	
	if (options & MATE_VFS_FILE_INFO_GET_MIME_TYPE) {		
		if (S_ISDIR(st.st_mode)) {
			mime_type = "x-directory/normal";
		} else if (options & MATE_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE) {
			mime_type = mate_vfs_get_mime_type_common (uri);
		} else {
			mime_type = mate_vfs_mime_type_from_name_or_default (file_info->name, NULL);
		}
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		file_info->mime_type = g_strdup (mime_type);
	}

	DEBUG_SMB (("do_get_file_info()\n"
				"name: %s\n"
				"smb type: %d\n"
				"mimetype: %s\n"
				"type: %d\n",
				file_info->name, type,
				file_info->mime_type, file_info->type));

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
		MateVFSMethodHandle *method_handle,
		MateVFSFileInfo *file_info,
		MateVFSFileInfoOptions options,
		MateVFSContext *context)
{
	FileHandle *handle = (FileHandle *)method_handle;
	SmbAuthContext actx;
	struct stat st;
	int err = -1;

	LOCK_SMB();
	init_authentication (&actx, NULL);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		err = smb_context->fstat (smb_context, handle->file, &st);
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	
	UNLOCK_SMB();
	
	if (err < 0) 
		return actx.res;
	
	mate_vfs_stat_to_file_info (file_info, &st);

	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
	file_info->io_block_size = SMB_BLOCK_SIZE;
	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return FALSE;
}

typedef struct {
	GList *workgroups;
	SMBCFILE *dir;
	char *path;
} DirectoryHandle;

static void
add_workgroup (gpointer key,
	    gpointer value,
	    gpointer user_data)
{
	DirectoryHandle *directory_handle;

	directory_handle = user_data;
	directory_handle->workgroups = g_list_prepend (directory_handle->workgroups,
						       g_strdup (key));
}


static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)

{
	DirectoryHandle *directory_handle;
	MateVFSURI *new_uri = NULL;
	const char *host_name;
	char *path;
	SmbUriType type;
	SMBCFILE *dir = NULL;
	SmbAuthContext actx;

	DEBUG_SMB(("do_open_directory() %s\n",
		mate_vfs_uri_to_string (uri, 0)));

	type = smb_uri_type (uri);

	if (type == SMB_URI_WHOLE_NETWORK) {
		update_workgroup_cache ();
		
		directory_handle = g_new0 (DirectoryHandle, 1);
		g_hash_table_foreach (workgroups, add_workgroup, directory_handle);
		*method_handle = (MateVFSMethodHandle *) directory_handle;
		return MATE_VFS_OK;
	}

	if (type == SMB_URI_ERROR ||
	    type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		return MATE_VFS_ERROR_NOT_A_DIRECTORY;
	}

	/* if it is the magic default workgroup name, map it to the 
	 * SMBCCTX's workgroup, which comes from the smb.conf file. */
	host_name = mate_vfs_uri_get_host_name (uri);
	if (type == SMB_URI_WORKGROUP && host_name != NULL &&
	    !g_ascii_strcasecmp(host_name, DEFAULT_WORKGROUP_NAME)) {
		new_uri = mate_vfs_uri_dup (uri);
		mate_vfs_uri_set_host_name (new_uri,
					     smb_context->workgroup
					     ? smb_context->workgroup
					     : "WORKGROUP");
		uri = new_uri;
	}
		
	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	DEBUG_SMB(("do_open_directory() path %s\n", path));

	LOCK_SMB();
	init_authentication (&actx, uri);

	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		dir = smb_context->opendir (smb_context, path);
		actx.res = (dir != NULL) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	UNLOCK_SMB();
	
	if (new_uri) 
		mate_vfs_uri_unref (new_uri);
	
	if (dir == NULL) {
		g_free (path);
		return actx.res;
	}
	
	/* Construct the handle */
	directory_handle = g_new0 (DirectoryHandle, 1);
	directory_handle->dir = dir;
	directory_handle->path = path;
	*method_handle = (MateVFSMethodHandle *) directory_handle;

	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	DirectoryHandle *directory_handle = (DirectoryHandle *) method_handle;
	MateVFSResult res;
	SmbAuthContext actx;
	GList *l;
	int err = -1;

	DEBUG_SMB(("do_close_directory: %p\n", directory_handle));

	if (directory_handle == NULL)
		return MATE_VFS_OK;

	if (directory_handle->workgroups != NULL) {
		for (l = directory_handle->workgroups; l != NULL; l = l->next) {
			g_free (l->data);
		}
		g_list_free (directory_handle->workgroups);
	}

	res = MATE_VFS_OK;
	
	if (directory_handle->dir != NULL) {
		LOCK_SMB ();
		init_authentication (&actx, NULL);

		/* Important: perform_authentication leaves and re-enters the lock! */
		while (perform_authentication (&actx) > 0) {
			err = smb_context->closedir (smb_context, directory_handle->dir);
			actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
		}
		
		res = actx.res;
		UNLOCK_SMB ();
	}
	g_free (directory_handle->path);
	g_free (directory_handle);

	return res;
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	DirectoryHandle *dh = (DirectoryHandle *) method_handle;
	struct smbc_dirent *entry = NULL;
	SmbAuthContext actx;
	struct stat st;
	char *statpath;
	char *path;
	char *escaped;
	int r = -1;
	GList *l;

	DEBUG_SMB (("do_read_directory()\n"));

	if (dh->dir == NULL) {
		if (dh->workgroups == NULL) {
			return MATE_VFS_ERROR_EOF;
		} else {
			/* workgroup link */
			l = dh->workgroups;
			dh->workgroups = g_list_remove_link (dh->workgroups, l);
			file_info->name = l->data;
			g_list_free_1 (l);
			
			file_info->valid_fields =
				MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
				| MATE_VFS_FILE_INFO_FIELDS_TYPE;
			file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
			file_info->mime_type = g_strdup ("application/x-desktop");
			return MATE_VFS_OK;
		}
	}
	
	
	LOCK_SMB();
	do {
		errno = 0;
		
		init_authentication (&actx, NULL);
		
		/* Important: perform_authentication leaves and re-enters the lock! */
		while (perform_authentication (&actx) > 0) {
			entry = smb_context->readdir (smb_context, dh->dir);
			
			if(entry == NULL) {
				if(errno == 0)
					actx.res = MATE_VFS_ERROR_EOF;
				else
					actx.res = mate_vfs_result_from_errno ();
			} else {
				actx.res = MATE_VFS_OK;
			}
		}
		
		if (entry == NULL) {
			UNLOCK_SMB();
			return actx.res;
		}
		
	} while (entry->smbc_type == SMBC_COMMS_SHARE ||
		 entry->smbc_type == SMBC_IPC_SHARE ||
		 entry->smbc_type == SMBC_PRINTER_SHARE ||
		 entry->name == NULL ||
		 strlen (entry->name) == 0 ||
		 (entry->smbc_type == SMBC_FILE_SHARE &&
		  is_hidden_entry (entry->name)));
		
	UNLOCK_SMB();

	file_info->name = g_strndup (entry->name, entry->namelen);
	DEBUG_SMB (("do_read_directory (): read %s\n", file_info->name));

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	switch (entry->smbc_type) {
	case SMBC_FILE_SHARE:
		file_info->valid_fields = file_info->valid_fields
			| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
			| MATE_VFS_FILE_INFO_FIELDS_TYPE;
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->mime_type = g_strdup ("x-directory/smb-share");
		break;
	case SMBC_WORKGROUP:
	case SMBC_SERVER:
		file_info->valid_fields = file_info->valid_fields
			| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
			| MATE_VFS_FILE_INFO_FIELDS_TYPE;
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		file_info->mime_type = g_strdup ("application/x-desktop");
		break;
	case SMBC_PRINTER_SHARE:
		/* Ignored above for now */
		file_info->valid_fields = file_info->valid_fields
			| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE
			| MATE_VFS_FILE_INFO_FIELDS_TYPE;
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		file_info->mime_type = g_strdup ("application/x-smb-printer");
	case SMBC_COMMS_SHARE:
	case SMBC_IPC_SHARE:
		break;
	case SMBC_DIR:
	case SMBC_FILE:
		path = dh->path;
		escaped = mate_vfs_escape_string (file_info->name);

		if (path[strlen(path)-1] == '/') {
			statpath = g_strconcat (path, 
						escaped,
						NULL);
		} else {
			statpath = g_strconcat (path,
						"/",
						escaped,
						NULL);
		}
		g_free (escaped);

		/* TODO: might give an auth error, but should be rare due
		   to the succeeding opendir. If this happens and we can't
		   auth, we should terminate the readdir to avoid multiple
		   password dialogs
		*/
		
		LOCK_SMB();
		init_authentication (&actx, NULL);
		
		/* Important: perform_authentication leaves and re-enters the lock! */
		while (perform_authentication (&actx) > 0) {
			r = smb_context->stat (smb_context, statpath, &st);
			actx.res = (r == 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
		}
		UNLOCK_SMB();
		
		if (r == 0) {
			mate_vfs_stat_to_file_info (file_info, &st);
			file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
			file_info->io_block_size = SMB_BLOCK_SIZE;
		}
		g_free (statpath);

		file_info->valid_fields = file_info->valid_fields
			| MATE_VFS_FILE_INFO_FIELDS_TYPE;

		if (entry->smbc_type == SMBC_DIR) {
			file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
			file_info->mime_type = g_strdup ("x-directory/normal");
			file_info->valid_fields = file_info->valid_fields
				| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		} else {
			file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
			file_info->mime_type = g_strdup (
				mate_vfs_mime_type_from_name(file_info->name));
			file_info->valid_fields = file_info->valid_fields
				| MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		}
		break;
	case SMBC_LINK:
		g_warning ("smb links not supported");
		/*FIXME*/
		break;
	default:
		g_assert_not_reached ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_seek (MateVFSMethod *method,
		MateVFSMethodHandle *method_handle,
		MateVFSSeekPosition whence,
		MateVFSFileOffset offset,
		MateVFSContext *context)
{
	FileHandle *handle = (FileHandle *)method_handle;
	SmbAuthContext actx;
	int meth_whence;
	off_t ret = (off_t) -1;

	if (handle->is_data) {
		switch (whence) {
		case MATE_VFS_SEEK_START:
			handle->offset = MIN (offset, handle->file_size);
			break;
		case MATE_VFS_SEEK_CURRENT:
			handle->offset = MIN (handle->offset + offset, handle->file_size);
			break;
		case MATE_VFS_SEEK_END:
			if (offset > handle->file_size) {
				handle->offset = 0;
			} else {
				handle->offset = handle->file_size - offset;
			}
			break;
		default:
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		}
		return MATE_VFS_OK;
	}
	
	switch (whence) {
	case MATE_VFS_SEEK_START:
		meth_whence = SEEK_SET;
		break;
	case MATE_VFS_SEEK_CURRENT:
		meth_whence = SEEK_CUR;
		break;
	case MATE_VFS_SEEK_END:
		meth_whence = SEEK_END;
		break;
	default:
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	LOCK_SMB();
	init_authentication (&actx, NULL);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		ret = smb_context->lseek (smb_context, handle->file, (off_t) offset, meth_whence);
		actx.res = (ret != (off_t) -1) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	UNLOCK_SMB();
	
	return actx.res;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
		MateVFSMethodHandle *method_handle,
		MateVFSFileSize *offset_return)
{
	FileHandle *handle = (FileHandle *)method_handle;
	SmbAuthContext actx;
	off_t ret = (off_t) -1;

	if (handle->is_data) {
		*offset_return = handle->offset;
		return MATE_VFS_OK;
	}
	
	LOCK_SMB();
	init_authentication (&actx, NULL);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		ret = smb_context->lseek (smb_context, handle->file, (off_t) 0, SEEK_CUR);
		actx.res = (ret != (off_t) -1) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	UNLOCK_SMB();
	
	*offset_return = (ret == (off_t) -1) ? 0 : (MateVFSFileOffset) ret;
	return actx.res;
}

static MateVFSResult
do_unlink (MateVFSMethod *method,
	   MateVFSURI *uri,
	   MateVFSContext *context)
{
	char *path;
	SmbAuthContext actx;
	int type, err = -1;

	DEBUG_SMB (("do_unlink() %s\n",
				mate_vfs_uri_to_string (uri, 0)));

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE ||
	    type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	LOCK_SMB();
	init_authentication (&actx, uri);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		err = smb_context->unlink (smb_context, path);
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	
	UNLOCK_SMB();

	g_free (path);
	
	return actx.res;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
		  MateVFSURI *a,
		  MateVFSURI *b,
		  gboolean *same_fs_return,
		  MateVFSContext *context)
{
	char *server1;
	char *server2;
	char *path1;
	char *path2;
	char *p1, *p2;

	DEBUG_SMB (("do_check_same_fs()\n"));

	server1 =
		mate_vfs_unescape_string (mate_vfs_uri_get_host_name (a),
					   NULL);
	server2 =
		mate_vfs_unescape_string (mate_vfs_uri_get_host_name (b),
					   NULL);
	path1 =
		mate_vfs_unescape_string (mate_vfs_uri_get_path (a), NULL);
	path2 =
		mate_vfs_unescape_string (mate_vfs_uri_get_path (b), NULL);
                                                    
	if (!server1 || !server2 || !path1 || !path2 ||
	    (strcmp (server1, server2) != 0)) {
		g_free (server1);
		g_free (server2);
		g_free (path1);
		g_free (path2);
		*same_fs_return = FALSE;
		return MATE_VFS_OK;
	}
                             
	p1 = path1;
	p2 = path2;
	if (*p1 == '/') {
		p1++;
	}
	if (*p2 == '/') {
		p2++;
	}

	/* Make sure both URIs are on the same share: */
	while (*p1 && *p2 && *p1 == *p2 && *p1 != '/') {
		p1++;
		p2++;
	}
	if (*p1 == 0 || *p2 == 0 || *p1 != *p2) {
		*same_fs_return = FALSE;
	} else {
		*same_fs_return = TRUE;
	}
                                            
	g_free (server1);
	g_free (server2);
	g_free (path1);
	g_free (path2);

	return MATE_VFS_OK;
}

static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	char *old_path, *new_path;
	int errnox = 0, err = -1;
	gboolean tried_once;
	SmbAuthContext actx;
	int old_type, new_type;
	
	
	DEBUG_SMB (("do_move() %s %s\n",
				mate_vfs_uri_to_string (old_uri, 0),
				mate_vfs_uri_to_string (new_uri, 0)));

	old_type = smb_uri_type (old_uri);
	new_type = smb_uri_type (new_uri);

	if (old_type != SMB_URI_SHARE_FILE ||
	    new_type != SMB_URI_SHARE_FILE) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	/* Transform the URI into a completely unescaped string */
	old_path = mate_vfs_uri_to_string (old_uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);
	new_path = mate_vfs_uri_to_string (new_uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	tried_once = FALSE;
 retry:
	LOCK_SMB();
	init_authentication (&actx, old_uri);
	
	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		err = smb_context->rename (smb_context, old_path, smb_context, new_path);
		errnox = errno;
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	UNLOCK_SMB();
	
	if (err < 0) {
		if (errnox == EXDEV) {
			actx.res = MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
			
		} else if (err == EEXIST && force_replace != FALSE) {
			/* If the target exists and force_replace is TRUE */
			LOCK_SMB();
			init_authentication (&actx, new_uri);

			/* Important: perform_authentication leaves and re-enters the lock! */
			while (perform_authentication (&actx) > 0) {			
				err = smb_context->unlink (smb_context, new_path);
				actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
			}
			UNLOCK_SMB();

			if (err >= 0) {
				if (!tried_once) {
					tried_once = TRUE;
					goto retry;
				}
				actx.res = MATE_VFS_ERROR_FILE_EXISTS;
			}
		}
	}

	g_free (old_path);
	g_free (new_path);

	return actx.res;
}

static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)

{
	DEBUG_SMB(("do_truncate_handle\n"));
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	char *path;
	int type, err = -1;
	SmbAuthContext actx;

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE ||
	    type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	/* Transform the URI into a completely unescaped string */
	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	LOCK_SMB();
	init_authentication (&actx, uri);

	/* Important: perform_authentication leaves and re-enters the lock! */
	while (perform_authentication (&actx) > 0) {
		err = smb_context->mkdir (smb_context, path, perm);
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}

	UNLOCK_SMB();

	g_free (path);

	return actx.res;
}

static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	char *path;
	int err = -1, type;
	SmbAuthContext actx;

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE ||
	    type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	/* Transform the URI into a completely unescaped string */
	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);

	LOCK_SMB();
	init_authentication (&actx, uri);

	while (perform_authentication (&actx) > 0) {
		err = smb_context->rmdir (smb_context, path);
		actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
	}
	UNLOCK_SMB();

	g_free (path);

	return actx.res;
}

static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{
	char *path;
	int err = -1, errnox = 0, type;
	SmbAuthContext actx;	

	DEBUG_SMB (("do_set_file_info: mask %x\n", mask));

	type = smb_uri_type (uri);

	if (type == SMB_URI_ERROR) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	if (type == SMB_URI_WHOLE_NETWORK ||
	    type == SMB_URI_WORKGROUP ||
	    type == SMB_URI_SERVER ||
	    type == SMB_URI_SHARE ||
	    type == SMB_URI_WORKGROUP_LINK ||
	    type == SMB_URI_SERVER_LINK) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	path = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);
	
	if (mask & MATE_VFS_SET_FILE_INFO_NAME) {
		MateVFSURI *parent, *new_uri;
		char *new_path;

		parent = mate_vfs_uri_get_parent (uri);
		new_uri = mate_vfs_uri_append_file_name (parent, info->name);
		mate_vfs_uri_unref (parent);
		new_path = mate_vfs_uri_to_string (new_uri, MATE_VFS_URI_HIDE_USER_NAME | MATE_VFS_URI_HIDE_PASSWORD);
		mate_vfs_uri_unref (new_uri);


		LOCK_SMB();
		init_authentication (&actx, uri);
		
		while (perform_authentication (&actx) > 0) {
			err = smb_context->rename (smb_context, path, smb_context, new_path);
			errnox = errno;
			actx.res = (err >= 0) ? MATE_VFS_OK : mate_vfs_result_from_errno ();
		}
		
		UNLOCK_SMB();

		if (err < 0 && errnox == EXDEV)
			actx.res = MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
		
		g_free (path);
		path = new_path;

		if (actx.res != MATE_VFS_OK) {
			g_free (path);
			return actx.res;
		}
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (path);
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_PERMISSIONS) {
		g_free (path);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_OWNER) {
		g_free (path);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	if (mask & MATE_VFS_SET_FILE_INFO_TIME) {
		g_free (path);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	g_free (path);
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
	NULL, /* do_truncate */
	NULL, /* do_find_directory */
	NULL  /* do_create_symbolic_link */
};

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	smb_lock = g_mutex_new ();

	DEBUG_SMB (("<-- smb module init called -->\n"));

	if (try_init ()) {
		return &method;
	} else {
		return NULL;
	}
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	LOCK_SMB();
	if (smb_context != NULL) {
		smbc_free_context (smb_context, 1);
		smb_context = NULL;
	}
	UNLOCK_SMB();

	g_hash_table_destroy (server_cache);
	g_hash_table_destroy (workgroups);
        g_hash_table_destroy (user_cache);
	
	g_mutex_free (smb_lock);

	DEBUG_SMB (("<-- smb module shutdown called -->\n"));
}

