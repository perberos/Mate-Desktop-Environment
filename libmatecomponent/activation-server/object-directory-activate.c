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
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include "matecomponent-activation/matecomponent-activation-private.h"

#include "server.h"
#include "activation-server-corba-extensions.h"

static CORBA_Object
od_server_activate_factory (MateComponent_ServerInfo                  *si,
			    ODActivationInfo                   *actinfo,
			    const MateComponent_ActivationEnvironment *environment,
                            MateComponent_ActivationClient             client,
			    CORBA_Environment                  *ev)
{
	MateComponent_ActivationResult *res;
	MateComponent_StringList        selorder;
	MateComponent_ActivationFlags   flags;
	CORBA_Object             retval = CORBA_OBJECT_NIL;
	CORBA_Object             factory = CORBA_OBJECT_NIL;
	char                    *requirements;
        char                    *iid;

	memset (&selorder, 0, sizeof (MateComponent_StringList));

	requirements = g_alloca (strlen (si->location_info) + sizeof ("iid == ''"));
	sprintf (requirements, "iid == '%s'", si->location_info);
	flags = ((actinfo->flags | MateComponent_ACTIVATION_FLAG_NO_LOCAL) & (~MateComponent_ACTIVATION_FLAG_PRIVATE));

        iid = g_strdup (si->iid);
	res = MateComponent_ActivationContext_activateMatchingFull (
			actinfo->ac, requirements, &selorder,
			environment, flags, client, actinfo->ctx, ev);
        si = NULL; /* si can have been freed - due to re-enterancy */

	if (ev->_major != CORBA_NO_EXCEPTION)
		goto out;

	switch (res->res._d) {
	case MateComponent_ACTIVATION_RESULT_NONE:
		CORBA_free (res);
		goto out;
		break;
	case MateComponent_ACTIVATION_RESULT_OBJECT:
		factory = res->res._u.res_object;
		break;
	default:
		g_assert_not_reached ();
		break;
	}

        /* Here comes the too clever by 1/2 bit:
         *   we drop the (recursive) 'server_lock' - so we
         *   can get other threads re-entering / doing activations
         *   here. cf. od_server_activate_exe.
         */
        {
                ServerLockState state;

                state = server_lock_drop ();

                retval = MateComponent_GenericFactory_createObject (factory, iid, ev);
                if (ev->_major != CORBA_NO_EXCEPTION)
                        retval = CORBA_OBJECT_NIL;

                server_lock_resume (state);
        }

	CORBA_free (res);

 out:
        g_free (iid);

	return retval;
}

/* Copied largely from goad.c, goad_server_activate_exe() */
static CORBA_Object
od_server_activate_exe (MateComponent_ServerInfo                  *si,
			ODActivationInfo                   *actinfo,
			CORBA_Object                        od_obj,
			const MateComponent_ActivationEnvironment *environment,
			CORBA_Environment                  *ev)
{
	char **args;
	char *extra_arg, *ctmp, *ctmp2;
        int fd_arg;
	int i;
        char *iorstr, *iid;
        CORBA_Object retval;

	/* Munge the args */
	args = g_alloca (36 * sizeof (char *));
#ifndef G_OS_WIN32
        /* Split location_info into executable pathname and command-line
         * arguments.
         */
	for (i = 0, ctmp = ctmp2 = si->location_info; i < 32; i++) {
		while (*ctmp2 && !g_ascii_isspace ((guchar) *ctmp2))
			ctmp2++;
		if (!*ctmp2)
			break;

		args[i] = g_alloca (ctmp2 - ctmp + 1);
		strncpy (args[i], ctmp, ctmp2 - ctmp);
		args[i][ctmp2 - ctmp] = '\0';

		ctmp = ctmp2;
		while (*ctmp2 && g_ascii_isspace ((guchar) *ctmp2))
			ctmp2++;
		if (!*ctmp2)
			break;
		ctmp = ctmp2;
	}
	if (!g_ascii_isspace ((guchar) *ctmp) && i < 32)
		args[i++] = ctmp;
        if (i > 1)
                g_warning ("Passing command-line arguments in .server files is deprecated: \"%s\"", si->location_info);
#else
        /* We don't support command-line arguments in the location on
         * Win32, as the executable pathname might well contain spaces
         * itself (C:\Program Files\Evolution 2.6.2\libexec\...).
         * location_info is just the executable's pathname.
         */
        args[0] = g_alloca (strlen (si->location_info) + 1);
        strcpy (args[0], si->location_info);
        i = 1;
#endif

	extra_arg =
		g_alloca (strlen (si->iid) +
			    sizeof ("--oaf-activate-iid="));
	args[i++] = extra_arg;
	sprintf (extra_arg, "--oaf-activate-iid=%s", si->iid);

        fd_arg = i;
	extra_arg = g_alloca (sizeof ("--oaf-ior-fd=") + 10);
	args[i++] = "--oaf-ior-fd=%d";


        iorstr = CORBA_ORB_object_to_string (
                matecomponent_activation_orb_get (), od_obj, ev);

        if (ev->_major != CORBA_NO_EXCEPTION)
	  iorstr = NULL;

        if(actinfo->flags & MateComponent_ACTIVATION_FLAG_PRIVATE) {
                extra_arg = g_alloca (sizeof ("--oaf-private"));
                args[i++] = extra_arg;
                g_snprintf (extra_arg, sizeof ("--oaf-private"),
                            "--oaf-private");
        }

	args[i] = NULL;

        iid = g_strdup (si->iid);

        /* Here comes the too clever by 1/2 bit:
         *   we drop the (recursive) 'server_lock' - so we
         *   can get other threads re-entering / doing activations
         *   here. cf. od_server_activate_factory
         */
        {
                ServerLockState state;

                state = server_lock_drop ();
                /*
                 * We set the process group of activated servers to our process group;
                 * this allows people to destroy all OAF servers along with oafd
                 * if necessary
                 */
                retval = matecomponent_activation_server_by_forking
                        ( (const char **) args, TRUE, fd_arg, environment, iorstr,
                          iid, FALSE, matecomponent_object_directory_re_check_fn, actinfo, ev);

                server_lock_resume (state);
        }

        g_free (iid);

	CORBA_free (iorstr);

        return retval;
}

CORBA_Object
od_server_activate (MateComponent_ServerInfo                  *si,
		    ODActivationInfo                   *actinfo,
		    CORBA_Object                        od_obj,
		    const MateComponent_ActivationEnvironment *environment,
                    MateComponent_ActivationClient             client,
		    CORBA_Environment                  *ev)
{
        g_return_val_if_fail (ev->_major == CORBA_NO_EXCEPTION,
                              CORBA_OBJECT_NIL);

	if (!strcmp (si->server_type, "exe"))
		return od_server_activate_exe (si, actinfo, od_obj, environment, ev);

	else if (!strcmp (si->server_type, "factory"))
		return od_server_activate_factory (si, actinfo, environment, client, ev);

	else if (!strcmp (si->server_type, "shlib"))
		g_warning (_("We don't handle activating shlib objects in a remote process yet"));

	return CORBA_OBJECT_NIL;
}
