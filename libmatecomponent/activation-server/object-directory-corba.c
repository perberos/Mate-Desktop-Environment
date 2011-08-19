/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  oafd: OAF CORBA dameon.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 1999, 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Elliot Lee <sopwith@redhat.com>,
 *
 */

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <glib/gi18n.h>
#include "server.h"
#include "object-directory.h"
#include "matecomponent-activation/matecomponent-activation-private.h"
#include "activation-server-corba-extensions.h"

#include <glib/gstdio.h>

/* no longer used. */
#define RESIDUAL_SERVERS 0

static GObjectClass *parent_class = NULL;
static gboolean finished_internal_registration = FALSE;

typedef struct {
	char *iid;
	int   n_servers;
	struct {
		MateComponent_ActivationEnvironment environment;
		CORBA_Object                 server;
	} servers [1]; /* flexible array */
} ActiveServerList;

typedef struct {
          /* client's environment */
        MateComponent_ActivationEnvironment *env;
          /* "runtime" servers registered by this client */
        CORBA_sequence_CORBA_string *runtime_iids;
} ClientContext;

static void
client_context_free (ClientContext *self)
{
        CORBA_free (self->env);
        if (self->runtime_iids)
                CORBA_free (self->runtime_iids);
        g_free (self);
}

static ObjectDirectory *main_dir = NULL;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
static void
od_dump_list (ObjectDirectory * od)
{
#if 0
	int i, j, k;

	for (i = 0; i < od->attr_servers->_length; i++) {
		g_print ("IID %s, type %s, location %s\n",
			 od->attr_servers->_buffer[i].iid,
			 od->attr_servers->_buffer[i].server_type,
			 od->attr_servers->_buffer[i].location_info);
		for (j = 0; j < od->attr_servers->_buffer[i].props._length;
		     j++) {
			MateComponent_ActivationProperty *prop =
				&(od->attr_servers->_buffer[i].
				  props._buffer[j]);
                        if (strchr (prop->name, '-') != NULL) /* Translated, likely to
                                                         be annoying garbage value */
                                continue;

			g_print ("    %s = ", prop->name);
			switch (prop->v._d) {
			case MateComponent_ACTIVATION_P_STRING:
				g_print ("\"%s\"\n", prop->v._u.value_string);
				break;
			case MateComponent_ACTIVATION_P_NUMBER:
				g_print ("%f\n", prop->v._u.value_number);
				break;
			case MateComponent_ACTIVATION_P_BOOLEAN:
				g_print ("%s\n",
					 prop->v.
					 _u.value_boolean ? "TRUE" : "FALSE");
				break;
			case MateComponent_ACTIVATION_P_STRINGV:
				g_print ("[");
				for (k = 0;
				     k < prop->v._u.value_stringv._length;
				     k++) {
					g_print ("\"%s\"",
						 prop->v._u.
						 value_stringv._buffer[k]);
					if (k <
					    (prop->v._u.
					     value_stringv._length - 1))
						g_print (", ");
				}
				g_print ("]\n");
				break;
			}
		}
	}
#endif
}
#endif

static gboolean
registry_directory_needs_update (ObjectDirectory *od,
                                 const char      *directory)
{
        gboolean needs_update;
        struct stat statbuf;
        time_t old_mtime;

        if (g_stat (directory, &statbuf) != 0) {
                return FALSE;
        }
 
        old_mtime = (time_t) g_hash_table_lookup (
                od->registry_directory_mtimes, directory);

        g_hash_table_insert (od->registry_directory_mtimes,
                             (gpointer) directory,
                             (gpointer) statbuf.st_mtime);

        needs_update = (old_mtime != statbuf.st_mtime);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        if (needs_update)
                g_warning ("Compare old_mtime on '%s' with %ld ==? %ld",
                           directory,
                           (long) old_mtime, (long) statbuf.st_mtime);
#endif

        return needs_update;
}

static void
update_registry (ObjectDirectory *od, gboolean force_reload)
{
        int i;
        time_t cur_time;
        gboolean must_load;
        static gboolean doing_reload = FALSE;

        if (doing_reload)
                return;
        doing_reload = TRUE;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_warning ("Update registry %p", od->by_iid);
#endif

        /* get first time init right */
        must_load = (od->by_iid == NULL);
        
        cur_time = time (NULL);

        if (cur_time - 5 > od->time_did_stat) {
                od->time_did_stat = cur_time;
                
                for (i = 0; od->registry_source_directories[i] != NULL; i++) {
                        if (registry_directory_needs_update 
                            (od, od->registry_source_directories[i]))
                                must_load = TRUE;
                }
        }
        
        if (must_load || force_reload) {
                /*
                 * FIXME bugzilla.eazel.com 2727: we should only reload those
                 * directories that have actually changed instead of reloading
                 * all when any has changed. 
                 */
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                g_warning ("Re-load %d %d", must_load, force_reload);
#endif
                if (od->attr_servers)
                        CORBA_free (od->attr_servers);
                od->attr_servers = CORBA_sequence_MateComponent_ServerInfo__alloc ();
                matecomponent_server_info_load (od->registry_source_directories,
                                         od->attr_servers,
                                         od->attr_runtime_servers,
                                         &od->by_iid,
                                         matecomponent_activation_hostname_get ());
                od->time_did_stat = od->time_list_changed = time (NULL);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                od_dump_list (od);
#endif
                if (must_load)
                        activation_clients_cache_notify ();
        }

        doing_reload = FALSE;
}

static gchar **
split_path_unique (const char *colon_delimited_path)
{
        int i, max;
        gboolean different;
        gchar **ret, **wrk;
        GSList *l, *tmp = NULL;

        g_return_val_if_fail (colon_delimited_path != NULL, NULL);

        wrk = g_strsplit (colon_delimited_path, G_SEARCHPATH_SEPARATOR_S, -1);

        g_return_val_if_fail (wrk != NULL, NULL);

        for (max = i = 0; wrk [i]; i++) {
                different = TRUE;
                for (l = tmp; l; l = l->next) {
                        if (!strcmp (l->data, wrk [i])) {
                                different = FALSE;
                        } else if (wrk [i] == '\0') {
                                different = FALSE;
                        }
                }
                if (different) {
                        tmp = g_slist_prepend (tmp, g_strdup (wrk [i]));
                        max++;
                }
        }

        tmp = g_slist_reverse (tmp);

        ret = g_new (char *, max + 1);

        for (l = tmp, i = 0; l; l = l->next)
                ret [i++] = l->data;

        ret [i] = NULL;

        g_slist_free (tmp);
        g_strfreev (wrk);

        return ret;
}

static MateComponent_ServerInfoListCache *
impl_MateComponent_ObjectDirectory__get_servers (
        PortableServer_Servant           servant,
        MateComponent_CacheTime                 only_if_newer,
        CORBA_Environment               *ev)
{
	ObjectDirectory *od;
	MateComponent_ServerInfoListCache *retval;

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

        update_registry (od, FALSE);

	retval = MateComponent_ServerInfoListCache__alloc ();

	retval->_d = (only_if_newer < od->time_list_changed);
	if (retval->_d) {
		retval->_u.server_list = *od->attr_servers;
		CORBA_sequence_set_release (&retval->_u.server_list,
					    CORBA_FALSE);
	}

        server_unlock ();

	return retval;
}

typedef struct {
	MateComponent_ImplementationID *seq;
	int                      last_used;
} StateCollectionInfo;

static void
collate_active_server (char *key, gpointer value, StateCollectionInfo *sci)
{
	sci->seq [(sci->last_used)++] = CORBA_string_dup (key);
}

static MateComponent_ServerStateCache *
impl_MateComponent_ObjectDirectory_get_active_servers (
        PortableServer_Servant           servant,
        MateComponent_CacheTime                 only_if_newer,
        CORBA_Environment               *ev)
{
	ObjectDirectory *od;
	MateComponent_ServerStateCache *retval;

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

	retval = MateComponent_ServerStateCache__alloc ();

	retval->_d = (only_if_newer < od->time_active_changed);
	if (retval->_d) {
		StateCollectionInfo sci;

		retval->_u.active_servers._length =
			g_hash_table_size (od->active_server_lists);
		retval->_u.active_servers._buffer = sci.seq =
			CORBA_sequence_MateComponent_ImplementationID_allocbuf
			(retval->_u.active_servers._length);
		sci.last_used = 0;

		g_hash_table_foreach (od->active_server_lists,
				      (GHFunc) collate_active_server, &sci);
		CORBA_sequence_set_release (&(retval->_u.active_servers),
					    CORBA_TRUE);
	}

        server_unlock ();

	return retval;
}

static CORBA_Object 
od_get_active_server (ObjectDirectory    *od,
		      const char                         *iid,
		      const MateComponent_ActivationEnvironment *environment)
{
	ActiveServerList *servers;
	CORBA_Object      retval;
	int               i;

	servers = g_hash_table_lookup (od->active_server_lists, iid);
	if (!servers)
		return CORBA_OBJECT_NIL;

	retval = CORBA_OBJECT_NIL;

	for (i = 0; i < servers->n_servers; i++) {
		if (MateComponent_ActivationEnvironment_match (
				&servers->servers [i].environment,
				environment)) {
			retval = servers->servers [i].server;
			break;
		}
        }
	if (retval != CORBA_OBJECT_NIL) {
                gboolean non_existent;
                ServerLockState state;

                /* With dead objects, this path can cause lengthy
                   re-connection attempts, blocking all other
                   activation, so drop the lock. g#512520 */
                state = server_lock_drop();
                non_existent = CORBA_Object_non_existent (retval, NULL);
                server_lock_resume (state);

                if (!non_existent)
                        return CORBA_Object_duplicate (retval, NULL);
        }

	return CORBA_OBJECT_NIL;
}


/*
 * returns (@merged_environment) new environment as result of
 * merging activation request environment and client registered
 * environment; the activation supplied environment takes precedence
 * over the client one
 */
static void
od_merge_client_environment (ObjectDirectory                    *od,
                             MateComponent_ServerInfo const            *server,
                             const MateComponent_ActivationEnvironment *environment,
                             MateComponent_ActivationEnvironment       *merged_environment,
                             MateComponent_ActivationClient             client)
{
        GArray *array;
        int i, serverinfo_env_idx;
        const MateComponent_ActivationEnvironment *client_env;
        const MateComponent_StringList *serverinfo_env = NULL;
        ClientContext *client_context;

        array = g_array_new (FALSE, FALSE, sizeof (MateComponent_ActivationEnvValue));

          /* copy all values from @environment */
        for (i = 0; i < environment->_length; ++i)
                g_array_append_val (array, environment->_buffer[i]);

        if (G_UNLIKELY (client == CORBA_OBJECT_NIL))
                goto exit;

        client_context = ((ClientContext *) g_hash_table_lookup
                          (od->client_contexts, client));
        if (G_UNLIKELY (!client_context))
                goto exit;

        client_env = client_context->env;
        if (G_UNLIKELY (!client_env))
                goto exit;

        /* scan through server properties */
        if (!server) goto exit;
        for (i = 0; i < server->props._length; ++i) {
                if (strcmp (server->props._buffer[i].name, "matecomponent:environment") == 0)
                {
                        MateComponent_ActivationPropertyValue const *prop =
                                &server->props._buffer[i].v;
                        if (prop->_d == MateComponent_ACTIVATION_P_STRINGV)
                                serverinfo_env = &prop->_u.value_stringv;
                        else
                                g_warning ("matecomponent:environment should have type stringv");
                        break;
                }
        }
        if (!serverinfo_env)
                goto exit;

        /* do the actual merging */
        for (serverinfo_env_idx = 0;
             serverinfo_env_idx < serverinfo_env->_length; ++serverinfo_env_idx)
        {
                CORBA_char *env = serverinfo_env->_buffer[serverinfo_env_idx];
                gboolean duplicated_env = FALSE;

                /* check if array already has this environment */
                for (i = 0; i < environment->_length; ++i) {
                        if (strcmp (environment->_buffer[i].name, env) == 0) {
                                duplicated_env = TRUE;
                                break;
                        }
                }
                if (duplicated_env)
                        continue;

                /* look for environment in client_env */
                for (i = 0; i < client_env->_length; ++i) {
                        if (strcmp (client_env->_buffer[i].name, env) == 0) {
                                g_array_append_val (array, client_env->_buffer[i]);
                                break;
                        }
                }
        }
exit:
        /* return the resulting environment */
        merged_environment->_buffer = (MateComponent_ActivationEnvValue *) array->data;
        merged_environment->_length = merged_environment->_maximum = array->len;
        g_array_free (array, FALSE);
}


static CORBA_Object
impl_MateComponent_ObjectDirectory_activate (
	PortableServer_Servant              servant,
	const CORBA_char                   *iid,
	const MateComponent_ActivationContext      ac,
	const MateComponent_ActivationEnvironment *environment,
	const MateComponent_ActivationFlags        flags,
	MateComponent_ActivationClient             client,
	CORBA_Context                       ctx,
	CORBA_Environment                  *ev)
{
	ObjectDirectory   *od;
	CORBA_Object       retval = NULL;
	MateComponent_ServerInfo *si;
	ODActivationInfo   ai;

        MateComponent_ActivationEnvironment merged_environment = { 0, };

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

        od_merge_client_environment (od, (MateComponent_ServerInfo *)
                                     g_hash_table_lookup (od->by_iid, iid),
                                     environment, &merged_environment, client);

	retval = CORBA_OBJECT_NIL;

        update_registry (od, FALSE);

        if (!(flags & MateComponent_ACTIVATION_FLAG_PRIVATE)) {
                retval = od_get_active_server (od, iid, &merged_environment);

                if (retval != CORBA_OBJECT_NIL)
                        goto act_out;
        }

	if (flags & MateComponent_ACTIVATION_FLAG_EXISTING_ONLY)
                goto act_out;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        fprintf (stderr, "thread %p start activate '%s'\n",
                 g_thread_self(), iid);
#endif

	ai.ac = ac;
	ai.flags = flags;
	ai.ctx = ctx;

	si = g_hash_table_lookup (od->by_iid, iid);

	if (si) {
		retval = od_server_activate (
				si, &ai, MATECOMPONENT_OBJREF (od), &merged_environment, client, ev);
                /* NB. si can now be invalid - due to re-enterancy */

                /* If we failed to activate - it may be because our
                 * request re-entered _during_ the activation
                 * process resulting in a second process being started
                 * but failing to register - so we'll look up again here
                 * to see if we can get it.
                 * FIXME: we should not be forking redundant processes
                 * while an activation of that same process is on the
                 * stack.
                 * FIXME: we only get away with this hack because we
                 * try and fork another process & thus allow the reply
                 * from the initial process to be handled in the event
                 * loop.
                 */
                /* FIXME: this path is theoretically redundant now */
                if (ev->_major != CORBA_NO_EXCEPTION ||
                    retval == CORBA_OBJECT_NIL) {
                        retval = od_get_active_server (od, iid, &merged_environment);

                        if (retval != CORBA_OBJECT_NIL)
                                CORBA_exception_free (ev);
                }
        }

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        fprintf (stderr, "thread %p end activate '%s' = %p ['%s']\n",
                 g_thread_self(), iid, retval,
                 matecomponent_exception_get_text (ev)
                 );
#endif

 act_out:
        g_free (merged_environment._buffer);

        server_unlock ();

	return retval;
}

extern GMainLoop *main_loop;

static gboolean
quit_server_timeout (gpointer user_data)
{
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_warning ("Quit server !");
#endif

        if (!main_dir ||
            main_dir->n_active_servers > RESIDUAL_SERVERS ||
            !activation_clients_is_empty_scan ())
                g_warning ("Serious error handling server count, not quitting");
        else
                g_main_loop_quit (main_loop);

        main_dir->no_servers_timeout = 0;

        return FALSE;
}

void od_finished_internal_registration (void)
{
	finished_internal_registration = TRUE;
}

void
check_quit (void)
{
        ObjectDirectory *od = main_dir;

        /* We had some activity - so push out the shutdown timeout */
        if (od->no_servers_timeout != 0)
                g_source_remove (od->no_servers_timeout);
        od->no_servers_timeout = 0;

        if (od->n_active_servers <= RESIDUAL_SERVERS &&
            activation_clients_is_empty_scan ())
                od->no_servers_timeout = g_timeout_add_seconds (
                        SERVER_IDLE_QUIT_TIMEOUT, quit_server_timeout, NULL);

	od->time_active_changed = time (NULL);
}

static void
remove_active_server_entry (ActiveServerList *servers,
			    int               index)
{
	CORBA_Object_release (servers->servers [index].server, NULL);
	CORBA_free (servers->servers [index].environment._buffer);

	if (index != servers->n_servers - 1)
		memcpy (&servers->servers [index],
			&servers->servers [servers->n_servers - 1],
			sizeof (servers->servers [index]));

	servers->n_servers--;
}

static ActiveServerList *
add_active_server_entry (ActiveServerList                   *servers,
			 const MateComponent_ActivationEnvironment *environment,
			 CORBA_Object                        object)
{
	int index, i;

	index = servers->n_servers - 1;

	if (index != 0)
		servers = g_realloc (servers,
				     sizeof (*servers) + sizeof (servers->servers [0]) * index);

	servers->servers [index].server = CORBA_Object_duplicate (object, NULL);

	servers->servers [index].environment._length  = environment->_length;
	servers->servers [index].environment._maximum = environment->_maximum;
	servers->servers [index].environment._buffer  =
				MateComponent_ActivationEnvironment_allocbuf (environment->_length);
	servers->servers [index].environment._release = TRUE;

	for (i = 0; i < environment->_length; i++)
		MateComponent_ActivationEnvValue_copy (
			&servers->servers [index].environment._buffer [i],
			&environment->_buffer [i]);

	return servers;
}

static gboolean
prune_dead_servers (gpointer key,
                    gpointer value,
                    gpointer user_data)
{
	ObjectDirectory *od = user_data;
	ActiveServerList                *servers = value;
	int                              i;

	for (i = 0; i < servers->n_servers; i++) {
		MateCORBAConnectionStatus  status;
		gboolean               dead;

		status = MateCORBA_small_get_connection_status (
					servers->servers [i].server);

		dead = (status == MATECORBA_CONNECTION_DISCONNECTED);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
		fprintf (stderr, "IID '%20s' (%p), %s \n",
			 (char *) key, servers->servers [i].server,
			 dead ? "dead" : "alive");
#endif
		if (dead) {
			remove_active_server_entry (servers, i);

			od->n_active_servers--;
			i--;
		}
	}
        
        return !servers->n_servers;
}

static gboolean
as_rescan (gpointer is_idle_rescan)
{
        ObjectDirectory *od;
        static gboolean in_rescan = FALSE;
        static guint idle_id = 0;

        server_lock ();

        if (GPOINTER_TO_UINT (is_idle_rescan))
                idle_id = 0;

        if (!(od = main_dir)) { /* shutting down */
                server_unlock ();
                return FALSE;
        }

        /* We tend to get a lot of 'broken' callbacks at once */
        if (in_rescan) {
                if (!idle_id)
                        idle_id = g_timeout_add (100, as_rescan, GUINT_TO_POINTER (1));
                server_unlock ();
                return FALSE;
        }
        in_rescan = TRUE;

        g_hash_table_foreach_remove (od->active_server_lists,
                                     prune_dead_servers, od);
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_warning ("After prune: %d live servers",
                   od->n_active_servers - RESIDUAL_SERVERS);
#endif

        check_quit ();

        in_rescan = FALSE;
        server_unlock ();

        return FALSE;
}

static void
active_server_cnx_broken (MateCORBAConnection *cnx,
                          gpointer         dummy)

{
        as_rescan (NULL);
}

static void
add_active_server (ObjectDirectory    *od,
		   const char                         *iid,
		   const MateComponent_ActivationEnvironment *environment,
		   CORBA_Object                       object)
{
	ActiveServerList *servers;
        MateCORBAConnection  *cnx;

        cnx = MateCORBA_small_get_connection (object);
        if (cnx) {
                if (!g_object_get_data (G_OBJECT (cnx), "object_count")) {
                        g_object_set_data (
                                G_OBJECT (cnx), "object_count", GUINT_TO_POINTER (1));
                        
                        MateCORBA_small_listen_for_broken
                                (object, G_CALLBACK (active_server_cnx_broken), NULL);
                }
        } else
                g_assert (!strcmp (iid, NAMING_CONTEXT_IID) ||
                          !strcmp(iid, EVENT_SOURCE_IID));

	servers = g_hash_table_lookup (od->active_server_lists, iid);
	if (!servers) {
		servers = g_new0 (ActiveServerList, 1);

		servers->iid       = g_strdup (iid);
		servers->n_servers = 1;

		servers = add_active_server_entry (
				servers, environment, object);

		g_hash_table_insert (
			od->active_server_lists, servers->iid, servers);
	} else {
		ActiveServerList *new_servers;

		g_assert (servers->n_servers > 0);

		servers->n_servers++;

		new_servers = add_active_server_entry (
					servers, environment, object);

		if (new_servers != servers) { /* Need to reset the pointer */
			g_hash_table_steal (od->active_server_lists, new_servers->iid);

			g_hash_table_insert (
				od->active_server_lists, new_servers->iid, new_servers);
		}
	}

	if (finished_internal_registration)
		od->n_active_servers++;

        if (cnx)
                check_quit ();
}

static void
active_server_list_free (gpointer data)
{
	ActiveServerList *servers = data;
	int               i;

	for (i = 0; i < servers->n_servers; i++) {
		CORBA_Object_release (servers->servers [i].server, NULL);
		CORBA_free (servers->servers [i].environment._buffer);
	}

	g_free (servers);
}

static gboolean
remove_active_server (ObjectDirectory *od,
                      const char                      *iid,
                      CORBA_Object                     object)
{
	ActiveServerList *servers;
	gboolean          removed = FALSE;
	int               i;

	servers = g_hash_table_lookup (od->active_server_lists, iid);
        if (!servers)
                return FALSE;

	for (i = 0; i < servers->n_servers; i++)
		if (CORBA_Object_is_equivalent (
				servers->servers [i].server, object, NULL)) {
			remove_active_server_entry (servers, i);
			removed = TRUE;
			break;
		}

	if (removed)
		od->n_active_servers--;

	if (servers->n_servers == 0)
		g_hash_table_remove (od->active_server_lists, iid);

        check_quit ();

	return removed;
}

  /* Parse server description and register it, replacing older
   * definition if necessary.  Returns the regsitered ServerInfo */
static MateComponent_ServerInfo const *
od_register_runtime_server_info (ObjectDirectory         *od,
                                 const char              *iid,
                                 const CORBA_char        *description,
                                 MateComponent_ActivationClient  client)
{
        MateComponent_ServerInfo *old_serverinfo, *new_serverinfo;
        GSList *parsed_serverinfo = NULL, *l;
        int     i;
        ClientContext *context;

        update_registry (od, FALSE);

        old_serverinfo = (MateComponent_ServerInfo *) g_hash_table_lookup (od->by_iid, iid);
        if (old_serverinfo)
                return old_serverinfo;
        if (!(*description)) /* empty description? */
                return NULL;

          /* parse description */
         matecomponent_parse_server_info_memory (description, &parsed_serverinfo,
                                          matecomponent_activation_hostname_get ());

           /* check for zero entries */
         if (!parsed_serverinfo)
                 return NULL;
           /* check for more than one entry */
         if (parsed_serverinfo->next) {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                 g_warning ("More than one <oaf_server> specified, ignoring all");
#endif
                 for (l = parsed_serverinfo; l; l = l->next) {
                         MateComponent_ServerInfo__freekids (l->data, NULL);
                         g_free (l->data);
                 }
                 g_slist_free (parsed_serverinfo);
                 return NULL;
         }
         new_serverinfo = (MateComponent_ServerInfo *) parsed_serverinfo->data;
         g_slist_free (parsed_serverinfo);

         g_ptr_array_add (od->attr_runtime_servers, new_serverinfo);
         MateCORBA_sequence_append (od->attr_servers, new_serverinfo);
           /* rebuild od->by_iid hash table, because
            * MateCORBA_sequence_append reallocs _buffer, and that
            * sometimes changes the addresses of the
            * serverinfo items */
         g_hash_table_destroy (od->by_iid);
         od->by_iid = g_hash_table_new (g_str_hash, g_str_equal);
         for (i = 0; i < od->attr_servers->_length; ++i)
                 g_hash_table_insert (od->by_iid,
                                      od->attr_servers->_buffer[i].iid,
                                      od->attr_servers->_buffer + i);
 
           /* Take note that this client registered this iid, so that
            * when the client disconnects we unregister its
            * corresponding serverinfo's */
         context = g_hash_table_lookup (od->client_contexts, client);
         g_return_val_if_fail (context, new_serverinfo);
         if (context->runtime_iids)
                 MateCORBA_sequence_append (context->runtime_iids, iid);
         else {
                 context->runtime_iids = MateCORBA_sequence_alloc 
                         (TC_CORBA_sequence_CORBA_string, 1);
                 MateCORBA_sequence_index (context->runtime_iids, 0) =
                         CORBA_string_dup (iid);
         }

         od->time_list_changed = time (NULL);
         activation_clients_cache_notify ();
         return new_serverinfo;
}

static MateComponent_RegistrationResult
impl_MateComponent_ObjectDirectory_register_new_full (
	PortableServer_Servant              servant,
	const CORBA_char                   *iid,
	const MateComponent_ActivationEnvironment *environment,
	const CORBA_Object                  obj,
        MateComponent_RegistrationFlags            flags,
        const CORBA_char                   *description,
        CORBA_Object                       *existing,
        MateComponent_ActivationClient             client,
	CORBA_Environment                  *ev)
{
	ObjectDirectory              *od;
	CORBA_Object                  oldobj;
        MateComponent_ActivationEnvironment  merged_environment;
        MateComponent_ServerInfo const      *serverinfo;
        MateComponent_RegistrationResult     retval = MateComponent_ACTIVATION_REG_SUCCESS;

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

	oldobj = od_get_active_server (od, iid, environment);
        *existing = oldobj;

        serverinfo = od_register_runtime_server_info (od, iid, description, client);
        od_merge_client_environment (od, serverinfo, environment,
                                     &merged_environment, client);

	oldobj = od_get_active_server (od, iid, &merged_environment);
	if (oldobj != CORBA_OBJECT_NIL) {
		if (!CORBA_Object_non_existent (oldobj, ev)) {
                        if (CORBA_Object_is_equivalent (oldobj, obj, ev))
                                retval = MateComponent_ACTIVATION_REG_SUCCESS;
                        else
                                retval = MateComponent_ACTIVATION_REG_ALREADY_ACTIVE;
                        goto reg_out;
                }
	}

        if (!serverinfo) {
                if (!(flags&MateComponent_REGISTRATION_FLAG_NO_SERVERINFO)) {
                        retval = MateComponent_ACTIVATION_REG_NOT_LISTED;
                        goto reg_out;
                }
        }

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_warning ("Server register. '%s' : %p", iid, obj);
#endif

        add_active_server (od, iid, &merged_environment, obj);
	
        matecomponent_event_source_notify_listeners
                (od->event_source,
                 "MateComponent/ObjectDirectory:activation:register",
                 NULL, NULL);

 reg_out:
        g_free (merged_environment._buffer);
        server_unlock ();

	return retval;
}

static MateComponent_RegistrationResult
impl_MateComponent_ObjectDirectory_register_new (
	PortableServer_Servant              servant,
	const CORBA_char                   *iid,
	const MateComponent_ActivationEnvironment *environment,
	const CORBA_Object                  obj,
        MateComponent_RegistrationFlags            flags,
        const CORBA_char                   *description,
        CORBA_Object                       *existing,
	CORBA_Environment                  *ev)
{
        return impl_MateComponent_ObjectDirectory_register_new_full
                (servant, iid, environment, obj, flags,
                 description, existing, CORBA_OBJECT_NIL, ev);
}

static void
impl_MateComponent_ObjectDirectory_unregister (
	PortableServer_Servant  servant,
	const CORBA_char       *iid,
	const CORBA_Object      obj,
	CORBA_Environment      *ev)
{
	ObjectDirectory *od;

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

        if (!remove_active_server (od, iid, obj))
                CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                                     ex_MateComponent_ObjectDirectory_NotRegistered,
                                     NULL);
	else
                matecomponent_event_source_notify_listeners
                        (od->event_source,
                         "MateComponent/ObjectDirectory:activation:unregister",
                         NULL, NULL);

        server_unlock ();
}

static MateComponent_DynamicPathLoadResult 
impl_MateComponent_ObjectDirectory_add_path(
	PortableServer_Servant		servant,
	const CORBA_char *		add_path,
	CORBA_Environment               *ev)
{
	ObjectDirectory *od;
	int i, j, dir_num, max;
	char **add_directoies, **ret;
	GSList *l, *tmp = NULL;
	gboolean different;
        MateComponent_DynamicPathLoadResult retval = MateComponent_DYNAMIC_LOAD_SUCCESS;

        server_lock ();

        od = OBJECT_DIRECTORY (servant);

	if (!od->registry_source_directories) {
		od->registry_source_directories = split_path_unique (add_path);
                goto add_path_out;
	} else
		add_directoies = split_path_unique (add_path);

	if (!add_directoies) {
		retval = MateComponent_DYNAMIC_LOAD_ERROR;
                goto add_path_out;
        }

	for (max = i = 0; od->registry_source_directories[i]; i++) {
		tmp = g_slist_append(tmp,g_strdup(od->registry_source_directories[i]));
		max++;
	}

	dir_num = max;

	for (i = 0; add_directoies[i]; i++) {
		different = TRUE;
		for (j = 0; od->registry_source_directories[j]; j++) {
			if (!strcmp(add_directoies[i], od->registry_source_directories[j])) {
				different = FALSE;
				break;
			}
		}	
		if (different) {
			tmp = g_slist_append(tmp, g_strdup(add_directoies[i]));	
			max++;
		}
	}

	if (max == dir_num) {
		g_strfreev(add_directoies);
		g_slist_free(tmp);
		retval = MateComponent_DYNAMIC_LOAD_ALREADY_LISTED;
                goto add_path_out;
	}

	ret = g_new(char *, max + 1);
	for (l = tmp, i = 0; l; l = l->next)
		ret[i++]=l->data;

	ret[i] = NULL;

	g_slist_free(tmp);
	g_strfreev(add_directoies);
	g_strfreev(od->registry_source_directories);

	od->registry_source_directories = ret;
	update_registry(od, TRUE);	

 add_path_out:
        server_unlock ();

	return retval;
}

static MateComponent_DynamicPathLoadResult 
impl_MateComponent_ObjectDirectory_remove_path(
        PortableServer_Servant          servant,
        const CORBA_char *              remove_path,
        CORBA_Environment               *ev)
{
	ObjectDirectory *od;
	char **remove_directoies, **ret;
	int i, j, max;
	GSList *l, *tmp = NULL;
	gboolean different;
        MateComponent_DynamicPathLoadResult retval = MateComponent_DYNAMIC_LOAD_SUCCESS;

        server_lock();

        od = OBJECT_DIRECTORY (servant);

	remove_directoies = split_path_unique (remove_path);
	if (!remove_directoies) {
                retval = MateComponent_DYNAMIC_LOAD_ERROR;
		goto rm_path_out;
        }

	for (max = i = 0; od->registry_source_directories[i]; i++) {
		different = TRUE;
		for (j = 0; remove_directoies[j]; j++) {
			if (!strcmp(od->registry_source_directories[i], remove_directoies[j])) {
				different = FALSE;
				break;
			}
		}

		if (different) {
			tmp = g_slist_append(tmp, g_strdup(od->registry_source_directories[i]));
			max++;
		}
	}

	if (max == i) {
		g_slist_free(tmp);
		g_strfreev(remove_directoies);
                retval = MateComponent_DYNAMIC_LOAD_NOT_LISTED;
		goto rm_path_out;
	}	
	ret = g_new(char *, max + 1);
	for (l = tmp, i = 0; l; l = l->next)
		ret[i++]=l->data;
	ret[i] = NULL;
	
	g_slist_free(tmp);
	g_strfreev(remove_directoies);
	g_strfreev(od->registry_source_directories);

	od->registry_source_directories = ret;
	update_registry(od, TRUE);
 rm_path_out:
        server_unlock();
	return retval;
}


static void
client_cnx_broken (MateCORBAConnection *cnx,
                   const MateComponent_ActivationClient  client)
{
        ObjectDirectory *od;
        ClientContext *context;
        int i;

        server_lock();

        if (!(od = main_dir)) { /* shutting down */
                server_unlock();
                return;
        }

        /* unregister runtime server definitions */
        context = g_hash_table_lookup (od->client_contexts, client);
        if (context->runtime_iids) {
                for (i = 0; i < context->runtime_iids->_length; ++i)
                {
                        CORBA_char *iid = MateCORBA_sequence_index
                                (context->runtime_iids, i);
                        int j;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                        fprintf (stderr, "Removing runtime definition '%s' from hash table\n", iid);
#endif
                        g_hash_table_remove (od->by_iid, iid);
                        for (j = 0; j < od->attr_runtime_servers->len; ++j) {
                                MateComponent_ServerInfo *server =  g_ptr_array_index
                                        (od->attr_runtime_servers, j);
                                if (strcmp (server->iid, iid) == 0) {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                                        fprintf (stderr, "Removing from od->attr_runtime_servers[%i]\n", j);
#endif
                                        g_ptr_array_remove_index
                                                (od->attr_runtime_servers, j);
                                        MateComponent_ServerInfo__freekids (server, NULL);
                                        g_free (server);
                                        break;
                                }
                        }
                        for (j = 0; j < od->attr_servers->_length; ++j) {
                                MateComponent_ServerInfo *server =
                                        &MateCORBA_sequence_index (od->attr_servers, j);
                                if (strcmp (server->iid, iid) == 0) {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                                        fprintf (stderr, "Removing from od->attr_servers[%i]\n", j);
#endif
                                        MateCORBA_sequence_remove (od->attr_servers, j);
                                        break;
                                }
                        }

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                        fprintf (stderr, "Runtime definition '%s' cleaned\n", iid);
#endif
                        
                }
        }
        g_hash_table_remove (od->client_contexts, client);
        od->time_list_changed = time (NULL);
        activation_clients_cache_notify ();

        server_unlock();
}

static void
impl_MateComponent_ObjectDirectory_addClientEnv (
        PortableServer_Servant         servant,
        const MateComponent_ActivationClient  client,
        const MateComponent_StringList       *client_env,
        CORBA_Environment             *ev)
{
        MateComponent_ActivationEnvironment *env;
	ObjectDirectory *od;
        ClientContext *context;
        int i;

        server_lock();

        od = OBJECT_DIRECTORY (servant);
        
        env = MateComponent_ActivationEnvironment__alloc ();
        env->_length  = env->_maximum = client_env->_length;
        env->_buffer  = MateComponent_ActivationEnvironment_allocbuf (env->_length);
        env->_release = CORBA_TRUE;

        for (i = 0; i < client_env->_length; ++i)
        {
                const char *keyval = client_env->_buffer[i];
                const char *equals = strchr (keyval, '=');
                guint       keylen;

                if (!equals) {
                        g_warning ("Duff env. var '%s'", keyval);
                        continue;
                }

                keylen = (guint) (equals - keyval);

                env->_buffer[i].name = CORBA_string_alloc (keylen + 1);
                strncpy (env->_buffer[i].name, keyval, keylen);
                env->_buffer[i].name[keylen] = 0;
                env->_buffer[i].value = CORBA_string_dup (equals + 1);
                env->_buffer[i].flags = 0;
        }

        context = g_new (ClientContext, 1);
        context->env = env;
        context->runtime_iids = NULL;

        g_hash_table_insert (od->client_contexts, client, context);

        MateCORBA_small_listen_for_broken (client, G_CALLBACK (client_cnx_broken),
                                       (gpointer) client);

        server_unlock();
}


MateComponent_ObjectDirectory
matecomponent_object_directory_get (void)
{
        if (!main_dir)
                return CORBA_OBJECT_NIL;
        else
                return MATECOMPONENT_OBJREF (main_dir);
}

MateComponent_EventSource
matecomponent_object_directory_event_source_get (void)
{
     if (!main_dir)
    	      return CORBA_OBJECT_NIL;
     else
              return MATECOMPONENT_OBJREF (main_dir->event_source);
}

void
matecomponent_object_directory_init (PortableServer_POA poa,
                              const char        *registry_path,
                              CORBA_Environment *ev)
{
        g_assert (main_dir == NULL);

        main_dir = g_object_new (OBJECT_TYPE_DIRECTORY,
                                 "poa", poa, NULL);

        main_dir->registry_source_directories = split_path_unique (registry_path);
        update_registry (main_dir, FALSE);
}

void
matecomponent_object_directory_shutdown (PortableServer_POA poa,
                                  CORBA_Environment *ev)
{
        matecomponent_object_set_immortal (MATECOMPONENT_OBJECT (main_dir), FALSE);
        matecomponent_object_unref (MATECOMPONENT_OBJECT (main_dir));
}

CORBA_Object
matecomponent_object_directory_re_check_fn (const MateComponent_ActivationEnvironment *environment,
				     const char                         *act_iid,
				     gpointer                            user_data)
{
        CORBA_Object retval;

        server_lock ();

        retval = od_get_active_server (
                main_dir, (MateComponent_ImplementationID) act_iid, environment);
        
        server_unlock ();

        return retval;
}

void
matecomponent_object_directory_reload (void)
{
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "reloading our object directory!");

        update_registry (main_dir, TRUE);
}

static void 
object_directory_finalize (GObject *object)
{
        ObjectDirectory *od = (ObjectDirectory *) object;

        main_dir = NULL;

        g_hash_table_destroy (od->active_server_lists);
        g_hash_table_destroy (od->registry_directory_mtimes);

        g_strfreev (od->registry_source_directories);

        if (od->client_contexts) {
                g_hash_table_destroy (od->client_contexts);
                od->client_contexts = NULL;
        }

        parent_class->finalize (object);
}

static void
object_directory_class_init (ObjectDirectoryClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
	POA_MateComponent_ObjectDirectory__epv *epv = &klass->epv;

        parent_class = g_type_class_peek_parent (klass);
        object_class->finalize = object_directory_finalize;

        epv->get_servers         = impl_MateComponent_ObjectDirectory__get_servers;
        epv->get_active_servers  = impl_MateComponent_ObjectDirectory_get_active_servers;
	epv->activate            = impl_MateComponent_ObjectDirectory_activate;
	epv->register_new        = impl_MateComponent_ObjectDirectory_register_new;
	epv->register_new_full   = impl_MateComponent_ObjectDirectory_register_new_full;
	epv->unregister          = impl_MateComponent_ObjectDirectory_unregister;
	epv->dynamic_add_path    = impl_MateComponent_ObjectDirectory_add_path;
	epv->dynamic_remove_path = impl_MateComponent_ObjectDirectory_remove_path;
        epv->addClientEnv        = impl_MateComponent_ObjectDirectory_addClientEnv;
}

static void
object_directory_init (ObjectDirectory *od)
{
        matecomponent_object_set_immortal (MATECOMPONENT_OBJECT (od), TRUE);

	od->by_iid = NULL;

        od->registry_directory_mtimes = g_hash_table_new (g_str_hash, g_str_equal);

        od->active_server_lists =
                g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free, active_server_list_free);
        od->no_servers_timeout = 0;

        od->attr_runtime_servers = g_ptr_array_new ();
	
	od->event_source = matecomponent_event_source_new ();
        od->client_contexts = g_hash_table_new_full
                (NULL, NULL, NULL,
                 (GDestroyNotify) client_context_free);
}

MATECOMPONENT_TYPE_FUNC_FULL (ObjectDirectory,
                       MateComponent_ObjectDirectory,
                       MATECOMPONENT_TYPE_OBJECT,
                       object_directory)
