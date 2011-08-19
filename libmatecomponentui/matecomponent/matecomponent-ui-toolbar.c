/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar.h
 *
 * Author:
 *    Ettore Perazzoli (ettore@ximian.com)
 *
 * Copyright (C) 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-toolbar.h>
#include <matecomponent/matecomponent-ui-toolbar-item.h>
#include <matecomponent/matecomponent-ui-toolbar-popup-item.h>

G_DEFINE_TYPE (MateComponentUIToolbar, matecomponent_ui_toolbar, GTK_TYPE_CONTAINER)

enum {
	PROP_0,
	PROP_ORIENTATION,
	PROP_IS_FLOATING,
	PROP_PREFERRED_WIDTH,
	PROP_PREFERRED_HEIGHT
};

struct _MateComponentUIToolbarPrivate {
	/* The orientation of this toolbar.  */
	GtkOrientation orientation;

	/* Is the toolbar currently floating */
	gboolean is_floating;

	/* The style of this toolbar.  */
	MateComponentUIToolbarStyle style;

	/* Styles to use in different orientations */
	MateComponentUIToolbarStyle hstyle;
	MateComponentUIToolbarStyle vstyle;

	/* Sizes of the toolbar.  This is actually the height for
           horizontal toolbars and the width for vertical toolbars.  */
	int max_width, max_height;
	int total_width, total_height;

	/* position of left edge of left-most pack-end item */
	int end_position;

	/* List of all the items in the toolbar.  Both the ones that have been
           unparented because they don't fit, and the ones that are visible.
           The MateComponentUIToolbarPopupItem is not here though.  */
	GList *items;

	/* Pointer to the first element in the `items' list that doesn't fit in
           the available space.  This is updated at size_allocate.  */
	GList *first_not_fitting_item;

	/* The pop-up button.  When clicked, it pops up a window with all the
           items that don't fit.  */
	MateComponentUIToolbarItem *popup_item;

	/* The window we pop-up when the pop-up item is clicked.  */
	GtkWidget *popup_window;

	/* The vbox within the pop-up window.  */
	GtkWidget *popup_window_vbox;

	/* Whether we have moved items to the pop-up window.  This is to
           prevent the size_allocation code to incorrectly hide the pop-up
           button in that case.  */
	gboolean items_moved_to_popup_window;

	GtkTooltips *tooltips;
};

enum {
	SET_ORIENTATION,
	STYLE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Width of the pop-up window.  */

#define POPUP_WINDOW_WIDTH 200

/* Utility functions.  */

static void
parentize_widget (MateComponentUIToolbar *toolbar,
		  GtkWidget       *widget)
{
	g_assert (widget->parent == NULL);

	/* The following is done according to the Bible, widget_system.txt, IV, 1.  */

	gtk_widget_set_parent (widget, GTK_WIDGET (toolbar));
}

static void
set_attributes_on_child (MateComponentUIToolbarItem *item,
			 GtkOrientation orientation,
			 MateComponentUIToolbarStyle style)
{
	matecomponent_ui_toolbar_item_set_orientation (item, orientation);

	switch (style) {
	case MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT:
		if (! matecomponent_ui_toolbar_item_get_want_label (item))
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		else if (orientation == GTK_ORIENTATION_HORIZONTAL)
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;

	case MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_AND_TEXT:
		if (orientation == GTK_ORIENTATION_VERTICAL)
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL);
		else
			matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL);
		break;
	case MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_ONLY:
		matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY);
		break;
	case MATECOMPONENT_UI_TOOLBAR_STYLE_TEXT_ONLY:
		matecomponent_ui_toolbar_item_set_style (item, MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY);
		break;
	default:
		g_assert_not_reached ();
	}
}

/* Callbacks to do widget housekeeping.  */

static void
item_destroy_cb (GtkObject *object,
		 void *data)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;

	toolbar = MATECOMPONENT_UI_TOOLBAR (data);
	priv = toolbar->priv;

	priv->items = g_list_remove (priv->items, object);
	g_object_unref (object);
}

static void
item_activate_cb (MateComponentUIToolbarItem *item,
		  void *data)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;

	toolbar = MATECOMPONENT_UI_TOOLBAR (data);
	priv = toolbar->priv;

	matecomponent_ui_toolbar_toggle_button_item_set_active (
		MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (priv->popup_item), FALSE);
}

static void
item_set_want_label_cb (MateComponentUIToolbarItem *item,
			gboolean want_label,
			void *data)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;

	toolbar = MATECOMPONENT_UI_TOOLBAR (data);
	priv = toolbar->priv;

	set_attributes_on_child (item, priv->orientation, priv->style);

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

/* The pop-up window foo.  */

/* Return TRUE if there are actually any items in the pop-up menu.  */
static void
create_popup_window (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GtkWidget *hbox;
	GList *p;
	int row_width;

	priv = toolbar->priv;

	row_width = 0;
	hbox = NULL;

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next) {
		GtkRequisition item_requisition;
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);

		if (! GTK_WIDGET_VISIBLE (item_widget) ||
			matecomponent_ui_toolbar_item_get_pack_end (MATECOMPONENT_UI_TOOLBAR_ITEM (item_widget)))

			continue;

		if (item_widget->parent != NULL)
			gtk_container_remove (GTK_CONTAINER (item_widget->parent), item_widget);

		gtk_widget_get_child_requisition (item_widget, &item_requisition);

		set_attributes_on_child (MATECOMPONENT_UI_TOOLBAR_ITEM (item_widget),
					 GTK_ORIENTATION_HORIZONTAL,
					 priv->style);

		if (hbox == NULL
		    || (row_width > 0 && item_requisition.width + row_width > POPUP_WINDOW_WIDTH)) {
			hbox = gtk_hbox_new (FALSE, 0);
			gtk_box_pack_start (GTK_BOX (priv->popup_window_vbox), hbox, FALSE, TRUE, 0);
			gtk_widget_show (hbox);
			row_width = 0;
		}

		gtk_box_pack_start (GTK_BOX (hbox), item_widget, FALSE, TRUE, 0);

		row_width += item_requisition.width;
	}
}

static void
hide_popup_window (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	gdk_display_pointer_ungrab
		(gtk_widget_get_display (priv->popup_window),
		 GDK_CURRENT_TIME);

	gtk_grab_remove (priv->popup_window);
	gtk_widget_hide (priv->popup_window);

	priv->items_moved_to_popup_window = FALSE;

	/* Reset the attributes on all the widgets that were moved to the
           window and move them back to the toolbar.  */
	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar)) {
			set_attributes_on_child (MATECOMPONENT_UI_TOOLBAR_ITEM (item_widget),
						 priv->orientation, priv->style);
			gtk_container_remove (GTK_CONTAINER (item_widget->parent), item_widget);
			parentize_widget (toolbar, item_widget);
		}
	}

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
popup_window_button_release_cb (GtkWidget *widget,
				GdkEventButton *event,
				void *data)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;

	toolbar = MATECOMPONENT_UI_TOOLBAR (data);
	priv = toolbar->priv;

	matecomponent_ui_toolbar_toggle_button_item_set_active
		(MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM (priv->popup_item), FALSE);
}

static void
popup_window_map_cb (GtkWidget *widget,
		     void *data)
{

	if (gdk_pointer_grab (widget->window, TRUE,
			      (GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK
			       | GDK_ENTER_NOTIFY_MASK
			       | GDK_LEAVE_NOTIFY_MASK
			       | GDK_POINTER_MOTION_MASK),
			      NULL, NULL, GDK_CURRENT_TIME) != 0) {
		g_warning ("Toolbar pop-up pointer grab failed.");
		return;
	}

	gtk_grab_add (widget);
}

static void
show_popup_window (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	const GtkAllocation *toolbar_allocation;
	gint x, y;
	GdkScreen *screen;
	gint screen_width, screen_height;
	gint window_width, window_height;

	priv = toolbar->priv;

	priv->items_moved_to_popup_window = TRUE;

	create_popup_window (toolbar);

	gdk_window_get_origin (GTK_WIDGET (toolbar)->window, &x, &y);
	toolbar_allocation = & GTK_WIDGET (toolbar)->allocation;
	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		x += toolbar_allocation->x + toolbar_allocation->width;
	else
		y += toolbar_allocation->y + toolbar_allocation->height;

	gtk_window_get_size (GTK_WINDOW (priv->popup_window),
			     &window_width, &window_height);
	screen = gtk_widget_get_screen (GTK_WIDGET (toolbar));
	screen_width = gdk_screen_get_width (screen);
	screen_height = gdk_screen_get_height (screen);
	if ((x + window_width) > screen_width)
		x -= window_width;
	if ((y + window_height) > screen_height)
		x += toolbar_allocation->width;

	gtk_window_move (GTK_WINDOW (priv->popup_window), x, y);

	g_signal_connect (priv->popup_window, "map",
			  G_CALLBACK (popup_window_map_cb), toolbar);

	gtk_widget_show (priv->popup_window);
}

static void
popup_item_toggled_cb (MateComponentUIToolbarToggleButtonItem *toggle_button_item,
		       void *data)
{
	MateComponentUIToolbar *toolbar;
	gboolean active;

	toolbar = MATECOMPONENT_UI_TOOLBAR (data);

	active = matecomponent_ui_toolbar_toggle_button_item_get_active (toggle_button_item);

	if (active)
		show_popup_window (toolbar);
	else 
		hide_popup_window (toolbar);
}

/* Layout handling.  */

static int
get_popup_item_size (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GtkRequisition requisition;

	priv = toolbar->priv;

	gtk_widget_get_child_requisition (
		GTK_WIDGET (priv->popup_item), &requisition);

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		return requisition.width;
	else
		return requisition.height;
}

/* Update the various sizes.  This is performed during ::size_request.  */

static void
accumulate_item_size (MateComponentUIToolbarPrivate *priv,
		      GtkWidget              *item_widget)
{
	GtkRequisition item_requisition;

	gtk_widget_size_request (item_widget, &item_requisition);

	priv->max_width     = MAX (priv->max_width,  item_requisition.width);
	priv->total_width  += item_requisition.width;
	priv->max_height    = MAX (priv->max_height, item_requisition.height);
	priv->total_height += item_requisition.height;
}

static void
update_sizes (MateComponentUIToolbar *toolbar)
{
	GList *p;
	MateComponentUIToolbarPrivate *priv;

	priv = toolbar->priv;

	priv->max_width = priv->total_width = 0;
	priv->max_height = priv->total_height = 0;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (! GTK_WIDGET_VISIBLE (item_widget) ||
		    item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		accumulate_item_size (priv, item_widget);
	}

	if (priv->items_moved_to_popup_window)
		accumulate_item_size (priv, GTK_WIDGET (priv->popup_item));
}

static void
allocate_popup_item (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GtkRequisition popup_item_requisition;
	GtkAllocation popup_item_allocation;
	GtkAllocation *toolbar_allocation;
	int border_width;

	priv = toolbar->priv;

	/* FIXME what if there is not enough space?  */

	toolbar_allocation = & GTK_WIDGET (toolbar)->allocation;

	border_width = GTK_CONTAINER (toolbar)->border_width;

	gtk_widget_get_child_requisition (
		GTK_WIDGET (priv->popup_item), &popup_item_requisition);

	popup_item_allocation.x = toolbar_allocation->x;
	popup_item_allocation.y = toolbar_allocation->y;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
		popup_item_allocation.x      = priv->end_position - popup_item_requisition.width - border_width;
		popup_item_allocation.y      += border_width;
		popup_item_allocation.width  = popup_item_requisition.width;
		popup_item_allocation.height = toolbar_allocation->height - 2 * border_width;
	} else {
		popup_item_allocation.x      += border_width;
		popup_item_allocation.y      = priv->end_position - popup_item_requisition.height - border_width;
		popup_item_allocation.width  = toolbar_allocation->width - 2 * border_width;
		popup_item_allocation.height = popup_item_requisition.height;
	}

	gtk_widget_size_allocate (GTK_WIDGET (priv->popup_item), &popup_item_allocation);
}

static void
setup_popup_item (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	if (priv->items_moved_to_popup_window) {
		gtk_widget_show (GTK_WIDGET (priv->popup_item));
		allocate_popup_item (toolbar);
		return;
	}

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);

		if (GTK_WIDGET_VISIBLE (item_widget)) {
			gtk_widget_show (GTK_WIDGET (priv->popup_item));
			allocate_popup_item (toolbar);
			return;
		}
	}

	gtk_widget_hide (GTK_WIDGET (priv->popup_item));
}

/*
 * This is a dirty hack.  We cannot hide the items with gtk_widget_hide ()
 * because we want to let the user be in control of the physical hidden/shown
 * state, so we just move the widget to a non-visible area.
 */
static void
hide_not_fitting_items (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GtkAllocation child_allocation;
	GList *p;

	priv = toolbar->priv;

	child_allocation.x      = 40000;
	child_allocation.y      = 40000;
	child_allocation.width  = 1;
	child_allocation.height = 1;

	for (p = priv->first_not_fitting_item; p != NULL; p = p->next) {
		if (matecomponent_ui_toolbar_item_get_pack_end (MATECOMPONENT_UI_TOOLBAR_ITEM (p->data)))
			continue;
		gtk_widget_size_allocate (GTK_WIDGET (p->data), &child_allocation);
	}
}

static void
size_allocate_helper (MateComponentUIToolbar *toolbar,
		      const GtkAllocation *allocation)
{
	MateComponentUIToolbarPrivate *priv;
	GtkAllocation child_allocation;
	MateComponentUIToolbarItem *item;
	GtkRequisition child_requisition;
	int border_width;
	int space_required;
	int available_space;
	int extra_space;
	int num_expandable_items;
	int popup_item_size;
	int item_size_left_to_place;
	int acc_space;
	gboolean first_expandable;
	GList *p;

	GTK_WIDGET (toolbar)->allocation = *allocation;

	priv = toolbar->priv;

	border_width = GTK_CONTAINER (toolbar)->border_width;
	popup_item_size = get_popup_item_size (toolbar);

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		available_space = MAX ((int) allocation->width - 2 * border_width, popup_item_size);
	else
		available_space = MAX ((int) allocation->height - 2 * border_width, popup_item_size);

	child_allocation.x = allocation->x + border_width;
	child_allocation.y = allocation->y + border_width;

	/* 
	 * if there is exactly one toolbar item, handle it specially, by giving it all of the available space,
	 * even if it doesn't fit, since we never want everything in the pop-up.
	 */	 
	if (priv->items != NULL && priv->items->next == NULL) {
		item = MATECOMPONENT_UI_TOOLBAR_ITEM (priv->items->data);		
		gtk_widget_get_child_requisition (GTK_WIDGET (item), &child_requisition);
		child_allocation.width = child_requisition.width;
		child_allocation.height = child_requisition.height;

		if (matecomponent_ui_toolbar_item_get_expandable (item)) {
			if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
				child_allocation.width = available_space;
			else
				child_allocation.height = available_space;
		
		} 
		gtk_widget_size_allocate (GTK_WIDGET (item), &child_allocation);		
		
		return;
	}

	/* first, make a pass through the items to layout the ones that are packed on the right */
	priv->end_position = allocation->x + available_space;
	acc_space = 0;
	for (p = g_list_last (priv->items); p != NULL; p = p->prev) {

		item = MATECOMPONENT_UI_TOOLBAR_ITEM (p->data);
		if (! matecomponent_ui_toolbar_item_get_pack_end (item))
			continue;

		gtk_widget_get_child_requisition (GTK_WIDGET (item), &child_requisition);

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			acc_space += child_requisition.width;
			item_size_left_to_place -= child_requisition.width;
			priv->end_position -= child_requisition.width;
			
			child_allocation.x = priv->end_position;
			child_allocation.width = child_requisition.width;
			child_allocation.height = priv->max_height;
		} else {
			acc_space += child_requisition.height;
			priv->end_position -= child_requisition.height;
			
			child_allocation.y = priv->end_position;
			child_allocation.height = child_requisition.height;
			child_allocation.width = priv->max_width;
		}
		
		gtk_widget_size_allocate (GTK_WIDGET (item), &child_allocation);
	}
	available_space -= acc_space;
	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		item_size_left_to_place = priv->total_width - acc_space;
	else
		item_size_left_to_place = priv->total_height - acc_space;
	
	/* make a pass through the items to determine how many fit */	
	space_required = 0;
	num_expandable_items = 0;

	child_allocation.x = allocation->x + border_width;
	child_allocation.y = allocation->y + border_width;

	for (p = priv->items; p != NULL; p = p->next) {
		int item_size;

		item = MATECOMPONENT_UI_TOOLBAR_ITEM (p->data);
		if (! GTK_WIDGET_VISIBLE (item) || GTK_WIDGET (item)->parent != GTK_WIDGET (toolbar) ||
			matecomponent_ui_toolbar_item_get_pack_end (item))
			continue;

		gtk_widget_get_child_requisition (GTK_WIDGET (item), &child_requisition);

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
			item_size = child_requisition.width;
		else
			item_size = child_requisition.height;

/*		g_message ("Item  size %4d, space_required %4d, available %4d left to place %4d",
			   item_size, space_required, available_space, item_size_left_to_place); */
		
		if (item_size_left_to_place > available_space - space_required &&
		    space_required + item_size > available_space - popup_item_size)
			break;
		
		space_required += item_size;
		item_size_left_to_place -= item_size;

		if (matecomponent_ui_toolbar_item_get_expandable (item))
			num_expandable_items ++;
	}

	priv->first_not_fitting_item = p;

	/* determine the amount of space available for expansion */
	if (priv->first_not_fitting_item != NULL) {
		extra_space = 0;
	} else {
		extra_space = available_space - space_required;
		if (priv->first_not_fitting_item != NULL)
			extra_space -= popup_item_size;
	}

	first_expandable = FALSE;

	for (p = priv->items; p != priv->first_not_fitting_item; p = p->next) {
		int expansion_amount;

		item = MATECOMPONENT_UI_TOOLBAR_ITEM (p->data);
		if (! GTK_WIDGET_VISIBLE (item) || GTK_WIDGET (item)->parent != GTK_WIDGET (toolbar) ||
			matecomponent_ui_toolbar_item_get_pack_end (item))
			continue;

		gtk_widget_get_child_requisition (GTK_WIDGET (item), &child_requisition);

		if (! matecomponent_ui_toolbar_item_get_expandable (item)) {
			expansion_amount = 0;
		} else {
			g_assert (num_expandable_items != 0);

			expansion_amount = extra_space / num_expandable_items;
			if (first_expandable) {
				expansion_amount += extra_space % num_expandable_items;
				first_expandable = FALSE;
			}
		}

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			child_allocation.width  = child_requisition.width + expansion_amount;
			child_allocation.height = priv->max_height;
		} else {
			child_allocation.width  = priv->max_width;
			child_allocation.height = child_requisition.height + expansion_amount;
		}

		gtk_widget_size_allocate (GTK_WIDGET (item), &child_allocation);

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
			child_allocation.x += child_allocation.width;
		else
			child_allocation.y += child_allocation.height;
	}

	hide_not_fitting_items (toolbar);
	setup_popup_item (toolbar);
}

/* GObject methods.  */

static void
impl_dispose (GObject *object)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	GList *items, *p, *next;

	toolbar = MATECOMPONENT_UI_TOOLBAR (object);
	priv = toolbar->priv;

	items = priv->items;
	for (p = items; p; p = next) {
		GtkWidget *item_widget;

		next = p->next;
		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent == NULL) {
			items = g_list_remove (items, item_widget);
			gtk_widget_destroy (item_widget);
		}
	}

	if (priv->popup_item &&
	    GTK_WIDGET (priv->popup_item)->parent == NULL)
		gtk_widget_destroy (GTK_WIDGET (priv->popup_item));
	priv->popup_item = NULL;

	if (priv->popup_window != NULL)
		gtk_widget_destroy (priv->popup_window);
	priv->popup_window = NULL;

	if (priv->tooltips)
		g_object_ref_sink (GTK_OBJECT (priv->tooltips));
	priv->tooltips = NULL;

	G_OBJECT_CLASS (matecomponent_ui_toolbar_parent_class)->dispose (object);
}

static void
impl_finalize (GObject *object)
{
	MateComponentUIToolbar *toolbar = (MateComponentUIToolbar *) object;
	
	g_list_free (toolbar->priv->items);
	g_free (toolbar->priv);

	G_OBJECT_CLASS (matecomponent_ui_toolbar_parent_class)->finalize (object);
}

/* GtkWidget methods.  */

static void
impl_size_request (GtkWidget *widget,
		   GtkRequisition *requisition)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	int border_width;

	toolbar = MATECOMPONENT_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	g_assert (priv->popup_item != NULL);

	update_sizes (toolbar);

	border_width = GTK_CONTAINER (toolbar)->border_width;

	if (priv->is_floating) {
		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			requisition->width  = priv->total_width;
			requisition->height = priv->max_height;
		} else {
			requisition->width  = priv->max_width;
			requisition->height = priv->total_height;
		}
	} else {
		GtkRequisition popup_item_requisition;

		gtk_widget_size_request (GTK_WIDGET (priv->popup_item), &popup_item_requisition);

		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
			requisition->width  = popup_item_requisition.width;
			requisition->height = MAX (popup_item_requisition.height, priv->max_height);
		} else {
			requisition->width  = MAX (popup_item_requisition.width,  priv->max_width);
			requisition->height = popup_item_requisition.height;
		}
	}

	requisition->width  += 2 * border_width;
	requisition->height += 2 * border_width;
}

static void
impl_size_allocate (GtkWidget *widget,
		    GtkAllocation *allocation)
{
	MateComponentUIToolbar *toolbar;

	toolbar = MATECOMPONENT_UI_TOOLBAR (widget);

	size_allocate_helper (toolbar, allocation);
}

static void
impl_map (GtkWidget *widget)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	toolbar = MATECOMPONENT_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	GTK_WIDGET_SET_FLAGS (toolbar, GTK_MAPPED);

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (GTK_WIDGET_VISIBLE (item_widget) && ! GTK_WIDGET_MAPPED (item_widget))
			gtk_widget_map (item_widget);
	}

	if (GTK_WIDGET_VISIBLE (priv->popup_item) && ! GTK_WIDGET_MAPPED (priv->popup_item))
		gtk_widget_map (GTK_WIDGET (priv->popup_item));
}

static void
impl_unmap (GtkWidget *widget)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	toolbar = MATECOMPONENT_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (GTK_WIDGET_VISIBLE (item_widget) && GTK_WIDGET_MAPPED (item_widget))
			gtk_widget_unmap (item_widget);
	}

	if (GTK_WIDGET_VISIBLE (priv->popup_item) && GTK_WIDGET_MAPPED (priv->popup_item))
		gtk_widget_unmap (GTK_WIDGET (priv->popup_item));
}

static int
impl_expose_event (GtkWidget *widget,
		   GdkEventExpose *event)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	GtkShadowType shadow_type;
	GList *p;

	if (! GTK_WIDGET_DRAWABLE (widget))
		return TRUE;

	toolbar = MATECOMPONENT_UI_TOOLBAR (widget);
	priv = toolbar->priv;

	gtk_widget_style_get (widget, "shadow_type", &shadow_type, NULL);

	gtk_paint_box (widget->style,
		       widget->window,
		       GTK_WIDGET_STATE (widget),
		       shadow_type,
		       &event->area, widget, "toolbar",
		       widget->allocation.x,
		       widget->allocation.y,
		       widget->allocation.width,
		       widget->allocation.height);

	for (p = priv->items; p != NULL; p = p->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (p->data);
		if (item_widget->parent != GTK_WIDGET (toolbar))
			continue;

		if (! GTK_WIDGET_NO_WINDOW (item_widget))
			continue;

		gtk_container_propagate_expose (
			GTK_CONTAINER (widget), item_widget, event);
	}

	gtk_container_propagate_expose (
		GTK_CONTAINER (widget), GTK_WIDGET (priv->popup_item), event);

	return TRUE;
}


/* GtkContainer methods.  */

static void
impl_remove (GtkContainer *container,
	     GtkWidget    *child)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;

	toolbar = MATECOMPONENT_UI_TOOLBAR (container);
	priv = toolbar->priv;

	if (child == (GtkWidget *) priv->popup_item)
		priv->popup_item = NULL;

	gtk_widget_unparent (child);

	gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
impl_forall (GtkContainer *container,
	     gboolean include_internals,
	     GtkCallback callback,
	     void *callback_data)
{
	MateComponentUIToolbar *toolbar;
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	toolbar = MATECOMPONENT_UI_TOOLBAR (container);
	priv = toolbar->priv;

	p = priv->items;
	while (p != NULL) {
		GtkWidget *child;
		GList *pnext;

		pnext = p->next;

		child = GTK_WIDGET (p->data);
		if (child->parent == GTK_WIDGET (toolbar))
			(* callback) (child, callback_data);

		p = pnext;
	}

	if (priv->popup_item)
		(* callback) (GTK_WIDGET (priv->popup_item),
			      callback_data);
}


/* MateComponentUIToolbar signals.  */

static void
impl_set_orientation (MateComponentUIToolbar *toolbar,
		      GtkOrientation orientation)
{
	MateComponentUIToolbarPrivate *priv;
	GList *p;

	priv = toolbar->priv;

	if (orientation == priv->orientation)
		return;

	priv->orientation = orientation;

	for (p = priv->items; p != NULL; p = p->next) {
		MateComponentUIToolbarItem *item;

		item = MATECOMPONENT_UI_TOOLBAR_ITEM (p->data);
		set_attributes_on_child (item, orientation, priv->style);
	}

	matecomponent_ui_toolbar_item_set_orientation (
		MATECOMPONENT_UI_TOOLBAR_ITEM (priv->popup_item), orientation);

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
impl_style_changed (MateComponentUIToolbar *toolbar)
{
	GList *p;
	MateComponentUIToolbarStyle style;
	MateComponentUIToolbarPrivate *priv;

	priv = toolbar->priv;

	style = (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ? priv->hstyle : priv->vstyle;

	if (style == priv->style)
		return;

	priv->style = style;

	for (p = priv->items; p != NULL; p = p->next) {
		MateComponentUIToolbarItem *item;

		item = MATECOMPONENT_UI_TOOLBAR_ITEM (p->data);
		set_attributes_on_child (item, priv->orientation, style);
	}

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));
}

static void
impl_get_property (GObject    *object,
		   guint       property_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	MateComponentUIToolbar *toolbar = MATECOMPONENT_UI_TOOLBAR (object);
	MateComponentUIToolbarPrivate *priv = toolbar->priv;
	gint border_width;

	border_width = GTK_CONTAINER (object)->border_width;
	
	switch (property_id) {
	case PROP_ORIENTATION:
		g_value_set_uint (
			value, matecomponent_ui_toolbar_get_orientation (toolbar));
		break;
	case PROP_IS_FLOATING:
		g_value_set_boolean (value, priv->is_floating);
		break;
	case PROP_PREFERRED_WIDTH:
		update_sizes (toolbar);
		if (matecomponent_ui_toolbar_get_orientation (toolbar) ==
		    GTK_ORIENTATION_HORIZONTAL)
			g_value_set_uint (value, priv->total_width + 2 * border_width);
		else
			g_value_set_uint (value, priv->max_width + 2 * border_width);
		break;
	case PROP_PREFERRED_HEIGHT:
		update_sizes (toolbar);
		if (matecomponent_ui_toolbar_get_orientation (toolbar) ==
		    GTK_ORIENTATION_HORIZONTAL)
			g_value_set_uint (value, priv->max_height + 2 * border_width);
		else
			g_value_set_uint (value, priv->total_height + 2 * border_width);
		break;
	default:
		break;
	};
}

static void
impl_set_property (GObject      *object,
		   guint         property_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	MateComponentUIToolbar *toolbar = MATECOMPONENT_UI_TOOLBAR (object);
	MateComponentUIToolbarPrivate *priv = toolbar->priv;

	switch (property_id) {
	case PROP_ORIENTATION:
		matecomponent_ui_toolbar_set_orientation (
			toolbar, g_value_get_enum (value));
		break;
	case PROP_IS_FLOATING:
		priv->is_floating = g_value_get_boolean (value);
		break;
	default:
		break;
	};
}

static void
matecomponent_ui_toolbar_class_init (MateComponentUIToolbarClass *toolbar_class)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	gobject_class = (GObjectClass *) toolbar_class;
	gobject_class->finalize     = impl_finalize;
	gobject_class->dispose      = impl_dispose;
	gobject_class->get_property = impl_get_property;
	gobject_class->set_property = impl_set_property;

	widget_class = GTK_WIDGET_CLASS (toolbar_class);
	widget_class->size_request  = impl_size_request;
	widget_class->size_allocate = impl_size_allocate;
	widget_class->map           = impl_map;
	widget_class->unmap         = impl_unmap;
	widget_class->expose_event  = impl_expose_event;

	container_class = GTK_CONTAINER_CLASS (toolbar_class);
	container_class->remove = impl_remove;
	container_class->forall = impl_forall;

	toolbar_class->set_orientation = impl_set_orientation;
	toolbar_class->style_changed   = impl_style_changed;


	g_object_class_install_property (
		gobject_class,
		PROP_ORIENTATION,
		g_param_spec_enum ("orientation",
				   _("Orientation"),
				   _("Orientation"),
				   GTK_TYPE_ORIENTATION,
				   GTK_ORIENTATION_HORIZONTAL,
				   G_PARAM_READWRITE));

	g_object_class_install_property (
		gobject_class,
		PROP_IS_FLOATING,
		g_param_spec_boolean ("is_floating",
				      _("is floating"),
				      _("whether the toolbar is floating"),
				      FALSE,
				      G_PARAM_READWRITE));

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_WIDTH,
		g_param_spec_uint ("preferred_width",
				   _("Preferred width"),
				   _("Preferred width"),
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_PREFERRED_HEIGHT,
		g_param_spec_uint ("preferred_height",
				   _("Preferred height"),
				   _("Preferred height"),
				   0, G_MAXINT, 0,
				   G_PARAM_READABLE));

	signals[SET_ORIENTATION]
		= g_signal_new ("set_orientation",
				G_TYPE_FROM_CLASS (gobject_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarClass,
						 set_orientation),
				NULL, NULL,
				g_cclosure_marshal_VOID__INT,
				G_TYPE_NONE, 1, G_TYPE_INT);

	signals[STYLE_CHANGED]
		= g_signal_new ("set_style",
				G_TYPE_FROM_CLASS (gobject_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (MateComponentUIToolbarClass,
						 style_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

	gtk_widget_class_install_style_property (
	        widget_class,
		g_param_spec_enum ("shadow_type",
				   _("Shadow type"),
				   _("Style of bevel around the toolbar"),
				   GTK_TYPE_SHADOW_TYPE,
				   GTK_SHADOW_OUT,
				   G_PARAM_READABLE));
}

static void
matecomponent_ui_toolbar_init (MateComponentUIToolbar *toolbar)
{
	AtkObject *ao;
	MateComponentUIToolbarStyle style;
	MateComponentUIToolbarPrivate *priv;

	GTK_WIDGET_SET_FLAGS (toolbar, GTK_NO_WINDOW);

	priv = g_new (MateComponentUIToolbarPrivate, 1);

	style = MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_AND_TEXT;

	priv->orientation                 = GTK_ORIENTATION_HORIZONTAL;
	priv->is_floating		  = FALSE;
	priv->style                       = style;
	priv->hstyle                      = style;
	priv->vstyle                      = style;
	priv->max_width			  = 0;
	priv->total_width		  = 0;
	priv->max_height		  = 0;
	priv->total_height		  = 0;
	priv->popup_item                  = NULL;
	priv->items                       = NULL;
	priv->first_not_fitting_item      = NULL;
	priv->popup_window                = NULL;
	priv->popup_window_vbox           = NULL;
	priv->items_moved_to_popup_window = FALSE;
	priv->tooltips                    = gtk_tooltips_new ();

	toolbar->priv = priv;

	ao = gtk_widget_get_accessible (GTK_WIDGET (toolbar));
	if (ao)
		atk_object_set_role (ao, ATK_ROLE_TOOL_BAR);
}

void
matecomponent_ui_toolbar_construct (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;
	GtkWidget *frame;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar));

	priv = toolbar->priv;

	priv->popup_item = MATECOMPONENT_UI_TOOLBAR_ITEM (matecomponent_ui_toolbar_popup_item_new ());
	matecomponent_ui_toolbar_item_set_orientation (priv->popup_item, priv->orientation);
	parentize_widget (toolbar, GTK_WIDGET (priv->popup_item));

	g_signal_connect (G_OBJECT (priv->popup_item), "toggled",
			  G_CALLBACK (popup_item_toggled_cb), toolbar);

	priv->popup_window = gtk_window_new (GTK_WINDOW_POPUP);
	g_signal_connect (G_OBJECT (priv->popup_window), "button_release_event",
			  G_CALLBACK (popup_window_button_release_cb), toolbar);

	frame = gtk_frame_new (NULL);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (priv->popup_window), frame);

	priv->popup_window_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->popup_window_vbox);
	gtk_container_add (GTK_CONTAINER (frame), priv->popup_window_vbox);
}

GtkWidget *
matecomponent_ui_toolbar_new (void)
{
	MateComponentUIToolbar *toolbar;

	toolbar = g_object_new (matecomponent_ui_toolbar_get_type (), NULL);

	matecomponent_ui_toolbar_construct (toolbar);

	return GTK_WIDGET (toolbar);
}


void
matecomponent_ui_toolbar_set_orientation (MateComponentUIToolbar *toolbar,
				   GtkOrientation orientation)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar));
	g_return_if_fail (orientation == GTK_ORIENTATION_HORIZONTAL ||
			  orientation == GTK_ORIENTATION_VERTICAL);

	g_signal_emit (toolbar, signals[SET_ORIENTATION], 0, orientation);
	g_signal_emit (toolbar, signals[STYLE_CHANGED], 0);
}

GtkOrientation
matecomponent_ui_toolbar_get_orientation (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;

	g_return_val_if_fail (toolbar != NULL, GTK_ORIENTATION_HORIZONTAL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

	priv = toolbar->priv;

	return priv->orientation;
}


MateComponentUIToolbarStyle
matecomponent_ui_toolbar_get_style (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;

	g_return_val_if_fail (toolbar != NULL, MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar), MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT);

	priv = toolbar->priv;

	return priv->style;
}

GtkTooltips *
matecomponent_ui_toolbar_get_tooltips (MateComponentUIToolbar *toolbar)
{
	MateComponentUIToolbarPrivate *priv;

	g_return_val_if_fail (toolbar != NULL, NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar), NULL);

	priv = toolbar->priv;

	return priv->tooltips;
}

void
matecomponent_ui_toolbar_insert (MateComponentUIToolbar *toolbar,
			  MateComponentUIToolbarItem *item,
			  int position)
{
	MateComponentUIToolbarPrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar));
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR_ITEM (item));

	priv = toolbar->priv;

	/*
	 *  This ugly hack is here since we might have unparented
	 * a widget and then re-added it to the toolbar at a later
	 * date, and un-parenting doesn't work quite properly yet.
	 *
	 *  Un-parenting is down to the widget possibly being a
	 * child of either this widget, or the popup window.
	 */
	if (!g_list_find (priv->items, item)) {
		g_object_ref_sink (item);
		priv->items = g_list_insert (priv->items, item, position);
	}

	g_signal_connect_object (
		item, "destroy",
		G_CALLBACK (item_destroy_cb),
		toolbar, 0);
	g_signal_connect_object (
		item, "activate",
		G_CALLBACK (item_activate_cb),
		toolbar, 0);
	g_signal_connect_object (
		item, "set_want_label",
		G_CALLBACK (item_set_want_label_cb),
		toolbar, 0);

	g_object_ref (toolbar);
	g_object_ref (item);

	set_attributes_on_child (item, priv->orientation, priv->style);
	parentize_widget (toolbar, GTK_WIDGET (item));

	gtk_widget_queue_resize (GTK_WIDGET (toolbar));

	g_object_unref (item);
	g_object_unref (toolbar);
}

GList *
matecomponent_ui_toolbar_get_children (MateComponentUIToolbar *toolbar)
{
	GList *ret = NULL, *l;

	g_return_val_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar), NULL);

	for (l = toolbar->priv->items; l; l = l->next) {
		GtkWidget *item_widget;

		item_widget = GTK_WIDGET (l->data);
		if (item_widget->parent != NULL) /* Unparented but still here */
			ret = g_list_prepend (ret, item_widget);
	}

	return g_list_reverse (ret);
}

void
matecomponent_ui_toolbar_set_hv_styles (MateComponentUIToolbar      *toolbar,
				 MateComponentUIToolbarStyle  hstyle,
				 MateComponentUIToolbarStyle  vstyle)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar));

	toolbar->priv->hstyle = hstyle;
	toolbar->priv->vstyle = vstyle;

	g_signal_emit (toolbar, signals [STYLE_CHANGED], 0);
}

void
matecomponent_ui_toolbar_show_tooltips (MateComponentUIToolbar *toolbar,
				 gboolean         show_tips)
{
	g_return_if_fail (MATECOMPONENT_IS_UI_TOOLBAR (toolbar));

	if (show_tips)
		gtk_tooltips_enable (toolbar->priv->tooltips);
	else
		gtk_tooltips_disable (toolbar->priv->tooltips);
}
