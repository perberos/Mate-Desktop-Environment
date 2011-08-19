/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Red Hat, Inc.
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#include <syslog.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "gdm-signal-handler.h"

#define GDM_SIGNAL_HANDLER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_SIGNAL_HANDLER, GdmSignalHandlerPrivate))

typedef struct {
        int                  signal_number;
        GdmSignalHandlerFunc func;
        gpointer             data;
        guint                id;
} CallbackData;

struct GdmSignalHandlerPrivate
{
        GHashTable    *lookup;
        GHashTable    *id_lookup;
        GHashTable    *action_lookup;
        guint          next_id;
        GDestroyNotify fatal_func;
        gpointer       fatal_data;
};

static void     gdm_signal_handler_class_init   (GdmSignalHandlerClass *klass);
static void     gdm_signal_handler_init         (GdmSignalHandler      *signal_handler);
static void     gdm_signal_handler_finalize     (GObject               *object);

static gpointer signal_handler_object = NULL;
static int      signal_pipes[2];
static int      signals_blocked = 0;
static sigset_t signals_block_mask;
static sigset_t signals_oldmask;

G_DEFINE_TYPE (GdmSignalHandler, gdm_signal_handler, G_TYPE_OBJECT)

static void
block_signals_push (void)
{
        signals_blocked++;

        if (signals_blocked == 1) {
                /* Set signal mask */
                sigemptyset (&signals_block_mask);
                sigfillset (&signals_block_mask);
                sigprocmask (SIG_BLOCK, &signals_block_mask, &signals_oldmask);
        }
}

static void
block_signals_pop (void)
{
        signals_blocked--;

        if (signals_blocked == 0) {
                /* Set signal mask */
                sigprocmask (SIG_SETMASK, &signals_oldmask, NULL);
        }
}

static gboolean
signal_io_watch (GIOChannel       *ioc,
                 GIOCondition      condition,
                 GdmSignalHandler *handler)
{
        char     buf[256];
        gboolean is_fatal;
        gsize    bytes_read;
        int      i;

        block_signals_push ();

        g_io_channel_read_chars (ioc, buf, sizeof (buf), &bytes_read, NULL);

        is_fatal = FALSE;

        for (i = 0; i < bytes_read; i++) {
                int     signum;
                GSList *handlers;
                GSList *l;

                signum = (gint32)buf[i];

                g_debug ("GdmSignalHandler: handling signal %d", signum);
                handlers = g_hash_table_lookup (handler->priv->lookup, GINT_TO_POINTER (signum));

                g_debug ("GdmSignalHandler: Found %u callbacks", g_slist_length (handlers));
                for (l = handlers; l != NULL; l = l->next) {
                        gboolean      res;
                        CallbackData *data;

                        data = g_hash_table_lookup (handler->priv->id_lookup, l->data);
                        if (data != NULL) {
                                if (data->func != NULL) {
                                        g_debug ("GdmSignalHandler: running %d handler: %p", signum, data->func);
                                        res = data->func (signum, data->data);
                                        if (! res) {
                                                is_fatal = TRUE;
                                        }
                                }
                        }
                }
        }

        block_signals_pop ();

        if (is_fatal) {
                if (handler->priv->fatal_func != NULL) {
                        g_debug ("GdmSignalHandler: Caught termination signal - calling fatal func");
                        handler->priv->fatal_func (handler->priv->fatal_data);
                } else {
                        g_debug ("GdmSignalHandler: Caught termination signal - exiting");
                        exit (1);
                }

                return FALSE;
        }

        g_debug ("GdmSignalHandler: Done handling signals");

        return TRUE;
}

static void
fallback_get_backtrace (void)
{
#if HAVE_EXECINFO_H
        void *  frames[64];
        size_t  size;
        char ** strings;
        size_t  i;

        size = backtrace (frames, G_N_ELEMENTS (frames));
        if ((strings = backtrace_symbols (frames, size))) {
                syslog (LOG_CRIT, "******************* START ********************************");
                for (i = 0; i < size; i++) {
                        syslog (LOG_CRIT, "Frame %zd: %s", i, strings[i]);
                }
                free (strings);
                syslog (LOG_CRIT, "******************* END **********************************");
        } else {
#endif
                g_warning ("GDM crashed, but symbols couldn't be retrieved.");
#if HAVE_EXECINFO_H
        }
#endif
}


static gboolean
crashlogger_get_backtrace (void)
{
        gboolean success = FALSE;
        int      pid;

        pid = fork ();
        if (pid > 0) {
                /* Wait for the child to finish */
                int estatus;
                if (waitpid (pid, &estatus, 0) != -1) {
                        /* Only succeed if the crashlogger succeeded */
                        if (WIFEXITED (estatus) && (WEXITSTATUS (estatus) == 0)) {
                                success = TRUE;
                        }
                }
        } else if (pid == 0) {
                /* Child process */
                execl (LIBEXECDIR "/gdm-crash-logger",
                       LIBEXECDIR "/gdm-crash-logger", NULL);
        }

        return success;
}


static void
gdm_signal_handler_backtrace (void)
{
        struct stat s;
        gboolean    fallback = TRUE;

        /* Try to use gdb via gdm-crash-logger if it exists, since
         * we get much better information out of it.  Otherwise
         * fall back to execinfo.
         */
        if (g_stat (LIBEXECDIR "/gdm-crash-logger", &s) == 0) {
                fallback = crashlogger_get_backtrace () ? FALSE : TRUE;
        }

        if (fallback) {
                fallback_get_backtrace ();
        }
}

static void
signal_handler (int signo)
{
        static int in_fatal = 0;
        int        ignore;
        guchar     signo_byte = signo;

        /* avoid loops */
        if (in_fatal > 0) {
                return;
        }

        ++in_fatal;

        switch (signo) {
        case SIGSEGV:
        case SIGBUS:
        case SIGILL:
        case SIGABRT:
        case SIGTRAP:
                gdm_signal_handler_backtrace ();
                exit (1);
                break;
        case SIGFPE:
        case SIGPIPE:
                /* let the fatal signals interrupt us */
                --in_fatal;
                gdm_signal_handler_backtrace ();
                ignore = write (signal_pipes [1], &signo_byte, 1);
                break;
        default:
                --in_fatal;
                ignore = write (signal_pipes [1], &signo_byte, 1);
                break;
        }
}

static void
catch_signal (GdmSignalHandler *handler,
              int               signal_number)
{
        struct sigaction  action;
        struct sigaction *old_action;

        g_debug ("GdmSignalHandler: Registering for %d signals", signal_number);

        action.sa_handler = signal_handler;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;

        old_action = g_new0 (struct sigaction, 1);

        sigaction (signal_number, &action, old_action);

        g_hash_table_insert (handler->priv->action_lookup,
                             GINT_TO_POINTER (signal_number),
                             old_action);
}

static void
uncatch_signal (GdmSignalHandler *handler,
                int               signal_number)
{
        struct sigaction *old_action;

        g_debug ("GdmSignalHandler: Unregistering for %d signals", signal_number);

        old_action = g_hash_table_lookup (handler->priv->action_lookup,
                                          GINT_TO_POINTER (signal_number));
        g_hash_table_remove (handler->priv->action_lookup,
                             GINT_TO_POINTER (signal_number));

        sigaction (signal_number, old_action, NULL);

        g_free (old_action);
}

guint
gdm_signal_handler_add (GdmSignalHandler    *handler,
                        int                  signal_number,
                        GdmSignalHandlerFunc callback,
                        gpointer             data)
{
        CallbackData *cdata;
        GSList       *list;

        g_return_val_if_fail (GDM_IS_SIGNAL_HANDLER (handler), 0);

        cdata = g_new0 (CallbackData, 1);
        cdata->signal_number = signal_number;
        cdata->func = callback;
        cdata->data = data;
        cdata->id = handler->priv->next_id++;

        g_debug ("GdmSignalHandler: Adding handler %u: signum=%d %p", cdata->id, cdata->signal_number, cdata->func);

        if (g_hash_table_lookup (handler->priv->action_lookup, GINT_TO_POINTER (signal_number)) == NULL) {
                catch_signal (handler, signal_number);
        }

        /* ID lookup owns the CallbackData */
        g_hash_table_insert (handler->priv->id_lookup, GUINT_TO_POINTER (cdata->id), cdata);

        list = g_hash_table_lookup (handler->priv->lookup, GINT_TO_POINTER (signal_number));
        list = g_slist_prepend (list, GUINT_TO_POINTER (cdata->id));

        g_hash_table_insert (handler->priv->lookup, GINT_TO_POINTER (signal_number), list);

        return cdata->id;
}

void
gdm_signal_handler_add_fatal (GdmSignalHandler *handler)
{
        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (handler));

        gdm_signal_handler_add (handler, SIGILL, NULL, NULL);
        gdm_signal_handler_add (handler, SIGBUS, NULL, NULL);
        gdm_signal_handler_add (handler, SIGSEGV, NULL, NULL);
        gdm_signal_handler_add (handler, SIGABRT, NULL, NULL);
        gdm_signal_handler_add (handler, SIGTRAP, NULL, NULL);
}

static void
callback_data_free (CallbackData *d)
{
        g_free (d);
}

static void
gdm_signal_handler_remove_and_free_data (GdmSignalHandler *handler,
                                         CallbackData     *cdata)
{
        GSList *list;

        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (handler));

        list = g_hash_table_lookup (handler->priv->lookup, GINT_TO_POINTER (cdata->signal_number));
        list = g_slist_remove_all (list, GUINT_TO_POINTER (cdata->id));
        if (list == NULL) {
                uncatch_signal (handler, cdata->signal_number);
        }

        g_debug ("GdmSignalHandler: Removing handler %u: signum=%d %p", cdata->signal_number, cdata->id, cdata->func);
        /* put changed list back in */
        g_hash_table_insert (handler->priv->lookup, GINT_TO_POINTER (cdata->signal_number), list);

        g_hash_table_remove (handler->priv->id_lookup, GUINT_TO_POINTER (cdata->id));
}

void
gdm_signal_handler_remove (GdmSignalHandler    *handler,
                           guint                id)
{
        CallbackData *found;

        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (handler));

        found = g_hash_table_lookup (handler->priv->id_lookup, GUINT_TO_POINTER (id));
        if (found != NULL) {
                gdm_signal_handler_remove_and_free_data (handler, found);
                found = NULL;
        }
}

static CallbackData *
find_callback_data_by_func (GdmSignalHandler    *handler,
                            guint                signal_number,
                            GdmSignalHandlerFunc callback,
                            gpointer             data)
{
        GSList       *list;
        GSList       *l;
        CallbackData *found;

        found = NULL;

        list = g_hash_table_lookup (handler->priv->lookup, GINT_TO_POINTER (signal_number));

        for (l = list; l != NULL; l = l->next) {
                guint         id;
                CallbackData *d;

                id = GPOINTER_TO_UINT (l->data);

                d = g_hash_table_lookup (handler->priv->id_lookup, GUINT_TO_POINTER (id));
                if (d != NULL
                    && d->func == callback
                    && d->data == data) {
                        found = d;
                        break;
                }
        }

        return found;
}

void
gdm_signal_handler_remove_func (GdmSignalHandler    *handler,
                                guint                signal_number,
                                GdmSignalHandlerFunc callback,
                                gpointer             data)
{
        CallbackData *found;

        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (handler));

        found = find_callback_data_by_func (handler, signal_number, callback, data);

        if (found != NULL) {
                gdm_signal_handler_remove_and_free_data (handler, found);
                found = NULL;
        }

        /* FIXME: once all handlers are removed deregister signum handler */
}

static void
gdm_signal_handler_class_init (GdmSignalHandlerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gdm_signal_handler_finalize;

        g_type_class_add_private (klass, sizeof (GdmSignalHandlerPrivate));
}

static void
signal_list_free (GSList *list)
{
        g_slist_free (list);
}

void
gdm_signal_handler_set_fatal_func (GdmSignalHandler       *handler,
                                   GdmShutdownHandlerFunc  func,
                                   gpointer                user_data)
{
        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (handler));

        handler->priv->fatal_func = func;
        handler->priv->fatal_data = user_data;
}

static void
gdm_signal_handler_init (GdmSignalHandler *handler)
{
        GIOChannel *ioc;

        handler->priv = GDM_SIGNAL_HANDLER_GET_PRIVATE (handler);

        handler->priv->next_id = 1;

        handler->priv->lookup = g_hash_table_new (NULL, NULL);
        handler->priv->id_lookup = g_hash_table_new (NULL, NULL);
        handler->priv->action_lookup = g_hash_table_new (NULL, NULL);

        if (pipe (signal_pipes) == -1) {
                g_error ("Could not create pipe() for signal handling");
        }

        ioc = g_io_channel_unix_new (signal_pipes[0]);
        g_io_channel_set_flags (ioc, G_IO_FLAG_NONBLOCK, NULL);
        g_io_add_watch_full (ioc, G_PRIORITY_HIGH, G_IO_IN,
                             (GIOFunc) signal_io_watch, handler, NULL);
        g_io_channel_set_close_on_unref (ioc, TRUE);
        g_io_channel_unref (ioc);
}

static void
gdm_signal_handler_finalize (GObject *object)
{
        GdmSignalHandler *handler;
        GList            *l;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SIGNAL_HANDLER (object));

        handler = GDM_SIGNAL_HANDLER (object);

        g_debug ("GdmSignalHandler: Finalizing signal handler");

        g_return_if_fail (handler->priv != NULL);
        for (l = g_hash_table_get_values (handler->priv->lookup);
             l != NULL; l = l->next) {
                signal_list_free ((GSList *) l->data);
        }
        g_hash_table_destroy (handler->priv->lookup);
        for (l = g_hash_table_get_values (handler->priv->id_lookup);
             l != NULL; l = l->next) {
                callback_data_free ((CallbackData *) l->data);
        }
        g_hash_table_destroy (handler->priv->id_lookup);
        for (l = g_hash_table_get_values (handler->priv->action_lookup);
             l != NULL; l = l->next) {
                g_free (l->data);
        }
        g_hash_table_destroy (handler->priv->action_lookup);

        close (signal_pipes [0]);
        close (signal_pipes [1]);

        G_OBJECT_CLASS (gdm_signal_handler_parent_class)->finalize (object);
}

GdmSignalHandler *
gdm_signal_handler_new (void)
{
        if (signal_handler_object != NULL) {
                g_object_ref (signal_handler_object);
        } else {
                signal_handler_object = g_object_new (GDM_TYPE_SIGNAL_HANDLER, NULL);
                g_object_add_weak_pointer (signal_handler_object,
                                           (gpointer *) &signal_handler_object);
        }

        return GDM_SIGNAL_HANDLER (signal_handler_object);
}
