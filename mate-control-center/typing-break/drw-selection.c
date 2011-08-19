/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright © 2002 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Owen Taylor, Red Hat, Inc.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "drw-selection.h"

struct _DrwSelection
{
	GdkWindow *owner_window;
	GtkWidget *invisible;
};

#define SELECTION_NAME "_CODEFACTORY_DRWRIGHT"

static GdkFilterReturn drw_selection_filter     (GdkXEvent   *xevent,
						 GdkEvent    *event,
						 gpointer     data);
static void            drw_selection_negotiate  (DrwSelection *drw_selection);

static void
drw_selection_reset (DrwSelection *drw_selection)
{
	if (drw_selection->owner_window) {
		gdk_window_remove_filter (drw_selection->owner_window,
					  drw_selection_filter, drw_selection);
		g_object_unref (drw_selection->owner_window);
		drw_selection->owner_window = NULL;
	}

	if (drw_selection->invisible) {
		gtk_widget_destroy (drw_selection->invisible);
		drw_selection->invisible = NULL;
	}
}

static void
drw_selection_clear (GtkWidget         *widget,
		    GdkEventSelection *event,
		    gpointer           user_data)
{
	DrwSelection *drw_selection = user_data;

	drw_selection_reset (drw_selection);
	drw_selection_negotiate (drw_selection);
}

static gboolean
drw_selection_find_existing (DrwSelection *drw_selection)
{
	Display *xdisplay = GDK_DISPLAY ();
	Window old;

	gdk_error_trap_push ();
	old = XGetSelectionOwner (xdisplay,
				  gdk_x11_get_xatom_by_name (SELECTION_NAME));
	if (old) {
		XSelectInput (xdisplay, old, StructureNotifyMask);
		drw_selection->owner_window = gdk_window_foreign_new (old);
	}
	XSync (xdisplay, False);

	if (gdk_error_trap_pop () == 0 && drw_selection->owner_window) {
		gdk_window_add_filter (drw_selection->owner_window,
				       drw_selection_filter, drw_selection);

		XUngrabServer (xdisplay);

		return TRUE;
	} else {
		if (drw_selection->owner_window) {
			g_object_unref (drw_selection->owner_window);
			drw_selection->owner_window = NULL;
		}

		return FALSE;
	}
}

static gboolean
drw_selection_claim (DrwSelection *drw_selection)
{
	drw_selection->invisible = gtk_invisible_new ();
	g_signal_connect (drw_selection->invisible, "selection-clear-event",
			  G_CALLBACK (drw_selection_clear), drw_selection);


	if (gtk_selection_owner_set (drw_selection->invisible,
				     gdk_atom_intern (SELECTION_NAME, FALSE),
				     GDK_CURRENT_TIME)) {
		return TRUE;
	} else {
		drw_selection_reset (drw_selection);
		return FALSE;
	}
}

static void
drw_selection_negotiate (DrwSelection *drw_selection)
{
	Display *xdisplay = GDK_DISPLAY ();
	gboolean found = FALSE;

	/* We don't need both the XGrabServer() and the loop here;
	 * the XGrabServer() should make sure that we only go through
	 * the loop once. It also works if you remove the XGrabServer()
	 * and just have the loop, but then the selection ownership
	 * can get transfered a bunch of times before things
	 * settle down.
	 */
	while (!found)
	{
		XGrabServer (xdisplay);

		if (drw_selection_find_existing (drw_selection))
			found = TRUE;
		else if (drw_selection_claim (drw_selection))
			found = TRUE;

		XUngrabServer (xdisplay);
	}
}

static GdkFilterReturn
drw_selection_filter (GdkXEvent *xevent,
		     GdkEvent  *event,
		     gpointer   data)
{
	DrwSelection *drw_selection = data;
	XEvent *xev = (XEvent *)xevent;

	if (xev->xany.type == DestroyNotify &&
	    xev->xdestroywindow.window == xev->xdestroywindow.event)
	{
		drw_selection_reset (drw_selection);
		drw_selection_negotiate (drw_selection);

		return GDK_FILTER_REMOVE;
	}

	return GDK_FILTER_CONTINUE;
}

DrwSelection *
drw_selection_start (void)
{
	DrwSelection *drw_selection = g_new (DrwSelection, 1);

	drw_selection->owner_window = NULL;
	drw_selection->invisible = NULL;

	drw_selection_negotiate (drw_selection);

	return drw_selection;
}

void
drw_selection_stop (DrwSelection *drw_selection)
{
	drw_selection_reset (drw_selection);
	g_free (drw_selection);
}

gboolean
drw_selection_is_master (DrwSelection *drw_selection)
{
	return drw_selection->invisible != NULL;
}
