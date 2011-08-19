/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "search-context-picker.h"

#include <gtk/gtk.h>

typedef struct
{
	GtkImage *cur_icon;
	int cur_context;
	GtkWidget *menu;
} NldSearchContextPickerPrivate;

#define NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NLD_TYPE_SEARCH_CONTEXT_PICKER, NldSearchContextPickerPrivate))

enum
{
	CONTEXT_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void nld_search_context_picker_class_init (NldSearchContextPickerClass *);
static void nld_search_context_picker_init (NldSearchContextPicker *);

static void nld_search_context_picker_clicked (GtkButton *);

G_DEFINE_TYPE (NldSearchContextPicker, nld_search_context_picker, GTK_TYPE_BUTTON)

static void nld_search_context_picker_class_init (NldSearchContextPickerClass *
	nld_search_context_picker_class)
{
	GtkButtonClass *button_class = GTK_BUTTON_CLASS (nld_search_context_picker_class);

	button_class->clicked = nld_search_context_picker_clicked;

	g_type_class_add_private (nld_search_context_picker_class,
		sizeof (NldSearchContextPickerPrivate));

	signals[CONTEXT_CHANGED] = g_signal_new ("context-changed",
		G_TYPE_FROM_CLASS (nld_search_context_picker_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		G_STRUCT_OFFSET (NldSearchContextPickerClass, context_changed),
		NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
nld_search_context_picker_init (NldSearchContextPicker * picker)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (picker);
	GtkBox *hbox;

	hbox = GTK_BOX (gtk_hbox_new (FALSE, 10));
	gtk_container_add (GTK_CONTAINER (picker), GTK_WIDGET (hbox));

	priv->cur_icon = GTK_IMAGE (gtk_image_new ());
	gtk_box_pack_start (hbox, GTK_WIDGET (priv->cur_icon), FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, gtk_vseparator_new (), FALSE, FALSE, 0);
	gtk_box_pack_start (hbox, gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE), FALSE, FALSE, 0);

	gtk_widget_show_all (GTK_WIDGET (hbox));

	priv->cur_context = -1;

	priv->menu = gtk_menu_new ();
}

GtkWidget *
nld_search_context_picker_new (void)
{
	return g_object_new (NLD_TYPE_SEARCH_CONTEXT_PICKER, NULL);
}

static void
menu_position_func (GtkMenu * menu, int *x, int *y, gboolean * push_in, gpointer picker)
{
	GtkWidget *widget = GTK_WIDGET (picker);

	gdk_window_get_origin (widget->window, x, y);
	*x += widget->allocation.x;
	*y += widget->allocation.y + widget->allocation.height;

	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
	{
		GtkRequisition req;
		gtk_widget_size_request (GTK_WIDGET (menu), &req);
		*x += widget->allocation.width - req.width;
	}

	*push_in = FALSE;
}

static void
nld_search_context_picker_clicked (GtkButton * button)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (button);

	gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, menu_position_func, button, 1,
		gtk_get_current_event_time ());
}

static void
item_activated (GtkMenuItem * item, gpointer picker)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (picker);
	GtkImage *image;
	const char *icon_name;
	GtkIconSize icon_size;

	image = GTK_IMAGE (gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (item)));
	gtk_image_get_icon_name (image, &icon_name, &icon_size);
	gtk_image_set_from_icon_name (priv->cur_icon, icon_name, icon_size);

	priv->cur_context =
		GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
			"NldSearchContextPicker:context_id"));
	g_signal_emit (picker, signals[CONTEXT_CHANGED], 0);
}

void
nld_search_context_picker_add_context (NldSearchContextPicker * picker, const char *label,
	const char *icon_name, int context_id)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (picker);
	GtkWidget *item = gtk_image_menu_item_new_with_label (label);
	GtkWidget *image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	GList *children = gtk_container_get_children (GTK_CONTAINER (priv->menu));
	gboolean first = children == NULL;

	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	g_object_set_data (G_OBJECT (item), "NldSearchContextPicker:context_id",
		GINT_TO_POINTER (context_id));
	g_signal_connect (item, "activate", G_CALLBACK (item_activated), picker);
	gtk_widget_show_all (item);

	gtk_container_add (GTK_CONTAINER (priv->menu), item);
	if (first) {
		item_activated (GTK_MENU_ITEM (item), picker);
		g_list_free (children);
	}
}

int
nld_search_context_picker_get_context (NldSearchContextPicker * picker)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (picker);

	return priv->cur_context;
}

void
nld_search_context_picker_set_context (NldSearchContextPicker * picker, int context_id)
{
	NldSearchContextPickerPrivate *priv = NLD_SEARCH_CONTEXT_PICKER_GET_PRIVATE (picker);
	GList *children;

	children = gtk_container_get_children (GTK_CONTAINER (priv->menu));
	while (children)
	{
		GtkMenuItem *item = children->data;
		int item_id =
			GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
				"NldSearchContextPicker:content_id"));

		if (item_id == context_id)
		{
			item_activated (item, picker);
			return;
		}

		children = children->next;
	}
	g_list_free (children);

	priv->cur_context = -1;
	g_signal_emit (picker, signals[CONTEXT_CHANGED], 0);
}
