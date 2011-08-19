/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation: A library for accessing matecomponent-activation-server.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000, 2001 Eazel, Inc.
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

#include <matecomponent-activation/matecomponent-activation-private.h>
#include <glib/gi18n-lib.h>
#include <matecomponent-activation/matecomponent-activation-init.h>
#include <matecomponent-activation/MateComponent_ActivationContext.h>

#include <matecorba/matecorba.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>

static GMutex *thread_lock = NULL;
static GCond  *thread_cond = NULL;
static GSList *running_activations = NULL;

#define RUNNING_LIST_LOCK()   if (thread_lock) g_mutex_lock (thread_lock);
#define RUNNING_LIST_UNLOCK() if (thread_lock) g_mutex_unlock (thread_lock);

/* Whacked from mate-libs/libgnorba/matecorbans.c */

#define IORBUFSIZE 2048

typedef struct {
        gboolean   done;
	char iorbuf[IORBUFSIZE];
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	char *do_srv_output;
#endif
        GIOChannel *ioc;

        /* For list compares */
        const MateComponent_ActivationEnvironment *environment;

        const char *act_iid;
        const char *exename;
        MateComponentForkReCheckFn re_check;
        gpointer            user_data;
} EXEActivateInfo;

static CORBA_Object
exe_activate_info_to_retval (EXEActivateInfo *ai, CORBA_Environment *ev)
{
        CORBA_Object retval;

        g_strstrip (ai->iorbuf);
        if (!strncmp (ai->iorbuf, "IOR:", 4)) {
                retval = CORBA_ORB_string_to_object (matecomponent_activation_orb_get (),
                                                     ai->iorbuf, ev);
                if (ev->_major != CORBA_NO_EXCEPTION)
                        retval = CORBA_OBJECT_NIL;
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                if (ai->do_srv_output)
                        g_message ("Did string_to_object on %s = '%p' (%s)",
                                   ai->iorbuf, retval,
                                   ev->_major == CORBA_NO_EXCEPTION?
                                   "no-exception" : ev->_id);
#endif
        } else {
                MateComponent_GeneralError *errval;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                if (ai->do_srv_output)
                        g_warning ("string doesn't match IOR:");
#endif

                errval = MateComponent_GeneralError__alloc ();

                if (*ai->iorbuf == '\0')
                        errval->description =
                                CORBA_string_dup (_("Child process did not give an error message, unknown failure occurred"));
                else
                        errval->description = CORBA_string_dup (ai->iorbuf);
                CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
                                     ex_MateComponent_GeneralError, errval);
                retval = CORBA_OBJECT_NIL;
        }

        return retval;
}

static EXEActivateInfo *
find_on_list (EXEActivateInfo *seek_ai, CORBA_Environment *ev)
{
        GSList *l;

        for (l = running_activations; l; l = l->next) {
                EXEActivateInfo *ai = l->data;

                if (strcmp (seek_ai->exename, ai->exename))
                        continue;

                if (seek_ai->environment && ai->environment) {
                        if (!MateComponent_ActivationEnvironment_match (seek_ai->environment,
								 ai->environment))
                                continue;

                } else if (seek_ai->environment || ai->environment)
                        continue;

                return ai;
        }

        return NULL;
}

static CORBA_Object
scan_list (EXEActivateInfo *seek_ai, CORBA_Environment *ev)
{
        EXEActivateInfo *ai;
        CORBA_Object retval = CORBA_OBJECT_NIL;


        if (thread_cond) {
                /*
                 * someone might have registered while we
                 * dropped the main server lock => re-check,
                 * inelegant but ...
                 */
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                g_message ("Pre-check for ... '%s' \n", seek_ai->act_iid);
#endif
                retval = seek_ai->re_check (seek_ai->environment,
                                            seek_ai->act_iid,
                                            seek_ai->user_data);
                if (retval != CORBA_OBJECT_NIL)
                        return retval;
        }

        if (!(ai = find_on_list (seek_ai, ev)))
                return CORBA_OBJECT_NIL;

        if (thread_cond) {
                while (ai && !ai->done) {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                        g_message ("thread %p: activation of '%s' already pending, waiting ...\n",
                                   g_thread_self(), seek_ai->act_iid);
#endif
                        /* Wait for something to happen */
                        g_cond_wait (thread_cond, thread_lock);
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                        g_message ("thread %p: activation of '%s' woken up to retry ...\n",
                                   g_thread_self(), seek_ai->act_iid);
#endif
                        ai = find_on_list (seek_ai, ev);
                }
        } else {
                /* We run the loop too ... */
                while (!ai->done)
                        g_main_context_iteration (NULL, TRUE);
        }

        if (ai && !strcmp (seek_ai->act_iid, ai->act_iid)) {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                g_message ("thread %p hit the jackpot '%s' '%s'\n",
                           g_thread_self(), seek_ai->act_iid, ai->act_iid);
#endif
                retval = exe_activate_info_to_retval (ai, ev);
                if (ev->_major != CORBA_NO_EXCEPTION) g_message ("URGH ! 3\n");
        } else if (seek_ai->re_check) {
                /* It might have just registered the IID */
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                g_message ("thread %p re-check for ... '%s' \n",
                           g_thread_self(), seek_ai->act_iid);
#endif
                retval = seek_ai->re_check (seek_ai->environment,
                                            seek_ai->act_iid,
                                            seek_ai->user_data);
        } else {
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                g_warning ("thread %p: very unusual dual activation failure: '%s'\n",
                           g_thread_self(), seek_ai->act_iid);
#endif
        }
                

        return retval;
}

static gboolean
handle_exepipe (GIOChannel * source,
		GIOCondition condition, 
                EXEActivateInfo * data)
{
	gboolean retval = TRUE;

        /* The expected thing is to get this callback maybe twice,
         * once with G_IO_IN and once G_IO_HUP, of course we need to handle
         * other cases.
         */
        
        if (data->iorbuf[0] == '\0' &&
            (condition & (G_IO_IN | G_IO_PRI))) {
                GString *str = g_string_new ("");
                GError *error = NULL;
                GIOStatus status;

                status = g_io_channel_read_line_string (data->ioc, str, NULL, &error);
                if (status == G_IO_STATUS_ERROR) {
                        g_snprintf (data->iorbuf, IORBUFSIZE,
                                    _("Failed to read from child process: %s\n"),
                                    error->message);
                        g_error_free (error);
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                        fprintf (stderr, "b-a-f-s failed to read from child '%s'\n", error->message);
#endif
                        error = NULL;
                        retval = FALSE;
                } else if (status == G_IO_STATUS_EOF) {
                        g_snprintf (data->iorbuf, IORBUFSIZE,
                                    _("EOF from child process\n"));
                        retval = FALSE;
                } else {
                        strncpy (data->iorbuf, str->str, IORBUFSIZE);
                        retval = TRUE;
                }
                g_string_free (str, TRUE);
        } else {                
                retval = FALSE;
        }

	if (retval && !strncmp (data->iorbuf, "IOR:", 4))
		retval = FALSE;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
	if (data->do_srv_output)
		g_message ("srv output[%d]: '%s'", retval, data->iorbuf);
#endif

	if (!retval)
                data->done = TRUE;

	return retval;
}

void
matecomponent_activation_server_fork_init (gboolean threaded)
{
        if (threaded) {
                thread_lock = g_mutex_new ();
                thread_cond = g_cond_new ();
        }
}

#ifndef G_OS_WIN32

static void
child_setup (gpointer user_data)
{
        int pipe_fd = GPOINTER_TO_INT (user_data);

        /* unset close on exec */
        fcntl (pipe_fd, F_SETFD, 0);
}

#else

#define child_setup NULL

#endif

CORBA_Object
matecomponent_activation_server_by_forking (
	const char                         **cmd_const,
	gboolean                             set_process_group,
	int                                  fd_arg, 
	const MateComponent_ActivationEnvironment  *environment,
	const char                          *od_iorstr,
	const char                          *act_iid,
        gboolean                             use_new_loop,
	MateComponentForkReCheckFn                  re_check,
	gpointer                             user_data,
	CORBA_Environment                   *ev)
{
	gint iopipes[2];
	CORBA_Object retval = CORBA_OBJECT_NIL;
        EXEActivateInfo ai;
        GError *error = NULL;
        GSource *source;
        GMainContext *context;
        char **newenv = NULL;
        char **cmd;

        g_return_val_if_fail (cmd_const != NULL, CORBA_OBJECT_NIL);
        g_return_val_if_fail (cmd_const [0] != NULL, CORBA_OBJECT_NIL);
        g_return_val_if_fail (act_iid != NULL, CORBA_OBJECT_NIL);

        ai.environment = environment;
        ai.act_iid = act_iid;
        ai.exename = cmd_const [0];
        ai.re_check = re_check;
        ai.user_data = user_data;

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        ai.do_srv_output = getenv ("MATECOMPONENT_ACTIVATION_DEBUG_EXERUN");
#endif

        RUNNING_LIST_LOCK();
        if (!use_new_loop &&
            (retval = scan_list (&ai, ev)) != CORBA_OBJECT_NIL) {
                RUNNING_LIST_UNLOCK();
                return retval;
        }

        if (thread_lock) /* don't allow re-enterancy in this thread */
                use_new_loop = TRUE;
        
#ifdef G_OS_WIN32
        _pipe (iopipes, 4096, _O_BINARY);
#else
     	pipe (iopipes);
#endif

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        if (ai.do_srv_output)
                fprintf (stderr, " SPAWNING: '%s' for '%s'\n", cmd_const[0], act_iid);
#endif

#ifdef G_OS_WIN32
        ai.ioc = g_io_channel_win32_new_fd (iopipes[0]);
#else
        ai.ioc = g_io_channel_unix_new (iopipes[0]);
#endif
        g_io_channel_set_encoding (ai.ioc, NULL, NULL);

        source = g_io_create_watch
                (ai.ioc, G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_NVAL | G_IO_ERR);
        g_source_set_callback (source, (GSourceFunc) handle_exepipe, &ai, NULL);
        g_source_set_can_recurse (source, TRUE);

        if (use_new_loop)
                context = g_main_context_new ();
        else
                context = g_main_context_default ();
        g_source_attach (source, context);

        /* Set up environment for child */
        if (environment && environment->_length > 0) {
                int i, n;
                char **env, **ep;

                n = environment->_length;

                env = ep = g_listenv ();
                n += g_strv_length (env);
                        
                newenv = g_new (char *, n+1);

                for (i = n = 0; i < environment->_length; i++, n++) {
                        newenv [n] = g_strconcat (environment->_buffer [i].name,
                                                  "=",
                                                  environment->_buffer [i].value,
                                                  NULL);
                }

                while (*ep) {
                        /* No need to check if the environment entry
                         * is well-formed (name=value), g_listenv()
                         * already does and returns only the proper
                         * environment variable names.
                         */
                        for (i = 0; i < environment->_length; i++)
                                if (strcmp (*ep, environment->_buffer [i].name) == 0)
                                        break;
                        if (i == environment->_length) {
                                newenv [n] = g_strconcat (*ep, "=", g_getenv (*ep), NULL);
                                n++;
                        }
                        ep++;
                }
                g_strfreev (env);
                newenv [n] = NULL;
        }

        /* Pass the IOR pipe's write end to the child */ 
        cmd = g_strdupv ((char **)cmd_const);
        if (fd_arg != 0)
        {
                g_free (cmd[fd_arg]);
                cmd[fd_arg] = g_strdup_printf (cmd_const[fd_arg], iopipes[1]);
        }

        ai.iorbuf[0] = '\0';
        ai.done = FALSE;
        
        running_activations = g_slist_prepend (running_activations, &ai);
        RUNNING_LIST_UNLOCK();

	/* Spawn */
        if (!g_spawn_async (NULL, (gchar **) cmd, newenv,
#ifdef G_OS_WIN32
                            /* win32 g_spawn doesn't handle child_setup, so
                               leave all fds open */
                            G_SPAWN_LEAVE_DESCRIPTORS_OPEN |
#endif
                            G_SPAWN_SEARCH_PATH |
                            G_SPAWN_CHILD_INHERITS_STDIN,
                            child_setup, GINT_TO_POINTER (iopipes[1]),
                            NULL,
                            &error)) {
                MateComponent_GeneralError *errval;
                gchar *error_message = g_strconcat (_("Couldn't spawn a new process"),
                                                    ":",
                                                    error->message,
                                                    NULL);
#ifdef MATECOMPONENT_ACTIVATION_DEBUG
                fprintf (stderr, "g_spawn_async error '%s'\n", error->message);
#endif
                g_error_free (error);
                error = NULL;

		errval = MateComponent_GeneralError__alloc ();
		errval->description = CORBA_string_dup (error_message);
                g_free (error_message);

		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_MateComponent_GeneralError, errval);

                RUNNING_LIST_LOCK();
                running_activations = g_slist_remove (running_activations, &ai);
                RUNNING_LIST_UNLOCK();

                if (thread_cond)
                        g_cond_broadcast (thread_cond);

                close (iopipes[1]);

                g_source_destroy (source);
                g_source_unref (source);
                
                g_io_channel_shutdown (ai.ioc, FALSE, NULL);
                g_io_channel_unref (ai.ioc);
                
                if (use_new_loop)
                        g_main_context_unref (context);

                g_strfreev (newenv);
                g_strfreev (cmd);

                close (iopipes[0]);

		return CORBA_OBJECT_NIL;
	}

        close (iopipes[1]);

        g_strfreev (newenv);
        g_strfreev (cmd);

        /* Get the IOR from the pipe */
        while (!ai.done) {
                g_main_context_iteration (context, TRUE);
        }

#ifdef MATECOMPONENT_ACTIVATION_DEBUG
        g_message ("thread %p: broadcast activation of '%s' complete ...\n",
                   g_thread_self(), act_iid);
#endif
        if (thread_cond)
                g_cond_broadcast (thread_cond);

        g_source_destroy (source);
        g_source_unref (source);
        
        g_io_channel_shutdown (ai.ioc, FALSE, NULL);
        g_io_channel_unref (ai.ioc);
        
        if (use_new_loop)
                g_main_context_unref (context);
        
        RUNNING_LIST_LOCK();
        running_activations = g_slist_remove (running_activations, &ai);
        RUNNING_LIST_UNLOCK();

        retval = exe_activate_info_to_retval (&ai, ev); 

        close (iopipes[0]);
                
	return retval;
}
