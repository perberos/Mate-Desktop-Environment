/* mate-propertybox.c - Property dialog box.

   Copyright (C) 1998 Tom Tromey

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

/* Note that the property box is constructed so that we could later
   change how the buttons work.  For instance, we could put an Apply
   button inside each page; this kind of Apply button would only
   affect the current page.  Please do not change the API in a way
   that would violate this goal.  */

#include <config.h>
#include <libmate/mate-macros.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <libmate/mate-util.h>

#include "mate-propertybox.h"

enum
{
	APPLY,
	HELP,
	LAST_SIGNAL
};

/*
 * These four are called from dialog_clicked_cb(), depending
 * on which button was clicked.
 */
static void global_apply     (MatePropertyBox *property_box);
static void help             (MatePropertyBox *property_box);
static void apply_and_close  (MatePropertyBox *property_box);
static void just_close       (MatePropertyBox *property_box);

static void dialog_clicked_cb (MateDialog * dialog, gint button,
			       gpointer data);

static gint property_box_signals [LAST_SIGNAL] = { 0 };

MATE_CLASS_BOILERPLATE(MatePropertyBox, mate_property_box,
			MateDialog, MATE_TYPE_DIALOG)
static void
mate_property_box_class_init (MatePropertyBoxClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) klass;

	parent_class = g_type_class_peek_parent (klass);

	property_box_signals[APPLY] =
		g_signal_new ("apply",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MatePropertyBoxClass, apply),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);
	property_box_signals[HELP] =
		g_signal_new ("help",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MatePropertyBoxClass, help),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);

	klass->apply = NULL;
	klass->help = NULL;
}

static void
mate_property_box_instance_init (MatePropertyBox *property_box)
{
	GList * button_list;

	property_box->notebook = gtk_notebook_new ();

	mate_dialog_append_buttons (MATE_DIALOG (property_box),
				     MATE_STOCK_BUTTON_HELP,
				     MATE_STOCK_BUTTON_APPLY,
				     MATE_STOCK_BUTTON_CLOSE,
				     MATE_STOCK_BUTTON_OK,
				     NULL);

	mate_dialog_set_default (MATE_DIALOG (property_box), 3);

	/* This is sort of unattractive */

	button_list = MATE_DIALOG(property_box)->buttons;

	property_box->help_button = GTK_WIDGET(button_list->data);
	button_list = button_list->next;

	property_box->apply_button = GTK_WIDGET(button_list->data);
	button_list = button_list->next;
	gtk_widget_set_sensitive (property_box->apply_button, FALSE);

	property_box->cancel_button = GTK_WIDGET(button_list->data);
	button_list = button_list->next;
	property_box->ok_button = GTK_WIDGET(button_list->data);

	g_signal_connect (property_box, "clicked",
			  G_CALLBACK(dialog_clicked_cb),
			  NULL);

	gtk_box_pack_start (GTK_BOX(MATE_DIALOG(property_box)->vbox),
			    property_box->notebook,
			    TRUE, TRUE, 0);

	gtk_widget_show (property_box->notebook);
}

/**
 * mate_property_box_new: [constructor]
 *
 * Creates a new MatePropertyBox widget.  The PropertyBox widget
 * is useful for making consistent configuration dialog boxes.
 *
 * When a setting has been made to a property in the PropertyBox
 * your program needs to invoke the mate_property_box_changed to signal
 * a change (this will enable the Ok/Apply buttons).
 *
 * Returns a newly created MatePropertyBox widget.
 */
GtkWidget *
mate_property_box_new (void)
{
	return g_object_new (MATE_TYPE_PROPERTY_BOX, NULL);
}

static void
dialog_clicked_cb(MateDialog * dialog, gint button, gpointer data)
{
	MatePropertyBox *pbox;
	GtkWidget *page;
	gboolean dirty = FALSE;
	gint cur_page_no;

	g_return_if_fail(dialog != NULL);
	g_return_if_fail(MATE_IS_PROPERTY_BOX(dialog));

	pbox = MATE_PROPERTY_BOX (dialog);

	cur_page_no = gtk_notebook_get_current_page (GTK_NOTEBOOK (pbox->notebook));
        if (cur_page_no >= 0) {
	    gint page_no = 0;

	    page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (pbox->notebook), 0);
	    while (page != NULL) {
		dirty = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (page), MATE_PROPERTY_BOX_DIRTY));
		if (dirty)
		    break;

		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (pbox->notebook),
						  ++page_no);
	    }
        } else {
                page = NULL;
                dirty = FALSE;
        }

	switch(button) {
	case 0:
		help (MATE_PROPERTY_BOX (dialog));
		break;
	case 1:
		global_apply (MATE_PROPERTY_BOX (dialog));
		break;
	case 2:
		just_close (MATE_PROPERTY_BOX (dialog));
		break;
	case 3:
		if (dirty)
			apply_and_close (MATE_PROPERTY_BOX (dialog));
		else
			just_close (MATE_PROPERTY_BOX (dialog));
		break;

	default:
		g_assert_not_reached();
	}
}


static void
set_sensitive (MatePropertyBox *property_box, gint dirty)
{
	if (property_box->apply_button)
		gtk_widget_set_sensitive (property_box->apply_button, dirty);
}

/**
 * mate_property_box_changed:
 * @property_box: The MatePropertyBox that contains the changed data
 *
 * When a setting has changed, the code needs to invoke this routine
 * to make the Ok/Apply buttons sensitive.
 */
void
mate_property_box_changed (MatePropertyBox *property_box)
{
	gint cur_page_no;
	GtkWidget *page;

	g_return_if_fail (property_box != NULL);
	g_return_if_fail (MATE_IS_PROPERTY_BOX (property_box));
	g_return_if_fail (property_box->notebook);

	cur_page_no = gtk_notebook_get_current_page (GTK_NOTEBOOK (property_box->notebook));
	if (cur_page_no < 0)
	    return;

	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (property_box->notebook),
					  cur_page_no);
	g_assert (page != NULL);

	g_object_set_data(G_OBJECT(page), MATE_PROPERTY_BOX_DIRTY, GINT_TO_POINTER(1));

	set_sensitive (property_box, 1);
}

/**
 * mate_property_box_set_modified:
 * @property_box: The MatePropertyBox that contains the changed data
 * @state:        The state. %TRUE means modified, %FALSE means unmodified.
 *
 * This sets the modified flag of the MatePropertyBox to the value in @state.
 * Affects whether the OK/Apply buttons are sensitive.
 */
void
mate_property_box_set_modified (MatePropertyBox *property_box, gboolean state)
{
	gint cur_page_no;
	GtkWidget *page;

	g_return_if_fail (property_box != NULL);
	g_return_if_fail (MATE_IS_PROPERTY_BOX (property_box));
	g_return_if_fail (property_box->notebook);
	g_return_if_fail (GTK_NOTEBOOK (property_box->notebook)->cur_page);

	cur_page_no = gtk_notebook_get_current_page (GTK_NOTEBOOK (property_box->notebook));
	if (cur_page_no < 0)
	    return;

	page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (property_box->notebook),
					  cur_page_no);
	g_assert (page != NULL);

	g_object_set_data(G_OBJECT(page), MATE_PROPERTY_BOX_DIRTY, GINT_TO_POINTER(state ? 1 : 0));

	set_sensitive (property_box, state);
}

/**
 * mate_property_box_set_state:
 * @property_box: The MatePropertyBox that contains the changed data
 * @state: The state. %TRUE means modified, %FALSE means unmodified.
 *
 * An alias for  mate_property_box_set_modified().
 */
void
mate_property_box_set_state (MatePropertyBox *property_box, gboolean state)
{
#ifdef MATE_ENABLE_DEBUG
        g_warning("s/mate_property_box_set_state/mate_property_box_set_modified/g");
#endif
        mate_property_box_set_modified (property_box, state);
}

static void
global_apply (MatePropertyBox *property_box)
{
	GtkWidget *page;
	gint n;

	g_return_if_fail(GTK_NOTEBOOK(property_box->notebook)->children != NULL);

	n = 0;
	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(property_box->notebook), n);
	while (page != NULL) {
		if (g_object_get_data(G_OBJECT(page), MATE_PROPERTY_BOX_DIRTY)) {
			g_signal_emit (property_box, property_box_signals[APPLY], 0,
				       n);
			g_object_set_data(G_OBJECT(page), MATE_PROPERTY_BOX_DIRTY, GINT_TO_POINTER(0));
		}

		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(property_box->notebook), ++n);
	}

	/* Emit an apply signal with a button of -1.  This means we
	   just finished a global apply.  Is this a hack?  */
	g_signal_emit (property_box, property_box_signals[APPLY], 0,
		       (gint) -1);

	/* Doesn't matter which item we use. */
	set_sensitive (property_box, 0);
}

static void
help (MatePropertyBox *property_box)
{
	gint page;

	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (property_box->notebook));
	g_signal_emit (property_box, property_box_signals[HELP], 0,
		       page);
}

static void
just_close (MatePropertyBox *property_box)
{
	mate_dialog_close(MATE_DIALOG(property_box));
}

static void
apply_and_close (MatePropertyBox *property_box)
{
	global_apply (property_box);
	just_close (property_box);
}

/**
 * mate_property_box_append_page:
 * @property_box: The property box where we are inserting a new page
 * @child:        The widget that is being inserted
 * @tab_label:    The widget used as the label for this confiugration page
 *
 * Appends a new page to the MatePropertyBox.
 *
 * Returns the assigned index of the page inside the MatePropertyBox or
 * -1 if one of the arguments is invalid.
 */
gint
mate_property_box_append_page (MatePropertyBox *property_box,
				GtkWidget *child,
				GtkWidget *tab_label)
{
	g_return_val_if_fail (property_box != NULL, -1);
	g_return_val_if_fail (MATE_IS_PROPERTY_BOX (property_box), -1);
	g_return_val_if_fail (child != NULL, -1);
	g_return_val_if_fail (GTK_IS_WIDGET (child), -1);
	g_return_val_if_fail (tab_label != NULL, -1);
	g_return_val_if_fail (GTK_IS_WIDGET (tab_label), -1);

	gtk_notebook_append_page (GTK_NOTEBOOK (property_box->notebook),
				  child, tab_label);

	return g_list_length (
		GTK_NOTEBOOK(property_box->notebook)->children) - 1;
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
