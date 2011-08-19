/* MATE GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>
#include <libmate/mate-macros.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <stdarg.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include "mate-messagebox.h"

#include <libmate/mate-triggers.h>
#include <libmate/mate-util.h>
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include <libmateui/mate-uidefs.h>

#define MATE_MESSAGE_BOX_WIDTH  425
#define MATE_MESSAGE_BOX_HEIGHT 125

struct _MateMessageBoxPrivate {
	/* not used currently, if something is added
	 * make sure to update _init and finalize */
	int dummy;
};

MATE_CLASS_BOILERPLATE (MateMessageBox, mate_message_box,
			 MateDialog, MATE_TYPE_DIALOG)

static void
mate_message_box_class_init (MateMessageBoxClass *klass)
{
	/*
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	*/
}

static void
mate_message_box_instance_init (MateMessageBox *message_box)
{
	/*
 	message_box->_priv = g_new0(MateMessageBoxPrivate, 1); 
	*/
}

/**
 * mate_message_box_construct:
 * @messagebox: The message box to construct
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @buttons: a NULL terminated array with the buttons to insert.
 *
 * For language bindings or subclassing, from C use #mate_message_box_new or
 * #mate_message_box_newv
 */
void
mate_message_box_construct (MateMessageBox       *messagebox,
			     const gchar           *message,
			     const gchar           *message_box_type,
			     const gchar          **buttons)
{
	GtkWidget *hbox;
	GtkWidget *pixmap = NULL;
	GtkWidget *alignment;
	GtkWidget *label;
	char *s;
        const gchar* title_prefix = NULL;
        const gchar* appname;

	g_return_if_fail (messagebox != NULL);
	g_return_if_fail (MATE_IS_MESSAGE_BOX (messagebox));
	g_return_if_fail (message != NULL);
	g_return_if_fail (message_box_type != NULL);

	atk_object_set_role (gtk_widget_get_accessible (GTK_WIDGET (messagebox)), ATK_ROLE_ALERT);

	/* Make noises, basically */
	mate_triggers_vdo(message, message_box_type, NULL);

	if (strcmp(MATE_MESSAGE_BOX_INFO, message_box_type) == 0)
	{
                title_prefix = _("Information");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	}
	else if (strcmp(MATE_MESSAGE_BOX_WARNING, message_box_type) == 0)
	{
                title_prefix = _("Warning");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
		
	}
	else if (strcmp(MATE_MESSAGE_BOX_ERROR, message_box_type) == 0)
	{
                title_prefix = _("Error");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_DIALOG);

	}
	else if (strcmp(MATE_MESSAGE_BOX_QUESTION, message_box_type) == 0)
	{
                title_prefix = _("Question");
		pixmap = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	}
	else
	{
                title_prefix = _("Message");
	}

        g_assert(title_prefix != NULL);
        s = NULL;
        appname = mate_program_get_human_readable_name(mate_program_get());
        if (appname) {
                s = g_strdup_printf("%s (%s)", title_prefix, appname);
        }

	mate_dialog_constructv (MATE_DIALOG (messagebox), s ? s : title_prefix, buttons);
	g_free (s);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX(MATE_DIALOG(messagebox)->vbox),
			    hbox, TRUE, TRUE, 10);
	gtk_widget_show (hbox);

	if (pixmap) {
		gtk_box_pack_start (GTK_BOX(hbox), 
				    pixmap, FALSE, TRUE, 0);
		gtk_widget_show (pixmap);
	}

	label = gtk_label_new (message);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_misc_set_padding (GTK_MISC (label), MATE_PAD, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	/* Add some extra space on the right to balance the pixmap */
	if (pixmap) {
		alignment = gtk_alignment_new (0., 0., 0., 0.);
		gtk_widget_set_size_request (alignment, MATE_PAD, -1);
		gtk_widget_show (alignment);
		
		gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);
	}
	
	mate_dialog_set_close (MATE_DIALOG (messagebox),
				TRUE );
}

/**
 * mate_message_box_new:
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @...: A NULL terminated list of strings to use in each button.
 *
 * Creates a dialog box of type @message_box_type with @message.  A number
 * of buttons are inserted on it.  You can use the MATE stock identifiers
 * to create mate-stock-buttons.
 *
 * Returns a widget that has the dialog box.
 */
GtkWidget*
mate_message_box_new (const gchar           *message,
		       const gchar           *message_box_type, ...)
{
	va_list ap;
	MateMessageBox *message_box;

	g_return_val_if_fail (message != NULL, NULL);
	g_return_val_if_fail (message_box_type != NULL, NULL);
        
	va_start (ap, message_box_type);
	
	message_box = g_object_new (MATE_TYPE_MESSAGE_BOX, NULL);

	mate_message_box_construct (message_box, message,
				     message_box_type, NULL);
	
	/* we need to add buttons by hand here */
	while (TRUE) {
		gchar * button_name;

		button_name = va_arg (ap, gchar *);

		if (button_name == NULL) {
			break;
		}

		mate_dialog_append_button ( MATE_DIALOG(message_box), 
					     button_name);
	}
	
	va_end (ap);

	if (MATE_DIALOG (message_box)->buttons) {
		gtk_widget_grab_focus(
			g_list_last (MATE_DIALOG (message_box)->buttons)->data);
	}

	return GTK_WIDGET (message_box);
}

/**
 * mate_message_box_newv:
 * @message: The message to be displayed.
 * @message_box_type: The type of the message
 * @buttons: a NULL terminated array with the buttons to insert.
 *
 * Creates a dialog box of type @message_box_type with @message.  A number
 * of buttons are inserted on it, the messages come from the @buttons array.
 * You can use the MATE stock identifiers to create mate-stock-buttons.
 * The buttons array can be NULL if you wish to add buttons yourself later.
 *
 * Returns a widget that has the dialog box.
 */
GtkWidget*
mate_message_box_newv (const gchar           *message,
		        const gchar           *message_box_type,
			const gchar 	     **buttons)
{
	MateMessageBox *message_box;

	g_return_val_if_fail (message != NULL, NULL);
	g_return_val_if_fail (message_box_type != NULL, NULL);

	message_box = g_object_new (MATE_TYPE_MESSAGE_BOX, NULL);

	mate_message_box_construct (message_box, message,
				     message_box_type, buttons);

	return GTK_WIDGET (message_box);
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
