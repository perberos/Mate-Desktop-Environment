/* mate-href.c
 * Copyright (C) 1998-2001, James Henstridge <james@daa.com.au>
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

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include "config.h"
#include <libmate/mate-macros.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <string.h> /* for strlen */

#include <gtk/gtk.h>
#include <libmate/mate-url.h>
#include "mate-href.h"
#include "libmateui-access.h"
#include "mate-url.h"

struct _MateHRefPrivate {
	gchar *url;
	GtkWidget *label;
};

static void mate_href_clicked		(GtkButton *button);
static void mate_href_destroy		(GtkObject *object);
static void mate_href_finalize		(GObject *object);
static void mate_href_get_property	(GObject *object,
					 guint param_id,
					 GValue *value,
					 GParamSpec * pspec);
static void mate_href_set_property	(GObject *object,
					 guint param_id,
					 const GValue * value,
					 GParamSpec * pspec);
static void mate_href_realize		(GtkWidget *widget);
static void mate_href_style_set        (GtkWidget *widget,
					 GtkStyle *previous_style);
static void drag_data_get    		(MateHRef          *href,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 gpointer            data);

enum {
	PROP_0,
	PROP_URL,
	PROP_TEXT
};

/**
 * mate_href_get_type
 *
 * Returns the type assigned to the MATE href widget.
 **/
/* The following defines the get_type */
MATE_CLASS_BOILERPLATE (MateHRef, mate_href,
			 GtkButton, GTK_TYPE_BUTTON)

static void
mate_href_class_init (MateHRefClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
	GtkButtonClass *button_class = (GtkButtonClass *)klass;

	object_class->destroy = mate_href_destroy;

	gobject_class->finalize = mate_href_finalize;
	gobject_class->set_property = mate_href_set_property;
	gobject_class->get_property = mate_href_get_property;

	widget_class->realize = mate_href_realize;
	widget_class->style_set = mate_href_style_set;
	button_class->clicked = mate_href_clicked;

	/* By default we link to The World Food Programme */
	g_object_class_install_property (gobject_class,
					 PROP_URL,
					 g_param_spec_string ("url",
							      _("URL"),
							      _("The URL that MateHRef activates"),
							      "http://www.wfp.org",
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
							      _("Text"),
							      _("The text on the button"),
							      _("End World Hunger"),
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));

	gtk_widget_class_install_style_property (GTK_WIDGET_CLASS (gobject_class),
						 g_param_spec_boxed ("link_color",
								     _("Link color"),
								     _("Color used to draw the link"),
								     GDK_TYPE_COLOR,
								     G_PARAM_READABLE));
}

static void
mate_href_instance_init (MateHRef *href)
{
	href->_priv = g_new0(MateHRefPrivate, 1);

	href->_priv->label = gtk_label_new(NULL);
	g_object_ref(href->_priv->label);


	gtk_button_set_relief(GTK_BUTTON(href), GTK_RELIEF_NONE);

	gtk_container_add(GTK_CONTAINER(href), href->_priv->label);
	gtk_widget_show(href->_priv->label);

	href->_priv->url = NULL;

	/* the source dest is set on set_url */
	g_signal_connect (href, "drag_data_get",
			  G_CALLBACK (drag_data_get), NULL);

	/* Set our accessible description.  We don't set the name as we want it
	 * to be the contents of the label, which is the default anyways.
	 */

	_add_atk_name_desc (GTK_WIDGET (href),
			    NULL,
			    _("This button will take you to the URI that it displays."));
}

static void
drag_data_get(MateHRef          *href,
	      GdkDragContext     *context,
	      GtkSelectionData   *selection_data,
	      guint               info,
	      guint               time,
	      gpointer            data)
{
	g_return_if_fail (href != NULL);
	g_return_if_fail (MATE_IS_HREF (href));

	if( ! href->_priv->url) {
		/*FIXME: cancel the drag*/
		return;
	}

	/* if this doesn't look like an url, it's probably a file */
	if(strchr(href->_priv->url, ':') == NULL) {
		char *s = g_strdup_printf("file:%s\r\n", href->_priv->url);
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, (unsigned char *)s, strlen(s)+1);
		g_free(s);
	} else {
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, (unsigned char *)href->_priv->url, strlen(href->_priv->url)+1);
	}
}

/**
 * mate_href_new
 * @url: URL assigned to this object.
 * @text: Text associated with the URL.
 *
 * Description:
 * Created a MATE href object, a label widget with a clickable action
 * and an associated URL.  If @text is set to %NULL, @url is used as
 * the text for the label.
 *
 * Returns:  Pointer to new MATE href widget.
 **/

GtkWidget *
mate_href_new(const gchar *url, const gchar *text)
{
  MateHRef *href;

  g_return_val_if_fail(url != NULL, NULL);

  href = g_object_new (MATE_TYPE_HREF,
		       "url", url,
		       "text", text,
		       NULL);

  return GTK_WIDGET (href);
}


/**
 * mate_href_get_url
 * @href: Pointer to MateHRef widget
 *
 * Description:
 * Returns the pointer to the URL associated with the @href href object.  Note
 * that the string should not be freed as it is internal memory.
 *
 * Returns:  Pointer to an internal URL string, or %NULL if failure.
 **/

const gchar *mate_href_get_url(MateHRef *href) {
  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(MATE_IS_HREF(href), NULL);
  return href->_priv->url;
}


/**
 * mate_href_set_url
 * @href: Pointer to MateHRef widget
 * @url: String containing the URL to be stored within @href.
 *
 * Description:
 * Sets the internal URL value within @href to the value of @url.
 **/

void mate_href_set_url(MateHRef *href, const gchar *url) {
  g_return_if_fail(href != NULL);
  g_return_if_fail(MATE_IS_HREF(href));
  g_return_if_fail(url != NULL);

  if (href->_priv->url) {
	  gtk_drag_source_unset(GTK_WIDGET(href));
	  g_free(href->_priv->url);
  }
  href->_priv->url = g_strdup(url);
  if(strncmp(url, "http://", 7) == 0 ||
     strncmp(url, "https://", 8) == 0) {
	  const GtkTargetEntry http_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "x-url/http",          0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       http_drop_types, G_N_ELEMENTS (http_drop_types),
			       GDK_ACTION_COPY);
  } else if(strncmp(url, "ftp://", 6) == 0) {
	  const GtkTargetEntry ftp_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "x-url/ftp",           0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       ftp_drop_types, G_N_ELEMENTS (ftp_drop_types),
			       GDK_ACTION_COPY);
  } else {
	  const GtkTargetEntry other_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       other_drop_types, G_N_ELEMENTS (other_drop_types),
			       GDK_ACTION_COPY);
  }
}


/**
 * mate_href_get_text
 * @href: Pointer to MateHRef widget
 *
 * Description:
 * Returns the contents of the label widget used to display the link text.
 * Note that the string should not be freed as it points to internal memory.
 *
 * Returns:  Pointer to text contained in the label widget.
 **/

const gchar *
mate_href_get_text(MateHRef *href)
{

  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(MATE_IS_HREF(href), NULL);

  return gtk_label_get_text (GTK_LABEL(href->_priv->label));
}


/**
 * mate_href_set_text
 * @href: Pointer to MateHRef widget
 * @text: New link text for the href object.
 *
 * Description:
 * Sets the internal label widget text (used to display a URL's link
 * text) to the value given in @label.
 **/
void
mate_href_set_text (MateHRef *href, const gchar *text)
{
  gchar *markup;

  g_return_if_fail(href != NULL);
  g_return_if_fail(MATE_IS_HREF(href));
  g_return_if_fail(text != NULL);

  /* underline the string */
  markup = g_strdup_printf("<u>%s</u>", text);
  gtk_label_set_markup(GTK_LABEL(href->_priv->label), markup);
  g_free(markup);
}

/**
 * mate_href_get_label
 * @href: Pointer to MateHRef widget
 *
 * Deprecated, use #mate_href_get_text.
 *
 * Returns:
 **/

const gchar *
mate_href_get_label(MateHRef *href)
{
	g_warning("mate_href_get_label is deprecated, use mate_href_get_text");
	return mate_href_get_text(href);
}


/**
 * mate_href_set_label
 * @href: Pointer to MateHRef widget
 * @label: New link text for the href object.
 *
 * Description:
 * deprecated, use #mate_href_set_text
 **/

void
mate_href_set_label (MateHRef *href, const gchar *label)
{
	g_return_if_fail(href != NULL);
	g_return_if_fail(MATE_IS_HREF(href));

	g_warning("mate_href_set_label is deprecated, use mate_href_set_text");
	mate_href_set_text(href, label);
}

static void
mate_href_clicked (GtkButton *button)
{
  MateHRef *href;

  g_return_if_fail(button != NULL);
  g_return_if_fail(MATE_IS_HREF(button));

  MATE_CALL_PARENT (GTK_BUTTON_CLASS, clicked, (button));

  href = MATE_HREF(button);

  g_return_if_fail(href->_priv->url != NULL);

  /* FIXME: Use the error variable from mate_url_show_on_screen */
  if(!mate_url_show_on_screen (href->_priv->url, gtk_widget_get_screen (GTK_WIDGET (href)), NULL)) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					       GTK_BUTTONS_OK,
					       _("An error has occurred while trying to launch the "
						 "default web browser.\n"
						 "Please check your settings in the "
						 "'Preferred Applications' preference tool."));
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

static void
mate_href_destroy (GtkObject *object)
{
	MateHRef *href;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_HREF(object));

	href = MATE_HREF (object);

	if (href->_priv->label != NULL) {
		g_object_unref (href->_priv->label);
		href->_priv->label = NULL;
	}

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_href_finalize (GObject *object)
{
	MateHRef *href;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_HREF(object));

	href = MATE_HREF (object);

	g_free (href->_priv->url);
	href->_priv->url = NULL;

	g_free (href->_priv);
	href->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
mate_href_realize(GtkWidget *widget)
{
	GdkCursor *cursor;

	MATE_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));

	cursor = gdk_cursor_new (GDK_HAND2);
	gdk_window_set_cursor (GTK_BUTTON (widget)->event_window, cursor);
	gdk_cursor_unref (cursor);
}

static void
mate_href_style_set (GtkWidget *widget,
		      GtkStyle *previous_style)
{
	GdkColor *link_color;
	GdkColor blue = { 0, 0x0000, 0x0000, 0xffff };
	MateHRef *href;

	href = MATE_HREF (widget);
	
	gtk_widget_style_get (widget,
			      "link_color", &link_color,
			      NULL);

	if (!link_color)
		link_color = &blue;

	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_NORMAL, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_ACTIVE, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_PRELIGHT, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_SELECTED, link_color);

	if (link_color != &blue)
		gdk_color_free (link_color);
}

static void
mate_href_set_property (GObject *object,
			 guint param_id,
			 const GValue * value,
			 GParamSpec * pspec)
{
	MateHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_HREF (object));

	self = MATE_HREF (object);

	switch (param_id) {
	case PROP_URL:
		mate_href_set_url(self, g_value_get_string (value));
		break;
	case PROP_TEXT:
		mate_href_set_text(self, g_value_get_string (value));
		break;
	default:
		break;
	}
}

static void
mate_href_get_property (GObject *object,
		      guint param_id,
		      GValue *value,
		      GParamSpec * pspec)
{
	MateHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_HREF (object));

	self = MATE_HREF (object);

	switch (param_id) {
	case PROP_URL:
		g_value_set_string (value,
				    mate_href_get_url(self));
		break;
	case PROP_TEXT:
		g_value_set_string (value,
				    mate_href_get_text(self));
		break;
	default:
		break;
	}
}

#endif /* not MATE_DISABLE_DEPRECATED_SOURCE */
