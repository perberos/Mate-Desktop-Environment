/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-item.c
 *
 * Author: Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent-ui-toolbar-item.h>

G_DEFINE_TYPE (MateComponentUIToolbarItem, matecomponent_ui_toolbar_item, GTK_TYPE_BIN)

struct _MateComponentUIToolbarItemPrivate {
	/* Whether this item wants to have a label when the toolbar style is
           `MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT'.  */
	gboolean want_label;

	/* Whether this item wants to be expanded to all the available
           width/height.  */
	gboolean expandable;

	/* if set, pack this item on the right side of the toolbar */
	gboolean pack_end;

	/* Orientation for this item.  */
	GtkOrientation orientation;

	/* Style for this item.  */
	MateComponentUIToolbarItemStyle style;	
	
	/* minimum width (or height, if rotated) for this item */
	int minimum_width;
};

enum {
	SET_ORIENTATION,
	SET_STYLE,
	SET_WANT_LABEL,
	ACTIVATE,
	STATE_ALTERED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

/* GObject methods.  */

static void
impl_finalize (GObject *object)
{
	MateComponentUIToolbarItem *toolbar_item = (MateComponentUIToolbarItem *) object;

	g_free (toolbar_item->priv);

	G_OBJECT_CLASS (matecomponent_ui_toolbar_item_parent_class)->finalize (object);
}

/* GtkWidget methods.  */

static void
impl_size_request (GtkWidget      *widget,
		   GtkRequisition *requisition_return)
{
	MateComponentUIToolbarItem *toolbar_item;
	MateComponentUIToolbarItemPrivate *priv;
	GtkRequisition child_requisition;
	GtkWidget *child;
	int border_width;

	toolbar_item = MATECOMPONENT_UI_TOOLBAR_ITEM (widget);
	priv = toolbar_item->priv;

	border_width = GTK_CONTAINER (widget)->border_width;
	requisition_return->width  = border_width;
	requisition_return->height = border_width;

	child = GTK_BIN (widget)->child;
	if (child == NULL)
		return;

	gtk_widget_size_request (child, &child_requisition);

	if (child_requisition.width < priv->minimum_width)
		child_requisition.width = priv->minimum_width;
	
	requisition_return->width  += child_requisition.width;
	requisition_return->height += child_requisition.height;
}

static void
impl_size_allocate (GtkWidget     *widget,
		    GtkAllocation *allocation)
{
	GtkAllocation child_allocation;
	GtkWidget *child;
	int border_width;

	widget->allocation = *allocation;

	child = GTK_BIN (widget)->child;
	if (child == NULL || !GTK_WIDGET_VISIBLE (child))
		return;

	border_width = GTK_CONTAINER (widget)->border_width;

	if (allocation->width > border_width) {
		child_allocation.x = allocation->x + border_width;
		child_allocation.width = allocation->width - border_width;
	} else {
		child_allocation.x = allocation->x;
		child_allocation.width = allocation->width;
	}

	if (allocation->height > border_width) {
		child_allocation.y = allocation->y + border_width;
		child_allocation.height = allocation->height - border_width;
	} else {
		child_allocation.y = allocation->y;
		child_allocation.height = allocation->height;
	}

	gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_allocation);
}

/* MateComponentUIToolbarItem signals.  */

static void
impl_set_orientation (MateComponentUIToolbarItem *item,
		      GtkOrientation       orientation)
{
	MateComponentUIToolbarItemPrivate *priv;

	priv = item->priv;

	if (priv->orientation == orientation)
		return;

	priv->orientation = orientation;

	gtk_widget_queue_resize (GTK_WIDGET (item));
}

static void
impl_set_style (MateComponentUIToolbarItem *item,
		MateComponentUIToolbarItemStyle style)
{
	gtk_widget_queue_resize (GTK_WIDGET (item));
}

static void
impl_set_want_label (MateComponentUIToolbarItem *item,
		     gboolean             want_label)
{
	MateComponentUIToolbarItemPrivate *priv;

	priv = item->priv;

	priv->want_label = want_label;
}

/* Gtk+ object initialization.  */

static void
matecomponent_ui_toolbar_item_class_init (MateComponentUIToolbarItemClass *object_class)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	MateComponentUIToolbarItemClass *toolbar_item_class;

	gobject_class = (GObjectClass *) object_class;
	gobject_class->finalize = impl_finalize;

	widget_class = GTK_WIDGET_CLASS (object_class);
	widget_class->size_request  = impl_size_request;
	widget_class->size_allocate = impl_size_allocate;

	toolbar_item_class = MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS (object_class);
	toolbar_item_class->set_orientation = impl_set_orientation;
	toolbar_item_class->set_style       = impl_set_style;
	toolbar_item_class->set_want_label  = impl_set_want_label;

	signals[SET_ORIENTATION]
		= g_signal_new ("set_orientation",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarItemClass,
						 set_orientation),
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1,
				G_TYPE_INT);

	signals[SET_STYLE]
		= g_signal_new ("set_style",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarItemClass,
						 set_style),
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1,
				G_TYPE_INT);

	signals[SET_WANT_LABEL] =
		g_signal_new ("set_want_label",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateComponentUIToolbarItemClass,
					       set_want_label),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);
	
	signals[STATE_ALTERED]
		= g_signal_new ("state_altered",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarItemClass,
						 activate),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);
	
	signals[ACTIVATE]
		= g_signal_new ("activate",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarItemClass,
						 activate),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
}

static void
matecomponent_ui_toolbar_item_init (MateComponentUIToolbarItem *toolbar_item)
{
	MateComponentUIToolbarItemPrivate *priv;

	priv = g_new (MateComponentUIToolbarItemPrivate, 1);

	priv->want_label    = FALSE;
	priv->orientation   = GTK_ORIENTATION_HORIZONTAL;
	priv->style         = MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL;
	priv->expandable    = FALSE;
	priv->pack_end	    = FALSE;
	priv->minimum_width = 0;
	
	toolbar_item->priv = priv;
}

GtkWidget *
matecomponent_ui_toolbar_item_new (void)
{
	MateComponentUIToolbarItem *new;

	new = g_object_new (matecomponent_ui_toolbar_item_get_type (), NULL);

	return GTK_WIDGET (new);
}


void
matecomponent_ui_toolbar_item_set_tooltip (MateComponentUIToolbarItem *item,
				    GtkTooltips         *tooltips,
				    const char          *tooltip)
{
	MateComponentUIToolbarItemClass *klass;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	klass = MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS (GTK_OBJECT_GET_CLASS (item));

	if (klass->set_tooltip)
		klass->set_tooltip (item, tooltips, tooltip);

	/* FIXME: implement setting of tooltips */
}

void
matecomponent_ui_toolbar_item_set_state (MateComponentUIToolbarItem *item,
				  const char          *new_state)
{
	MateComponentUIToolbarItemClass *klass;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	klass = MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS (GTK_OBJECT_GET_CLASS (item));

	if (klass->set_state)
		klass->set_state (item, new_state);
}

void
matecomponent_ui_toolbar_item_set_orientation (MateComponentUIToolbarItem *item,
					GtkOrientation orientation)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));
	g_return_if_fail (orientation == GTK_ORIENTATION_HORIZONTAL ||
			  orientation == GTK_ORIENTATION_VERTICAL);

	g_signal_emit (item, signals[SET_ORIENTATION], 0, orientation);
}

GtkOrientation
matecomponent_ui_toolbar_item_get_orientation (MateComponentUIToolbarItem *item)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item), GTK_ORIENTATION_HORIZONTAL);

	priv = item->priv;

	return priv->orientation;
}


void
matecomponent_ui_toolbar_item_set_style (MateComponentUIToolbarItem *item,
				  MateComponentUIToolbarItemStyle style)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));
	g_return_if_fail (style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY ||
			  style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY ||
			  style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL ||
			  style == MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);

	priv = item->priv;

	if (priv->style == style)
		return;

	priv->style = style;

	g_signal_emit (item, signals[SET_STYLE], 0, style);
}

MateComponentUIToolbarItemStyle
matecomponent_ui_toolbar_item_get_style (MateComponentUIToolbarItem *item)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_val_if_fail (item != NULL,
			      MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item),
			      MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);

	priv = item->priv;

	return priv->style;
}

void
matecomponent_ui_toolbar_item_set_want_label (MateComponentUIToolbarItem *item,
				    gboolean want_label)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	g_signal_emit (item, signals[SET_WANT_LABEL], 0, want_label);
}

gboolean
matecomponent_ui_toolbar_item_get_want_label (MateComponentUIToolbarItem *item)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item), FALSE);

	priv = item->priv;

	return priv->want_label;
}

void
matecomponent_ui_toolbar_item_set_expandable (MateComponentUIToolbarItem *item,
				       gboolean expandable)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	priv = item->priv;

	if ((priv->expandable && expandable) || (! priv->expandable && ! expandable))
		return;

	priv->expandable = expandable;
	gtk_widget_queue_resize (GTK_WIDGET (item));
}

gboolean
matecomponent_ui_toolbar_item_get_expandable (MateComponentUIToolbarItem *item)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item), FALSE);

	priv = item->priv;
	return priv->expandable;
}

void
matecomponent_ui_toolbar_item_set_pack_end (MateComponentUIToolbarItem *item,
				     gboolean pack_end)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	priv = item->priv;

	if ((priv->pack_end && pack_end) || (! priv->pack_end && ! pack_end))
		return;

	priv->pack_end = pack_end;
	gtk_widget_queue_resize (GTK_WIDGET (item));
}

gboolean
matecomponent_ui_toolbar_item_get_pack_end (MateComponentUIToolbarItem *item)
{
	MateComponentUIToolbarItemPrivate *priv;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item), FALSE);

	priv = item->priv;
	return priv->pack_end;
}

void
matecomponent_ui_toolbar_item_activate (MateComponentUIToolbarItem *item)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	g_signal_emit (item, signals[ACTIVATE], 0);
}

void
matecomponent_ui_toolbar_item_set_minimum_width (MateComponentUIToolbarItem *item, int minimum_width)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	item->priv->minimum_width = minimum_width;
}
