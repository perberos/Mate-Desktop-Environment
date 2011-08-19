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
 *
 */

#include <config.h>
#include <matecomponent-activation/matecomponent-activation-register.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <matecomponent-activation/matecomponent-activation-init.h>
#include <matecomponent-activation/MateComponent_ObjectDirectory.h>

#include <stdio.h>
#include <unistd.h>

typedef struct {
	char *name;
	char *value;
} RegistrationEnvValue;

static gboolean check_registration = TRUE;
static gboolean need_ior_printout  = TRUE;

static MateComponent_ActivationEnvironment global_reg_env;

void
matecomponent_activation_timeout_reg_check_set (gboolean on)
{
        check_registration = on;
}

gboolean
matecomponent_activation_timeout_reg_check (gpointer data)
{
        if (!check_registration ||
            g_getenv ("MATECOMPONENT_ACTIVATION_NO_TIMEOUT"))
                return FALSE;

        if (need_ior_printout) {
                g_error ("This process has not registered the required OAFIID "
                         "your source code should register '%s'. If your code is "
                         "performing delayed registration and this message is trapped "
                         "in error, see matecomponent_activation_idle_reg_check_set.",
                         matecomponent_activation_iid_get ());
        }

        return FALSE;
}

gboolean
MateComponent_ActivationEnvironment_match (const MateComponent_ActivationEnvironment *a,
				    const MateComponent_ActivationEnvironment *b)
{
	int i, start = 0;

	for (i = 0; i < a->_length; i++) {
		int j;

		for (j = start; j < b->_length; j++)
			if (!strcmp (a->_buffer [i].name, b->_buffer [j].name))
				break;

		if (j >= b->_length)
			continue;

		if (strcmp (a->_buffer [i].value, b->_buffer [j].value) != 0)
			return FALSE;

		if (j == start)
			start++;
	}

	return TRUE;
}

void
MateComponent_ActivationEnvValue_copy (MateComponent_ActivationEnvValue *dest,
				MateComponent_ActivationEnvValue *src)
{
	g_return_if_fail (dest != NULL);
	g_return_if_fail (src != NULL);

	dest->name  = CORBA_string_dup (src->name);
	dest->value = CORBA_string_dup (src->value);
	dest->flags = src->flags;
}

void
MateComponent_ActivationEnvValue_set (MateComponent_ActivationEnvValue *env,
			       const char                *name,
			       const char                *value)
{
	g_return_if_fail (env != NULL);
	g_return_if_fail (name != NULL);

        CORBA_free (env->name);
        CORBA_free (env->value);

	env->name  = CORBA_string_dup (name);
	env->value = value ?
			CORBA_string_dup (value) :
			CORBA_string_dup ("");

	env->flags = 0;
        if (!value)
                env->flags |= MateComponent_ACTIVATION_ENV_FLAG_UNSET;
}

static void
copy_env_list_to_sequence (MateComponent_ActivationEnvironment *environment,
			   GSList                       *reg_env)
{
	GSList *l;
	int     i;

	g_assert (reg_env != NULL);

	environment->_length  = environment->_maximum = g_slist_length (reg_env);
	environment->_buffer  = MateComponent_ActivationEnvironment_allocbuf (environment->_length);
	environment->_release = TRUE;

	for (l = reg_env, i = 0; l; l = l->next, i++) {
		RegistrationEnvValue *val = l->data;

		MateComponent_ActivationEnvValue_set (
			&environment->_buffer [i], val->name, val->value);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
		g_message ("Registration environment for '%s' = '%s'%s",
			   environment->_buffer [i].name,
			   environment->_buffer [i].value,
			   (environment->_buffer [i].flags &
                            MateComponent_ACTIVATION_ENV_FLAG_UNSET) ? "(unset)" : "");
#endif
	}
}

static char *
registration_result_to_string (MateComponent_RegistrationResult result)
{
	switch (result) {
	case MateComponent_ACTIVATION_REG_SUCCESS:
		return "(success)";
		break;
	case MateComponent_ACTIVATION_REG_NOT_LISTED:
		return "(not listed)";
		break;
	case MateComponent_ACTIVATION_REG_ALREADY_ACTIVE:
		return "(already active)";
		break;
	case MateComponent_ACTIVATION_REG_ERROR:
		return "(error)";
		break;
	default:
                g_warning("Invalid registration result code %i", result);
		break;
	}

	return "(invalid)";
}

/**
 * matecomponent_activation_register_active_server:
 * @iid: IID of the server to register.
 * @obj: CORBA::Object to register.
 * @reg_env: the registration environment.
 *
 * Registers @obj with @iid with the local matecomponent-activation-server
 * daemon.
 *
 * If @reg_env is not %NULL, @obj will be registered in such a
 * way that if a client who's environment differs from the
 * environment specified in @reg_env, then another attempt
 * to activate @iid will not result in a reference to @obj
 * being returned, but rather another instance of @iid being
 * activated.
 *
 * So, for example, you can ensure that a seperate instance
 * of the component is activated for each distinct X display
 * (and screen) by:
 *
 * <informalexample><programlisting>
 *   display_name = gdk_display_get_name (gdk_display_get_default());
 *   reg_env = matecomponent_activation_registration_env_set (
 *                              reg_env, "DISPLAY", display_name);
 *   matecomponent_activation_register_active_server (iid, active_server, reg_env);
 *   matecomponent_activation_registration_env_free (reg_env);
 * </programlisting></informalexample>
 *
 * If @reg_env is %NULL, the global registration environment
 * list will be used if it is set. See
 * matecomponent_activation_registration_env_set_global().
 *
 * Return value: status of the registration.
 */
MateComponent_RegistrationResult
matecomponent_activation_register_active_server (const char   *iid,
					  CORBA_Object  obj,
					  GSList       *reg_env)
{
        MateComponent_RegistrationResult rv;
        CORBA_Object              existing;
        rv = matecomponent_activation_register_active_server_ext
                (iid, obj, reg_env, 0, &existing, NULL);
	if (existing != CORBA_OBJECT_NIL)
		CORBA_Object_release (existing, NULL);
        return rv;
}

/**
 * matecomponent_activation_register_active_server_ext:
 * @iid: IID of the server to register.
 * @obj: CORBA::Object to register.
 * @reg_env: the registration environment.
 * @flags: registration flags
 * @existing: in case an object with the same IID has already been
 * registered, *existing will contain a reference to the existing
 * object.
 * @description: An optional component description to be registered
 * with the server for no server-info components.
 * 
 * This function is the same as
 * matecomponent_activation_register_active_server(), except that: 1. you can
 * specify registration flags; 2. in case registration fails because
 * there is already an object registered with that IID, a reference to
 * the first object registered is returned.
 *
 * At the moment, only the only flag available is
 * MateComponent_REGISTRATION_FLAG_NO_SERVERINFO, which means to allow
 * registration of an active server which doesn't have a corresponding
 * .server.  Note that matecomponent activation queries will ignore objects
 * registered this way.  This feature is not meant to be used directly
 * by applications, so beware!
 * 
 * Return value: status of the registration.
 **/
MateComponent_RegistrationResult
matecomponent_activation_register_active_server_ext (const char               *iid,
                                              CORBA_Object              obj,
                                              GSList                   *reg_env,
                                              MateComponent_RegistrationFlags  flags,
                                              CORBA_Object             *existing,
                                              const char               *description)
{
	MateComponent_ObjectDirectory     od;
	CORBA_Environment          ev;
	MateComponent_RegistrationResult  retval;
	const char                *actid;

        g_return_val_if_fail (existing != NULL, MateComponent_ACTIVATION_REG_ERROR);
        *existing = CORBA_OBJECT_NIL;

	CORBA_exception_init (&ev);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_message ("About to register '%s': %p%s",
                   iid, obj,
                   CORBA_Object_non_existent (obj, &ev) ? " (nonexistent)" : "");
#endif

	actid = matecomponent_activation_iid_get ();

        if (actid && strcmp (actid, iid) == 0 && matecomponent_activation_private) {
                retval = MateComponent_ACTIVATION_REG_SUCCESS;
        } else {
		MateComponent_ActivationEnvironment environment;

                od = matecomponent_activation_object_directory_get (
                        matecomponent_activation_username_get (),
                        matecomponent_activation_hostname_get ());
                
                if (CORBA_Object_is_nil (od, &ev))
                        return MateComponent_ACTIVATION_REG_ERROR;

		if (reg_env)
			copy_env_list_to_sequence (&environment, reg_env);

                retval = MateComponent_ObjectDirectory_register_new_full
                        (od, iid, reg_env ? &environment : &global_reg_env,
                         obj, flags, description ? description : "",
                         existing, matecomponent_activation_client_get (), &ev);

                if (ev._major == CORBA_SYSTEM_EXCEPTION &&
                    !strcmp (ev._id, ex_CORBA_BAD_OPERATION))
                {
                            /* fall-back */
                        CORBA_exception_free (&ev);
                        retval = MateComponent_ObjectDirectory_register_new
                                (od, iid, reg_env ? &environment : &global_reg_env,
                                 obj, flags, description ? description : "",
                                 existing, &ev);
                }

                if (ev._major != CORBA_NO_EXCEPTION) {
                        retval = MateComponent_ACTIVATION_REG_ERROR;
                        g_warning ("Strange exception (%s) from active server registration",
                                   ev._id);
                }

		if (reg_env)
			CORBA_free (environment._buffer);
        }

#ifndef MATECOMPONENT_ACTIVATION_DEBUG
	if (g_getenv ("MATECOMPONENT_DEBUG") && retval != MateComponent_ACTIVATION_REG_SUCCESS)
#endif
        	g_message ("registration of '%s' returns %s", iid,
			   registration_result_to_string (retval));

	if (actid && strcmp (actid, iid) == 0 && need_ior_printout) {
		char *iorstr;
		FILE *fh;
		int iorfd = matecomponent_activation_ior_fd_get ();

		need_ior_printout = FALSE;

		if (iorfd == 1)
			fh = stdout;
		else {
			fh = fdopen (iorfd, "w");
			if (!fh)
				fh = stdout;
		}

		iorstr = CORBA_ORB_object_to_string (
                        matecomponent_activation_orb_get (), obj, &ev);

		if (ev._major == CORBA_NO_EXCEPTION) {
			fprintf (fh, "%s\n", iorstr);
			CORBA_free (iorstr);
		}

		if (fh != stdout) {
			fclose (fh);
		} else if (iorfd > 2) {
			close (iorfd);
                }
	}
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        else if (actid && need_ior_printout) {
                g_warning ("Unusual '%s' was activated, but "
                           "'%s' is needed", iid, actid);
        }
#endif

	CORBA_exception_free (&ev);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_message ("Successfully registered `%s'", iid);
#endif

	return retval;
}

/**
 * matecomponent_activation_active_server_register:
 * @registration_id: IID of the server to register.
 * @obj: CORBA::Object to register.
 *
 * Registers @obj with @iid with the local matecomponent-activation-server
 * daemon.
 *
 * This function is now deprecated and should no longer be
 * used. matecomponent_activation_register_active_server() should now
 * be used.
 *
 * Return value: status of the registration.
 */
MateComponent_RegistrationResult
matecomponent_activation_active_server_register (const char   *registration_id, 
                                          CORBA_Object  obj)
{
	MateComponent_RegistrationResult  retval;
	char                      *iid;

	iid = strrchr (registration_id, ',');
	if (!iid) {
		retval = matecomponent_activation_register_active_server (
						registration_id, obj, NULL);
	} else {
		GSList *reg_env = NULL;
		char   *display_name;
		int     len;

		len = iid - registration_id;
		display_name = g_alloca (len + 1);
		strncpy (display_name, registration_id, len);
		display_name [len] = '\0';

		iid++;

		reg_env = matecomponent_activation_registration_env_set (
					reg_env, "DISPLAY", display_name);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
		g_message ("Registering iid '%s' with display '%s'", iid, display_name);
#endif

		retval = matecomponent_activation_register_active_server (iid, obj, reg_env);

		matecomponent_activation_registration_env_free (reg_env);
	}

	return retval;
}

/**
 * matecomponent_activation_unregister_active_server:
 * @iid: IID of the server to unregister.
 * @obj: CORBA::Object to unregister.
 *
 * Unregisters @obj with @iid with the local matecomponent-activation-server
 * daemon.
 */
void
matecomponent_activation_unregister_active_server (const char   *iid,
					    CORBA_Object  obj)
{
	MateComponent_ObjectDirectory        od;
	CORBA_Environment             ev;
	const char                   *actid;

	actid = matecomponent_activation_iid_get ();
	if(actid && strcmp (actid, iid) == 0 && matecomponent_activation_private) {
		return;
        }

	od = matecomponent_activation_object_directory_get (
                matecomponent_activation_username_get (), 
                matecomponent_activation_hostname_get ());

	CORBA_exception_init (&ev);
	if (CORBA_Object_is_nil (od, &ev))
		return;

	MateComponent_ObjectDirectory_unregister (od, (char *) iid, obj, &ev);

	CORBA_exception_free (&ev);
}

void
matecomponent_activation_active_server_unregister (const char   *iid,  
					    CORBA_Object  obj)
{
	matecomponent_activation_unregister_active_server (iid, obj);
}

/**
 * matecomponent_activation_registration_env_set:
 * @reg_env: a GSList pointer.
 * @name: the name of the env variable (must not be %NULL).
 * @value: the value of the env variable (may be %NULL).
 *
 * Sets the environment variable @name to @value in the
 * registration environment list @reg_env.
 *
 * Return value: the new start of @reg_env.
 */
GSList *
matecomponent_activation_registration_env_set (GSList     *reg_env,
					const char *name,
					const char *value)
{
	RegistrationEnvValue *env_value;

	g_return_val_if_fail (name != NULL, reg_env);

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	{
		GSList *l;

		for (l = reg_env; l; l = l->next) {
			RegistrationEnvValue *v = l->data;

			g_assert (v != NULL);
			g_assert (v->name != NULL);

			if (!strcmp (v->name, name))
				g_warning ("Duplicate env values set - %s=%s and %s=%s",
					   v->name, v->value ? v->value : "(null)",
					   name, value ? value : "(null)");
		}
	}
#endif

	env_value = g_new (RegistrationEnvValue, 1);

	env_value->name  = g_strdup (name);
	env_value->value = g_strdup (value);

	reg_env = g_slist_prepend (reg_env, env_value);

	return reg_env;
}

/**
 * matecomponent_activation_registration_env_free:
 * @reg_env: a GSList pointer.
 *
 * Frees the registration environment list, @reg_env.
 */
void
matecomponent_activation_registration_env_free (GSList *reg_env)
{
	GSList *l;

	for (l = reg_env; l; l = l->next) {
                RegistrationEnvValue *env_value = l->data;

                g_free (env_value->name);
                g_free (env_value->value);
                g_free (env_value);
        }

	g_slist_free (reg_env);
}

/**
 * matecomponent_activation_registration_env_set_global:
 * @reg_env: a GSList pointer.
 * @append_if_existing: whether or not to append to the global list.
 *
 * Sets the global registration environment list with the
 * contents of @reg_env. If @append_if_existing is set to
 * %FALSE, the an existing global list will be overwritten.
 */
void
matecomponent_activation_registration_env_set_global (GSList   *reg_env,
					       gboolean  append_if_existing)
{
	MateComponent_ActivationEnvValue *old_buffer;
	int                        old_length = 0;

	if (append_if_existing)
		old_length = global_reg_env._length;

	old_buffer = global_reg_env._buffer;

	if (reg_env) {
		GSList *l;
		int     i;

		global_reg_env._length  = global_reg_env._maximum = old_length + g_slist_length (reg_env);
		global_reg_env._buffer  = MateComponent_ActivationEnvironment_allocbuf (global_reg_env._length);
		global_reg_env._release = TRUE;

		for (i = 0; i < old_length; i++)
			MateComponent_ActivationEnvValue_copy (
				&global_reg_env._buffer [i], &old_buffer [i]);

		for (l = reg_env; l; l = l->next) {
			RegistrationEnvValue *val = l->data;

			MateComponent_ActivationEnvValue_set (
				&global_reg_env._buffer [++i], val->name, val->value);
		}

		g_assert (i == global_reg_env._length - 1);
	} else
		memset (&global_reg_env, 0, sizeof (MateComponent_ActivationEnvironment));

	if (old_buffer)
		CORBA_free (old_buffer);
}

/**
 * matecomponent_activation_make_registration_id:
 * @iid: IID of the server to unregister.
 * @display: the X display name with which you wish to register.
 *
 * Creates matecomponent-activation registration ID suitable for use
 * with matecomponent_activation_active_server_register(). If @display
 * is the name of an X display, then using this registration
 * ID will ensure that the server will only be used by clients
 * using the same X display.
 *
 * This method is now deprecated. Instead you can achieve the
 * same effect by:
 * 
 * <informalexample><programlisting>
 *   display_name = gdk_display_get_name (gdk_display_get_default());
 *   reg_env = matecomponent_activation_registration_env_set (
 *                              reg_env, "DISPLAY", display_name);
 *   matecomponent_activation_register_active_server (iid, active_server, reg_env);
 *   matecomponent_activation_registration_env_free (reg_env);
 * </programlisting></informalexample>
 *
 * Return value: newly allocated registration id.
 */
char *
matecomponent_activation_make_registration_id (const char *iid, const char *display)
{
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_message ("Make registration id from '%s' '%s'", iid, display);
#endif
        if (display == NULL) {
                return g_strdup (iid);
        } else {
                return g_strconcat (display, ",", iid, NULL);
        }
}
