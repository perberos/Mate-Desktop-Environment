/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation-client.c: A client client to enable caching
 *
 *  Copyright (C) 2002 Ximian Inc.
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
 *  Author: Michael Meeks (michael@ximian.com)
 */

#include <config.h>
#include <unistd.h>
#include <stdlib.h>
#include <matecomponent-activation/matecomponent-activation.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <matecomponent-activation/matecomponent-activation-client.h>
#include <matecomponent-activation/MateComponent_ActivationContext.h>

static GSList *reset_notify_callbacks = NULL;

static void
reset_caches (void)
{
        GSList   *l;
        GVoidFunc cb;

        MATECOMPONENT_ACTIVATION_LOCK ();

        for (l = reset_notify_callbacks; l; l = l->next) {
                cb = l->data;
                cb ();
        }

        MATECOMPONENT_ACTIVATION_UNLOCK ();
}

void
matecomponent_activation_add_reset_notify (GVoidFunc fn)
{
        MATECOMPONENT_ACTIVATION_LOCK ();

        if (!g_slist_find (reset_notify_callbacks, fn))
                reset_notify_callbacks = g_slist_prepend (
                        reset_notify_callbacks, fn);

        MATECOMPONENT_ACTIVATION_UNLOCK ();
}

typedef struct {
        POA_MateComponent_ActivationClient servant;
} impl_POA_MateComponent_ActivationClient;

static void
impl_MateComponent_ActivationClient__finalize (PortableServer_Servant servant,
                                        CORBA_Environment     *ev)
{
        g_free (servant);
}

static void
impl_MateComponent_ActivationClient_resetCache (PortableServer_Servant servant,
                                         CORBA_Environment     *ev)
{
        /* Reset the cache ! */
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_message ("Reset cache");
#endif
        reset_caches ();
}

static CORBA_long
impl_MateComponent_ActivationClient_getVersion (PortableServer_Servant  servant,
                                          CORBA_Environment      *ev)
{
        return (MATECOMPONENT_ACTIVATION_MAJOR_VERSION*10000 + 
                MATECOMPONENT_ACTIVATION_MINOR_VERSION*100 +
                MATECOMPONENT_ACTIVATION_MICRO_VERSION);
}

static PortableServer_ServantBase__epv impl_MateComponent_ActivationClient_base_epv = {
        NULL, /* private data */
        impl_MateComponent_ActivationClient__finalize,
        NULL, /* default_POA routine */
};
static POA_MateComponent_ActivationClient__epv impl_MateComponent_ActivationClient_epv = {
        NULL, /* private */
        &impl_MateComponent_ActivationClient_resetCache,
        &impl_MateComponent_ActivationClient_getVersion
};

static POA_MateComponent_Unknown__epv impl_MateComponent_Unknown_epv = {
	NULL, /* private data */
	NULL,
	NULL,
        NULL
};

static POA_MateComponent_ActivationClient__vepv impl_MateComponent_ActivationClient_vepv = {
        &impl_MateComponent_ActivationClient_base_epv,
        &impl_MateComponent_Unknown_epv,
        &impl_MateComponent_ActivationClient_epv
};

static CORBA_Object
matecomponent_activation_corba_client_new (void)
{
        CORBA_ORB orb;
        CORBA_Object retval;
        CORBA_Environment *ev, real_ev;
        PortableServer_POA poa;
        PortableServer_POAManager manager;
        impl_POA_MateComponent_ActivationClient *newservant;

        ev = &real_ev;
        CORBA_exception_init (ev);

        orb = matecomponent_activation_orb_get ();

        poa = (PortableServer_POA) CORBA_ORB_resolve_initial_references (orb, "RootPOA", ev);
        manager = PortableServer_POA__get_the_POAManager (poa, ev);
        PortableServer_POAManager_activate (manager, ev);

        newservant = g_new0 (impl_POA_MateComponent_ActivationClient, 1);
        newservant->servant.vepv = &impl_MateComponent_ActivationClient_vepv;

        POA_MateComponent_ActivationClient__init ((PortableServer_Servant) newservant, ev);
        retval = PortableServer_POA_servant_to_reference (poa, newservant, ev);

        CORBA_Object_release ((CORBA_Object) manager, ev);
        CORBA_Object_release ((CORBA_Object) poa, ev);

        CORBA_exception_free (ev);

        return retval;
}

static CORBA_Object client = CORBA_OBJECT_NIL;

void
matecomponent_activation_release_corba_client (void)
{
        CORBA_Environment ev;

        CORBA_exception_init (&ev);

        CORBA_Object_release (client, &ev);
        reset_caches ();

        CORBA_exception_free (&ev);
        client = CORBA_OBJECT_NIL;
}

static char *
get_lang_list (void)
{
        static char *result = NULL;
        static gboolean result_set = FALSE;
        GString *str;
        gboolean add_comma = FALSE;
	const char * const* languages;
        int i;
        
        if (result_set)
                return result;

        MATECOMPONENT_ACTIVATION_LOCK ();
        
        str = g_string_new (NULL);
	languages = g_get_language_names ();
	for (i = 0; languages[i] != NULL; i++) {
		if (add_comma)
			g_string_append (str, ",");
		else
			add_comma = TRUE;
		g_string_append (str, languages[i]);
	}

        result_set = TRUE;
        
        result = str->str ? str->str : "";
        g_string_free (str, FALSE);

        MATECOMPONENT_ACTIVATION_UNLOCK ();
        
        return result;
}

void
matecomponent_activation_register_client (MateComponent_ActivationContext context,
                                   CORBA_Environment       *ev)
{
        MateComponent_StringList      client_env;
        int                    i;
        MateComponent_ObjectDirectory od;
        char                 **env;
        char                 **newenv;

        if (client == CORBA_OBJECT_NIL)
                client = matecomponent_activation_corba_client_new ();

        MateComponent_ActivationContext_addClient (context, client, get_lang_list (), ev);
        if (ev->_major != CORBA_NO_EXCEPTION)
                return;

        od = matecomponent_activation_object_directory_get (
                matecomponent_activation_username_get (),
                matecomponent_activation_hostname_get ());

        /* send environment to activation server */
        env = g_listenv ();
        newenv = g_new (char *, g_strv_length (env) + 1);
        for (i = 0; env[i]; i++)
                newenv[i] = g_strconcat (env[i], "=", g_getenv (env[i]), NULL);
        newenv[i] = NULL;
        g_strfreev (env);

        client_env._buffer = newenv;
        client_env._length = g_strv_length (newenv);

        MateComponent_ObjectDirectory_addClientEnv (od, client, &client_env, ev);
        CORBA_exception_init (ev); /* bin potential missing method exception */
        g_strfreev (newenv);
}

MateComponent_ActivationClient
matecomponent_activation_client_get (void)
{
        CORBA_Environment ev;

        if (client == CORBA_OBJECT_NIL) {
                CORBA_exception_init (&ev);
                matecomponent_activation_register_client
                        ((MateComponent_ActivationContext)
                         matecomponent_activation_activation_context_get (), &ev);
                if (ev._major != CORBA_NO_EXCEPTION)
                        g_warning ("Failed to register MateComponent::ActivationClient");
                CORBA_exception_free (&ev);
        }
        return client;
}

