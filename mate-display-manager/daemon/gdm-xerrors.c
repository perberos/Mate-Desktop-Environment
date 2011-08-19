/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 William Jon McCann <jmccann@redhat.com>
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

/* Most of this is derived from GTK+ (copyright GTK+ team) */

#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <stdlib.h>

#include "gdm-xerrors.h"

typedef struct _GdmErrorTrap  GdmErrorTrap;

struct _GdmErrorTrap
{
        int (*old_handler) (Display *, XErrorEvent *);
        int error_warnings;
        int error_code;
};

static int          gdm_x_error                  (Display     *display,
                                                  XErrorEvent *error);
static int          gdm_x_io_error               (Display     *display);

/* Private variable declarations
 */
static GSList *gdm_error_traps = NULL;               /* List of error traps */
static GSList *gdm_error_trap_free_list = NULL;      /* Free list */

static int                _gdm_error_warnings = TRUE;
static int                _gdm_error_code = 0;

void
gdm_xerrors_init (void)
{
        static gboolean initialized = FALSE;

        if (initialized) {
                return;
        }

        XSetErrorHandler (gdm_x_error);
        XSetIOErrorHandler (gdm_x_io_error);
        initialized = TRUE;
}

/*
 *--------------------------------------------------------------
 * gdm_x_error
 *
 *   The X error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *   "error" is the XErrorEvent that we are handling.
 *
 * Results:
 *   Either we were expecting some sort of error to occur,
 *   in which case we set the "_gdm_error_code" flag, or this
 *   error was unexpected, in which case we will print an
 *   error message and exit. (Since trying to continue will
 *   most likely simply lead to more errors).
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdm_x_error (Display     *display,
             XErrorEvent *error)
{
        if (error->error_code) {
                if (_gdm_error_warnings) {
                        gchar buf[64];
                        gchar *msg;

                        XGetErrorText (display, error->error_code, buf, 63);

                        msg =
                                g_strdup_printf ("The program '%s' received an X Window System error.\n"
                                                 "This probably reflects a bug in the program.\n"
                                                 "The error was '%s'.\n"
                                                 "  (Details: serial %ld error_code %d request_code %d minor_code %d)\n"
                                                 "  (Note to programmers: normally, X errors are reported asynchronously;\n"
                                                 "   that is, you will receive the error a while after causing it.\n"
                                                 "   To debug your program, run it with the --sync command line\n"
                                                 "   option to change this behavior. You can then get a meaningful\n"
                                                 "   backtrace from your debugger if you break on the gdm_x_error() function.)",
                                                 g_get_prgname (),
                                                 buf,
                                                 error->serial,
                                                 error->error_code,
                                                 error->request_code,
                                                 error->minor_code);

#ifdef G_ENABLE_DEBUG
                        g_error ("%s", msg);
#else /* !G_ENABLE_DEBUG */
                        g_fprintf (stderr, "%s\n", msg);

                        exit (1);
#endif /* G_ENABLE_DEBUG */
                }
                _gdm_error_code = error->error_code;
        }

        return 0;
}

/*
 *--------------------------------------------------------------
 * gdm_x_io_error
 *
 *   The X I/O error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *
 * Results:
 *   An X I/O error basically means we lost our connection
 *   to the X server. There is not much we can do to
 *   continue, so simply print an error message and exit.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdm_x_io_error (Display *display)
{
        /* This is basically modelled after the code in XLib. We need
         * an explicit error handler here, so we can disable our atexit()
         * which would otherwise cause a nice segfault.
         * We fprintf(stderr, instead of g_warning() because g_warning()
         * could possibly be redirected to a dialog
         */
        if (errno == EPIPE) {
                g_fprintf (stderr,
                           "The application '%s' lost its connection to the display %s;\n"
                           "most likely the X server was shut down or you killed/destroyed\n"
                           "the application.\n",
                           g_get_prgname (),
                           display ? DisplayString (display) : "<unknown>");
        } else {
                g_fprintf (stderr, "%s: Fatal IO error %d (%s) on X server %s.\n",
                           g_get_prgname (),
                           errno, g_strerror (errno),
                           display ? DisplayString (display) : "<unknown>");
        }

        exit(1);
}

/*************************************************************
 * gdm_error_trap_push:
 *     Push an error trap. X errors will be trapped until
 *     the corresponding gdm_error_pop(), which will return
 *     the error code, if any.
 *   arguments:
 *
 *   results:
 *************************************************************/

void
gdm_error_trap_push (void)
{
        GSList *node;
        GdmErrorTrap *trap;

        if (gdm_error_trap_free_list) {
                node = gdm_error_trap_free_list;
                gdm_error_trap_free_list = gdm_error_trap_free_list->next;
        } else {
                node = g_slist_alloc ();
                node->data = g_new (GdmErrorTrap, 1);
        }

        node->next = gdm_error_traps;
        gdm_error_traps = node;

        trap = node->data;
        trap->old_handler = XSetErrorHandler (gdm_x_error);
        trap->error_code = _gdm_error_code;
        trap->error_warnings = _gdm_error_warnings;

        _gdm_error_code = 0;
        _gdm_error_warnings = 0;
}

/*************************************************************
 * gdm_error_trap_pop:
 *     Pop an error trap added with gdm_error_push()
 *   arguments:
 *
 *   results:
 *     0, if no error occured, otherwise the error code.
 *************************************************************/

gint
gdm_error_trap_pop (void)
{
        GSList *node;
        GdmErrorTrap *trap;
        gint result;

        g_return_val_if_fail (gdm_error_traps != NULL, 0);

        node = gdm_error_traps;
        gdm_error_traps = gdm_error_traps->next;

        node->next = gdm_error_trap_free_list;
        gdm_error_trap_free_list = node;

        result = _gdm_error_code;

        trap = node->data;
        _gdm_error_code = trap->error_code;
        _gdm_error_warnings = trap->error_warnings;
        XSetErrorHandler (trap->old_handler);

        return result;
}
