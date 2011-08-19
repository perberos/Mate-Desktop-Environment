/* Popup menus for MATE
 *
 * Copyright (C) 1998 Mark Crichton
 * All rights reserved.
 *
 * Authors: Mark Crichton <mcrichto@purdue.edu>
 *          Federico Mena <federico@nuclecu.unam.mx>
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

#include <config.h>
#include <gtk/gtk.h>
#include "mate-popup-menu.h"

static GtkWidget* global_menushell_hack = NULL;
#define TOPLEVEL_MENUSHELL_KEY "mate-popup-menu:toplevel_menushell_key"

static GtkWidget*
get_toplevel(GtkWidget* menuitem)
{
	return g_object_get_data (G_OBJECT (menuitem), TOPLEVEL_MENUSHELL_KEY);
}

/* Our custom marshaller for menu items.  We need it so that it can extract the per-attachment
 * user_data pointer from the parent menu shell and pass it to the callback.  This overrides the
 * user-specified data from the MateUIInfo structures.
 */

typedef void (* ActivateFunc) (GtkObject *object, gpointer data, GtkWidget *for_widget);

static void
popup_marshal_func (GtkObject *object, gpointer data, guint n_args, GtkArg *args)
{
	ActivateFunc func;
	gpointer user_data;
	GObject *tl;
	GtkWidget *for_widget;

	tl = G_OBJECT (get_toplevel(GTK_WIDGET (object)));
	user_data = g_object_get_data (tl, "mate_popup_menu_do_popup_user_data");
	for_widget = g_object_get_data (tl, "mate_popup_menu_do_popup_for_widget");

	g_object_set_data (G_OBJECT (get_toplevel(GTK_WIDGET (object))),
			   "mate_popup_menu_active_item",
			   object);

	func = (ActivateFunc) data;
	if (func)
		(* func) (object, user_data, for_widget);
}

/* This is our custom signal connection function for popup menu items -- see below for the
 * marshaller information.  We pass the original callback function as the data pointer for the
 * marshaller (uiinfo->moreinfo).
 */
static void
popup_connect_func (MateUIInfo *uiinfo, const char *signal_name, MateUIBuilderData *uibdata)
{
	g_assert (uibdata->is_interp);

	g_object_set_data (G_OBJECT (uiinfo->widget),
			   TOPLEVEL_MENUSHELL_KEY,
			   global_menushell_hack);

	g_signal_connect (uiinfo->widget, signal_name,
			  G_CALLBACK (popup_marshal_func),
			  uiinfo->moreinfo);
}

/**
 * mate_popup_menu_new_with_accelgroup
 * @uiinfo: A #MateUIInfo array describing the new menu.
 * @accelgroup: A #GtkAccelGroup describing the accelerator key structure.
 *
 * Creates a popup menu out of the specified @uiinfo array.  Use
 * mate_popup_menu_do_popup() to pop the menu up, or attach it to a
 * window with mate_popup_menu_attach().
 *
 * Returns: A menu widget.
 */
GtkWidget *
mate_popup_menu_new_with_accelgroup (MateUIInfo *uiinfo,
				      GtkAccelGroup *accelgroup)
{
	GtkWidget *menu;
	GtkAccelGroup *my_accelgroup;

	/* We use our own callback marshaller so that it can fetch the
	 * popup user data from the popup menu and pass it on to the
	 * user-defined callbacks.
	 */

	menu = gtk_menu_new ();

	if(accelgroup)
	  my_accelgroup = accelgroup;
	else
	  my_accelgroup = gtk_accel_group_new();
	gtk_menu_set_accel_group (GTK_MENU (menu), my_accelgroup);
	if(!accelgroup)
	  g_object_unref (G_OBJECT (my_accelgroup));

	mate_popup_menu_append (menu, uiinfo);

	return menu;
}

/**
 * mate_popup_menu_new
 * @uiinfo: A #MateUIInfo array describing the new menu.
 *
 * This function behaves just like
 * mate_popup_menu_new_with_accelgroup(), except that it creates an
 * accelgroup for you and attaches it to the menu object.  Use
 * mate_popup_menu_get_accel_group() to get the accelgroup that is
 * created.
 *
 * Returns: A menu widget.
 *
 */
GtkWidget *
mate_popup_menu_new (MateUIInfo *uiinfo)
{
  return mate_popup_menu_new_with_accelgroup(uiinfo, NULL);
}

/**
 * mate_popup_menu_get_accel_group
 * @menu: A menu widget.
 *
 * This function is used to retrieve the accelgroup that was created by
 * mate_popup_menu_new(). If you want to specify the accelgroup that the popup
 * menu accelerators use, then use mate_popup_menu_new_with_accelgroup().
 *
 * Returns: The accelgroup associated with the specified #GtkMenu.
 */
GtkAccelGroup *
mate_popup_menu_get_accel_group(GtkMenu *menu)
{
	g_return_val_if_fail (menu != NULL, NULL);
        g_return_val_if_fail (GTK_IS_MENU (menu), NULL);

#if GTK_CHECK_VERSION(1,2,1)
        return gtk_menu_get_accel_group (menu);
#else
	return NULL;
#endif
}

/* Callback used when a button is pressed in a widget attached to a popup
 * menu.  It decides whether the menu should be popped up and does the
 * appropriate stuff.
 */
static gint
real_popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkWidget *popup;
	gpointer user_data;

	popup = data;

	/*
	 * Fetch the attachment user data from the widget and install
	 * it in the popup menu -- it will be used by the callback
	 * mashaller to pass the data to the callbacks.
	 *
	 */

	user_data = g_object_get_data (G_OBJECT (widget), "mate_popup_menu_attach_user_data");

	mate_popup_menu_do_popup (popup, NULL, NULL, event, user_data, widget);

	return TRUE;
}

static gint
popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	if (event->button != 3)
		return FALSE;

	g_signal_stop_emission_by_name (widget, "button_press_event");

	return real_popup_button_pressed (widget, event, data);
}

static gint
popup_menu_pressed (GtkWidget *widget, gpointer data)
{
	g_signal_stop_emission_by_name (widget, "popup_menu");

	return real_popup_button_pressed (widget, NULL, data);
}

static gint
relay_popup_button_pressed (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
        GtkWidget *new_widget = NULL;

	if (event->button != 3)
		return FALSE;

	g_signal_stop_emission_by_name (widget, "button_press_event");

	if (GTK_IS_CONTAINER(widget))
	  {

	    do {
	      GList *children;
	      GList *child;

	      children = gtk_container_get_children (GTK_CONTAINER (widget));

	      for (child = children, new_widget = NULL; child; child = child->next)
		{
		  GtkWidget *cur;

		  cur = (GtkWidget *)child->data;

		  if(! GTK_WIDGET_NO_WINDOW(cur) )
		    continue;

		  if(cur->allocation.x <= event->x
		     && cur->allocation.y <= event->y
		     && (cur->allocation.x + cur->allocation.width) > event->x
		     && (cur->allocation.y + cur->allocation.height) > event->y
		     && g_object_get_data (G_OBJECT(cur), "mate_popup_menu_nowindow"))
		    {
		      new_widget = cur;
		      break;
		    }
		}
	      if(new_widget)
		widget = new_widget;
	      else
		break;
	    } while(widget && GTK_IS_CONTAINER (widget) && GTK_WIDGET_NO_WINDOW(widget));

	    if(!widget || !g_object_get_data (G_OBJECT(widget), "mate_popup_menu"))
	      {
		return TRUE;
	      }
	  }
	else
	  new_widget = widget;

	return real_popup_button_pressed (new_widget, event, data);
}


/* Callback used to unref the popup menu when the widget it is attached to gets destroyed */
static void
popup_attach_widget_destroyed (GtkWidget *widget, gpointer data)
{
	GtkWidget *popup;

	popup = data;

	g_object_unref (G_OBJECT (popup));
}

/**
 * mate_popup_menu_attach:
 * @popup: A menu widget.
 * @widget: The widget to attach the popup menu to.
 * @user_data: Application specific data passed to the callback.
 *
 * Attaches the specified popup menu to the specified widget.  The
 * menu can then be activated by pressing mouse button 3 over the
 * widget.  When a menu item callback is invoked, the specified
 * user_data will be passed to it.
 *
 * This function requires the widget to have its own window
 * (i.e. GTK_WIDGET_NO_WINDOW (widget) == FALSE), This function will
 * try to set the GDK_BUTTON_PRESS_MASK flag on the widget's event
 * mask if it does not have it yet; if this is the case, then the
 * widget must not be realized for it to work.
 *
 * The popup menu can be attached to different widgets at the same
 * time.  A reference count is kept on the popup menu; when all the
 * widgets it is attached to are destroyed, the popup menu will be
 * destroyed as well.
 *
 * Under the current implementation, setting a popup menu for a NO_WINDOW
 * widget and then reparenting that widget will cause Bad Things to happen.
 */
void
mate_popup_menu_attach (GtkWidget *popup, GtkWidget *widget,
			 gpointer user_data)
{
        GtkWidget *ev_widget;

	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_MENU (popup));
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	if(g_object_get_data (G_OBJECT (widget), "mate_popup_menu"))
	  return;

	g_object_set_data (G_OBJECT (widget), "mate_popup_menu", popup);

	/* This operation can fail if someone is trying to set a popup on e.g. an uncontained label, so we do it first. */
	for(ev_widget = widget; ev_widget && GTK_WIDGET_NO_WINDOW(ev_widget); ev_widget = ev_widget->parent)
	  {
	    g_object_set_data (G_OBJECT (ev_widget), "mate_popup_menu_nowindow", GUINT_TO_POINTER(1));
	  }

	g_return_if_fail (ev_widget);

	/* Ref/sink the popup menu so that we take "ownership" of it */

	g_object_ref_sink (popup);

	/* Store the user data pointer in the widget -- we will use it later when the menu has to be
	 * invoked.
	 */

	g_object_set_data (G_OBJECT (widget), "mate_popup_menu_attach_user_data", user_data);
	g_object_set_data (G_OBJECT (widget), "mate_popup_menu", user_data);

	/* Prepare the widget to accept button presses -- the proper assertions will be
	 * shouted by gtk_widget_set_events().
	 */

	gtk_widget_add_events (ev_widget, GDK_BUTTON_PRESS_MASK |
			       GDK_KEY_PRESS_MASK);

	g_signal_connect (widget, "button_press_event",
			  G_CALLBACK (popup_button_pressed), popup);

	g_signal_connect (G_OBJECT (widget), "popup_menu",
			  G_CALLBACK (popup_menu_pressed), popup);

	if (ev_widget != widget) {
		GClosure *closure;

		closure = g_cclosure_new (G_CALLBACK (relay_popup_button_pressed),
					  popup,
					  NULL);
		g_object_watch_closure (G_OBJECT (widget), closure);
		g_signal_connect_closure (ev_widget, "button_press_event",
					  closure, FALSE);
	}

	/* This callback will unref the popup menu when the widget it is attached to gets destroyed. */
	g_signal_connect (widget, "destroy",
			  G_CALLBACK (popup_attach_widget_destroyed),
			  popup);
}

/**
 * mate_popup_menu_do_popup:
 * @popup: A menu widget.
 * @pos_func: A user supplied function to position the menu or %NULL.
 * @pos_data: User supplied data to pass to @pos_func.
 * @event: A #GdkEventButton structure containing the event that triggered the
 * menu (or %NULL).
 * @user_data: Application specific data passed to the menu callback.
 * @for_widget: The widget for which the popup was triggered.
 *
 * You can use this function to pop up a menu.  When a menu item
 * callback is invoked, the specified user_data will be passed to it.
 *
 * The pos_func and pos_data parameters are the same as for
 * gtk_menu_popup(), i.e. you can use them to specify a function to
 * position the menu explicitly.  If you want the default position
 * (near the mouse), pass NULL for these parameters.
 *
 * The event parameter is needed to figure out the mouse button that
 * activated the menu and the time at which this happened.  If you
 * pass in NULL, then no button and the current time will be used as
 * defaults.
 */
void
mate_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
			   GdkEventButton *event, gpointer user_data, GtkWidget *for_widget)
{
	guint button;
	guint32 timestamp;

	g_return_if_fail (popup != NULL);
	g_return_if_fail (GTK_IS_WIDGET (popup));

	/* Store the user data in the menu for when a callback is activated -- if it is a
	 * Mate-generated menu, then the user data will be passed on to callbacks by our custom
	 * marshaller.
	 */

	g_object_set_data (G_OBJECT (popup), "mate_popup_menu_do_popup_user_data", user_data);
	g_object_set_data (G_OBJECT (popup), "mate_popup_menu_do_popup_for_widget", for_widget);

	if (event) {
		button = event->button;
		timestamp = event->time;
	} else {
		button = 0;
		timestamp = GDK_CURRENT_TIME;
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, pos_func, pos_data, button, timestamp);
}

/* Convenience callback to exit the main loop of a modal popup menu when it is deactivated*/
static void
menu_shell_deactivated (GtkMenuShell *menu_shell, gpointer data)
{
	gtk_main_quit ();
}

/* Returns the index of the active item in the specified menu, or -1 if none is active */
static int
get_active_index (GtkMenu *menu)
{
	GList *l;
	GtkWidget *active;
	int i;

	active = g_object_get_data (G_OBJECT (menu), "mate_popup_menu_active_item");

	for (i = 0, l = GTK_MENU_SHELL (menu)->children; l; l = l->next, i++)
		if (active == l->data)
			return i;

	return -1;
}

/**
 * mate_popup_menu_do_popup_modal:
 * @popup: A menu widget.
 * @pos_func: A user supplied function to position the menu or %NULL.
 * @pos_data: User supplied data to pass to @pos_func.
 * @event: A #GdkEventButton structure containing the event that triggered the
 * menu (or %NULL).
 * @user_data: Application specific data passed to the menu callback.
 * @for_widget: The widget for which the popup was triggered.
 *
 * Same as mate_popup_do_modal(), but runs the popup menu modally.
 *
 * Returns: The index of the selected item or -1 if no item selected.
 */
int
mate_popup_menu_do_popup_modal (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				 GdkEventButton *event, gpointer user_data, GtkWidget *for_widget)
{
	guint id;
	guint button;
	guint32 timestamp;

	g_return_val_if_fail (popup != NULL, -1);
	g_return_val_if_fail (GTK_IS_WIDGET (popup), -1);

	/* Connect to the deactivation signal to be able to quit our modal main loop */

	id = g_signal_connect (popup, "deactivate",
			       G_CALLBACK (menu_shell_deactivated),
			       NULL);

	g_object_set_data (G_OBJECT (popup), "mate_popup_menu_active_item", NULL);
	g_object_set_data (G_OBJECT (popup), "mate_popup_menu_do_popup_user_data", user_data);
	g_object_set_data (G_OBJECT (popup), "mate_popup_menu_do_popup_for_widget", for_widget);

	if (event) {
		button = event->button;
		timestamp = event->time;
	} else {
		button = 0;
		timestamp = GDK_CURRENT_TIME;
	}
#ifdef HAVE_GTK_MULTIHEAD
	gtk_menu_set_screen (GTK_MENU (popup), gtk_widget_get_screen (for_widget));
#endif	
	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, pos_func, pos_data, button, timestamp);
	gtk_grab_add (popup);
	gtk_main ();
	gtk_grab_remove (popup);

	g_signal_handler_disconnect (G_OBJECT (popup), id);

	return get_active_index (GTK_MENU (popup));
}

/**
 * mate_popup_menu_append:
 * @popup: An existing popup menu.
 * @uiinfo: A #MateUIInfo array describing new menu elements.
 *
 * Appends the menu items in @uiinfo the @popup menu.
 */
void
mate_popup_menu_append (GtkWidget *popup, MateUIInfo *uiinfo)
{
	MateUIBuilderData uibdata;
	gint length;

	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = popup_connect_func;
	uibdata.data = NULL;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	for (length = 0; uiinfo[length].type != MATE_APP_UI_ENDOFINFO; length++)
		if (uiinfo[length].type == MATE_APP_UI_ITEM_CONFIGURABLE)
			mate_app_ui_configure_configurable (uiinfo + length);

        global_menushell_hack = popup;
	mate_app_fill_menu_custom (GTK_MENU_SHELL (popup), uiinfo,
				    &uibdata, gtk_menu_get_accel_group(GTK_MENU(popup)), FALSE, 0);
        global_menushell_hack = NULL;
}


/**
 * mate_gtk_widget_add_popup_items:
 * @widget: The widget to append popup menu items for.
 * @uiinfo: A #MateUIInfo array describing new menu elements.
 * @user_data: The user_data to pass to the callbacks for the menu items.
 *
 * This creates a new popup menu for @widget if none exists, and then adds the
 * items in @uiinfo to the popup menu of @widget.
 */
void
mate_gtk_widget_add_popup_items (GtkWidget *widget, MateUIInfo *uiinfo, gpointer user_data)
{
  GtkWidget *prev_popup;

  prev_popup = g_object_get_data (G_OBJECT (widget), "mate_popup_menu");

  if(!prev_popup)
    {
      prev_popup = mate_popup_menu_new(uiinfo);
      mate_popup_menu_attach(prev_popup, widget, user_data);
    }
  else
    mate_popup_menu_append(prev_popup, uiinfo);
}
