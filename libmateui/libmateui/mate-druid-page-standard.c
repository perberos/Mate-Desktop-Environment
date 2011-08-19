/* mate-druid-page-standard.c
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2001  James M. Cape <jcape@ignore-your.tv>
 * Copyright (C) 2001  Jonathan Blandford <jrb@alum.mit.edu>
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

#include <config.h>
#include <libmate/mate-macros.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include "mate-druid.h"
#include "mate-uidefs.h"

#include <gtk/gtk.h>
#include <string.h>
#include "mate-druid-page-standard.h"

struct _MateDruidPageStandardPrivate
{
	GtkWidget *background;
	GtkWidget *watermark;
	GtkWidget *logo;
	GtkWidget *title_label;

	GtkWidget *evbox;

	guint title_foreground_set : 1;
	guint background_set : 1;
	guint logo_background_set : 1;
	guint contents_background_set : 1;
};


static void mate_druid_page_standard_get_property  (GObject                     *object,
						     guint                        prop_id,
						     GValue                      *value,
						     GParamSpec                  *pspec);
static void mate_druid_page_standard_set_property  (GObject                     *object,
						     guint                        prop_id,
						     const GValue                *value,
						     GParamSpec                  *pspec);
static void mate_druid_page_standard_finalize      (GObject                     *widget);
static void mate_druid_page_standard_destroy       (GtkObject                   *object);
static void mate_druid_page_standard_realize       (GtkWidget                   *widget);
static void mate_druid_page_standard_style_set     (GtkWidget                   *widget,
						     GtkStyle                    *old_style);
static void mate_druid_page_standard_prepare       (MateDruidPage              *page,
						     GtkWidget                   *druid);
static void mate_druid_page_standard_set_color     (MateDruidPageStandard      *druid_page_standard);

enum {
	PROP_0,
	PROP_TITLE,
	PROP_LOGO,
	PROP_TOP_WATERMARK,
	PROP_TITLE_FOREGROUND,
	PROP_TITLE_FOREGROUND_GDK,
	PROP_TITLE_FOREGROUND_SET,
	PROP_BACKGROUND,
	PROP_BACKGROUND_GDK,
	PROP_BACKGROUND_SET,
	PROP_LOGO_BACKGROUND,
	PROP_LOGO_BACKGROUND_GDK,
	PROP_LOGO_BACKGROUND_SET,
	PROP_CONTENTS_BACKGROUND,
	PROP_CONTENTS_BACKGROUND_GDK,
	PROP_CONTENTS_BACKGROUND_SET
};

MATE_CLASS_BOILERPLATE (MateDruidPageStandard, mate_druid_page_standard,
			 MateDruidPage, MATE_TYPE_DRUID_PAGE)

static void
mate_druid_page_standard_class_init (MateDruidPageStandardClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	MateDruidPageClass *druid_page_class;

	object_class = (GtkObjectClass*) class;
	gobject_class = (GObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;
	druid_page_class = (MateDruidPageClass*) class;

	gobject_class->get_property = mate_druid_page_standard_get_property;
	gobject_class->set_property = mate_druid_page_standard_set_property;
	gobject_class->finalize = mate_druid_page_standard_finalize;
	object_class->destroy = mate_druid_page_standard_destroy;
	widget_class->realize = mate_druid_page_standard_realize;
	widget_class->style_set = mate_druid_page_standard_style_set;
	druid_page_class->prepare = mate_druid_page_standard_prepare;

	g_object_class_install_property (gobject_class,
					 PROP_TITLE,
					 g_param_spec_string ("title",
							      _("Title"),
							      _("Title of the druid"),
							      NULL,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO,
					 g_param_spec_object ("logo",
							      _("Logo"),
							      _("Logo image"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TOP_WATERMARK,
					 g_param_spec_object ("top_watermark",
							      _("Top Watermark"),
							      _("Watermark image for the top"),
							      GDK_TYPE_PIXBUF,
							      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND,
					 g_param_spec_string ("title_foreground",
							      _("Title Foreground"),
							      _("Foreground color of the title"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND_GDK,
					 g_param_spec_boxed ("title_foreground_gdk",
							     _("Title Foreground Color"),
							     _("Foreground color of the title as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_TITLE_FOREGROUND_SET,
					 g_param_spec_boolean ("title_foreground_set",
							       _("Title Foreground color set"),
							       _("Foreground color of the title is set"),
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND,
					 g_param_spec_string ("background",
							      _("Background Color"),
							      _("Background color"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND_GDK,
					 g_param_spec_boxed ("background_gdk",
							     _("Background Color"),
							     _("Background color as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_BACKGROUND_SET,
					 g_param_spec_boolean ("background_set",
							       _("Background color set"),
							       _("Background color is set"),
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_CONTENTS_BACKGROUND,
					 g_param_spec_string ("contents_background",
							      _("Contents Background Color"),
							      _("Contents Background color"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_CONTENTS_BACKGROUND_GDK,
					 g_param_spec_boxed ("contents_background_gdk",
							     _("Contents Background Color"),
							     _("Contents Background color as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_CONTENTS_BACKGROUND_SET,
					 g_param_spec_boolean ("contents_background_set",
							       _("Contents Background color set"),
							       _("Contents Background color is set"),
							       FALSE,
							       G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND,
					 g_param_spec_string ("logo_background",
							      _("Logo Background Color"),
							      _("Logo Background color"),
							      NULL,
							      G_PARAM_WRITABLE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND_GDK,
					 g_param_spec_boxed ("logo_background_gdk",
							     _("Logo Background Color"),
							     _("Logo Background color as a GdkColor"),
							     GDK_TYPE_COLOR,
							     G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_LOGO_BACKGROUND_SET,
					 g_param_spec_boolean ("logo_background_set",
							       _("Logo Background color set"),
							       _("Logo Background color is set"),
							       FALSE,
							       G_PARAM_READWRITE));
}

static void
mate_druid_page_standard_instance_init (MateDruidPageStandard *druid_page_standard)
{
	GtkWidget *table;

	druid_page_standard->_priv = g_new0(MateDruidPageStandardPrivate, 1);

	/* FIXME: we should be a windowed widget, so we can set the
	 * style on ourself rather than doing this eventbox hack 
	 */

	/* Top bar background */
	druid_page_standard->_priv->background = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (druid_page_standard),
			   druid_page_standard->_priv->background);
	gtk_widget_show (druid_page_standard->_priv->background);

	/* Top bar layout widget */
	table = gtk_table_new (2, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (druid_page_standard->_priv->background),
			   table);
	gtk_widget_show (table);

	/* Top Bar Watermark */
	druid_page_standard->_priv->watermark = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (druid_page_standard->_priv->watermark), 0.0, 0.0);
	gtk_widget_show (druid_page_standard->_priv->watermark);
	
	/* Title label */
	druid_page_standard->_priv->title_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (druid_page_standard->_priv->title_label), 0.0, 0.5);
	gtk_widget_show (druid_page_standard->_priv->title_label);

	/* Top bar logo*/
	druid_page_standard->_priv->logo = gtk_image_new ();
	gtk_misc_set_alignment (GTK_MISC (druid_page_standard->_priv->logo), 1.0, 0.5);
	gtk_widget_show (druid_page_standard->_priv->logo);

	/* Contents Event Box (used for styles) */
	druid_page_standard->_priv->evbox = gtk_event_box_new ();
	gtk_widget_show (druid_page_standard->_priv->evbox);

	/* Contents Vbox */
	druid_page_standard->vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_standard->vbox), 16);
	gtk_container_add (GTK_CONTAINER (druid_page_standard->_priv->evbox), druid_page_standard->vbox);
	gtk_widget_show (druid_page_standard->vbox);

	/* do this order exactly, so that it displays with the correct
	 * z-ordering */
	gtk_table_attach (GTK_TABLE (table),
			  druid_page_standard->_priv->logo,
			  1, 2, 0, 1,
			  GTK_FILL, GTK_FILL,
			  5, 5);

	gtk_table_attach (GTK_TABLE (table),
			  druid_page_standard->_priv->title_label,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_FILL,
			  5, 5);

	gtk_table_attach (GTK_TABLE (table),
			  druid_page_standard->_priv->watermark,
			  0, 2, 0, 1, 
			  GTK_EXPAND | GTK_FILL,
			  GTK_FILL,
			  0, 0);

	gtk_table_attach (GTK_TABLE (table),
			  druid_page_standard->_priv->evbox,
			  0, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  1, 1);
}

static void
mate_druid_page_standard_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
	MateDruidPageStandard *druid_page_standard = MATE_DRUID_PAGE_STANDARD (object);

	switch (prop_id) {
	case PROP_TITLE:
		g_value_set_string (value, druid_page_standard->title);
		break;
	case PROP_LOGO:
		g_value_set_object (value, druid_page_standard->logo);
		break;
	case PROP_TOP_WATERMARK:
		g_value_set_object (value, druid_page_standard->top_watermark);
		break;
	case PROP_TITLE_FOREGROUND_GDK:
		g_value_set_boxed (value, &druid_page_standard->title_foreground);
		break;
	case PROP_TITLE_FOREGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->title_foreground_set);
		break;
	case PROP_BACKGROUND_GDK:
		g_value_set_boxed (value, &druid_page_standard->background);
		break;
	case PROP_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->background_set);
		break;
	case PROP_LOGO_BACKGROUND_GDK:
		g_value_set_boxed (value, &druid_page_standard->logo_background);
		break;
	case PROP_LOGO_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->logo_background_set);
		break;
	case PROP_CONTENTS_BACKGROUND_GDK:
		g_value_set_boxed (value, &druid_page_standard->contents_background);
		break;
	case PROP_CONTENTS_BACKGROUND_SET:
		g_value_set_boolean (value, druid_page_standard->_priv->contents_background_set);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
mate_druid_page_standard_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
	MateDruidPageStandard *druid_page_standard;
	GdkColor color;

	druid_page_standard = MATE_DRUID_PAGE_STANDARD (object);

	switch (prop_id) {
	case PROP_TITLE:
		mate_druid_page_standard_set_title (druid_page_standard, g_value_get_string (value));
		break;
	case PROP_LOGO:
		mate_druid_page_standard_set_logo (druid_page_standard, g_value_get_object (value));
		break;
	case PROP_TOP_WATERMARK:
		mate_druid_page_standard_set_top_watermark (druid_page_standard, g_value_get_object (value));
		break;
	case PROP_TITLE_FOREGROUND:
	case PROP_TITLE_FOREGROUND_GDK:
		if (prop_id == PROP_TITLE_FOREGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		mate_druid_page_standard_set_title_foreground (druid_page_standard, &color);
		break;
	case PROP_TITLE_FOREGROUND_SET:
		druid_page_standard->_priv->title_foreground_set = g_value_get_boolean (value);
		break;
	case PROP_BACKGROUND:
	case PROP_BACKGROUND_GDK:
		if (prop_id == PROP_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		mate_druid_page_standard_set_background (druid_page_standard, &color);
		break;
	case PROP_BACKGROUND_SET:
		druid_page_standard->_priv->background_set = g_value_get_boolean (value);
		break;
	case PROP_LOGO_BACKGROUND:
	case PROP_LOGO_BACKGROUND_GDK:
		if (prop_id == PROP_LOGO_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		mate_druid_page_standard_set_logo_background (druid_page_standard, &color);
		break;
	case PROP_LOGO_BACKGROUND_SET:
		druid_page_standard->_priv->logo_background_set = g_value_get_boolean (value);
		break;
	case PROP_CONTENTS_BACKGROUND:
	case PROP_CONTENTS_BACKGROUND_GDK:
		if (prop_id == PROP_CONTENTS_BACKGROUND_GDK)
			color = *((GdkColor *)g_value_get_boxed (value));
		else if (! gdk_color_parse (g_value_get_string (value), &color)) {
			g_warning ("Don't know color `%s'", g_value_get_string (value));
			break;
		}
		mate_druid_page_standard_set_contents_background (druid_page_standard, &color);
		break;
	case PROP_CONTENTS_BACKGROUND_SET:
		druid_page_standard->_priv->contents_background_set = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
mate_druid_page_standard_finalize (GObject *object)
{
	MateDruidPageStandard *druid_page_standard = MATE_DRUID_PAGE_STANDARD(object);

	g_free (druid_page_standard->title);
	druid_page_standard->title = NULL;

	g_free (druid_page_standard->_priv);
	druid_page_standard->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}


static void
mate_druid_page_standard_destroy (GtkObject *object)
{
	MateDruidPageStandard *druid_page_standard = MATE_DRUID_PAGE_STANDARD (object);

	/* remember, destroy can be run multiple times! */

	if (druid_page_standard->logo != NULL)
		g_object_unref (G_OBJECT (druid_page_standard->logo));
	druid_page_standard->logo = NULL;

	if (druid_page_standard->top_watermark != NULL)
		g_object_unref (G_OBJECT (druid_page_standard->top_watermark));
	druid_page_standard->top_watermark = NULL;

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_druid_page_standard_realize (GtkWidget *widget)
{
	MateDruidPageStandard *druid_page_standard = MATE_DRUID_PAGE_STANDARD (widget);

	MATE_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));
	
	mate_druid_page_standard_set_color (druid_page_standard);
}

static void
mate_druid_page_standard_style_set (GtkWidget *widget,
				     GtkStyle  *old_style)
{
	MateDruidPageStandard *druid_page_standard = MATE_DRUID_PAGE_STANDARD (widget);

	MATE_CALL_PARENT (GTK_WIDGET_CLASS, style_set, (widget, old_style));
	
	mate_druid_page_standard_set_color (druid_page_standard);
}

static void
mate_druid_page_standard_prepare (MateDruidPage *page,
				   GtkWidget *druid)
{
	mate_druid_set_buttons_sensitive (MATE_DRUID (druid), TRUE, TRUE, TRUE, TRUE);
	mate_druid_set_show_finish (MATE_DRUID (druid), FALSE);
	if (GTK_IS_WINDOW (gtk_widget_get_toplevel (druid)))
		gtk_widget_grab_default (MATE_DRUID (druid)->next);
}

/**
 * mate_druid_page_standard_new
 *
 * Construct a new #MateDruidPageStandard.
 *
 * Returns: A new #MateDruidPageStandard as a #GtkWidget pointer.
 */
GtkWidget *
mate_druid_page_standard_new (void)
{
	MateDruidPageStandard *retval;

	retval = g_object_new (MATE_TYPE_DRUID_PAGE_STANDARD, NULL);

	return GTK_WIDGET (retval);
}

/**
 * mate_druid_page_standard_new_with_vals:
 * @title: The title of the druid page.
 * @logo: The logo to put on the druid page.
 * @top_watermark: The watermark to put at the top of the druid page.
 *
 * Like mate_druid_page_standard_new(), but allows the caller to fill in some
 * of the values at the same time.
 *
 * Returns: A new #MateDruidPageStandard as a #GtkWidget pointer.
 */
GtkWidget *
mate_druid_page_standard_new_with_vals (const gchar *title,
					 GdkPixbuf   *logo,
					 GdkPixbuf   *top_watermark)
{
	MateDruidPageStandard *retval;

	retval = g_object_new (MATE_TYPE_DRUID_PAGE_STANDARD, NULL);

	mate_druid_page_standard_set_title (retval, title);
	mate_druid_page_standard_set_logo (retval, logo);
	mate_druid_page_standard_set_top_watermark (retval, top_watermark);
	return GTK_WIDGET (retval);
}



/**
 * mate_druid_page_standard_set_title:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @title: The string to use as the new title text.
 *
 * Description:  Sets the title to the value of @title.
 **/
void
mate_druid_page_standard_set_title (MateDruidPageStandard *druid_page_standard,
				     const gchar            *title)
{
	gchar *title_string;
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	g_free (druid_page_standard->title);
	druid_page_standard->title = g_strdup (title);
	title_string = g_strconcat ("<span size=\"xx-large\" weight=\"ultrabold\">",
				    title ? title : "", "</span>", NULL);
	gtk_label_set_label (GTK_LABEL (druid_page_standard->_priv->title_label), title_string);
	gtk_label_set_use_markup (GTK_LABEL (druid_page_standard->_priv->title_label), TRUE);
	g_free (title_string);
	g_object_notify (G_OBJECT (druid_page_standard), "title");
}


/**
 * mate_druid_page_standard_set_logo:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @logo_image: The #GdkPixbuf to use as a logo.
 *
 * Description:  Sets a #GdkPixbuf as the logo in the top right corner.
 * If %NULL, then no logo will be displayed.
 **/
void
mate_druid_page_standard_set_logo (MateDruidPageStandard *druid_page_standard,
				    GdkPixbuf              *logo_image)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (logo_image != NULL)
		g_object_ref (G_OBJECT (logo_image));
	if (druid_page_standard->logo)
		g_object_unref (G_OBJECT (druid_page_standard->logo));

	druid_page_standard->logo = logo_image;
	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_standard->_priv->logo), logo_image);
	g_object_notify (G_OBJECT (druid_page_standard), "logo");
}



/**
 * mate_druid_page_standard_set_top_watermark:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @top_watermark_image: The #GdkPixbuf to use as a top watermark.
 *
 * Description:  Sets a #GdkPixbuf as the watermark on top of the top
 * strip on the druid.  If #top_watermark_image is %NULL, it is reset
 * to the normal color.
 **/
void
mate_druid_page_standard_set_top_watermark (MateDruidPageStandard *druid_page_standard,
					     GdkPixbuf              *top_watermark_image)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (top_watermark_image != NULL)
		g_object_ref (G_OBJECT (top_watermark_image));
	if (druid_page_standard->top_watermark)
		g_object_unref (G_OBJECT (druid_page_standard->top_watermark));

	druid_page_standard->top_watermark = top_watermark_image;

	gtk_image_set_from_pixbuf (GTK_IMAGE (druid_page_standard->_priv->watermark), top_watermark_image);
	g_object_notify (G_OBJECT (druid_page_standard), "top_watermark");
}

/**
 * mate_druid_page_standard_set_title_foreground:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @color: The new color of the title text.
 *
 * Sets the title text to the specified color.
 */
void
mate_druid_page_standard_set_title_foreground (MateDruidPageStandard *druid_page_standard,
						GdkColor               *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->title_foreground = *color;

	gtk_widget_modify_fg (druid_page_standard->_priv->title_label, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (druid_page_standard), "title_foreground");
	if (druid_page_standard->_priv->title_foreground_set == FALSE) {
		druid_page_standard->_priv->title_foreground_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "title_foreground_set");
	}
}

/**
 * mate_druid_page_standard_set_background:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @color: The new background color.
 *
 * Sets the background color of the top section of the druid page to @color.
 */
void
mate_druid_page_standard_set_background (MateDruidPageStandard *druid_page_standard,
					  GdkColor               *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->background = *color;

	gtk_widget_modify_bg (druid_page_standard->_priv->background, GTK_STATE_NORMAL, color);
	g_object_notify (G_OBJECT (druid_page_standard), "background");
	if (druid_page_standard->_priv->background_set == FALSE) {
		druid_page_standard->_priv->background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "background_set");
	}
}

/**
 * mate_druid_page_standard_set_logo_background:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @color: The new color for the logo background.
 *
 * Sets the background of the logo to @color.
 */
void
mate_druid_page_standard_set_logo_background (MateDruidPageStandard *druid_page_standard,
					       GdkColor               *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->logo_background = *color;

	gtk_widget_modify_bg (druid_page_standard->_priv->logo, GTK_STATE_NORMAL, color);
	g_object_notify (G_OBJECT (druid_page_standard), "logo_background");
	if (druid_page_standard->_priv->logo_background_set == FALSE) {
		druid_page_standard->_priv->logo_background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "logo_background_set");
	}
}


/**
 * mate_druid_page_standard_set_contents_background:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @color: The new color for the main body's background.
 *
 * Sets the color of the main contents section's background to @color.
 */
void
mate_druid_page_standard_set_contents_background (MateDruidPageStandard *druid_page_standard,
					       GdkColor               *color)
{
	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->contents_background = *color;

	gtk_widget_modify_bg (druid_page_standard->_priv->evbox, GTK_STATE_NORMAL, color);

	g_object_notify (G_OBJECT (druid_page_standard), "contents_background");
	if (druid_page_standard->_priv->contents_background_set == FALSE) {
		druid_page_standard->_priv->contents_background_set = TRUE;
		g_object_notify (G_OBJECT (druid_page_standard), "contents_background_set");
	}
}


/**
 * mate_druid_page_standard_append_item:
 * @druid_page_standard: A #MateDruidPageStandard instance.
 * @question: The text to place above the item.
 * @item: The #GtkWidget to be included.
 * @additional_info: The text to be placed below the item in a smaller
 * font.
 *
 * Description: Convenience function to add a #GtkWidget to the
 * #MateDruidPageStandard vbox. This function creates a new contents section
 * that has the @question text followed by the @item widget and then the
 * @addition_info text, all stacked vertically from top to bottom. 
 *
 * The @item widget could be something like a set of radio checkbuttons
 * requesting a choice from the user.
 **/
void
mate_druid_page_standard_append_item (MateDruidPageStandard *druid_page_standard,
                                       const gchar            *question,
                                       GtkWidget              *item,
                                       const gchar            *additional_info)
{
	GtkWidget *q_label;
	GtkWidget *a_label;
	GtkWidget *vbox;
	gchar *a_text;

	g_return_if_fail (MATE_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (GTK_IS_WIDGET (item));

	/* Create the vbox to hold the three new items */
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (druid_page_standard->vbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show (vbox);

	/* Create the question label if question is not empty */
	if (question != NULL && strcmp (question, "") != 0) {
		q_label = gtk_label_new (NULL);
		gtk_label_set_label (GTK_LABEL (q_label),question);
		gtk_label_set_use_markup (GTK_LABEL (q_label), TRUE);
		gtk_label_set_use_underline (GTK_LABEL (q_label), TRUE);
		gtk_label_set_mnemonic_widget (GTK_LABEL (q_label), item);
		gtk_label_set_justify (GTK_LABEL (q_label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (q_label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (vbox), q_label, FALSE, FALSE, 0);
		gtk_widget_show (q_label);
	}

	/* Append/show the item */
	gtk_box_pack_start (GTK_BOX (vbox), item, FALSE, FALSE, 0);
	gtk_widget_show (item);

	/* Create the "additional info" label if additional_info is not empty */
	if (additional_info != NULL && additional_info[0] != '\000') {
		a_text = g_strconcat ("<span size=\"small\">", additional_info, "</span>", NULL);
		a_label = gtk_label_new (NULL);
		gtk_label_set_label (GTK_LABEL (a_label), a_text);
		gtk_label_set_use_markup (GTK_LABEL (a_label), TRUE);
		g_free (a_text);
		gtk_label_set_justify (GTK_LABEL (a_label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (a_label), 0.0, 0.5);
		gtk_misc_set_padding (GTK_MISC (a_label), 24, 0);
		gtk_box_pack_start (GTK_BOX (vbox), a_label, FALSE, FALSE, 0);
		gtk_widget_show (a_label);
	}
}

static void
mate_druid_page_standard_set_color (MateDruidPageStandard *druid_page_standard)
{
	GtkWidget *widget = GTK_WIDGET (druid_page_standard);
	if (druid_page_standard->_priv->background_set == FALSE) {
		druid_page_standard->background = widget->style->bg[GTK_STATE_SELECTED];
	}
	if (druid_page_standard->_priv->logo_background_set == FALSE) {
		druid_page_standard->logo_background = widget->style->bg[GTK_STATE_SELECTED];
	}
	if (druid_page_standard->_priv->title_foreground_set == FALSE) {
		druid_page_standard->title_foreground = widget->style->fg[GTK_STATE_SELECTED];
	}
	if (druid_page_standard->_priv->contents_background_set == FALSE) {
		druid_page_standard->contents_background = widget->style->bg[GTK_STATE_NORMAL];
	}

	gtk_widget_modify_fg (druid_page_standard->_priv->title_label, GTK_STATE_NORMAL, &(druid_page_standard->title_foreground));
	gtk_widget_modify_bg (druid_page_standard->_priv->background, GTK_STATE_NORMAL, &(druid_page_standard->background));
	gtk_widget_modify_bg (GTK_BIN (druid_page_standard->_priv->evbox)->child, GTK_STATE_NORMAL,&(druid_page_standard->contents_background));
}
