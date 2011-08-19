/* mate-druid.c
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

#include <glib/gi18n-lib.h>

#include <libmate/mate-macros.h>
#include <libmateui/mate-stock-icons.h>

#include "mate-uidefs.h"
#include "mate-druid.h"

struct _MateDruidPrivate
{
	MateDruidPage *current;
	GList *children;

	GtkWidget *bbox;

	guint show_finish : 1; /* if TRUE, then we are showing the finish button instead of the next button */
	guint show_help : 1;
};

enum {
	CANCEL,
	HELP,
	LAST_SIGNAL
};
enum {
  PROP_0,
  PROP_SHOW_FINISH,
  PROP_SHOW_HELP
};


static void    mate_druid_destroy         (GtkObject       *object);
static void    mate_druid_finalize        (GObject         *object);
static void    mate_druid_set_property    (GObject         *object,
					    guint            prop_id,
					    const GValue    *value,
					    GParamSpec      *pspec);
static void    mate_druid_get_property    (GObject         *object,
					    guint            prop_id,
					    GValue          *value,
					    GParamSpec      *pspec);
static void    mate_druid_size_request    (GtkWidget       *widget,
					    GtkRequisition  *requisition);
static void    mate_druid_size_allocate   (GtkWidget       *widget,
					    GtkAllocation   *allocation);
static gint    mate_druid_expose          (GtkWidget       *widget,
					    GdkEventExpose  *event);
static void    mate_druid_map             (GtkWidget       *widget);
static void    mate_druid_unmap           (GtkWidget       *widget);
static GType mate_druid_child_type      (GtkContainer    *container);
static void    mate_druid_add             (GtkContainer    *widget,
					    GtkWidget       *page);
static void    mate_druid_remove          (GtkContainer    *widget,
					    GtkWidget       *child);
static void    mate_druid_forall          (GtkContainer    *container,
					    gboolean         include_internals,
					    GtkCallback      callback,
					    gpointer         callback_data);
static void    mate_druid_back_callback   (GtkWidget       *button,
					    MateDruid      *druid);
static void    mate_druid_next_callback   (GtkWidget       *button,
					    MateDruid      *druid);
static void    mate_druid_cancel_callback (GtkWidget       *button,
					    GtkWidget       *druid);
static void    mate_druid_help_callback   (GtkWidget       *button,
					    MateDruid      *druid);


static guint druid_signals[LAST_SIGNAL] = { 0 };

/* Accessibility Support */
static AtkObject *mate_druid_get_accessible  (GtkWidget   *widget);

/* define the _get_type method and parent_class */
MATE_CLASS_BOILERPLATE(MateDruid, mate_druid,
			GtkContainer, GTK_TYPE_CONTAINER)

static void
mate_druid_class_init (MateDruidClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkContainerClass *container_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	container_class = (GtkContainerClass*) klass;

	druid_signals[CANCEL] =
		g_signal_new ("cancel",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidClass, cancel),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	druid_signals[HELP] =
		g_signal_new ("help",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateDruidClass, help),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class->destroy = mate_druid_destroy;
	gobject_class->set_property = mate_druid_set_property;
	gobject_class->get_property = mate_druid_get_property;
	gobject_class->finalize = mate_druid_finalize;
	widget_class->size_request = mate_druid_size_request;
	widget_class->size_allocate = mate_druid_size_allocate;
	widget_class->map = mate_druid_map;
	widget_class->unmap = mate_druid_unmap;
	widget_class->expose_event = mate_druid_expose;
	widget_class->get_accessible = mate_druid_get_accessible;

	container_class->forall = mate_druid_forall;
	container_class->add = mate_druid_add;
	container_class->remove = mate_druid_remove;
	container_class->child_type = mate_druid_child_type;


	g_object_class_install_property (gobject_class,
					 PROP_SHOW_FINISH,
					 g_param_spec_boolean ("show_finish",
							       _("Show Finish"),
							       _("Show the 'Finish' button instead of the 'Next' button"),
							       FALSE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
					 PROP_SHOW_HELP,
					 g_param_spec_boolean ("show_help",
							       _("Show Help"),
							       _("Show the 'Help' button"),
							       FALSE,
							       G_PARAM_READWRITE));
}

static void
mate_druid_instance_init (MateDruid *druid)
{
	druid->_priv = g_new0(MateDruidPrivate, 1);

	/* set the default border width */
	GTK_CONTAINER (druid)->border_width = MATE_PAD_SMALL;

	/* set up the buttons */
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (druid), GTK_NO_WINDOW);
	druid->back = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
	GTK_WIDGET_SET_FLAGS (druid->back, GTK_CAN_DEFAULT);
	druid->next = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
	GTK_WIDGET_SET_FLAGS (druid->next, GTK_CAN_DEFAULT);
	druid->cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	GTK_WIDGET_SET_FLAGS (druid->cancel, GTK_CAN_DEFAULT);
	druid->finish = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	GTK_WIDGET_SET_FLAGS (druid->finish, GTK_CAN_DEFAULT);
	druid->help = gtk_button_new_from_stock (GTK_STOCK_HELP);
	GTK_WIDGET_SET_FLAGS (druid->help, GTK_CAN_DEFAULT);
	gtk_widget_show (druid->back);
	gtk_widget_show (druid->next);
	gtk_widget_show (druid->cancel);

	druid->_priv->bbox = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (druid->_priv->bbox),
				   GTK_BUTTONBOX_END);
	/* FIXME: use a style property for these */
	gtk_box_set_spacing (GTK_BOX (druid->_priv->bbox), 10);
	gtk_container_set_border_width (GTK_CONTAINER (druid->_priv->bbox), 5);
	gtk_widget_set_parent (druid->_priv->bbox, GTK_WIDGET (druid));
	gtk_widget_show (druid->_priv->bbox);

	gtk_box_pack_end (GTK_BOX (druid->_priv->bbox), druid->help,
			  FALSE, TRUE, 0);
	gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (druid->_priv->bbox),
					    druid->help, TRUE);

	gtk_box_pack_end (GTK_BOX (druid->_priv->bbox), druid->cancel,
			  FALSE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (druid->_priv->bbox), druid->back,
			  FALSE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (druid->_priv->bbox), druid->next,
			  FALSE, TRUE, 0);

	gtk_box_pack_end (GTK_BOX (druid->_priv->bbox), druid->finish,
			  FALSE, TRUE, 0);

	/* other flags */
	druid->_priv->current = NULL;
	druid->_priv->children = NULL;
	druid->_priv->show_finish = FALSE;
	druid->_priv->show_help = FALSE;

	g_signal_connect (druid->back, "clicked",
			  G_CALLBACK (mate_druid_back_callback), druid);
	g_signal_connect (druid->next, "clicked",
			  G_CALLBACK (mate_druid_next_callback), druid);
	g_signal_connect (druid->cancel, "clicked",
			  G_CALLBACK (mate_druid_cancel_callback), druid);
	g_signal_connect (druid->finish, "clicked",
			  G_CALLBACK (mate_druid_next_callback), druid);
	g_signal_connect (druid->help, "clicked",
			  G_CALLBACK (mate_druid_help_callback), druid);

	gtk_widget_grab_focus (druid->next);
}



static void
mate_druid_destroy (GtkObject *object)
{
	MateDruid *druid;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_DRUID (object));

	druid = MATE_DRUID (object);

	if(druid->_priv->bbox) {
		gtk_widget_destroy (druid->_priv->bbox);
		druid->_priv->bbox = NULL;
		druid->back = NULL;
		druid->next = NULL;
		druid->cancel = NULL;
		druid->finish = NULL;
		druid->help = NULL;
	}

	/* Remove all children, we set current to NULL so
	 * that the remove code doesn't try to do anything funny */
	druid->_priv->current = NULL;
	while (druid->_priv->children != NULL) {
		MateDruidPage *child = druid->_priv->children->data;
		gtk_container_remove (GTK_CONTAINER (druid), GTK_WIDGET(child));
	}

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_druid_finalize (GObject *object)
{
	MateDruid *druid;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_DRUID (object));

	druid = MATE_DRUID (object);

	g_free(druid->_priv);
	druid->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
mate_druid_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_SHOW_FINISH:
      mate_druid_set_show_finish (MATE_DRUID (object), g_value_get_boolean (value));
      break;
    case PROP_SHOW_HELP:
      mate_druid_set_show_help (MATE_DRUID (object), g_value_get_boolean (value));
      break;
    default:
      break;
    }
}

static void
mate_druid_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_SHOW_FINISH:
		g_value_set_boolean (value, MATE_DRUID (object)->_priv->show_finish);
		break;
	case PROP_SHOW_HELP:
		g_value_set_boolean (value, MATE_DRUID (object)->_priv->show_help);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
mate_druid_size_request (GtkWidget *widget,
			  GtkRequisition *requisition)
{
	guint16 temp_width, temp_height;
	GList *list;
	MateDruid *druid;
	GtkRequisition child_requisition;
	MateDruidPage *child;
	int border;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));

	druid = MATE_DRUID (widget);
	temp_height = temp_width = 0;

	border = GTK_CONTAINER(widget)->border_width;

	/* We find the maximum size of all children widgets */
	for (list = druid->_priv->children; list; list = list->next) {
		child = MATE_DRUID_PAGE (list->data);
		if (GTK_WIDGET_VISIBLE (child)) {
			gtk_widget_size_request (GTK_WIDGET (child), &child_requisition);
			temp_width = MAX (temp_width, child_requisition.width);
			temp_height = MAX (temp_height, child_requisition.height);
			if (GTK_WIDGET_MAPPED (child) && child != druid->_priv->current)
				gtk_widget_unmap (GTK_WIDGET(child));
		}
	}

        requisition->width = temp_width + 2 * border;
        requisition->height = temp_height + 2 * border;

#if 0
	/* In an Attempt to show how the widgets are packed,
	 * here's a little diagram.
	 *
	 * [  Help  ] ------------- [  Back  ] [  Next  ]    [ Cancel ]
	 *             /
	 *     This part needs to be at least 1 button width.
	 *     In addition, there is 1/4 X Button width between Cancel and Next,
	 *     and a MATE_PAD_SMALL between Next and Back.
	 */
	/* our_button width is temp_width and temp_height */
	temp_height = 0;
	temp_width = 0;

	gtk_widget_size_request (druid->back, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->next, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->cancel, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->finish, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	gtk_widget_size_request (druid->help, &child_requisition);
	temp_width = MAX (temp_width, child_requisition.width);
	temp_height = MAX (temp_height, child_requisition.height);

	temp_width += border * 2;
	temp_height += MATE_PAD_SMALL;

	temp_width = temp_width * 21/4  + MATE_PAD_SMALL * 3;
#endif

	gtk_widget_size_request (druid->_priv->bbox, &child_requisition);

	/* pick which is bigger, the buttons, or the MateDruidPages */
	requisition->width = MAX (child_requisition.width, requisition->width);
	requisition->height += child_requisition.height + MATE_PAD_SMALL * 2;
}

static void
mate_druid_size_allocate (GtkWidget *widget,
			   GtkAllocation *allocation)
{
	MateDruid *druid;
	GtkAllocation child_allocation;
	gint button_height;
	GList *list;
	int border;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));

	druid = MATE_DRUID (widget);
	widget->allocation = *allocation;

	border = GTK_CONTAINER(widget)->border_width;

#if 0
	/* deal with the buttons */
	child_allocation.width = child_allocation.height = 0;
	child_allocation.width = druid->back->requisition.width;
	child_allocation.height = druid->back->requisition.height;
	child_allocation.width = MAX (child_allocation.width,
				      druid->next->requisition.width);
	child_allocation.height = MAX (child_allocation.height,
				       druid->next->requisition.height);
	child_allocation.width = MAX (child_allocation.width,
				      druid->cancel->requisition.width);
	child_allocation.height = MAX (child_allocation.height,
				       druid->cancel->requisition.height);
	child_allocation.width = MAX (child_allocation.width,
				      druid->help->requisition.width);
	child_allocation.height = MAX (child_allocation.height,
				       druid->help->requisition.height);

	child_allocation.height += MATE_PAD_SMALL;
	button_height = child_allocation.height;
	child_allocation.width += 2 * MATE_PAD_SMALL;
	child_allocation.x = allocation->x + allocation->width - MATE_PAD_SMALL - child_allocation.width;
	child_allocation.y = allocation->y + allocation->height - MATE_PAD_SMALL - child_allocation.height;
	gtk_widget_size_allocate (druid->cancel, &child_allocation);
	child_allocation.x -= (child_allocation.width * 5 / 4);
	gtk_widget_size_allocate (druid->next, &child_allocation);
	gtk_widget_size_allocate (druid->finish, &child_allocation);
	child_allocation.x -= (MATE_PAD_SMALL + child_allocation.width);
	gtk_widget_size_allocate (druid->back, &child_allocation);
	child_allocation.x = allocation->x + border;
	gtk_widget_size_allocate (druid->help, &child_allocation);
#endif
	button_height = druid->_priv->bbox->requisition.height;
	child_allocation.x = allocation->x;
	child_allocation.y = allocation->y + allocation->height - button_height;
	child_allocation.height = button_height;
	child_allocation.width = allocation->width;
	gtk_widget_size_allocate (druid->_priv->bbox, &child_allocation);

	/* Put up the MateDruidPage */
	child_allocation.x = allocation->x + border;
	child_allocation.y = allocation->y + border;
	child_allocation.width =
		((allocation->width - 2 * border) > 0) ?
		(allocation->width - 2 * border):0;
	child_allocation.height =
		((allocation->height - 2 * border - MATE_PAD_SMALL - button_height) > 0) ?
		(allocation->height - 2 * border - MATE_PAD_SMALL - button_height):0;
	for (list = druid->_priv->children; list; list=list->next) {
		GtkWidget *widget = GTK_WIDGET (list->data);
		if (GTK_WIDGET_VISIBLE (widget)) {
			gtk_widget_size_allocate (widget, &child_allocation);
		}
	}
}

static GType
mate_druid_child_type (GtkContainer *container)
{
	return MATE_TYPE_DRUID_PAGE;
}

static void
mate_druid_map (GtkWidget *widget)
{
	MateDruid *druid;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));

	druid = MATE_DRUID (widget);
	GTK_WIDGET_SET_FLAGS (druid, GTK_MAPPED);

#if 0
	gtk_widget_map (druid->back);
	if (druid->_priv->show_finish)
		gtk_widget_map (druid->finish);
	else
		gtk_widget_map (druid->next);
	if (druid->_priv->show_help)
		gtk_widget_map (druid->help);
	gtk_widget_map (druid->cancel);
#endif
	gtk_widget_map (druid->_priv->bbox);
	if (druid->_priv->current &&
	    GTK_WIDGET_VISIBLE (druid->_priv->current) &&
	    !GTK_WIDGET_MAPPED (druid->_priv->current)) {
		gtk_widget_map (GTK_WIDGET (druid->_priv->current));
	}
}

static void
mate_druid_unmap (GtkWidget *widget)
{
	MateDruid *druid;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));

	druid = MATE_DRUID (widget);
	GTK_WIDGET_UNSET_FLAGS (druid, GTK_MAPPED);
#if 0
	gtk_widget_unmap (druid->back);
	if (druid->_priv->show_finish)
		gtk_widget_unmap (druid->finish);
	else
		gtk_widget_unmap (druid->next);
	gtk_widget_unmap (druid->cancel);
	if (druid->_priv->show_help)
		gtk_widget_unmap (druid->help);
#endif
	gtk_widget_unmap (druid->_priv->bbox);
	if (druid->_priv->current &&
	    GTK_WIDGET_VISIBLE (druid->_priv->current) &&
	    GTK_WIDGET_MAPPED (druid->_priv->current))
		gtk_widget_unmap (GTK_WIDGET (druid->_priv->current));
}
static void
mate_druid_add (GtkContainer *widget,
		 GtkWidget *page)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));
	g_return_if_fail (page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (page));

	mate_druid_append_page (MATE_DRUID (widget), MATE_DRUID_PAGE (page));
}
static void
mate_druid_remove (GtkContainer *widget,
		    GtkWidget *child)
{
	MateDruid *druid;
	GList *list;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (MATE_IS_DRUID (widget));
	g_return_if_fail (child != NULL);

	druid = MATE_DRUID (widget);

	list = g_list_find (druid->_priv->children, child);
	/* Is it a page? */
	if (list != NULL) {
		/* If we are mapped and visible, we want to deal with changing the page. */
		if ((GTK_WIDGET_MAPPED (GTK_WIDGET (widget))) &&
		    (list->data == (gpointer) druid->_priv->current)) {
			if (list->next != NULL)
				mate_druid_set_page (druid, MATE_DRUID_PAGE (list->next->data));
			else if (list->prev != NULL)
				mate_druid_set_page (druid, MATE_DRUID_PAGE (list->prev->data));
			else
				/* Removing the only child, just set current to NULL */
				druid->_priv->current = NULL;
		}
	}
	druid->_priv->children = g_list_remove (druid->_priv->children, child);
	gtk_widget_unparent (child);
}

static void
mate_druid_forall (GtkContainer *container,
		    gboolean      include_internals,
		    GtkCallback   callback,
		    gpointer      callback_data)
{
	MateDruid *druid;
	MateDruidPage *child;
	GList *children;

	g_return_if_fail (container != NULL);
	g_return_if_fail (MATE_IS_DRUID (container));
	g_return_if_fail (callback != NULL);

	druid = MATE_DRUID (container);

	children = druid->_priv->children;
	while (children) {
		child = children->data;
		children = children->next;

		(* callback) (GTK_WIDGET (child), callback_data);
	}
	if (include_internals) {
		/* FIXME: should this be gtk_contianer_forall() ? */
		(* callback) (druid->_priv->bbox, callback_data);
#if 0
		(* callback) (druid->back, callback_data);
		(* callback) (druid->next, callback_data);
		(* callback) (druid->cancel, callback_data);
		(* callback) (druid->finish, callback_data);
		(* callback) (druid->help, callback_data);
#endif
	}
}

static gint
mate_druid_expose (GtkWidget      *widget,
		    GdkEventExpose *event)
{
	MateDruid *druid;
	GtkWidget *child;
	GList *children;

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (MATE_IS_DRUID (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (GTK_WIDGET_DRAWABLE (widget)) {
		druid = MATE_DRUID (widget);
		children = druid->_priv->children;

		while (children) {
			child = GTK_WIDGET (children->data);
			children = children->next;

			gtk_container_propagate_expose (GTK_CONTAINER (widget),
							child, event);
		}
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->_priv->bbox, event);
#if 0
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->back, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->next, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->cancel, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->finish, event);
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						druid->help, event);
#endif
	}
	return FALSE;
}

static void
mate_druid_back_callback (GtkWidget *button, MateDruid *druid)
{
	GList *list;
	g_return_if_fail (druid->_priv->current != NULL);

	if (mate_druid_page_back (druid->_priv->current))
		return;

	/* Make sure that we have a next list item */
	list = g_list_find (druid->_priv->children, druid->_priv->current);
	g_return_if_fail (list->prev != NULL);

	list = list->prev;
	while (list != NULL &&
	       ! GTK_WIDGET_VISIBLE (list->data))
		list = list->prev;
	
	if ( ! list)
		return;

	mate_druid_set_page (druid, MATE_DRUID_PAGE (list->data));
}
static void
mate_druid_next_callback (GtkWidget *button, MateDruid *druid)
{
	GList *list;
	g_return_if_fail (druid->_priv->current != NULL);

	if (druid->_priv->show_finish == FALSE) {
		if (mate_druid_page_next (druid->_priv->current))
			return;

		/* Make sure that we have a next list item */
		list = g_list_find (druid->_priv->children,
				    druid->_priv->current);
		/* this would be a bug */
		g_assert (list != NULL);

		list = list->next;
		while (list != NULL &&
		        ! GTK_WIDGET_VISIBLE (list->data))
			list = list->next;

		if ( ! list)
			return;

		mate_druid_set_page (druid, MATE_DRUID_PAGE (list->data));
	} else {
		mate_druid_page_finish (druid->_priv->current);
	}
}
static void
mate_druid_cancel_callback (GtkWidget *button, GtkWidget *druid)
{
     if (MATE_DRUID (druid)->_priv->current) {
	     if (mate_druid_page_cancel (MATE_DRUID (druid)->_priv->current))
		     return;

	     g_signal_emit (druid, druid_signals [CANCEL], 0);
     }
}

static void
mate_druid_help_callback (GtkWidget  *button,
			   MateDruid *druid)
{
	g_signal_emit (druid, druid_signals [HELP], 0);
}

/* Public Functions */
/**
 * mate_druid_new:
 *
 * Description: Creates a new #MateDruid widget.  You need to add this
 * to a dialog yourself, it is not a dialog.
 *
 * Returns:  A new #MateDruid widget
 **/
GtkWidget *
mate_druid_new (void)
{
	return g_object_new (MATE_TYPE_DRUID, NULL);
}

/**
 * mate_druid_new_with_window:
 * @title: A title of the window.
 * @parent: The parent of this window (transient_for).
 * @close_on_cancel: %TRUE if the window should be closed when cancel is
 * pressed.
 * @window: Optional return of the #GtkWindow created.
 *
 * Description: Creates a new #MateDruid widget. It also creates a new
 * toplevel window with the title of @title (which can be %NULL) and a parent
 * of @parent (which also can be %NULL). The window and the druid will both be
 * shown.  If you need the window widget pointer you can optionally get it
 * through the last argument.  When the druid gets destroyed, so will the
 * window that is created here.
 *
 * Returns:  A new #MateDruid widget.
 **/
GtkWidget *
mate_druid_new_with_window (const char *title,
			     GtkWindow *parent,
			     gboolean close_on_cancel,
			     GtkWidget **window)
{
	GtkWidget *druid = g_object_new (MATE_TYPE_DRUID, NULL);

	/* make sure we always set window to NULL, even in
	 * case of precondition errors */
	if (window != NULL)
		*window = NULL;

	g_return_val_if_fail (parent == NULL ||
			      GTK_IS_WINDOW (parent),
			      NULL);

	mate_druid_construct_with_window (MATE_DRUID (druid),
					   title,
					   parent,
					   close_on_cancel,
					   window);

	return druid;
}

/**
 * mate_druid_construct_with_window:
 * @druid: The #MateDruid.
 * @title: A title of the window.
 * @parent: The parent of this window (transient_for).
 * @close_on_cancel: %TRUE if the window should be closed when cancel is
 * pressed.
 * @window: Optional return of the #GtkWindow created.
 *
 * Description: Creates a new toplevel window with the title of @title (which
 * can be %NULL) and a parent of @parent (which also can be %NULL).  The @druid
 * will be placed inside this window. The window and the druid will both be
 * shown. If you need the window widget pointer you can optionally get it
 * through the last argument. When the druid gets destroyed, so will the
 * window that is created here.
 *
 * See also mate_druid_new_with_window().
 **/
void
mate_druid_construct_with_window (MateDruid *druid,
				   const char *title,
				   GtkWindow *parent,
				   gboolean close_on_cancel,
				   GtkWidget **window)
{
	GtkWidget *win;

	/* make sure we always set window to NULL, even in
	 * case of precondition errors */
	if (window != NULL)
		*window = NULL;

	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));
	g_return_if_fail (parent == NULL ||
			  GTK_IS_WINDOW (parent));

	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	if (title != NULL)
		gtk_window_set_title (GTK_WINDOW (win), title);
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (win),
					      parent);

	gtk_widget_show (GTK_WIDGET (druid));

	gtk_container_add (GTK_CONTAINER (win), GTK_WIDGET (druid));

	gtk_widget_show (win);

	if (close_on_cancel) {
		g_signal_connect_object (druid, "cancel",
					 G_CALLBACK (gtk_widget_destroy),
					 win,
					 G_CONNECT_SWAPPED);
	}

	/* When the druid gets destroyed so does the window */
	g_signal_connect_object (druid, "destroy",
				 G_CALLBACK (gtk_widget_destroy),
				 win,
				 G_CONNECT_SWAPPED);

	/* return the window */
	if (window != NULL)
		*window = win;
}


/**
 * mate_druid_set_buttons_sensitive
 * @druid: A Druid.
 * @back_sensitive: %TRUE if the back button is sensitive.
 * @next_sensitive: %TRUE if the next button is sensitive.
 * @cancel_sensitive: %TRUE if the cancel button is sensitive.
 * @help_sensitive: %TRUE if the help button is sensitive.
 *
 * Description: Sets the sensitivity of @druid's control-buttons.  If the
 * variables are %TRUE, then they will be clickable. This function is used
 * primarily by the actual MateDruidPage widgets.
 **/

void
mate_druid_set_buttons_sensitive (MateDruid *druid,
				   gboolean back_sensitive,
				   gboolean next_sensitive,
				   gboolean cancel_sensitive,
				   gboolean help_sensitive)
{
	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));

	gtk_widget_set_sensitive (druid->back, back_sensitive);
	gtk_widget_set_sensitive (druid->next, next_sensitive);
	gtk_widget_set_sensitive (druid->cancel, cancel_sensitive);
	gtk_widget_set_sensitive (druid->help, help_sensitive);
}

/**
 * mate_druid_set_show_finish
 * @druid: A #MateDruid widget.
 * @show_finish: If %TRUE, then the "Next" button is changed to be "Finish"
 *
 * Used to specify if @druid is currently showing the last page of the sequence
 * (and hence should display "Finish", rather than "Next").
 **/
void
mate_druid_set_show_finish (MateDruid *druid,
			     gboolean    show_finish)
{
	g_return_if_fail (MATE_IS_DRUID (druid));

	if ((show_finish?TRUE:FALSE) == druid->_priv->show_finish)
		return;

	if (show_finish) {
		gtk_widget_hide (druid->next);
		gtk_widget_show (druid->finish);
	} else {
		gtk_widget_hide (druid->finish);
		gtk_widget_show (druid->next);
	}
	druid->_priv->show_finish = show_finish?TRUE:FALSE;
}

/**
 * mate_druid_set_show_help
 * @druid: A #MateDruid.
 * @show_help: %TRUE, if the "Help" button is to be shown, %FALSE otherwise.
 *
 * Sets the "Help" button on the druid to be visible in the lower left corner of
 * the widget, if @show_help is %TRUE.
 **/
void
mate_druid_set_show_help (MateDruid *druid,
			   gboolean    show_help)
{
	g_return_if_fail (MATE_IS_DRUID (druid));

	if ((show_help?TRUE:FALSE) == druid->_priv->show_help)
		return;

	if (show_help) {
		gtk_widget_show (druid->help);
	} else {
		gtk_widget_hide (druid->help);
	}
	druid->_priv->show_help = show_help?TRUE:FALSE;
}


/**
 * mate_druid_prepend_page:
 * @druid: A Druid widget.
 * @page: The page to be inserted.
 *
 * Description: This will prepend a MateDruidPage into the internal list of
 * pages that the @druid has. Since #MateDruid is just a container, you will
 * need to also call gtk_widget_show() on the page, otherwise the page will not
 * be shown.
 **/
void
mate_druid_prepend_page (MateDruid *druid,
			  MateDruidPage *page)
{
	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (page));

	mate_druid_insert_page (druid, NULL, page);
}
/**
 * mate_druid_insert_page:
 * @druid: A #MateDruid widget.
 * @back_page: The page prior to the page to be inserted.
 * @page: The page to insert.
 *
 * Description: This will insert @page after @back_page into the list of
 * internal pages that the @druid has.  If @back_page is not present in the
 * list or %NULL, @page will be prepended to the list.  Since #MateDruid is
 * just a container, you will need to also call gtk_widget_show() on the page,
 * otherwise the page will not be shown.
 **/
void
mate_druid_insert_page (MateDruid *druid,
			 MateDruidPage *back_page,
			 MateDruidPage *page)
{
	GList *list;

	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (page));

	list = g_list_find (druid->_priv->children, back_page);
	if (list == NULL) {
		druid->_priv->children = g_list_prepend (druid->_priv->children, page);
	} else {
		GList *new_el = g_list_alloc ();
		new_el->next = list->next;
		new_el->prev = list;
		if (new_el->next)
			new_el->next->prev = new_el;
		new_el->prev->next = new_el;
		new_el->data = (gpointer) page;
	}
	gtk_widget_set_parent (GTK_WIDGET (page), GTK_WIDGET (druid));

	if (GTK_WIDGET_REALIZED (GTK_WIDGET (druid)))
		gtk_widget_realize (GTK_WIDGET (page));

	if (GTK_WIDGET_VISIBLE (GTK_WIDGET (druid)) && GTK_WIDGET_VISIBLE (GTK_WIDGET (page))) {
		if (GTK_WIDGET_MAPPED (GTK_WIDGET (page)))
			gtk_widget_unmap (GTK_WIDGET (page));
		gtk_widget_queue_resize (GTK_WIDGET (druid));
	}

	/* if it's the first and only page, we want to bring it to the foreground. */
	if (druid->_priv->children->next == NULL)
		mate_druid_set_page (druid, page);
}

/**
 * mate_druid_append_page:
 * @druid: A #MateDruid widget.
 * @page: The #MateDruidPage to be appended.
 *
 * Description: This will append @page onto the end of the internal list.
 * Since #MateDruid is just a container, you will need to also call
 * gtk_widget_show() on the page, otherwise the page will not be shown.
 **/
void
mate_druid_append_page (MateDruid *druid,
			 MateDruidPage *page)
{
	GList *list;
	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (page));

	list = g_list_last (druid->_priv->children);
	if (list) {
		mate_druid_insert_page (druid, MATE_DRUID_PAGE (list->data), page);
	} else {
		mate_druid_insert_page (druid, NULL, page);
	}
}
/**
 * mate_druid_set_page:
 * @druid: A #MateDruid widget.
 * @page: The #MateDruidPage to be brought to the foreground.
 *
 * Description: This will make @page the currently showing page in the druid.
 * @page must already be in the druid.
 **/
void
mate_druid_set_page (MateDruid *druid,
		      MateDruidPage *page)
{
	GList *list;
	GtkWidget *old = NULL;
	g_return_if_fail (druid != NULL);
	g_return_if_fail (MATE_IS_DRUID (druid));
	g_return_if_fail (page != NULL);
	g_return_if_fail (MATE_IS_DRUID_PAGE (page));

	if (druid->_priv->current == page)
	     return;
	list = g_list_find (druid->_priv->children, page);
	g_return_if_fail (list != NULL);

	if ((druid->_priv->current) && (GTK_WIDGET_VISIBLE (druid->_priv->current)) && (GTK_WIDGET_MAPPED (druid))) {
		old = GTK_WIDGET (druid->_priv->current);
	}
	druid->_priv->current = MATE_DRUID_PAGE (list->data);
	mate_druid_page_prepare (druid->_priv->current);
	if (GTK_WIDGET_VISIBLE (druid->_priv->current) && (GTK_WIDGET_MAPPED (druid))) {
		gtk_widget_map (GTK_WIDGET (druid->_priv->current));
		gtk_widget_set_sensitive (GTK_WIDGET (druid->_priv->current), TRUE);
	}
	if (old && GTK_WIDGET_MAPPED (old)) {
		gtk_widget_unmap (old);
		gtk_widget_set_sensitive (old, FALSE);
	}
}

static int
mate_druid_accessible_get_n_children (AtkObject *accessible)
{
	MateDruid *druid;
	GtkWidget *widget;

	widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return 0;

	druid = MATE_DRUID (widget);

	return g_list_length (druid->_priv->children) + 1;
}


static AtkObject *
mate_druid_accessible_ref_child (AtkObject *accessible,
                                  gint       index)
{
	MateDruid *druid;
	GtkWidget *widget;
	GList *children;
	GList *tmp_list;
	GtkWidget *child;
	AtkObject *obj;

	widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return NULL;
	
	if (index < 0)
		return NULL;
	druid = MATE_DRUID (widget);
	children = druid->_priv->children;
	if (index < g_list_length (children)) {
		tmp_list = g_list_nth (children, index);
		child = tmp_list->data;
	} else if (index == g_list_length (children)) {
		child = druid->_priv->bbox;
	} else {
		return NULL;
	}
	obj = gtk_widget_get_accessible (child);
	g_object_ref (obj);
	return obj;
}

static void
mate_druid_accessible_class_init (AtkObjectClass *klass)
{
	klass->get_n_children = mate_druid_accessible_get_n_children;
	klass->ref_child = mate_druid_accessible_ref_child;
}

static GType
mate_druid_accessible_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo tinfo = {
			0, /* class size */
			(GBaseInitFunc) NULL, /* base init */
			(GBaseFinalizeFunc) NULL, /* base finalize */
			(GClassInitFunc) mate_druid_accessible_class_init,
			(GClassFinalizeFunc) NULL, /* class finalize */
			NULL, /* class data */
			0, /* instance size */
			0, /* nb preallocs */
			(GInstanceInitFunc) NULL, /* instance init */
			NULL /* value table */
		};
		/*
		 * Figure out the size of the class and instance
		 * we are deriving from
		 */
		AtkObjectFactory *factory;
		GType derived_type;
		GTypeQuery query;
		GType derived_atk_type;

		derived_type = g_type_parent (MATE_TYPE_DRUID);
		factory = atk_registry_get_factory (atk_get_default_registry (),
						    derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		g_type_query (derived_atk_type, &query);
		tinfo.class_size = query.class_size;
		tinfo.instance_size = query.instance_size;
 
		type = g_type_register_static (derived_atk_type, 
					       "MateDruidAccessible", 
					       &tinfo, 0);
	}
	return type;
}

static AtkObject *
mate_druid_accessible_new (GObject *obj)
{
	AtkObject *accessible;

	g_return_val_if_fail (MATE_IS_DRUID (obj), NULL);

	accessible = g_object_new (mate_druid_accessible_get_type (), NULL); 
	atk_object_initialize (accessible, obj);

	return accessible;
}

static GType
mate_druid_accessible_factory_get_accessible_type (void)
{
	return mate_druid_accessible_get_type ();
}

static AtkObject*
mate_druid_accessible_factory_create_accessible (GObject *obj)
{
	return mate_druid_accessible_new (obj);
}

static void
mate_druid_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
	klass->create_accessible = mate_druid_accessible_factory_create_accessible;
	klass->get_accessible_type = mate_druid_accessible_factory_get_accessible_type;
}

static GType
mate_druid_accessible_factory_get_type (void)
{
	static GType type = 0;

	if (!type) {
		const GTypeInfo tinfo = {
			sizeof (AtkObjectFactoryClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) mate_druid_accessible_factory_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (AtkObjectFactory),
			0,             /* n_preallocs */
			NULL, NULL
		};

		type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY, 
					       "MateDruidAccessibleFactory",
					       &tinfo, 0);
	}
	return type;
}

static AtkObject *
mate_druid_get_accessible (GtkWidget *widget)
{
	static gboolean first_time = TRUE;

	if (first_time) {
		AtkObjectFactory *factory;
		AtkRegistry *registry;
 		GType derived_type; 
		GType derived_atk_type; 

		/*
		 * Figure out whether accessibility is enabled by looking at the
		 * type of the accessible object which would be created for
		 * the parent type of MateDruid.
		 */
		derived_type = g_type_parent (MATE_TYPE_DRUID);

		registry = atk_get_default_registry ();
		factory = atk_registry_get_factory (registry,
						    derived_type);
		derived_atk_type = atk_object_factory_get_accessible_type (factory);
		if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE))  {
			atk_registry_set_factory_type (registry, 
						       MATE_TYPE_DRUID,
						       mate_druid_accessible_factory_get_type ());
		}
		first_time = FALSE;
	} 
	return GTK_WIDGET_CLASS (parent_class)->get_accessible (widget);
}

