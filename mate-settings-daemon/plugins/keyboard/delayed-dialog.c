/*
 * Copyright © 2006 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "delayed-dialog.h"

static gboolean        delayed_show_timeout (gpointer   data);
static GdkFilterReturn message_filter       (GdkXEvent *xevent,
                                             GdkEvent  *event,
                                             gpointer   data);

static GSList *dialogs = NULL;

/**
 * gsd_delayed_show_dialog:
 * @dialog: the dialog
 *
 * Shows the dialog as with gtk_widget_show(), unless a window manager
 * hasn't been started yet, in which case it will wait up to 5 seconds
 * for that to happen before showing the dialog.
 **/
void
gsd_delayed_show_dialog (GtkWidget *dialog)
{
        GdkDisplay *display = gtk_widget_get_display (dialog);
        Display *xdisplay = GDK_DISPLAY_XDISPLAY (display);
        GdkScreen *screen = gtk_widget_get_screen (dialog);
        GdkAtom manager_atom;
        char selection_name[10];
        Atom selection_atom;

        /* We can't use gdk_selection_owner_get() for this, because
         * it's an unknown out-of-process window.
         */
        snprintf (selection_name, sizeof (selection_name), "WM_S%d",
                  gdk_screen_get_number (screen));
        selection_atom = XInternAtom (xdisplay, selection_name, True);
        if (selection_atom &&
            XGetSelectionOwner (xdisplay, selection_atom) != None) {
                gtk_widget_show (dialog);
                return;
        }

        dialogs = g_slist_prepend (dialogs, dialog);

        manager_atom = gdk_atom_intern ("MANAGER", FALSE);
        gdk_display_add_client_message_filter (display, manager_atom,
                                               message_filter, NULL);

        g_timeout_add (5000, delayed_show_timeout, NULL);
}

static gboolean
delayed_show_timeout (gpointer data)
{
        GSList *l;

        for (l = dialogs; l; l = l->next)
                gtk_widget_show (l->data);
        g_slist_free (dialogs);
        dialogs = NULL;

        /* FIXME: There's no gdk_display_remove_client_message_filter */

        return FALSE;
}

static GdkFilterReturn
message_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
        XClientMessageEvent *evt = (XClientMessageEvent *)xevent;
        char *selection_name = XGetAtomName (evt->display, evt->data.l[1]);
        int screen;
        GSList *l, *next;

        if (!dialogs)
                return GDK_FILTER_CONTINUE;

        if (strncmp (selection_name, "WM_S", 4) != 0) {
                XFree (selection_name);
                return GDK_FILTER_CONTINUE;
        }

        screen = atoi (selection_name + 4);

        for (l = dialogs; l; l = next) {
                GtkWidget *dialog = l->data;
                next = l->next;

                if (gdk_screen_get_number (gtk_widget_get_screen (dialog)) == screen) {
                        gtk_widget_show (dialog);
                        dialogs = g_slist_remove (dialogs, dialog);
                }
        }

        if (!dialogs) {
                /* FIXME: There's no gdk_display_remove_client_message_filter */
        }

        return GDK_FILTER_CONTINUE;
}
