/* mate-druid-page.c
 * Copyright (C) 1999 Red Hat, Inc.
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

#include <config.h>
#include <libmate/mate-macros.h>

#include "mate-druid-page.h"
#include "mate-marshal.h"

#include <libmateuiP.h>

enum {
	NEXT,
	PREPARE,
	BACK,
	FINISH,
	CANCEL,
	CONFIGURE_CANVAS,
	LAST_SIGNAL
};

static void mate_druid_page_size_request       (GtkWidget               *widget,
						 GtkRequisition          *requisition);
static void mate_druid_page_size_allocate      (GtkWidget		 *widget,
						 GtkAllocation           *allocation);
static gint mate_druid_page_expose             (GtkWidget               *widget,
						 GdkEventExpose          *event);
static void mate_druid_page_realize            (GtkWidget		 *widget);


static guint druid_page_signals[LAST_SIGNAL] = { 0 };

MATE_CLASS_BOILERPLATE (MateDruidPage, mate_druid_page,
			 GtkBin, GTK_TYPE_BIN)

static void
mate_druid_page_class_init (MateDruidPageClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) class;
	widget_class = (GtkWidgetClass*) class;

	druid_page_signals[NEXT] =
		g_signal_new ("next",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidPageClass, next),
			      NULL, NULL,
			      _mate_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      GTK_TYPE_WIDGET);
	druid_page_signals[PREPARE] =
		g_signal_new ("prepare",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidPageClass, prepare),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GTK_TYPE_WIDGET);
	druid_page_signals[BACK] =
		g_signal_new ("back",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidPageClass, back),
			      NULL, NULL,
			      _mate_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      GTK_TYPE_WIDGET);
	druid_page_signals[FINISH] =
		g_signal_new ("finish",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidPageClass, finish),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GTK_TYPE_WIDGET);
	druid_page_signals[CANCEL] =
		g_signal_new ("cancel",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidPageClass, cancel),
			      NULL, NULL,
			      _mate_marshal_BOOLEAN__OBJECT,
			      G_TYPE_BOOLEAN, 1,
			      GTK_TYPE_WIDGET);

	widget_class->size_request = mate_druid_page_size_request;
	widget_class->size_allocate = mate_druid_page_size_allocate;
	widget_class->expose_event = mate_druid_page_expose;
	widget_class->realize = mate_druid_page_realize;

	class->next = NULL;
	class->prepare = NULL;
	class->back = NULL;
	class->finish = NULL;
}

static void
mate_druid_page_instance_init (MateDruidPage *druid_page)
{
	/* enable if you add privates, also add a destructor */
	/*druid_page->_priv = g_new0 (MateDruidPagePrivate, 1);*/
	druid_page->_priv = NULL;

	GTK_WIDGET_UNSET_FLAGS (druid_page, GTK_NO_WINDOW);
}

static void
mate_druid_page_size_request (GtkWidget *widget,
			       GtkRequisition *requisition)
{
	GtkBin *bin;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (widget));
	g_return_if_fail (requisition != NULL);
	bin = GTK_BIN (widget);
	requisition->width = GTK_CONTAINER (widget)->border_width * 2;
	requisition->height = GTK_CONTAINER (widget)->border_width * 2;

	if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) {
		GtkRequisition child_requisition;

		gtk_widget_size_request (bin->child, &child_requisition);

		requisition->width += child_requisition.width;
		requisition->height += child_requisition.height;
	}
}
static void
mate_druid_page_size_allocate (GtkWidget *widget,
				GtkAllocation *allocation)
{
	GtkBin *bin;
	GtkAllocation child_allocation;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (widget));
	g_return_if_fail (allocation != NULL);
	widget->allocation = *allocation;
	bin = GTK_BIN (widget);

	child_allocation.x = 0;
	child_allocation.y = 0;
	child_allocation.width = MAX (allocation->width - GTK_CONTAINER (widget)->border_width * 2, 0);
	child_allocation.height = MAX (allocation->height - GTK_CONTAINER (widget)->border_width * 2, 0);

	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (widget->window,
					allocation->x + GTK_CONTAINER (widget)->border_width,
					allocation->y + GTK_CONTAINER (widget)->border_width,
					child_allocation.width,
					child_allocation.height);
	}
	if (bin->child) {
		gtk_widget_size_allocate (bin->child, &child_allocation);
	}
}
static void
mate_druid_page_paint (GtkWidget     *widget,
			GdkRectangle *area)
{
	gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
			    GTK_SHADOW_NONE, area, widget, "base", 0, 0, -1, -1);
}

static gint
mate_druid_page_expose (GtkWidget               *widget,
			 GdkEventExpose          *event)
{
	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (MATE_IS_DRUID_PAGE (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (!GTK_WIDGET_APP_PAINTABLE (widget))
		mate_druid_page_paint (widget, &event->area);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		return MATE_CALL_PARENT_WITH_DEFAULT
			(GTK_WIDGET_CLASS, expose_event, (widget, event), FALSE);
	}

	return FALSE;
}

static void
mate_druid_page_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	gint border_width;
	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	border_width = GTK_CONTAINER (widget)->border_width;

	attributes.x = widget->allocation.x + border_width;
	attributes.y = widget->allocation.y + border_width;
	attributes.width = widget->allocation.width - 2*border_width;
	attributes.height = widget->allocation.height - 2*border_width;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget)
		| GDK_BUTTON_MOTION_MASK
		| GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK
		| GDK_EXPOSURE_MASK
		| GDK_ENTER_NOTIFY_MASK
		| GDK_LEAVE_NOTIFY_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);
	gdk_window_set_back_pixmap (widget->window, NULL, FALSE);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}


/**
 * mate_druid_page_new:
 *
 * Creates a new #MateDruidPage.
 *
 * Returns: The newly created #MateDruidPage.
 **/
GtkWidget *
mate_druid_page_new (void)
{
     GtkWidget *widget = g_object_new (MATE_TYPE_DRUID_PAGE, NULL);

     return widget;
}

/**
 * mate_druid_page_next:
 * @druid_page: A DruidPage widget.
 *
 * Description: This will emit the "next" signal for that particular page.  It
 * is called by mate-druid exclusively.  It is expected that non-linear Druid's
 * will override this signal and return TRUE if it handles changing pages.
 *
 * Return value: This function will return FALSE by default.
 **/
/* Public functions */
gboolean
mate_druid_page_next     (MateDruidPage *druid_page)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (druid_page != NULL, FALSE);
	g_return_val_if_fail (MATE_IS_DRUID_PAGE (druid_page), FALSE);

	g_signal_emit (druid_page, druid_page_signals [NEXT], 0,
		       GTK_WIDGET (druid_page)->parent,
		       &retval);

	return retval;
}
/**
 * mate_druid_page_prepare:
 * @druid_page: A DruidPage widget.
 *
 * Description: This emits the "prepare" signal for the page.  It is called by
 * mate-druid exclusively. This function is called immediately prior to a druid
 * page being shown so that it can "prepare" for display.
 **/
void
mate_druid_page_prepare  (MateDruidPage *druid_page)
{
	g_return_if_fail (druid_page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (druid_page));

	g_signal_emit (druid_page, druid_page_signals [PREPARE], 0,
		       GTK_WIDGET (druid_page)->parent);
}
/**
 * mate_druid_page_back:
 * @druid_page: A DruidPage widget.
 *
 * Description: This will emit the "back" signal for that particular page.  It
 * is called by mate-druid exclusively.  It is expected that non-linear Druid's
 * will override this signal and return TRUE if it handles changing pages.
 *
 * Return value: This function will return FALSE by default.
 **/
gboolean
mate_druid_page_back     (MateDruidPage *druid_page)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (druid_page != NULL, FALSE);
	g_return_val_if_fail (MATE_IS_DRUID_PAGE (druid_page), FALSE);

	g_signal_emit (druid_page, druid_page_signals [BACK], 0,
		       GTK_WIDGET (druid_page)->parent,
		       &retval);

	return retval;
}
/**
 * mate_druid_page_finish:
 * @druid_page: A DruidPage widget.
 *
 * Description: This emits the "finish" signal for the page.  It is called by
 * mate-druid exclusively.
 **/
void
mate_druid_page_finish   (MateDruidPage *druid_page)
{
	g_return_if_fail (druid_page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (druid_page));

	g_signal_emit (druid_page, druid_page_signals [FINISH], 0,
		       GTK_WIDGET (druid_page)->parent);
}
/**
 * mate_druid_page_cancel:
 * @druid_page: A DruidPage widget.
 *
 * Description: This will emit the "cancel" signal for that particular page.  It
 * is called by mate-druid exclusively.  It is expected that a Druid will
 * override this signal and return TRUE if it does not want to exit.
 *
 * Return value: This function will return FALSE by default.
 **/
gboolean
mate_druid_page_cancel   (MateDruidPage *druid_page)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (druid_page != NULL, FALSE);
	g_return_val_if_fail (MATE_IS_DRUID_PAGE (druid_page), FALSE);

	g_signal_emit (druid_page, druid_page_signals [CANCEL], 0,
		       GTK_WIDGET (druid_page)->parent,
		       &retval);

	return retval;
}
