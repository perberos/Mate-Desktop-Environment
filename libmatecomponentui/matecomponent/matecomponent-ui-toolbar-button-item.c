/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-button-item.h: a toolbar button
 *
 * Author: Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <matecomponent-ui-private.h>
#include <matecomponent-ui-toolbar-button-item.h>

G_DEFINE_TYPE (MateComponentUIToolbarButtonItem,
	       matecomponent_ui_toolbar_button_item,
	       MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM)

/* Spacing between the icon and the label.  */
#define SPACING 2

struct _MateComponentUIToolbarButtonItemPrivate {
	/* The icon for the button.  */
	GtkWidget *icon;

	/* The label for the button.  */
	GtkWidget *label;

	/* The box that packs the icon and the label.  It can either be an hbox
           or a vbox, depending on the style.  */
	GtkWidget *box;

	/* The widget containing the button */
	GtkButton *button_widget;
};

enum {
	CLICKED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };


/* Utility functions.  */

static void
set_image (MateComponentUIToolbarButtonItem *button_item,
	   gpointer                   image)
{
	gboolean is_pixbuf;
	gboolean is_gtk_image;
	MateComponentUIToolbarButtonItemPrivate *priv;

	priv = button_item->priv;

	is_pixbuf    = image && GDK_IS_PIXBUF (image);
	is_gtk_image = priv->icon && GTK_IS_IMAGE (priv->icon);

	if (is_pixbuf && is_gtk_image)
		matecomponent_ui_image_set_pixbuf ((GtkImage *) priv->icon, image);

	else {
		if (priv->icon)
			gtk_widget_destroy (priv->icon);

		if (is_pixbuf)
			priv->icon = gtk_image_new_from_pixbuf (image);
		else {
			g_return_if_fail (!image || GTK_IS_WIDGET (image));
			priv->icon = image;
		}
	}
}

static void
set_label (MateComponentUIToolbarButtonItem *button_item,
	   const char *label)
{
	GtkLabel *l_widget;
	MateComponentUIToolbarButtonItemPrivate *priv;

	priv = button_item->priv;

	if (!priv->label) {
		if (!label)
			return;
		else
			priv->label = gtk_label_new (label);
	}

	if (!label) {
		gtk_widget_destroy (priv->label);
		priv->label = NULL;
		return;
	}

	l_widget = GTK_LABEL (priv->label);

	if (label && l_widget->label &&
	    !strcmp (label, l_widget->label))
		return;
	else
		gtk_label_set_text (l_widget, label);
}


/* Layout.  */

static void
unparent_items (MateComponentUIToolbarButtonItem *button_item)
{
	MateComponentUIToolbarButtonItemPrivate *priv;

	priv = button_item->priv;

	if (priv->icon != NULL) {
		if (priv->icon->parent != NULL) {
			g_object_ref (priv->icon);
			gtk_container_remove (GTK_CONTAINER (priv->icon->parent),
					      priv->icon);
		}
	}

	if (priv->label != NULL) {
		if (priv->label->parent != NULL) {
			g_object_ref (priv->label);
			gtk_container_remove (GTK_CONTAINER (priv->label->parent),
					      priv->label);
		}
	}
}

static void
layout_pixmap_and_label (MateComponentUIToolbarButtonItem *button_item,
			 MateComponentUIToolbarItemStyle   style)
{
	MateComponentUIToolbarButtonItemPrivate *priv;
	GtkWidget *button;
	gboolean   rebuild;

	priv = button_item->priv;
	button = GTK_BIN (button_item)->child;

	/* Ensure we have the right type of box */
	rebuild = FALSE;
	if (style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL) {
		if (!priv->box || !g_type_is_a (
			G_TYPE_FROM_INSTANCE (priv->box),
			GTK_TYPE_VBOX)) {
			
			unparent_items (button_item);

			if (priv->box)
				gtk_widget_destroy (priv->box);
			priv->box = gtk_vbox_new (FALSE, SPACING);
			rebuild = TRUE;
		}
	} else {
		if (!priv->box ||
		    !g_type_is_a (G_TYPE_FROM_INSTANCE (priv->box),
				  GTK_TYPE_HBOX)) {
			
			unparent_items (button_item);

			if (priv->box)
				gtk_widget_destroy (priv->box);
			priv->box = gtk_hbox_new (FALSE, SPACING);
			rebuild = TRUE;
		}
	}

	if (rebuild) {
		gtk_container_add (GTK_CONTAINER (button), priv->box);
		gtk_widget_show (priv->box);
	}

	if (priv->icon && !priv->icon->parent)
		gtk_box_pack_start (
			GTK_BOX (priv->box), priv->icon, TRUE, TRUE, 0);

	if (priv->label && !priv->label->parent)
		gtk_box_pack_end (
			GTK_BOX (priv->box), priv->label, FALSE, TRUE, 0);

	if (priv->icon) {
		if (style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY)
			gtk_widget_hide (priv->icon);
		else
			gtk_widget_show (priv->icon);
	}

	if (priv->label) {
		if (style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY)
			gtk_widget_hide (priv->label);
		else
			gtk_widget_show (priv->label);
	}
}


/* Callback for the GtkButton.  */

static void
button_widget_clicked_cb (GtkButton *button,
			  void *data)
{
	MateComponentUIToolbarButtonItem *button_item;

	button_item = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (data);

	g_signal_emit (button_item, signals[CLICKED], 0);

	matecomponent_ui_toolbar_item_activate (MATECOMPONENT_UI_TOOLBAR_ITEM (button_item));
}

/* GObject methods.  */

static void
impl_finalize (GObject *object)
{
	MateComponentUIToolbarButtonItem *button_item;

	button_item = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (object);

	g_free (button_item->priv);
	button_item->priv = NULL;

	G_OBJECT_CLASS (matecomponent_ui_toolbar_button_item_parent_class)->finalize (object);
}

/* MateComponentUIToolbarItem signals.  */

static void
impl_set_style (MateComponentUIToolbarItem     *item,
		MateComponentUIToolbarItemStyle style)
{
	MateComponentUIToolbarButtonItem *button_item;

	button_item = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (item);

	layout_pixmap_and_label (button_item, style);

	MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS (matecomponent_ui_toolbar_button_item_parent_class)->set_style
		(item, style);
}

static void
impl_set_tooltip (MateComponentUIToolbarItem *item,
		  GtkTooltips         *tooltips,
		  const char          *tooltip)
{
	GtkButton *button;
	MateComponentUIToolbarButtonItem *button_item;

	button_item = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM (item);

	if (tooltip && (button = button_item->priv->button_widget))
		gtk_tooltips_set_tip (
			tooltips, GTK_WIDGET (button), tooltip, NULL);
}

/* MateComponentUIToolbarButtonItem virtual methods.  */

static void
impl_set_icon  (MateComponentUIToolbarButtonItem *button_item,
		gpointer                   image)
{
	set_image (button_item, image);

	layout_pixmap_and_label (
		button_item,
		matecomponent_ui_toolbar_item_get_style (
			MATECOMPONENT_UI_TOOLBAR_ITEM (button_item)));
}

static void
impl_set_label (MateComponentUIToolbarButtonItem *button_item,
		const char                *label)
{
	set_label (button_item, label);

	layout_pixmap_and_label (
		button_item,
		matecomponent_ui_toolbar_item_get_style (
			MATECOMPONENT_UI_TOOLBAR_ITEM (button_item)));
}


/* GTK+ object initialization.  */

static void
matecomponent_ui_toolbar_button_item_class_init (MateComponentUIToolbarButtonItemClass *button_item_class)
{
	GObjectClass *object_class;
	MateComponentUIToolbarItemClass *item_class;

	object_class = (GObjectClass *) button_item_class;
	object_class->finalize = impl_finalize;

	item_class = MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS (button_item_class);
	item_class->set_style = impl_set_style;
	item_class->set_tooltip = impl_set_tooltip;

	button_item_class->set_icon  = impl_set_icon;
	button_item_class->set_label = impl_set_label;

	signals[CLICKED] = 
		g_signal_new ("clicked",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateComponentUIToolbarButtonItemClass,
					       clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
matecomponent_ui_toolbar_button_item_init (
	MateComponentUIToolbarButtonItem *toolbar_button_item)
{
	MateComponentUIToolbarButtonItemPrivate *priv;

	priv = g_new (MateComponentUIToolbarButtonItemPrivate, 1);
	priv->icon  = NULL;
	priv->label = NULL;
	priv->box   = NULL;

	toolbar_button_item->priv = priv;
}

void
matecomponent_ui_toolbar_button_item_construct (MateComponentUIToolbarButtonItem *button_item,
					 GtkButton *button_widget,
					 GdkPixbuf *pixbuf,
					 const char *label)
{
	MateComponentUIToolbarButtonItemPrivate *priv;

	g_return_if_fail (button_item != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));
	g_return_if_fail (button_widget != NULL);
	g_return_if_fail (GTK_IS_BUTTON (button_widget));
	g_return_if_fail (GTK_BIN (button_item)->child == NULL);

	priv = button_item->priv;
	g_assert (priv->icon == NULL);
	g_assert (priv->label == NULL);

	priv->button_widget = button_widget;
	gtk_widget_show (GTK_WIDGET (button_widget));

	g_signal_connect_object (button_widget, "clicked",
				 G_CALLBACK (button_widget_clicked_cb),
				 button_item, 0);

	gtk_button_set_relief (button_widget, GTK_RELIEF_NONE);

	gtk_container_add (GTK_CONTAINER (button_item), GTK_WIDGET (button_widget));

	set_image (button_item, pixbuf);
	set_label (button_item, label);

	layout_pixmap_and_label (
		button_item, matecomponent_ui_toolbar_item_get_style (
			MATECOMPONENT_UI_TOOLBAR_ITEM (button_item)));
}

/**
 * matecomponent_ui_toolbar_button_item_new:
 * @pixmap: 
 * @label: 
 * 
 * Create a new toolbar button item.
 * 
 * Return value: A pointer to the newly created widget.
 **/
GtkWidget *
matecomponent_ui_toolbar_button_item_new (GdkPixbuf  *icon,
				   const char *label)
{
	MateComponentUIToolbarButtonItem *button_item;
	GtkWidget *button_widget;

	button_item = g_object_new (
		matecomponent_ui_toolbar_button_item_get_type (), NULL);

	button_widget = gtk_button_new ();
	matecomponent_ui_toolbar_button_item_construct (
		button_item, GTK_BUTTON (button_widget), icon, label);

	return GTK_WIDGET (button_item);
}


void
matecomponent_ui_toolbar_button_item_set_image (MateComponentUIToolbarButtonItem *button_item,
					 gpointer                   image)
{
	MateComponentUIToolbarButtonItemClass *klass;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));

	klass = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_CLASS (
		(GTK_OBJECT_GET_CLASS (button_item)));

	if (klass->set_icon)
		klass->set_icon (button_item, image);
}

void
matecomponent_ui_toolbar_button_item_set_label (MateComponentUIToolbarButtonItem *button_item,
					 const char                *label)
{
	MateComponentUIToolbarButtonItemClass *klass;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM (button_item));

	klass = MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_CLASS (
		(GTK_OBJECT_GET_CLASS (button_item)));

	if (klass->set_label)
		klass->set_label (button_item, label);
}


GtkButton *
matecomponent_ui_toolbar_button_item_get_button_widget (MateComponentUIToolbarButtonItem *button_item)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM (button_item), NULL);

	return GTK_BUTTON (GTK_BIN (button_item)->child);
}

GtkWidget *
matecomponent_ui_toolbar_button_item_get_image (MateComponentUIToolbarButtonItem *button_item)
{
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM (button_item), NULL);

	return button_item->priv->icon;
}
