/* Eye Of Mate - Erro Message Area
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-message-area.h) by:
 * 	- Paolo Maggi <paolo@mate.org>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-error-message-area.h"
#include "eog-image.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

static void
set_message_area_text_and_icon (GtkInfoBar   *message_area,
				const gchar  *icon_stock_id,
				const gchar  *primary_text,
				const gchar  *secondary_text)
{
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;

	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);

	image = gtk_image_new_from_stock (icon_stock_id, GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);

	gtk_widget_show (primary_label);

	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), FALSE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);

	gtk_widget_set_can_focus (primary_label, TRUE);

	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

  	if (secondary_text != NULL) {
  		secondary_markup = g_strdup_printf ("<small>%s</small>",
  						    secondary_text);
		secondary_label = gtk_label_new (secondary_markup);
		g_free (secondary_markup);

		gtk_widget_show (secondary_label);

		gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);

		gtk_widget_set_can_focus (secondary_label, TRUE);

		gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	}

	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (message_area))), hbox_content, TRUE, TRUE, 0);
}

static GtkWidget *
create_error_message_area (const gchar *primary_text,
			   const gchar *secondary_text,
			   gboolean     recoverable)
{
	GtkWidget *message_area;

	if (recoverable)
		message_area = gtk_info_bar_new_with_buttons (_("_Retry"),
							      GTK_RESPONSE_OK,
							      NULL);
	else
		message_area = gtk_info_bar_new ();

	gtk_info_bar_set_message_type (GTK_INFO_BAR (message_area),
				       GTK_MESSAGE_ERROR);

	set_message_area_text_and_icon (GTK_INFO_BAR (message_area),
					GTK_STOCK_DIALOG_ERROR,
					primary_text,
					secondary_text);

	return message_area;
}

GtkWidget *
eog_image_load_error_message_area_new (const gchar  *caption,
				       const GError *error)
{
	GtkWidget *message_area;
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *pango_escaped_caption = NULL;

	g_return_val_if_fail (caption != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);

	/* Escape the caption string with respect to pango markup.
	   This is necessary because otherwise characters like "&" will
	   be interpreted as the beginning of a pango entity inside
	   the message area GtkLabel. */
	pango_escaped_caption = g_markup_escape_text (caption, -1);
	error_message = g_strdup_printf (_("Could not load image '%s'."),
					 pango_escaped_caption);

	message_details = g_strdup (error->message);

	message_area = create_error_message_area (error_message,
						  message_details,
						  TRUE);

	g_free (pango_escaped_caption);
	g_free (error_message);
	g_free (message_details);

	return message_area;
}

GtkWidget *
eog_no_images_error_message_area_new (GFile *file)
{
	GtkWidget *message_area;
	gchar *error_message = NULL;

	if (file != NULL) {
		gchar *uri_str, *unescaped_str, *pango_escaped_str;

		uri_str = g_file_get_uri (file);
		/* Unescape URI with respect to rules defined in RFC 3986. */
		unescaped_str = g_uri_unescape_string (uri_str, NULL);

		/* Escape the URI string with respect to pango markup.
		   This is necessary because the URI string can contain
		   for example "&" which will otherwise be interpreted
		   as a pango markup entity when inserted into a GtkLabel. */
		pango_escaped_str = g_markup_escape_text (unescaped_str, -1);
		error_message = g_strdup_printf (_("No images found in '%s'."),
						 pango_escaped_str);

		g_free (pango_escaped_str);
		g_free (uri_str);
		g_free (unescaped_str);
	} else {
		error_message = g_strdup (_("The given locations contain no images."));
	}

	message_area = create_error_message_area (error_message,
						  NULL,
						  FALSE);

	g_free (error_message);

	return message_area;
}
