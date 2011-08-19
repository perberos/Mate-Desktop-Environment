/* mate-druid-page-edge.c
 * Copyright (C) 1999  Red Hat, Inc.
 *
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

/*
 * FIXME 
 *
 * This widget needs properties like MateDruidPageStandard
 */

#include <config.h>
#include <libmate/mate-macros.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <libmatecanvas/mate-canvas-pixbuf.h>
#include <libmatecanvas/mate-canvas-rect-ellipse.h>
#include <libmatecanvas/mate-canvas-text.h>
#include "mate-druid.h"
#include "mate-uidefs.h"

#include "mate-druid-page-edge.h"

struct _MateDruidPageEdgePrivate
{
	GtkWidget *background;
	GtkWidget *top_watermark;
	GtkWidget *side_watermark;
	GtkWidget *title_label;
	GtkWidget *logo;
	GtkWidget *text_label;
	GtkWidget *contents;

	guint background_color_set : 1;
	guint textbox_color_set : 1;
	guint logo_background_color_set : 1;
	guint title_color_set : 1;
	guint text_color_set : 1;
};

static void mate_druid_page_edge_destroy 	(GtkObject                      *object);
static void mate_druid_page_edge_finalize 	(GObject                        *object);
static void mate_druid_page_edge_set_color     (MateDruidPageEdge             *druid_page_edge);
static void mate_druid_page_edge_style_set     (GtkWidget                      *widget,
						 GtkStyle                       *old_style);
static void mate_druid_page_edge_realize       (GtkWidget                      *widget);
static void mate_druid_page_edge_prepare	(MateDruidPage		        *page,
						 GtkWidget                      *druid);

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_HEIGHT 318
#define DRUID_PAGE_WIDTH 516
#define DRUID_PAGE_LEFT_WIDTH 100.0

MATE_CLASS_BOILERPLATE (MateDruidPageEdge, mate_druid_page_edge,
			 MateDruidPage, MATE_TYPE_DRUID_PAGE)

static void
mate_druid_page_edge_class_init (MateDruidPageEdgeClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	MateDruidPageClass *page_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	page_class = (MateDruidPageClass*) klass;

	object_class->destroy = mate_druid_page_edge_destroy;
	gobject_class->finalize = mate_druid_page_edge_finalize;
	widget_class->style_set = mate_druid_page_edge_style_set;
	widget_class->realize = mate_druid_page_edge_realize;

	page_class->prepare = mate_druid_page_edge_prepare;
}

static void
mate_druid_page_edge_instance_init (MateDruidPageEdge *page)
{
	GtkWidget *table;
	GtkWidget *box;
	MateDruidPageEdgePrivate *priv;

	priv = g_new0(MateDruidPageEdgePrivate, 1);
	page->_priv = priv;

	/* FIXME: we should be a windowed widget, so we can set the
	 * background style on ourself rather than doing this eventbox
	 * hack */
	priv->background = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (page), priv->background);
	gtk_widget_show (priv->background);

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (priv->background), table);
	gtk_widget_show (table);

	/* top bar watermark */
	priv->top_watermark = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->top_watermark), 0.0, 0.0);
	gtk_widget_show (priv->top_watermark);

	/* title label */
	priv->title_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->title_label), 0.0, 0.5);
	gtk_widget_show (priv->title_label);

	/* top bar logo */
	priv->logo = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->logo), 1.0, 0.5);
	gtk_widget_show (priv->logo);

	box = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (box);

	/* side watermark */
	priv->side_watermark = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (priv->side_watermark), 0.0, 1.0);
	gtk_widget_set_size_request (priv->side_watermark, DRUID_PAGE_LEFT_WIDTH, -1);
	gtk_box_pack_start (GTK_BOX (box), priv->side_watermark,
			    FALSE, TRUE, 0);
	gtk_widget_show (priv->side_watermark);

	/* contents event box (used for styles) */
	priv->contents = gtk_event_box_new ();
	gtk_box_pack_start (GTK_BOX (box), priv->contents,
			    TRUE, TRUE, 0);
	gtk_widget_show (priv->contents);

	/* contents label */
	priv->text_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (GTK_LABEL (priv->text_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->text_label), 0.5, 0.5);
	gtk_container_add (GTK_CONTAINER (priv->contents), priv->text_label);
	gtk_widget_show (priv->text_label);

	gtk_table_attach (GTK_TABLE (table),
			  priv->logo,
			  1, 2, 0, 1,
			  GTK_FILL, GTK_FILL,
			  5, 5);

	gtk_table_attach (GTK_TABLE (table),
			  priv->title_label,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_FILL,
			  5, 5);

	gtk_table_attach (GTK_TABLE (table),
			  priv->top_watermark,
			  0, 2, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_FILL,
			  0, 0);

	gtk_table_attach (GTK_TABLE (table),
			  box,
			  0, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);

}

/**
 * mate_druid_page_edge_construct:
 * @druid_page_edge: A #MateDruidPageEdge instance.
 * @position: The position of @druid_page_edge within the druid.
 * @antialiased: Unused in the current implementation. Set to %FALSE.
 * @title: The title of the page.
 * @text: The text in the body of the page.
 * @logo: The logo on the page.
 * @watermark: The watermark on the side of the page.
 * @top_watermark: The watermark on the top of the page.
 *
 * Description: Useful for subclassing and language bindings, this function
 * fills the given pieces of information into the existing @druid_page_edge.
 **/
void
mate_druid_page_edge_construct (MateDruidPageEdge *druid_page_edge,
				 MateEdgePosition   position,
				 gboolean            antialiased,
				 const gchar        *title,
				 const gchar        *text,
				 GdkPixbuf          *logo,
				 GdkPixbuf          *watermark,
				 GdkPixbuf          *top_watermark)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (position >= MATE_EDGE_START &&
			  position < MATE_EDGE_LAST);

	druid_page_edge->position = position;

	if (title)
		mate_druid_page_edge_set_title (druid_page_edge, title);

	if (text)
		mate_druid_page_edge_set_text  (druid_page_edge, text);

	if (logo)
		mate_druid_page_edge_set_logo (druid_page_edge, logo);

	if (watermark)
		mate_druid_page_edge_set_watermark (druid_page_edge, watermark);

	if (top_watermark)
		mate_druid_page_edge_set_top_watermark (druid_page_edge, top_watermark);
}

static void
mate_druid_page_edge_destroy(GtkObject *object)
{
	MateDruidPageEdge *druid_page_edge = MATE_DRUID_PAGE_EDGE(object);

	/* remember, destroy can be run multiple times! */

	if (druid_page_edge->logo_image != NULL)
		g_object_unref (G_OBJECT (druid_page_edge->logo_image));
	druid_page_edge->logo_image = NULL;

	if (druid_page_edge->watermark_image != NULL)
		g_object_unref (G_OBJECT (druid_page_edge->watermark_image));
	druid_page_edge->watermark_image = NULL;

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_druid_page_edge_finalize(GObject *object)
{
	MateDruidPageEdge *druid_page_edge = MATE_DRUID_PAGE_EDGE(object);

	g_free (druid_page_edge->text);
	druid_page_edge->text = NULL;
	g_free (druid_page_edge->title);
	druid_page_edge->title = NULL;

	g_free (druid_page_edge->_priv);
	druid_page_edge->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
mate_druid_page_edge_set_color (MateDruidPageEdge *page)
{
	MateDruidPageEdgePrivate *priv = page->_priv;
	GtkWidget *widget = GTK_WIDGET (page);
	
	if (!priv->background_color_set)
		page->background_color = widget->style->bg[GTK_STATE_SELECTED];

	if (!priv->textbox_color_set)
		page->textbox_color = widget->style->bg[GTK_STATE_NORMAL];

	if (!priv->logo_background_color_set)
		page->logo_background_color = widget->style->bg[GTK_STATE_SELECTED];

	if (!priv->title_color_set)
		page->title_color = widget->style->fg[GTK_STATE_SELECTED];

	if (!priv->text_color_set)
		page->text_color = widget->style->fg[GTK_STATE_NORMAL];

	gtk_widget_modify_bg (priv->background, GTK_STATE_NORMAL, &page->background_color);
	gtk_widget_modify_bg (priv->contents, GTK_STATE_NORMAL, &page->textbox_color);
	gtk_widget_modify_bg (priv->logo, GTK_STATE_NORMAL, &page->logo_background_color);
	gtk_widget_modify_fg (priv->title_label, GTK_STATE_NORMAL, &page->title_color);
	gtk_widget_modify_fg (priv->text_label, GTK_STATE_NORMAL, &page->text_color);
}

static void
mate_druid_page_edge_prepare (MateDruidPage *page,
			       GtkWidget *druid)
{
	switch (MATE_DRUID_PAGE_EDGE (page)->position) {
	case MATE_EDGE_START:
		mate_druid_set_buttons_sensitive (MATE_DRUID (druid), FALSE, TRUE, TRUE, TRUE);
		mate_druid_set_show_finish (MATE_DRUID (druid), FALSE);
		if (GTK_IS_WINDOW (gtk_widget_get_toplevel (druid)))
			gtk_widget_grab_default (MATE_DRUID (druid)->next);
		break;
	case MATE_EDGE_FINISH:
		mate_druid_set_buttons_sensitive (MATE_DRUID (druid), TRUE, FALSE, TRUE, TRUE);
		mate_druid_set_show_finish (MATE_DRUID (druid), TRUE);
		if (GTK_IS_WINDOW (gtk_widget_get_toplevel (druid)))
			gtk_widget_grab_default (MATE_DRUID (druid)->finish);
		break;
	case MATE_EDGE_OTHER:
		mate_druid_set_buttons_sensitive (MATE_DRUID (druid), TRUE, TRUE, TRUE, TRUE);
		mate_druid_set_show_finish (MATE_DRUID (druid), FALSE);
	default:
		break;
	}
}

/**
 * mate_druid_page_edge_new:
 * @position: Position in druid.
 *
 * Description: Creates a new MateDruidPageEdge widget.
 *
 * Returns: #GtkWidget pointer to a new #MateDruidPageEdge.
 **/
/* Public functions */
GtkWidget *
mate_druid_page_edge_new (MateEdgePosition position)
{
	MateDruidPageEdge *retval;

	g_return_val_if_fail (position >= MATE_EDGE_START &&
			      position < MATE_EDGE_LAST, NULL);

	retval = g_object_new (MATE_TYPE_DRUID_PAGE_EDGE, NULL);

	mate_druid_page_edge_construct (retval,
					 position,
					 FALSE,
					 NULL,
					 NULL,
					 NULL,
					 NULL,
					 NULL);

	return GTK_WIDGET (retval);
}

/**
 * mate_druid_page_edge_new_aa:
 * @position: Position in druid.
 *
 * Description: Creates a new MateDruidPageEdge widget.  The
 * internal canvas is created in an antialiased mode.
 *
 * Returns: #GtkWidget pointer to a new #MateDruidPageEdge.
 **/
/* Public functions */
GtkWidget *
mate_druid_page_edge_new_aa (MateEdgePosition position)
{
	MateDruidPageEdge *retval;

	g_return_val_if_fail (position >= MATE_EDGE_START &&
			      position < MATE_EDGE_LAST, NULL);

	retval = g_object_new (MATE_TYPE_DRUID_PAGE_EDGE, NULL);

	mate_druid_page_edge_construct (retval,
					 position,
					 TRUE,
					 NULL,
					 NULL,
					 NULL,
					 NULL,
					 NULL);

	return GTK_WIDGET (retval);
}

/**
 * mate_druid_page_edge_new_with_vals:
 * @position: Position in druid.
 * @antialiased: Use an antialiased canvas
 * @title: The title.
 * @text: The introduction text.
 * @logo: The logo in the upper right corner.
 * @watermark: The watermark on the left.
 * @top_watermark: The watermark on the left.
 *
 * This will create a new MATE Druid Edge page, with the values given.  It is
 * acceptable for any of them to be %NULL.  Position should be
 * %MATE_EDGE_START, %MATE_EDGE_FINISH or %MATE_EDGE_OTHER.
 *
 * Returns: #GtkWidget pointer to a new #MateDruidPageEdge.
 **/
GtkWidget *
mate_druid_page_edge_new_with_vals (MateEdgePosition position,
				     gboolean antialiased,
				     const gchar *title,
				     const gchar* text,
				     GdkPixbuf *logo,
				     GdkPixbuf *watermark,
				     GdkPixbuf *top_watermark)
{
	MateDruidPageEdge *retval;

	g_return_val_if_fail (position >= MATE_EDGE_START &&
			      position < MATE_EDGE_LAST, NULL);

	retval = g_object_new (MATE_TYPE_DRUID_PAGE_EDGE, NULL);

	mate_druid_page_edge_construct (retval,
					 position,
					 antialiased,
					 title,
					 text,
					 logo,
					 watermark,
					 top_watermark);
	return GTK_WIDGET (retval);
}

/**
 * mate_druid_page_edge_set_bg_color:
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @color: The new background color.
 *
 * Description:  This will set the background color to be the @color.  You do
 * not need to allocate the color, as the @druid_page_edge will do it for you.
 **/
void
mate_druid_page_edge_set_bg_color      (MateDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->background_color = *color;
	druid_page_edge->_priv->background_color_set = TRUE;

	gtk_widget_modify_bg (druid_page_edge->_priv->background, GTK_STATE_NORMAL, color);
}

/**
 * mate_druid_page_edge_set_textbox_color
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @color: The new textbox color.
 *
 * Sets the color of the background in the main text area of the page.
 */
void
mate_druid_page_edge_set_textbox_color (MateDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->textbox_color = *color;
	druid_page_edge->_priv->textbox_color_set = TRUE;

	gtk_widget_modify_bg (druid_page_edge->_priv->contents, GTK_STATE_NORMAL, color);
}

/**
 * mate_druid_page_edge_set_logo_bg_color
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @color: The new color of the logo's background.
 *
 * Set the color behind the druid page's logo.
 */
void
mate_druid_page_edge_set_logo_bg_color (MateDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->logo_background_color = *color;
	druid_page_edge->_priv->logo_background_color_set = TRUE;

	gtk_widget_modify_bg (druid_page_edge->_priv->logo, GTK_STATE_NORMAL, color);
}

/**
 * mate_druid_page_edge_set_title_color
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @color: The color of the title text.
 *
 * Sets the color of the title text on the current page.
 */
void
mate_druid_page_edge_set_title_color   (MateDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->title_color = *color;
	druid_page_edge->_priv->title_color_set = 1;

	gtk_widget_modify_fg (druid_page_edge->_priv->title_label, GTK_STATE_NORMAL, color);
}

/**
 * mate_druid_page_edge_set_text_color
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @color: The new test color.
 *
 * Sets the color of the text in the body of the druid page.
 */
void
mate_druid_page_edge_set_text_color    (MateDruidPageEdge *druid_page_edge,
					 GdkColor *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));
	g_return_if_fail (color != NULL);

	druid_page_edge->text_color = *color;
	druid_page_edge->_priv->text_color_set = TRUE;

	gtk_widget_modify_fg (druid_page_edge->_priv->text_label, GTK_STATE_NORMAL, color);
}

/**
 * mate_druid_page_edge_set_text
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @text: The text contents.
 *
 * Sets the contents of the text portion of the druid page.
 */
void
mate_druid_page_edge_set_text (MateDruidPageEdge *druid_page_edge,
				const gchar *text)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));

	g_free (druid_page_edge->text);
	druid_page_edge->text = g_strdup (text);
	gtk_label_set_text (GTK_LABEL (druid_page_edge->_priv->text_label), text);
}

/**
 * mate_druid_page_edge_set_title
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @title: The title text
 *
 * Sets the contents of the page's title text.
 */
void
mate_druid_page_edge_set_title         (MateDruidPageEdge *druid_page_edge,
					 const gchar *title)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));

	g_free (druid_page_edge->title);
	druid_page_edge->title = g_strconcat ("<span size=\"xx-large\" weight=\"ultrabold\">",
					      title ? title : "", "</span>", NULL);
	gtk_label_set_text (GTK_LABEL (druid_page_edge->_priv->title_label),
			    druid_page_edge->title);
	gtk_label_set_use_markup (GTK_LABEL (druid_page_edge->_priv->title_label), TRUE);
}

/**
 * mate_druid_page_edge_set_logo:
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @logo_image: The #GdkPixbuf to use as a logo.
 *
 * Description:  Sets a #GdkPixbuf as the logo in the top right corner.
 * If %NULL, then no logo will be displayed.
 **/
void
mate_druid_page_edge_set_logo (MateDruidPageEdge *druid_page_edge,
				GdkPixbuf *logo_image)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));

	if (druid_page_edge->logo_image != NULL)
		g_object_unref (G_OBJECT (druid_page_edge->logo_image));

	druid_page_edge->logo_image = logo_image;
	if (logo_image != NULL )
		g_object_ref (G_OBJECT (logo_image));
	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_edge->_priv->logo), logo_image);
}

/**
 * mate_druid_page_edge_set_watermark:
 * @druid_page_edge: A @MateDruidPageEdge instance.
 * @watermark: The #GdkPixbuf to use as a watermark.
 *
 * Description: Sets a #GdkPixbuf as the watermark on the left strip on the
 * druid. If @watermark is %NULL, it is reset to the normal color.
 **/
void
mate_druid_page_edge_set_watermark (MateDruidPageEdge *druid_page_edge,
				     GdkPixbuf *watermark)
{
	int width = 0;
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));

	if (druid_page_edge->watermark_image != NULL)
		g_object_unref (G_OBJECT (druid_page_edge->watermark_image));

	druid_page_edge->watermark_image = watermark;
	if (watermark != NULL )
		g_object_ref (G_OBJECT (watermark));
	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_edge->_priv->side_watermark), watermark);

	if (watermark)
		width = gdk_pixbuf_get_width (watermark);

	gtk_widget_set_size_request (druid_page_edge->_priv->side_watermark,
				     width ? -1 : DRUID_PAGE_LEFT_WIDTH, -1);
}

/**
 * mate_druid_page_edge_set_top_watermark:
 * @druid_page_edge: the #MateDruidPageEdge to work on
 * @top_watermark_image: The #GdkPixbuf to use as a top watermark
 *
 * Description:  Sets a #GdkPixbuf as the watermark on top of the top
 * strip on the druid.  If @top_watermark_image is %NULL, it is reset
 * to the normal color.
 **/
void
mate_druid_page_edge_set_top_watermark (MateDruidPageEdge *druid_page_edge,
					 GdkPixbuf *top_watermark_image)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_EDGE (druid_page_edge));

	if (druid_page_edge->top_watermark_image)
		g_object_unref (G_OBJECT (druid_page_edge->top_watermark_image));

	druid_page_edge->top_watermark_image = top_watermark_image;
	if (top_watermark_image != NULL)
		g_object_ref (G_OBJECT (top_watermark_image));
	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_edge->_priv->top_watermark), 
				   top_watermark_image);
}

static void
mate_druid_page_edge_style_set (GtkWidget *widget,
				  GtkStyle *old_style)
{
	MATE_CALL_PARENT (GTK_WIDGET_CLASS, style_set, (widget, old_style));
	mate_druid_page_edge_set_color (MATE_DRUID_PAGE_EDGE (widget));
}

static void
mate_druid_page_edge_realize (GtkWidget *widget)
{
	MATE_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));
	mate_druid_page_edge_set_color (MATE_DRUID_PAGE_EDGE (widget));
}
