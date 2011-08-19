/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation: A library for accessing matecomponent-activation-server.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Elliot Lee <sopwith@redhat.com>
 */
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include <matecomponent-activation/matecomponent-activation-activate.h>

#include <matecomponent-activation/matecomponent-activation-init.h>
#include <matecomponent-activation/matecomponent-activation-server-info.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <matecomponent-activation/matecomponent-activation-shlib.h>
#include <matecomponent-activation/matecomponent-activation-client.h>
#include <matecomponent-activation/matecomponent-activation-async.h>
#include <glib/gi18n-lib.h>
#include <matecomponent-activation/MateComponent_ActivationContext.h>

static MateComponent_ActivationEnvironment *activation_environment = NULL;

/* FIXME: deprecated internal functions. Should we just remove?
 */
void
matecomponent_activation_set_test_components_enabled (gboolean val)
{
}

gboolean
matecomponent_activation_get_test_components_enabled (void)
{
	return FALSE;
}

static void 
copy_strv_to_sequence (char *const       *selection_order,
		       MateComponent_StringList *str_seq)
{
	int len;

	if (!selection_order) {
		memset (str_seq, 0, sizeof (MateComponent_StringList));
		return;
	}

	for (len = 0; selection_order [len]; len++);

	str_seq->_length  = str_seq->_maximum = len;
	str_seq->_buffer  = (char **) selection_order;
	str_seq->_release = FALSE;
}

/* Limit of the number of cached queries */
#define QUERY_CACHE_MAX 32
#undef QUERY_CACHE_DEBUG

static GHashTable *query_cache = NULL;

typedef struct {
	char  *query;
	char **sort_criteria;

	MateComponent_ServerInfoList *list;
} QueryCacheEntry;

static void
query_cache_entry_free (gpointer data)
{
        QueryCacheEntry *entry = data;

#ifdef QUERY_CACHE_DEBUG
        g_message ("Blowing item %p", entry);
#endif /* QUERY_CACHE_DEBUG */

        g_free (entry->query);
        g_strfreev (entry->sort_criteria);
        CORBA_free (entry->list);
        g_free (entry);
}

static gboolean
cache_clean_half (gpointer  key,
                  gpointer  value,
                  gpointer  user_data)
{
        int *a = user_data;
        /* Blow half the elements */
        return (*a)++ % 2;
}

static gboolean
query_cache_equal (gconstpointer a, gconstpointer b)
{
	int i;
	char **strsa, **strsb;
	const QueryCacheEntry *entrya = a;
	const QueryCacheEntry *entryb = b;

	if (strcmp (entrya->query, entryb->query))
		return FALSE;

	strsa = entrya->sort_criteria;
	strsb = entryb->sort_criteria;

	if (!strsa && !strsb)
		return TRUE;

	if (!strsa || !strsb)
		return FALSE;

	for (i = 0; strsa [i] && strsb [i]; i++)
		if (strcmp (strsa [i], strsb [i]))
			return FALSE;

	if (strsa [i] || strsb [i])
		return FALSE;

	return TRUE;
}

static guint
query_cache_hash (gconstpointer a)
{
	guint hash, i;
	char **strs;
	const QueryCacheEntry *entry = a;
	
	hash = g_str_hash (entry->query);
	strs = entry->sort_criteria;

	for (i = 0; strs && strs [i]; i++)
		hash ^= g_str_hash (strs [i]);

	return hash;
}

static void
query_cache_reset (void)
{
        if (query_cache) {
                g_hash_table_destroy (query_cache);
                query_cache = NULL;
        }
}

static void
create_query_cache (void)
{
        query_cache = g_hash_table_new_full (
                query_cache_hash,
                query_cache_equal,
                query_cache_entry_free,
                NULL);
        matecomponent_activation_add_reset_notify (query_cache_reset);
}

static MateComponent_ServerInfoList *
query_cache_lookup (const char   *query,
		    char * const *sort_criteria,
                    gboolean *active)
{
	QueryCacheEntry  fake;
	QueryCacheEntry *entry;
        MateComponent_ServerInfoList *result;

        MATECOMPONENT_ACTIVATION_LOCK ();

        *active = FALSE;

	if (!query_cache) {
                create_query_cache ();
                MATECOMPONENT_ACTIVATION_UNLOCK ();
		return NULL;
	}

        if (strstr (query, "_active")) {
                *active = TRUE;
                return NULL;
        }

	fake.query = (char *) query;
	fake.sort_criteria = (char **) sort_criteria;
	if ((entry = g_hash_table_lookup (query_cache, &fake))) {
#ifdef QUERY_CACHE_DEBUG
		g_message ("\n\n ---  Hit (%p)  ---\n\n\n", entry->list);
#endif /* QUERY_CACHE_DEBUG */
		result = MateComponent_ServerInfoList_duplicate (entry->list);
	} else {
#ifdef QUERY_CACHE_DEBUG
		g_message ("Miss");
#endif /* QUERY_CACHE_DEBUG */
		result = NULL;
	}

        MATECOMPONENT_ACTIVATION_UNLOCK ();

        return result;
}

static void
query_cache_insert (const char   *query,
		    char * const *sort_criteria,
		    MateComponent_ServerInfoList *list)
{
        int idx = 0;
	QueryCacheEntry *entry = g_new (QueryCacheEntry, 1);

        if (!query_cache) {
                create_query_cache ();
        
        } else if (g_hash_table_size (query_cache) > QUERY_CACHE_MAX) {
                g_hash_table_foreach_remove (
                        query_cache, cache_clean_half, &idx);
        }

	entry->query = g_strdup (query);
	entry->sort_criteria = g_strdupv ((char **) sort_criteria);
	entry->list = MateComponent_ServerInfoList_duplicate (list);

	g_hash_table_replace (query_cache, entry, entry);

#ifdef QUERY_CACHE_DEBUG
	g_message ("Query cache size now %d",
                g_hash_table_size (query_cache));
#endif /* QUERY_CACHE_DEBUG */
}

/**
 * matecomponent_activation_query: 
 * @requirements: query string.
 * @selection_order: sort criterion for returned list.
 * @ev: a %CORBA_Environment structure which will contain 
 *      the CORBA exception status of the operation, or NULL
 *
 * Executes the @requirements query on the matecomponent-activation-server.
 * The result is sorted according to @selection_order. 
 * @selection_order can safely be NULL as well as @ev.
 * The returned list has to be freed with CORBA_free.
 *
 * Return value: the list of servers matching the requirements.
 */
MateComponent_ServerInfoList *
matecomponent_activation_query (const char        *requirements,
                         char * const      *selection_order,
                         CORBA_Environment *opt_ev)
{
        gboolean                  active;
	MateComponent_StringList         selorder;
	MateComponent_ServerInfoList    *retval;
	MateComponent_ActivationContext  ac;
	CORBA_Environment         tempenv, *ev;

	g_return_val_if_fail (requirements != NULL, CORBA_OBJECT_NIL);

	ac = matecomponent_activation_activation_context_get ();
	g_return_val_if_fail (ac != NULL, CORBA_OBJECT_NIL);

	retval = query_cache_lookup (requirements, selection_order, &active);
	if (retval)
		return retval;

	if (!opt_ev) {
		CORBA_exception_init (&tempenv);
		ev = &tempenv;
	} else
		ev = opt_ev;

	copy_strv_to_sequence (selection_order, &selorder);

	retval = MateComponent_ActivationContext_query (
                ac, requirements, &selorder,
                matecomponent_activation_context_get (), ev);

        if (ev->_major == CORBA_NO_EXCEPTION) {
                if (!active)
                        query_cache_insert (requirements, selection_order, retval);
        } else
                retval = NULL;

	if (!opt_ev)
		CORBA_exception_free (&tempenv);

	return retval;
}

static CORBA_Object
handle_activation_result (MateComponent_ActivationResult *result,
			  MateComponent_ActivationID     *ret_aid,
			  CORBA_Environment       *ev)
{
	CORBA_Object retval = CORBA_OBJECT_NIL;

	switch (result->res._d) {
	case MateComponent_ACTIVATION_RESULT_SHLIB:
		retval = matecomponent_activation_activate_shlib_server (result, ev);
		break;
	case MateComponent_ACTIVATION_RESULT_OBJECT:
		retval = CORBA_Object_duplicate (result->res._u.res_object, ev);
		break;
	case MateComponent_ACTIVATION_RESULT_NONE:
	default:
		break;
	}

	if (ret_aid) {
		if (result->aid && result->aid [0])
			*ret_aid = g_strdup (result->aid);
		else
			*ret_aid = NULL;
	}

	CORBA_free (result);

	return retval;
}

/**
 * matecomponent_activation_activate:
 * @requirements: query string.
 * @selection_order: sort criterion for returned list.
 * @flags: how to activate the object.
 * @ret_aid: AID of the activated object.
 * @ev: %CORBA_Environment structure which will contain 
 *      the CORBA exception status of the operation. 
 *
 * Activates a given object. @ret_aid can be safely NULLed as well
 * as @ev and @selection_order. @flags can be set to zero if you do 
 * not what to use.
 *
 * Return value: the CORBA object reference of the activated object.
 *               This value can be CORBA_OBJECT_NIL: you are supposed 
 *               to check @ev for success.
 */
CORBA_Object
matecomponent_activation_activate (const char             *requirements,
			    char *const            *selection_order,
			    MateComponent_ActivationFlags  flags,
			    MateComponent_ActivationID    *ret_aid,
			    CORBA_Environment      *opt_ev)
{
	MateComponent_ActivationContext  ac;
	MateComponent_ActivationResult  *result;
	CORBA_Environment         tempenv, *ev;
	MateComponent_StringList         selorder;
	CORBA_Object              retval = CORBA_OBJECT_NIL;

	g_return_val_if_fail (requirements != NULL, CORBA_OBJECT_NIL);

	ac = matecomponent_activation_activation_context_get ();
	g_return_val_if_fail (ac != NULL, CORBA_OBJECT_NIL);

	if (!opt_ev) {
		CORBA_exception_init (&tempenv);
		ev = &tempenv;
	} else
		ev = opt_ev;

	copy_strv_to_sequence (selection_order, &selorder);
        
        result = MateComponent_ActivationContext_activateMatchingFull
                (ac, requirements, &selorder, activation_environment,
                 flags, matecomponent_activation_client_get (),
                 matecomponent_activation_context_get (), ev);
        
        if (ev->_major == CORBA_SYSTEM_EXCEPTION &&
            !strcmp (ev->_id, ex_CORBA_BAD_OPERATION)) /* fall-back */
        {
                g_message ("TESTME: Fall-back activate");
                result = MateComponent_ActivationContext_activateMatching
                        (ac, requirements, &selorder, activation_environment,
                         flags, matecomponent_activation_context_get (), ev);
        }

	if (ev->_major == CORBA_NO_EXCEPTION)
		retval = handle_activation_result (result, ret_aid, ev);

	if (!opt_ev)
		CORBA_exception_free (&tempenv);

	return retval;
}

/**
 * matecomponent_activation_activate_from_id
 * @aid: AID or IID of the object to activate.
 * @flags: activation flag.
 * @ret_aid: AID of the activated server.
 * @ev: %CORBA_Environment structure which will contain 
 *      the CORBA exception status of the operation. 
 *
 * Activates the server corresponding to @aid. @ret_aid can be safely 
 * NULLed as well as @ev. @flags can be zero if you do not know what 
 * to do.
 *
 * Return value: a CORBA object reference to the newly activated 
 *               server. Do not forget to check @ev for failure!!
 */
CORBA_Object
matecomponent_activation_activate_from_id (const MateComponent_ActivationID aid, 
				    MateComponent_ActivationFlags    flags,
				    MateComponent_ActivationID      *ret_aid,
				    CORBA_Environment        *opt_ev)
{
	MateComponent_ActivationContext  ac;
	MateComponent_ActivationResult  *result;
	CORBA_Environment        *ev, tempenv;
	CORBA_Object              retval = CORBA_OBJECT_NIL;

	g_return_val_if_fail (aid != NULL, CORBA_OBJECT_NIL);

	if (!strncmp ("OAFIID:", aid, 7)) {
		char *requirements;

		requirements = g_alloca (strlen (aid) + sizeof ("iid == ''"));
		sprintf (requirements, "iid == '%s'", aid);

		return matecomponent_activation_activate (
				requirements, NULL, flags, ret_aid, opt_ev);
	}
	
	if (!opt_ev) {
		CORBA_exception_init (&tempenv);
		ev = &tempenv;
	} else
		ev = opt_ev;

	ac = matecomponent_activation_internal_activation_context_get_extended (
				(flags & MateComponent_ACTIVATION_FLAG_EXISTING_ONLY), ev);
	if (!ac) {
		if (!opt_ev)
			CORBA_exception_free (&tempenv);

		return CORBA_OBJECT_NIL;
	}

	result = MateComponent_ActivationContext_activateFromAidFull
                (ac, aid, flags, matecomponent_activation_client_get (),
                 matecomponent_activation_context_get (), ev);

        if (ev->_major == CORBA_SYSTEM_EXCEPTION &&
            !strcmp (ev->_id, ex_CORBA_BAD_OPERATION)) /* fall-back */
                result = MateComponent_ActivationContext_activateFromAid
                        (ac, aid, flags, matecomponent_activation_context_get (), ev);

	if (ev->_major == CORBA_NO_EXCEPTION)
		retval = handle_activation_result (result, ret_aid, ev);
        
	if (!opt_ev)
		CORBA_exception_free (&tempenv);

	return retval;
}

/* Async activation
 */

#define ASYNC_ERROR_NO_AID            (_("No ActivationID supplied"))
#define ASYNC_ERROR_NO_REQUIREMENTS   (_("No requirements supplied"))
#define ASYNC_ERROR_NO_CONTEXT        (_("Failed to initialise the ActivationContext"))
#define ASYNC_ERROR_INV_FAILED        (_("Failed to invoke method on the ActivationContext"))
#define ASYNC_ERROR_GENERAL_EXCEPTION (_("System exception: %s : %s"))
#define ASYNC_ERROR_EXCEPTION         (_("System exception: %s"))

static MateCORBA_IMethod *activate_matching_full_method = NULL;
static MateCORBA_IMethod *activate_from_aid_full_method = NULL;

typedef struct {
	MateComponentActivationCallback user_cb;
	gpointer                 user_data;
} AsyncActivationData;

static void
setup_methods (void)
{
	activate_matching_full_method = &MateComponent_ActivationContext__iinterface.methods._buffer [7];
	activate_from_aid_full_method = &MateComponent_ActivationContext__iinterface.methods._buffer [9];

	/* If these blow the IDL changed order, and the above
	   indexes need updating */
	g_assert (!strcmp (activate_matching_full_method->name, "activateMatchingFull"));
	g_assert (!strcmp (activate_from_aid_full_method->name, "activateFromAidFull"));
}

static void
activation_async_callback (CORBA_Object          object,
			   MateCORBA_IMethod        *m_data,
			   MateCORBAAsyncQueueEntry *aqe,
			   gpointer              user_data,
			   CORBA_Environment    *ev)
{
	MateComponent_ActivationResult *result = NULL;
	AsyncActivationData     *async_data = user_data;
	MateComponent_GeneralError     *err;
	CORBA_Object             retval;
	char                    *reason = NULL;

	g_return_if_fail (async_data != NULL);
	g_return_if_fail (async_data->user_cb != NULL);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto return_exception;

	MateCORBA_small_demarshal_async (aqe, &result, NULL, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto return_exception;

	retval = handle_activation_result (result, NULL, ev);

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto return_exception;

	async_data->user_cb (retval, NULL, async_data->user_data);

clean_out:
	g_free (async_data);
	return;

return_exception:
	if (!strcmp (ev->_id, "IDL:MateComponent/GeneralError:1.0")) {
		err = ev->_any._value;

		if (!err || !err->description)
			reason = g_strdup_printf (ASYNC_ERROR_GENERAL_EXCEPTION,
						  ev->_id, "(no description)");
		else
			reason = g_strdup_printf (ASYNC_ERROR_GENERAL_EXCEPTION,
						  ev->_id, err->description);
	} else
		reason = g_strdup_printf (ASYNC_ERROR_EXCEPTION, ev->_id);

	async_data->user_cb (CORBA_OBJECT_NIL, reason, async_data->user_data);
	g_free (reason);

	goto clean_out;
}

/**
 * matecomponent_activation_activate_async:
 * @requirements: the matecomponent-activation query string.
 * @selection_order: preference array.
 * @flags: activation flags.
 * @callback: callback function.
 * @user_data: data to be poassed to the callback function.
 * @ev: exception structure.
 *
 * This function will asynchronously try to activate a component
 * given the @requirements query string. When the component is
 * activated or when the activation fails, it will call @callback
 * with the given @user_data data as parameter.
 * callback will be called with a CORBA_OBJECT_NIL object if the
 * activation fails. If the activation fails, the callback will be
 * given a human-readable string containing a description of the
 * error. In case of sucess, the error string value is undefined.
 *
 * @selection_order can be safely NULLed as well as @ev and
 * @user_data. @flags can be set to 0 if you do not know what to
 * use.
 */
void
matecomponent_activation_activate_async (const char               *requirements,
				  char *const              *selection_order,
				  MateComponent_ActivationFlags    flags,
				  MateComponentActivationCallback  async_cb,
				  gpointer                  user_data,
				  CORBA_Environment        *opt_ev)
{
	MateComponent_ActivationContext  ac;
	AsyncActivationData      *async_data;
	CORBA_Environment        *ev, tempenv;
	MateComponent_StringList         selorder;
        MateComponent_ActivationClient   client;
	gpointer                  args [5];

	if (!requirements) {
		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_NO_REQUIREMENTS, user_data);
		return;
	}

	ac = matecomponent_activation_activation_context_get ();
	if (!ac) {
		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_NO_CONTEXT, user_data);
		return;
	}

	if (!opt_ev) {
		CORBA_exception_init (&tempenv);
		ev = &tempenv;
	} else
		ev = opt_ev;

	async_data = g_new (AsyncActivationData, 1);
	async_data->user_cb   = async_cb;
	async_data->user_data = user_data;

	copy_strv_to_sequence (selection_order, &selorder);

        client = matecomponent_activation_client_get ();

        args [0] = &requirements;
	args [1] = &selorder;
	args [2] = activation_environment;
	args [3] = &flags;
	args [4] = &client;

	if (!activate_matching_full_method)
		setup_methods ();

	MateCORBA_small_invoke_async (ac, activate_matching_full_method,
				  activation_async_callback, async_data,
				  args, matecomponent_activation_context_get (), ev);

	if (ev->_major != CORBA_NO_EXCEPTION) {
		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_INV_FAILED, user_data);
		g_free (async_data);
	}

	if (!opt_ev)
		CORBA_exception_free (&tempenv);

	return;
}

/**
 * matecomponent_activation_activate_from_id_async:
 * @aid: the AID or IID of the component to activate.
 * @flags: activation flags.
 * @callback: callback function.
 * @user_data: data to be poassed to the callback function.
 * @ev: exception structure.
 *
 * This function will asynchronously try to activate a component
 * with the given @aid. When the component is
 * activated or when the activation fails, it will call @callback
 * with the given @user_data data as parameter.
 * callback will be called with a CORBA_OBJECT_NIL object if the
 * activation fails. If the activation fails, the callback will be
 * given a human-readable string containing a description of the
 * error. In case of sucess, the error string value is undefined.
 *
 * @flags can be 0 if you do not know what to set it to and
 * @ev can be safely set to NULL.
 */
void
matecomponent_activation_activate_from_id_async (const MateComponent_ActivationID  aid,
					  MateComponent_ActivationFlags     flags,
					  MateComponentActivationCallback   async_cb,
					  gpointer                   user_data,
					  CORBA_Environment         *opt_ev)
{
	MateComponent_ActivationContext  ac;
	AsyncActivationData      *async_data;
	CORBA_Environment        *ev, tempenv;
        MateComponent_ActivationClient   client;
	gpointer                  args [3];

	if (!aid) {
		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_NO_AID, user_data);
		return;
	}

	if (!strncmp ("OAFIID:", aid, 7)) {
		char *requirements;

		requirements = g_alloca (strlen (aid) + sizeof ("iid == ''"));
		sprintf (requirements, "iid == '%s'", aid);

                matecomponent_activation_activate_async (
                        requirements, NULL, flags, async_cb, user_data, opt_ev);
                return;
	}
	
	if (!opt_ev) {
		CORBA_exception_init (&tempenv);
		ev = &tempenv;
	} else
		ev = opt_ev;

	ac = matecomponent_activation_internal_activation_context_get_extended (
				(flags & MateComponent_ACTIVATION_FLAG_EXISTING_ONLY), ev);
	if (!ac) {
		if (!opt_ev)
			CORBA_exception_free (&tempenv);

		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_NO_CONTEXT, user_data);
		return;
	}

	async_data = g_new (AsyncActivationData, 1);
	async_data->user_cb   = async_cb;
	async_data->user_data = user_data;

	if (!activate_from_aid_full_method)
		setup_methods ();

        client = matecomponent_activation_client_get ();

	args [0] = (gpointer) &aid;
	args [1] = &flags;
	args [2] = &client;

	MateCORBA_small_invoke_async (ac, activate_from_aid_full_method,
				  activation_async_callback, async_data,
				  args, matecomponent_activation_context_get (), ev);

	if (ev->_major != CORBA_NO_EXCEPTION) {
		async_cb (CORBA_OBJECT_NIL, ASYNC_ERROR_INV_FAILED, user_data);
		g_free (async_data);
	}

	if (!opt_ev)
		CORBA_exception_free (&tempenv);

	return;

}

void
matecomponent_activation_init_activation_env (void)
{
	int i;

	struct {
		const char *name;
		const char *value;
	} getenv_values[] = {
#ifndef G_OS_WIN32
		{ "DISPLAY",         NULL }, /* X display */
		{ "SESSION_MANAGER", NULL }, /* XSMP session manager */
		{ "AUDIODEV",        NULL }, /* Audio device on Sun systems */
                { "XAUTHORITY",      NULL },
		{ "LC_ALL",	     NULL }, /* locale information: see setlocale(3) */
		{ "LC_COLLATE",	     NULL }, 
		{ "LC_MESSAGES",     NULL },
		{ "LC_MONETARY",     NULL },
		{ "LC_NUMERIC",	     NULL },
		{ "LC_TIME",	     NULL },
#endif
		{ "LANG",            NULL }, /* Fallback locale name */
		{ NULL,		     NULL }
	};

        g_assert (activation_environment == NULL);
        activation_environment = MateComponent_ActivationEnvironment__alloc ();

	for (i = 0; getenv_values [i].name; i++) {
                MateComponent_ActivationEnvValue value;

		getenv_values [i].value = getenv (getenv_values [i].name);

		if (!getenv_values [i].value)
			continue;

                value.name = (CORBA_char *) getenv_values [i].name;
                value.value = (CORBA_char *) getenv_values [i].value;
                value.flags = 0;

                MateCORBA_sequence_append (activation_environment, &value);
	}
}

void
matecomponent_activation_set_activation_env_value (const char *name,
					    const char *value)
{
	int                        i;
        MateComponent_ActivationEnvValue env_value = { (CORBA_char *) name, (CORBA_char *) value, 0 };

	g_return_if_fail (name != NULL);

	for (i = 0; i < activation_environment->_length; i++) {
		if (!strcmp (activation_environment->_buffer [i].name, name)) {
                        MateCORBA_sequence_remove (activation_environment, i);
			break;
		}
        }
        MateCORBA_sequence_append (activation_environment, &env_value);
}

CORBA_char *
_matecomponent_activation_get_activation_env_value (const char *name)
{
	int                        i;

	g_return_val_if_fail (name != NULL, NULL);

	for (i = 0; i < activation_environment->_length; i++) {
		if (strcmp (activation_environment->_buffer [i].name, name) == 0) {
                        return activation_environment->_buffer [i].value;
		}
        }
        return NULL;
}


/**
 * matecomponent_activation_name_service_get:
 * @ev: %CORBA_Environment structure which will contain 
 *      the CORBA exception status of the operation. 
 *
 * Returns the name server of matecomponent-activation. @ev can be NULL.
 *
 * Return value: the name server of matecomponent-activation.
 */
CORBA_Object
matecomponent_activation_name_service_get (CORBA_Environment * ev)
{
	return matecomponent_activation_activate_from_id (
                "OAFIID:MateComponent_CosNaming_NamingContext", 0, NULL, ev);
}

/**
 * matecomponent_activation_dynamic_add_path:
 * @add_path: The path would be loaded in runtime.  
 * @ev: %CORBA_Environment structure which will contain
 *      the CORBA exception status of the operation.
 * This function could make BAS server load the specific
 * search path in runtime.
 * 
 * Return value: a result of dynamic path load
 */
MateComponent_DynamicPathLoadResult
matecomponent_activation_dynamic_add_path (const char *add_path,
				    CORBA_Environment * ev)
{
	MateComponent_ObjectDirectory        od;
	MateComponent_DynamicPathLoadResult  res;

	g_return_val_if_fail (add_path != NULL, MateComponent_DYNAMIC_LOAD_ERROR);
	od = matecomponent_activation_object_directory_get (
					matecomponent_activation_username_get (),
					matecomponent_activation_hostname_get ());
	if (CORBA_Object_is_nil (od, ev))
		return MateComponent_DYNAMIC_LOAD_ERROR;

	res = MateComponent_ObjectDirectory_dynamic_add_path(od, add_path, ev);
	if (ev->_major != CORBA_NO_EXCEPTION) 
		return MateComponent_DYNAMIC_LOAD_ERROR;

	return res;
					
}

/**
 * matecomponent_activation_dynamic_remove_path:
 * @remove_path: The path would be unloaded in runtime.
 * @ev: %CORBA_Environment structure which will contain
 * the CORBA exception status of the operation.
 * This function could make BAS server unload the specific
 * search path in runtime.
 * 
 * Return value: a result of dynamic path load
 */
MateComponent_DynamicPathLoadResult
matecomponent_activation_dynamic_remove_path (const char *remove_path,
				       CORBA_Environment * ev)
{
	MateComponent_ObjectDirectory        od;
	MateComponent_DynamicPathLoadResult  res;

	g_return_val_if_fail (remove_path != NULL, MateComponent_DYNAMIC_LOAD_ERROR);
	od = matecomponent_activation_object_directory_get (
						matecomponent_activation_username_get (),
						matecomponent_activation_hostname_get ());
	if (CORBA_Object_is_nil (od, ev))
		return MateComponent_DYNAMIC_LOAD_ERROR;

	res = MateComponent_ObjectDirectory_dynamic_remove_path(od, remove_path, ev);
	if (ev->_major != CORBA_NO_EXCEPTION)
		return MateComponent_DYNAMIC_LOAD_ERROR;

	return res;
}

