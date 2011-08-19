/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mate-mdi.c - implementation of the MateMDI object

   Copyright (C) 1997 - 2001 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Jaka Mocnik <jaka@gnu.org>
*/

#include <config.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <libmate/mate-macros.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glib/gi18n-lib.h>

#include <libmate/mate-config.h>
#include <libmate/mate-util.h>
#include "mate-marshal.h"
#include "mate-app.h"
#include "mate-app-helper.h"
#include <matecomponent/matecomponent-dock-layout.h>
#include "mate-mdi.h"
#include "mate-mdi-child.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void            mate_mdi_destroy        (GtkObject *);
static void            mate_mdi_finalize       (GObject *);

static void            child_list_menu_create     (MateMDI *, MateApp *);
static void            child_list_activated_cb    (GtkWidget *, MateMDI *);
void                   _mate_mdi_child_list_menu_remove_item(MateMDI *, MateMDIChild *);
void                   _mate_mdi_child_list_menu_add_item   (MateMDI *, MateMDIChild *);
static GtkWidget      *find_item_by_child        (GtkMenuShell *, MateMDIChild *);

static void            app_create               (MateMDI *, gchar *);
static void            app_clone                (MateMDI *, MateApp *);
static void            app_destroy              (MateApp *, MateMDI *);
static void            app_set_view             (MateMDI *, MateApp *, GtkWidget *);

static gint            app_close_top            (MateApp *, GdkEventAny *, MateMDI *);
static gint            app_close_book           (MateApp *, GdkEventAny *, MateMDI *);

static GtkWidget       *book_create             (MateMDI *);
static void            book_switch_page         (GtkNotebook *, GtkNotebookPage *,
						 gint, MateMDI *);
static gint            book_motion              (GtkWidget *widget, GdkEventMotion *e,
						 gpointer data);
static gint            book_button_press        (GtkWidget *widget, GdkEventButton *e,
						 gpointer data);
static gint            book_button_release      (GtkWidget *widget, GdkEventButton *e,
						 gpointer data);
static void            book_add_view            (GtkNotebook *, GtkWidget *);
static void            set_page_by_widget       (GtkNotebook *, GtkWidget *);

static void            toplevel_focus           (MateApp *, GdkEventFocus *, MateMDI *);

static void            top_add_view             (MateMDI *, MateMDIChild *, GtkWidget *);

static void            set_active_view          (MateMDI *, GtkWidget *);

static MateUIInfo     *copy_ui_info_tree       (const MateUIInfo *);
static void            free_ui_info_tree        (MateUIInfo *);
static gint            count_ui_info_items      (const MateUIInfo *);

/* convenience functions that call child's "virtual" functions */
static GList           *child_create_menus      (MateMDIChild *, GtkWidget *);
static GtkWidget       *child_set_label         (MateMDIChild *, GtkWidget *);

/* keys for stuff that we'll assign to our MateApps */
#define MATE_MDI_TOOLBAR_INFO_KEY           "MDIToolbarUIInfo"
#define MATE_MDI_MENUBAR_INFO_KEY           "MDIMenubarUIInfo"
#define MATE_MDI_CHILD_MENU_INFO_KEY        "MDIChildMenuUIInfo"
#define MATE_MDI_CHILD_KEY                  "MateMDIChild"
#define MATE_MDI_ITEM_COUNT_KEY             "MDIChildMenuItems"

static GdkCursor *drag_cursor = NULL;

enum {
	ADD_CHILD,
	REMOVE_CHILD,
	ADD_VIEW,
	REMOVE_VIEW,
	CHILD_CHANGED,
	VIEW_CHANGED,
	APP_CREATED,
	LAST_SIGNAL
};

typedef gboolean   (*MateMDISignal1) (GtkObject *, gpointer, gpointer);
typedef void       (*MateMDISignal2) (GtkObject *, gpointer, gpointer);

static gint mdi_signals[LAST_SIGNAL];

MATE_CLASS_BOILERPLATE (MateMDI, mate_mdi,
						 GtkObject, GTK_TYPE_OBJECT)

static void mate_mdi_class_init (MateMDIClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass*) class;
	gobject_class = (GObjectClass*) class;

	object_class->destroy = mate_mdi_destroy;
	gobject_class->finalize = mate_mdi_finalize;

	mdi_signals[ADD_CHILD] =
		g_signal_new ("add_child",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, add_child),
					  NULL, NULL,
					  _mate_marshal_BOOLEAN__OBJECT,
					  G_TYPE_BOOLEAN, 1,
					  MATE_TYPE_MDI_CHILD);
	mdi_signals[REMOVE_CHILD] =
		g_signal_new ("remove_child",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, remove_child),
					  NULL, NULL,
					  _mate_marshal_BOOLEAN__OBJECT,
					  G_TYPE_BOOLEAN, 1,
					  MATE_TYPE_MDI_CHILD);
	mdi_signals[ADD_VIEW] =
		g_signal_new ("add_view",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, add_view),
					  NULL, NULL,
					  _mate_marshal_BOOLEAN__OBJECT,
					  G_TYPE_BOOLEAN, 1,
					  GTK_TYPE_WIDGET);
	mdi_signals[REMOVE_VIEW] =
		g_signal_new ("remove_view",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, remove_view),
					  NULL, NULL,
					  _mate_marshal_BOOLEAN__OBJECT,
					  G_TYPE_BOOLEAN, 1,
					  GTK_TYPE_WIDGET);
	mdi_signals[CHILD_CHANGED] =
		g_signal_new ("child_changed",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, child_changed),
					  NULL, NULL,
					  _mate_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  MATE_TYPE_MDI_CHILD);
	mdi_signals[VIEW_CHANGED] =
		g_signal_new ("view_changed",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET(MateMDIClass, view_changed),
					  NULL, NULL,
					  _mate_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  GTK_TYPE_WIDGET);
	mdi_signals[APP_CREATED] =
		g_signal_new ("app_created",
					  G_TYPE_FROM_CLASS (gobject_class),
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET (MateMDIClass, app_created),
					  NULL, NULL,
					  _mate_marshal_VOID__OBJECT,
					  G_TYPE_NONE, 1,
					  MATE_TYPE_APP);

	class->add_child = NULL;
	class->remove_child = NULL;
	class->add_view = NULL;
	class->remove_view = NULL;
	class->child_changed = NULL;
	class->view_changed = NULL;
	class->app_created = NULL;

	parent_class = g_type_class_peek_parent (class);
}

static void mate_mdi_finalize (GObject *object)
{
    MateMDI *mdi;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_MDI(object));

#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDI: finalization!\n");
#endif

	mdi = MATE_MDI(object);

	g_free (mdi->child_menu_path);
	mdi->child_menu_path = NULL;
	g_free (mdi->child_list_path);
	mdi->child_list_path = NULL;

	g_free (mdi->appname);
	mdi->appname = NULL;
	g_free (mdi->title);
	mdi->title = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void mate_mdi_destroy (GtkObject *object)
{
	MateMDI *mdi;

	g_return_if_fail(object != NULL);
	g_return_if_fail(MATE_IS_MDI (object));

	mdi = MATE_MDI(object);

	mate_mdi_remove_all(mdi, TRUE);

	g_object_ref_sink(GTK_OBJECT(object));

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void mate_mdi_instance_init (MateMDI *mdi)
{
	/* FIXME!
	mdi->mode = mate_preferences_get_mdi_mode();
	mdi->tab_pos = mate_preferences_get_mdi_tab_pos();
	 */
	mdi->mode = MATE_MDI_NOTEBOOK;

	mdi->signal_id = 0;
	mdi->in_drag = FALSE;

	mdi->children = NULL;
	mdi->windows = NULL;
	mdi->registered = NULL;

	mdi->active_child = NULL;
	mdi->active_window = NULL;
	mdi->active_view = NULL;

	mdi->menu_template = NULL;
	mdi->toolbar_template = NULL;
	mdi->child_menu_path = NULL;
	mdi->child_list_path = NULL;
}

/**
 * mate_mdi_new:
 * @appname: Application name as used in filenames and paths.
 * @title: Title of the application windows.
 *
 * Description:
 * Creates a new MDI object. @appname and @title are used for
 * MDI's calling mate_app_new().
 *
 * Return value:
 * A pointer to a new MateMDI object.
 **/
GtkObject *mate_mdi_new(const gchar *appname, const gchar *title) {
	MateMDI *mdi;

	mdi = g_object_new (MATE_TYPE_MDI, NULL);

	mdi->appname = g_strdup(appname);
	mdi->title = g_strdup(title);

	return GTK_OBJECT (mdi);
}

static GList *child_create_menus (MateMDIChild *child, GtkWidget *view)
{
	if(MATE_MDI_CHILD_GET_CLASS(child)->create_menus)
		return MATE_MDI_CHILD_GET_CLASS(child)->create_menus(child, view, NULL);

	return NULL;
}

static GtkWidget *child_set_label (MateMDIChild *child, GtkWidget *label)
{
	return MATE_MDI_CHILD_GET_CLASS(child)->set_label(child, label, NULL);
}

/* the app-helper support routines
 * copying and freeing of MateUIInfo trees and counting items in them.
 */
static MateUIInfo *copy_ui_info_tree (const MateUIInfo source[])
{
	MateUIInfo *copy;
	int i, count;

	for(count = 0; source[count].type != MATE_APP_UI_ENDOFINFO; count++)
		;

	count++;

	copy = g_malloc(count*sizeof(MateUIInfo));

	memcpy(copy, source, count*sizeof(MateUIInfo));

	for(i = 0; i < count; i++) {
		if( (source[i].type == MATE_APP_UI_SUBTREE) ||
			(source[i].type == MATE_APP_UI_SUBTREE_STOCK) ||
			(source[i].type == MATE_APP_UI_RADIOITEMS) )
			copy[i].moreinfo = copy_ui_info_tree(source[i].moreinfo);
	}

	return copy;
}


static gint count_ui_info_items (const MateUIInfo *ui_info)
{
	gint num;
	gint count=0;

	for(num = 0; ui_info[num].type != MATE_APP_UI_ENDOFINFO; num++)
		if (ui_info[num].type != MATE_APP_UI_HELP &&
			ui_info[num].type != MATE_APP_UI_BUILDER_DATA)
			count++;

	return count;
}

static void free_ui_info_tree (MateUIInfo *root)
{
	int count;

	for(count = 0; root[count].type != MATE_APP_UI_ENDOFINFO; count++)
		if( (root[count].type == MATE_APP_UI_SUBTREE) ||
			(root[count].type == MATE_APP_UI_SUBTREE_STOCK) ||
			(root[count].type == MATE_APP_UI_RADIOITEMS) )
			free_ui_info_tree(root[count].moreinfo);

	g_free(root);
}

static void set_page_by_widget (GtkNotebook *book, GtkWidget *child)
{
	gint i;

#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDI: set_page_by_widget");
#endif

	i = gtk_notebook_page_num(book, child);
	if(i != gtk_notebook_get_current_page(book))
		gtk_notebook_set_current_page(book, i);
}

static GtkWidget *find_item_by_child (GtkMenuShell *shell, MateMDIChild *child)
{
	GList *node;

	node = shell->children;
	while(node) {
		if(g_object_get_data(G_OBJECT(node->data), MATE_MDI_CHILD_KEY) == child)
			return GTK_WIDGET(node->data);

		node = node->next;
	}

	return NULL;
}

static void child_list_activated_cb (GtkWidget *w, MateMDI *mdi)
{
	MateMDIChild *child;

	child = g_object_get_data(G_OBJECT(w), MATE_MDI_CHILD_KEY);

	if( child && (child != mdi->active_child) ) {
		if(child->views)
			mate_mdi_set_active_view(mdi, child->views->data);
		else
			mate_mdi_add_view(mdi, child);
	}
}

static void child_list_menu_create (MateMDI *mdi, MateApp *app)
{
	GtkWidget *submenu, *item, *label;
	GList *child;
	gint pos;

	if(mdi->child_list_path == NULL)
		return;

	submenu = mate_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);

	if(submenu == NULL)
		return;

	child = mdi->children;
	while(child) {
		item = gtk_menu_item_new();
		g_signal_connect(item, "activate",
						 G_CALLBACK(child_list_activated_cb), mdi);
		label = child_set_label(MATE_MDI_CHILD(child->data), NULL);
		gtk_widget_show(label);
		gtk_container_add(GTK_CONTAINER(item), label);
		g_object_set_data (G_OBJECT (item), MATE_MDI_CHILD_KEY, child->data);
		gtk_widget_show(item);

		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);

		child = g_list_next(child);
	}

	gtk_widget_queue_resize(submenu);
}

void
_mate_mdi_child_list_menu_remove_item (MateMDI *mdi, MateMDIChild *child)
{
	GtkWidget *item, *shell;
	MateApp *app;
	GList *app_node;
	gint pos;

	if(mdi->child_list_path == NULL)
		return;

	app_node = mdi->windows;
	while(app_node) {
		app = MATE_APP(app_node->data);

		shell = mate_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);


		if(shell) {
			item = find_item_by_child(GTK_MENU_SHELL(shell), child);
			if(item) {
				gtk_container_remove(GTK_CONTAINER(shell), item);
				gtk_widget_queue_resize (GTK_WIDGET (shell));
			}
		}

		app_node = app_node->next;
	}
}

void
_mate_mdi_child_list_menu_add_item (MateMDI *mdi, MateMDIChild *child)
{
	GtkWidget *item, *submenu, *label;
	MateApp *app;
	GList *app_node;
	gint pos;

	if(mdi->child_list_path == NULL)
		return;

	app_node = mdi->windows;
	while(app_node) {
		app = MATE_APP(app_node->data);

		submenu = mate_app_find_menu_pos(app->menubar, mdi->child_list_path, &pos);

		if(submenu) {
			item = gtk_menu_item_new();
			g_signal_connect(item, "activate",
							 G_CALLBACK(child_list_activated_cb), mdi);
			label = child_set_label(child, NULL);
			gtk_widget_show(label);
			gtk_container_add(GTK_CONTAINER(item), label);
			g_object_set_data (G_OBJECT (item), MATE_MDI_CHILD_KEY, child);
			gtk_widget_show(item);

			gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
			gtk_widget_queue_resize(submenu);
		}

		app_node = app_node->next;
	}
}

static gint book_motion (GtkWidget *widget, GdkEventMotion *e, gpointer data)
{
	MateMDI *mdi;

	mdi = MATE_MDI(data);

	if(!drag_cursor)
		drag_cursor = gdk_cursor_new(GDK_HAND2);

	if(e->window == widget->window) {
		mdi->in_drag = TRUE;
		gtk_grab_add(widget);
		gdk_pointer_grab(widget->window, FALSE,
						 GDK_POINTER_MOTION_MASK |
						 GDK_BUTTON_RELEASE_MASK, NULL,
						 drag_cursor, GDK_CURRENT_TIME);
		if(mdi->signal_id) {
			g_signal_handler_disconnect(widget, mdi->signal_id);
			mdi->signal_id = 0;
		}
	}

	return FALSE;
}

static gint book_button_press (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	MateMDI *mdi;

	mdi = MATE_MDI(data);

	if(e->button == 1 && e->window == widget->window)
		mdi->signal_id = g_signal_connect(widget, "motion_notify_event",
										  G_CALLBACK(book_motion), mdi);

	return FALSE;
}

static gint book_button_release (GtkWidget *widget, GdkEventButton *e, gpointer data)
{
	gint x = e->x_root, y = e->y_root;
	MateMDI *mdi;

	mdi = MATE_MDI(data);

	if(mdi->signal_id) {
		g_signal_handler_disconnect(widget, mdi->signal_id);
		mdi->signal_id = 0;
	}

	if(e->button == 1 && e->window == widget->window && mdi->in_drag) {
		GdkWindow *window;
		GList *child;
		MateApp *app;
		GtkWidget *view, *new_book;
		GtkNotebook *old_book = GTK_NOTEBOOK(widget);

		mdi->in_drag = FALSE;
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_grab_remove(widget);

		window = gdk_window_at_pointer(&x, &y);
		if(window)
			window = gdk_window_get_toplevel(window);

		child = mdi->windows;
		while(child) {
			if(window == GTK_WIDGET(child->data)->window) {
				int cur_page;

				/* page was dragged to another notebook */

				old_book = GTK_NOTEBOOK(widget);
				new_book = MATE_APP(child->data)->contents;

				if(old_book == (GtkNotebook *)new_book) /* page has been dropped on the source notebook */
					return FALSE;

				cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (old_book));

				if(cur_page >= 0) {
					view = gtk_notebook_get_nth_page (GTK_NOTEBOOK (old_book), cur_page);
					gtk_container_remove(GTK_CONTAINER(old_book), view);

					book_add_view(GTK_NOTEBOOK(new_book), view);

					app = mate_mdi_get_app_from_view(view);
					gdk_window_raise(GTK_WIDGET(app)->window);

					cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (old_book));

					if(cur_page < 0) {
						mdi->active_window = app;
						app = MATE_APP(gtk_widget_get_toplevel(GTK_WIDGET(old_book)));
						mdi->windows = g_list_remove(mdi->windows, app);
						gtk_widget_destroy(GTK_WIDGET(app));
					}
				}

				return FALSE;
			}

			child = child->next;
		}

		if(g_list_length(old_book->children) == 1)
			return FALSE;

		/* create a new toplevel */
		if(old_book->cur_page) {
			gint width, height;
			int cur_page = gtk_notebook_get_current_page (old_book);

			view = gtk_notebook_get_nth_page (old_book, cur_page);

			app = mate_mdi_get_app_from_view(view);

			width = view->allocation.width;
			height = view->allocation.height;

			gtk_container_remove(GTK_CONTAINER(old_book), view);

			app_clone(mdi, app);

			new_book = book_create(mdi);

			book_add_view(GTK_NOTEBOOK(new_book), view);

			gtk_window_set_position(GTK_WINDOW(mdi->active_window), GTK_WIN_POS_MOUSE);

			gtk_widget_set_size_request (view, width, height);

			if(!GTK_WIDGET_VISIBLE(mdi->active_window))
				gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
	}

	return FALSE;
}

static GtkWidget *book_create (MateMDI *mdi)
{
	GtkWidget *us;

	us = gtk_notebook_new();

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(us), mdi->tab_pos);

	gtk_widget_show(us);

	mate_app_set_contents(mdi->active_window, us);

	gtk_widget_add_events(us, GDK_BUTTON1_MOTION_MASK);

	g_signal_connect(us, "switch_page",
					 G_CALLBACK(book_switch_page), mdi);
	g_signal_connect(us, "button_press_event",
					 G_CALLBACK(book_button_press), mdi);
	g_signal_connect(us, "button_release_event",
					 G_CALLBACK(book_button_release), mdi);

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(us), TRUE);

	return us;
}

static void book_add_view (GtkNotebook *book, GtkWidget *view)
{
	MateMDIChild *child;
	GtkWidget *title;

	child = mate_mdi_get_child_from_view(view);

	title = child_set_label(child, NULL);

	gtk_notebook_append_page(book, view, title);

	if(g_list_length(book->children) > 1)
		set_page_by_widget(book, view);
}

static void book_switch_page(GtkNotebook *book, GtkNotebookPage *pg, gint page_num, MateMDI *mdi)
{
	MateApp *app;
	GtkWidget *page;

#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDI: switching pages");
#endif
	app = MATE_APP(gtk_widget_get_toplevel(GTK_WIDGET(book)));

	page = gtk_notebook_get_nth_page (book, page_num);

	if(page != NULL) {
		if(page != mdi->active_view)
			app_set_view(mdi, app, page);
	} else {
		app_set_view(mdi, app, NULL);
	}
}

static void toplevel_focus (MateApp *app, GdkEventFocus *event, MateMDI *mdi)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_if_fail(MATE_IS_APP(app));
#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDI: toplevel receiving focus");
#endif
	mdi->active_window = app;

	if((mdi->mode == MATE_MDI_TOPLEVEL) || (mdi->mode == MATE_MDI_MODAL)) {
		set_active_view(mdi, app->contents);
	} else if((mdi->mode == MATE_MDI_NOTEBOOK) && GTK_NOTEBOOK(app->contents)->cur_page) {
		int cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (app->contents));
		GtkWidget *child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (app->contents), cur_page);
		set_active_view(mdi, child);
	} else {
		set_active_view(mdi, NULL);
	}
}

static void app_clone(MateMDI *mdi, MateApp *app)
{
	MateComponentDockLayout *layout;
	gchar *layout_string = NULL;

	if(app) {
		layout = matecomponent_dock_get_layout(MATECOMPONENT_DOCK(app->dock));
		layout_string = matecomponent_dock_layout_create_string(layout);
		g_object_unref (G_OBJECT(layout));
	}

	app_create(mdi, layout_string);
	g_free(layout_string);
}

static gint app_close_top (MateApp *app, GdkEventAny *event, MateMDI *mdi)
{
	MateMDIChild *child = NULL;

	if(g_list_length(mdi->windows) == 1) {
		if(!mate_mdi_remove_all(mdi, FALSE))
			return TRUE;

		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));

		/* only destroy mdi if there are no external objects registered
		   with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else if(app->contents) {
		child = mate_mdi_get_child_from_view(app->contents);
		if(g_list_length(child->views) == 1) {
			/* if this is the last view, we have to remove the child! */
			if(!mate_mdi_remove_child(mdi, child, FALSE))
				return TRUE;
		}
		else
			mate_mdi_remove_view(mdi, app->contents, FALSE);
	}
	else {
		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));
	}

	return FALSE;
}

static gint app_close_book (MateApp *app, GdkEventAny *event, MateMDI *mdi)
{
	MateMDIChild *child;
	GtkWidget *view;
	gint handler_ret = TRUE;

	if(g_list_length(mdi->windows) == 1) {
		if(!mate_mdi_remove_all(mdi, FALSE))
			return TRUE;

		mdi->windows = g_list_remove(mdi->windows, app);
		gtk_widget_destroy(GTK_WIDGET(app));

		/* only destroy mdi if there are no non-MDI windows registered
		   with it. */
		if(mdi->registered == NULL)
			gtk_object_destroy(GTK_OBJECT(mdi));
	}
	else {
		GList *children = gtk_container_get_children (GTK_CONTAINER (app->contents));
		GList *li;

		/* first check if all the children in this notebook can be removed */
		if (children == NULL) {
			mdi->windows = g_list_remove(mdi->windows, app);
			gtk_widget_destroy(GTK_WIDGET(app));
			return FALSE;
		}

		for (li = children; li != NULL; li = li->next) {
			GList *node;
			view = li->data;

			child = mate_mdi_get_child_from_view(view);

			node = child->views;
			while(node) {
				if(mate_mdi_get_app_from_view(node->data) != app)
					break;

				node = node->next;
			}

			if(node == NULL) {   /* all the views reside in this MateApp */
				g_signal_emit(mdi, mdi_signals[REMOVE_CHILD], 0,
							  child,
							  &handler_ret);
				if(handler_ret == FALSE) {
					g_list_free (children);
					return TRUE;
				}
			}
		}

		/* now actually remove all children/views! */
		for (li = children; li != NULL; li = li->next) {
			view = li->data;

			child = mate_mdi_get_child_from_view(view);

			/* if this is the last view, remove the child */
			if(g_list_length(child->views) == 1)
				mate_mdi_remove_child(mdi, child, TRUE);
			else
				mate_mdi_remove_view(mdi, view, TRUE);
		}

		g_list_free (children);
	}

	return FALSE;
}

static void app_set_view (MateMDI *mdi, MateApp *app, GtkWidget *view)
{
	GList *menu_list = NULL, *children;
	GtkWidget *parent = NULL;
	MateMDIChild *child;
	MateUIInfo *ui_info;
	gint pos, items;

	/* free previous child ui-info */
	ui_info = g_object_get_data(G_OBJECT(app), MATE_MDI_CHILD_MENU_INFO_KEY);
	if(ui_info != NULL) {
		free_ui_info_tree(ui_info);
		g_object_set_data(G_OBJECT(app), MATE_MDI_CHILD_MENU_INFO_KEY, NULL);
	}
	ui_info = NULL;

	if(mdi->child_menu_path)
		parent = mate_app_find_menu_pos(app->menubar, mdi->child_menu_path, &pos);

	/* remove old child-specific menus */
	items = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(app), MATE_MDI_ITEM_COUNT_KEY));
	if(items > 0 && parent) {
		GtkWidget *widget;

		/* remove items; should be kept in sync with mate_app_remove_menus! */
		children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos);
		while(children && items > 0) {
			widget = GTK_WIDGET(children->data);
			children = children->next;

			/* if this item contains a gtkaccellabel, we have to set its
			   accel_widget to NULL so that the item gets unrefed. */
			if(GTK_IS_ACCEL_LABEL(GTK_BIN(widget)->child))
				gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(GTK_BIN(widget)->child), NULL);

			gtk_container_remove(GTK_CONTAINER(parent), widget);
			items--;
		}
	}

	items = 0;
	if(view) {
		child = mate_mdi_get_child_from_view(view);

		/* set the title */
		if( (mdi->mode == MATE_MDI_MODAL) || (mdi->mode == MATE_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;

			fullname = g_strconcat(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(app), fullname);
			g_free(fullname);
		}
		else
			gtk_window_set_title(GTK_WINDOW(app), mdi->title);

		/* create new child-specific menus */
		if(parent) {
			if( child->menu_template &&
				( (ui_info = copy_ui_info_tree(child->menu_template)) != NULL) ) {
				mate_app_insert_menus_with_data(app, mdi->child_menu_path, ui_info, child);
				g_object_set_data(G_OBJECT(app), MATE_MDI_CHILD_MENU_INFO_KEY, ui_info);
				items = count_ui_info_items(ui_info);
			}
			else {
				menu_list = child_create_menus(child, view);

				if(menu_list) {
					GList *menu;

					items = 0;
					menu = menu_list;
					while(menu) {
						gtk_menu_shell_insert(GTK_MENU_SHELL(parent), GTK_WIDGET(menu->data), pos);
						menu = menu->next;
						pos++;
						items++;
					}

					g_list_free(menu_list);
				}
				else
					items = 0;
			}
		}
	}
	else
		gtk_window_set_title(GTK_WINDOW(app), mdi->title);

	g_object_set_data(G_OBJECT(app), MATE_MDI_ITEM_COUNT_KEY, GINT_TO_POINTER(items));

	if(parent)
		gtk_widget_queue_resize(parent);

	set_active_view(mdi, view);
}

static void app_destroy (MateApp *app, MateMDI *mdi)
{
	MateUIInfo *ui_info;

	if(mdi->active_window == app)
		mdi->active_window = (mdi->windows != NULL)?MATE_APP(mdi->windows->data):NULL;

	/* free stuff that got allocated for this MateApp */

	ui_info = g_object_get_data(G_OBJECT(app), MATE_MDI_MENUBAR_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);

	ui_info = g_object_get_data(G_OBJECT(app), MATE_MDI_TOOLBAR_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);

	ui_info = g_object_get_data(G_OBJECT(app), MATE_MDI_CHILD_MENU_INFO_KEY);
	if(ui_info)
		free_ui_info_tree(ui_info);
}

static void app_create (MateMDI *mdi, gchar *layout_string)
{
	GtkWidget *window;
	MateApp *app;
	GCallback func = NULL;
	MateUIInfo *ui_info;

	window = mate_app_new(mdi->appname, mdi->title);

	app = MATE_APP(window);

	/* don't do automagical layout saving */
	app->enable_layout_config = FALSE;

	gtk_window_set_wmclass (GTK_WINDOW (window), mdi->appname, mdi->appname);

	mdi->windows = g_list_append(mdi->windows, window);

	if( (mdi->mode == MATE_MDI_TOPLEVEL) || (mdi->mode == MATE_MDI_MODAL))
		func = G_CALLBACK (app_close_top);
	else if(mdi->mode == MATE_MDI_NOTEBOOK)
		func = G_CALLBACK (app_close_book);

	g_signal_connect(window, "delete_event",
					 func, mdi);
	g_signal_connect(window, "focus_in_event",
                     G_CALLBACK(toplevel_focus), mdi);
	g_signal_connect(window, "destroy",
					 G_CALLBACK(app_destroy), mdi);

	/* set up menus */
	if(mdi->menu_template) {
		ui_info = copy_ui_info_tree(mdi->menu_template);
		mate_app_create_menus_with_data(app, ui_info, mdi);
		g_object_set_data(G_OBJECT(window), MATE_MDI_MENUBAR_INFO_KEY, ui_info);
	}

	/* create toolbar */
	if(mdi->toolbar_template) {
		ui_info = copy_ui_info_tree(mdi->toolbar_template);
		mate_app_create_toolbar_with_data(app, ui_info, mdi);
		g_object_set_data(G_OBJECT(window), MATE_MDI_TOOLBAR_INFO_KEY, ui_info);
	}

	mdi->active_window = app;
	mdi->active_child = NULL;
	mdi->active_view = NULL;

	g_signal_emit(mdi, mdi_signals[APP_CREATED], 0,
				  window);

	child_list_menu_create(mdi, app);

	if(layout_string && app->layout)
		matecomponent_dock_layout_parse_string(app->layout, layout_string);
}

static void top_add_view (MateMDI *mdi, MateMDIChild *child, GtkWidget *view)
{
	MateApp *window;

	if (mdi->active_window->contents != NULL)
		app_clone(mdi, mdi->active_window);

	window = mdi->active_window;

	if(child && view)
		mate_app_set_contents(window, view);

	app_set_view(mdi, window, view);

	if(!GTK_WIDGET_VISIBLE(window))
		gtk_widget_show(GTK_WIDGET(window));
}

static void set_active_view (MateMDI *mdi, GtkWidget *view)
{
	MateMDIChild *old_child;
	GtkWidget *old_view;

	old_child = mdi->active_child;
	old_view = mdi->active_view;

	if(!view) {
		mdi->active_view = NULL;
		mdi->active_child = NULL;
	}

	if(view == old_view)
		return;

	if(view) {
		mdi->active_child = mate_mdi_get_child_from_view(view);
		mdi->active_window = mate_mdi_get_app_from_view(view);
	}

	mdi->active_view = view;

	if(mdi->active_child != old_child)
		g_signal_emit(mdi, mdi_signals[CHILD_CHANGED], 0,
					  old_child);

	g_signal_emit(mdi, mdi_signals[VIEW_CHANGED], 0,
				  old_view);
}

/**
 * mate_mdi_set_active_view:
 * @mdi: A pointer to an MDI object.
 * @view: A pointer to the view that is to become the active one.
 *
 * Description:
 * Sets the active view to @view. It also raises the window containing it
 * and gives it focus.
 **/
void mate_mdi_set_active_view (MateMDI *mdi, GtkWidget *view)
{
	GtkWindow *window;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));
	g_return_if_fail(view != NULL);
	g_return_if_fail(GTK_IS_WIDGET(view));

#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDI: setting active view");
#endif

	if(mdi->mode == MATE_MDI_NOTEBOOK)
		set_page_by_widget(GTK_NOTEBOOK(view->parent), view);
	if(mdi->mode == MATE_MDI_MODAL) {
		if(mdi->active_window->contents) {
			mate_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}

		mate_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}

	window = GTK_WINDOW(mate_mdi_get_app_from_view(view));

	/* TODO: hmmm... I dont know how to give focus to the window, so that it
	   would receive keyboard events */
	gdk_window_raise(GTK_WIDGET(window)->window);

	set_active_view(mdi, view);
}

/**
 * mate_mdi_add_view:
 * @mdi: A pointer to a MateMDI object.
 * @child: A pointer to a child.
 *
 * Description:
 * Creates a new view of the child and adds it to the MDI. MateMDIChild
 * @child has to be added to the MDI with a call to mate_mdi_add_child
 * before its views are added to the MDI.
 * An "add_view" signal is emitted to the MDI after the view has been
 * created, but before it is shown and added to the MDI, with a pointer to
 * the created view as its parameter. The view is added to the MDI only if
 * the signal handler (if it exists) returns %TRUE. If the handler returns
 * %FALSE, the created view is destroyed and not added to the MDI.
 *
 * Return value:
 * %TRUE if adding the view succeeded and %FALSE otherwise.
 **/
gint mate_mdi_add_view (MateMDI *mdi, MateMDIChild *child)
{
	GtkWidget *view;
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI_CHILD(child), FALSE);

	if(mdi->mode != MATE_MDI_MODAL || child->views == NULL)
		view = mate_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->views->data);

		if(child == mdi->active_child)
			return TRUE;
	}

	g_signal_emit(mdi, mdi_signals[ADD_VIEW], 0,
				  view,
				  &ret);

	if(ret == FALSE) {
		mate_mdi_child_remove_view(child, view);
		return FALSE;
	}

	if(mdi->active_window == NULL) {
		app_create(mdi, NULL);
		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to mate_mdi_child_remove_view() */
	g_object_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->mode == MATE_MDI_NOTEBOOK) {
		if(mdi->active_window->contents == NULL)
			book_create(mdi);
		book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
	}
	else if(mdi->mode == MATE_MDI_TOPLEVEL)
		/* add a new toplevel unless the remaining one is empty */
		top_add_view(mdi, child, view);
	else if(mdi->mode == MATE_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(mdi->active_window->contents) {
			mate_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}

		mate_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}

	return TRUE;
}

/**
 * mate_mdi_add_toplevel_view:
 * @mdi: A pointer to a MateMDI object.
 * @child: A pointer to a MateMDIChild object to be added to the MDI.
 *
 * Description:
 * Creates a new view of the child and adds it to the MDI; it behaves the
 * same way as mate_mdi_add_view in %MATE_MDI_MODAL and %MATE_MDI_TOPLEVEL
 * modes, but in %MATE_MDI_NOTEBOOK mode, the view is added in a new
 * toplevel window unless the active one has no views in it.
 *
 * Return value:
 * %TRUE if adding the view succeeded and %FALSE otherwise.
 **/
gint mate_mdi_add_toplevel_view (MateMDI *mdi, MateMDIChild *child)
{
	GtkWidget *view;
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI_CHILD(child), FALSE);

	if(mdi->mode != MATE_MDI_MODAL || child->views == NULL)
		view = mate_mdi_child_add_view(child);
	else {
		view = GTK_WIDGET(child->views->data);

		if(child == mdi->active_child)
			return TRUE;
	}

	if(!view)
		return FALSE;

	g_signal_emit(mdi, mdi_signals[ADD_VIEW], 0,
				  view,
				  &ret);

	if(ret == FALSE) {
		mate_mdi_child_remove_view(child, view);
		return FALSE;
	}

	mate_mdi_open_toplevel(mdi);

	/* this reference will compensate the view's unrefing
	   when removed from its parent later, as we want it to
	   stay valid until removed from the child with a call
	   to mate_mdi_child_remove_view() */
	g_object_ref(view);

	if(!GTK_WIDGET_VISIBLE(view))
		gtk_widget_show(view);

	if(mdi->mode == MATE_MDI_NOTEBOOK)
		book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
	else if(mdi->mode == MATE_MDI_TOPLEVEL)
		/* add a new toplevel unless the remaining one is empty */
		top_add_view(mdi, child, view);
	else if(mdi->mode == MATE_MDI_MODAL) {
		/* replace the existing view if there is one */
		if(mdi->active_window->contents) {
			mate_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
			mdi->active_window->contents = NULL;
		}

		mate_app_set_contents(mdi->active_window, view);
		app_set_view(mdi, mdi->active_window, view);
	}

	return TRUE;
}

/**
 * mate_mdi_remove_view:
 * @mdi: A pointer to a MateMDI object.
 * @view: View to remove.
 * @force: If TRUE, the "remove_view" signal is not emitted.
 *
 * Description:
 * Removes a view from an MDI.
 * A "remove_view" signal is emitted to the MDI before actually removing
 * view. The view is removed only if the signal handler (if it exists and
 * the @force is set to %FALSE) returns %TRUE.
 *
 * Return value:
 * %TRUE if the view was removed and %FALSE otherwise.
 **/
gint mate_mdi_remove_view (MateMDI *mdi, GtkWidget *view, gint force)
{
	GtkWidget *parent;
	MateApp *window;
	MateMDIChild *child;
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(view != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_WIDGET(view), FALSE);

	if(!force)
		g_signal_emit(mdi, mdi_signals[REMOVE_VIEW], 0,
					  view,
					  &ret);

	if(ret == FALSE)
		return FALSE;

	child = mate_mdi_get_child_from_view(view);

	parent = view->parent;

	if(!parent)
		return TRUE;

	window = mate_mdi_get_app_from_view(view);

	gtk_container_remove(GTK_CONTAINER(parent), view);

	if(view == mdi->active_view)
	   mdi->active_view = NULL;

	if( (mdi->mode == MATE_MDI_TOPLEVEL) || (mdi->mode == MATE_MDI_MODAL) ) {
		window->contents = NULL;

		/* if this is NOT the last toplevel or a registered object exists,
		   destroy the toplevel */
		if(g_list_length(mdi->windows) > 1 || mdi->registered) {
			mdi->windows = g_list_remove(mdi->windows, window);
			gtk_widget_destroy(GTK_WIDGET(window));
			if(mdi->active_window && view == mdi->active_view)
				mate_mdi_set_active_view(mdi, mate_mdi_get_view_from_window(mdi, mdi->active_window));
		}
		else
			app_set_view(mdi, window, NULL);
	}
	else if(mdi->mode == MATE_MDI_NOTEBOOK) {
		if(GTK_NOTEBOOK(parent)->cur_page == NULL) {
			if(g_list_length(mdi->windows) > 1 || mdi->registered) {
				/* if this is NOT the last toplevel or a registered object
				   exists, destroy the toplevel */
				mdi->windows = g_list_remove(mdi->windows, window);
				gtk_widget_destroy(GTK_WIDGET(window));
				if(mdi->active_window && view == mdi->active_view)
					mdi->active_view = mate_mdi_get_view_from_window(mdi, mdi->active_window);
			}
			else
				app_set_view(mdi, window, NULL);
		}
		else
			app_set_view(mdi, window, gtk_notebook_get_nth_page(GTK_NOTEBOOK(parent), gtk_notebook_get_current_page(GTK_NOTEBOOK(parent))));
	}

	/* remove this view from the child's view list unless in MODAL mode */
	if(mdi->mode != MATE_MDI_MODAL)
		mate_mdi_child_remove_view(child, view);

	return TRUE;
}

/**
 * mate_mdi_add_child:
 * @mdi: A pointer to a MateMDI object.
 * @child: A pointer to a MateMDIChild to add to the MDI.
 *
 * Description:
 * Adds a new child to the MDI. No views are added: this has to be done with
 * a call to mate_mdi_add_view.
 * First an "add_child" signal is emitted to the MDI with a pointer to the
 * child as its parameter. The child is added to the MDI only if the signal
 * handler (if it exists) returns %TRUE. If the handler returns %FALSE, the
 * child is not added to the MDI.
 *
 * Return value:
 * %TRUE if the child was added successfully and %FALSE otherwise.
 **/
gint mate_mdi_add_child (MateMDI *mdi, MateMDIChild *child)
{
	gint ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI_CHILD(child), FALSE);

	g_signal_emit(mdi, mdi_signals[ADD_CHILD], 0,
				  child,
				  &ret);

	if(ret == FALSE)
		return FALSE;

	child->parent = GTK_OBJECT(mdi);

	mdi->children = g_list_append(mdi->children, child);

	_mate_mdi_child_list_menu_add_item(mdi, child);

	return TRUE;
}

/**
 * mate_mdi_remove_child:
 * @mdi: A pointer to a MateMDI object.
 * @child: Child to remove.
 * @force: If TRUE, the "remove_child" signal is not emitted
 *
 * Description:
 * Removes a child and all of its views from the MDI.
 * A "remove_child" signal is emitted to the MDI with @child as its parameter
 * before actually removing the child. The child is removed only if the signal
 * handler (if it exists and the @force is set to %FALSE) returns %TRUE.
 *
 * Return value:
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint mate_mdi_remove_child (MateMDI *mdi, MateMDIChild *child, gint force)
{
	gint ret = TRUE;
	GList *view_node;
	GtkWidget *view;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);
	g_return_val_if_fail(child != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI_CHILD(child), FALSE);

	/* if force is set to TRUE, don't call the remove_child handler (ie there is no way for the
	   user to stop removal of the child) */
	if(!force)
		g_signal_emit(mdi, mdi_signals[REMOVE_CHILD], 0,
					  child,
					  &ret);

	if(ret == FALSE)
		return FALSE;

	view_node = child->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);
		view_node = view_node->next;
		mate_mdi_remove_view(mdi, GTK_WIDGET(view), TRUE);
	}

	mdi->children = g_list_remove(mdi->children, child);

	_mate_mdi_child_list_menu_remove_item(mdi, child);

	if(child == mdi->active_child)
		mdi->active_child = NULL;

	child->parent = NULL;

	g_object_ref_sink(GTK_OBJECT(child));

	if(mdi->mode == MATE_MDI_MODAL && mdi->children) {
		MateMDIChild *next_child = mdi->children->data;

		if(next_child->views) {
			mate_app_set_contents(mdi->active_window,
								   GTK_WIDGET(next_child->views->data));
			app_set_view(mdi, mdi->active_window,
						 GTK_WIDGET(next_child->views->data));
		}
		else
			mate_mdi_add_view(mdi, next_child);
	}

	return TRUE;
}

/**
 * mate_mdi_remove_all:
 * @mdi: A pointer to a MateMDI object.
 * @force: If TRUE, the "remove_child" signal is not emitted
 *
 * Description:
 * Removes all children and all views from the MDI.
 * A "remove_child" signal is emitted to the MDI for each child before
 * actually trying to remove any. If signal handlers for all children (if
 * they exist and the @force is set to %FALSE) return %TRUE, all children
 * and their views are removed and none otherwise.
 *
 * Return value:
 * %TRUE if the removal was successful and %FALSE otherwise.
 **/
gint mate_mdi_remove_all (MateMDI *mdi, gint force)
{
	GList *child_node;
	gint handler_ret = TRUE;

	g_return_val_if_fail(mdi != NULL, FALSE);
	g_return_val_if_fail(MATE_IS_MDI(mdi), FALSE);

	/* first check if removal of any child will be prevented by the
	   remove_child signal handler */
	if(!force) {
		child_node = mdi->children;
		while(child_node) {
			g_signal_emit(mdi, mdi_signals[REMOVE_CHILD], 0,
						  child_node->data,
						  &handler_ret);

			/* if any of the children may not be removed, none will be */
			if(handler_ret == FALSE)
				return FALSE;

			child_node = child_node->next;
		}
	}

	/* remove all the children with force arg set to true so that remove_child
	   handlers are not called again */
	while(mdi->children)
		mate_mdi_remove_child(mdi, MATE_MDI_CHILD(mdi->children->data), TRUE);

	return TRUE;
}

/**
 * mate_mdi_open_toplevel:
 * @mdi: A pointer to a MateMDI object.
 *
 * Description:
 * Opens a new toplevel window (unless in %MATE_MDI_MODAL mode and a
 * toplevel window is already open). This is usually used only for opening
 * the initial window on startup (just before calling gtkmain()) if no
 * windows were open because a session was restored or children were added
 * because of command line args).
 **/
void mate_mdi_open_toplevel (MateMDI *mdi)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	if( mdi->mode != MATE_MDI_MODAL || mdi->windows == NULL ) {
		app_clone(mdi, mdi->active_window);

		if(mdi->mode == MATE_MDI_NOTEBOOK)
			book_create(mdi);

		gtk_widget_show(GTK_WIDGET(mdi->active_window));
	}
}

/**
 * mate_mdi_update_child:
 * @mdi: A pointer to a MateMDI object.
 * @child: Child to update.
 *
 * Description:
 * Updates all notebook labels of @child's views and their window titles
 * after its name changes. It is not required if mate_mdi_child_set_name()
 * is used for setting the child's name.
 **/
void mate_mdi_update_child (MateMDI *mdi, MateMDIChild *child)
{
	GtkWidget *view;
	GList *view_node;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));
	g_return_if_fail(child != NULL);
	g_return_if_fail(MATE_IS_MDI_CHILD(child));

	view_node = child->views;
	while(view_node) {
		view = GTK_WIDGET(view_node->data);

		/* for the time being all that update_child() does is update the
		   children's names */
		if( (mdi->mode == MATE_MDI_MODAL) ||
			(mdi->mode == MATE_MDI_TOPLEVEL) ) {
			/* in MODAL and TOPLEVEL modes the window title includes the active
			   child name: "child_name - mdi_title" */
			gchar *fullname;

			fullname = g_strconcat(child->name, " - ", mdi->title, NULL);
			gtk_window_set_title(GTK_WINDOW(mate_mdi_get_app_from_view(view)),
								 fullname);
			g_free(fullname);
		}

		view_node = g_list_next(view_node);
	}
}

/**
 * mate_mdi_find_child:
 * @mdi: A pointer to a MateMDI object.
 * @name: A string with a name of the child to find.
 *
 * Description:
 * Finds a child named @name.
 *
 * Return value:
 * A pointer to the MateMDIChild object if the child was found and NULL
 * otherwise.
 **/
MateMDIChild *mate_mdi_find_child (MateMDI *mdi, const gchar *name)
{
	GList *child_node;

	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(MATE_IS_MDI(mdi), NULL);

	child_node = mdi->children;
	while(child_node) {
		if(strcmp(MATE_MDI_CHILD(child_node->data)->name, name) == 0)
			return MATE_MDI_CHILD(child_node->data);

		child_node = g_list_next(child_node);
	}

	return NULL;
}

/**
 * mate_mdi_set_mode:
 * @mdi: A pointer to a MateMDI object.
 * @mode: New mode.
 *
 * Description:
 * Sets the MDI mode to mode. Possible values are %MATE_MDI_TOPLEVEL,
 * %MATE_MDI_NOTEBOOK, %MATE_MDI_MODAL and %MATE_MDI_DEFAULT.
 **/
void mate_mdi_set_mode (MateMDI *mdi, MateMDIMode mode)
{
	GtkWidget *view;
	MateMDIChild *child;
	GList *child_node, *view_node, *app_node;
	gint windows = (mdi->windows != NULL);
	guint16 width = 0, height = 0;

	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	/* FIXME!
	if(mode == MATE_MDI_DEFAULT_MODE)
		mode = mate_preferences_get_mdi_mode();
		*/
	mode = MATE_MDI_NOTEBOOK;

	if(mdi->active_view) {
		width = mdi->active_view->allocation.width;
		height = mdi->active_view->allocation.height;
	}

	/* remove all views from their parents */
	child_node = mdi->children;
	while(child_node) {
		child = MATE_MDI_CHILD(child_node->data);
		view_node = child->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(view->parent) {
				if( (mdi->mode == MATE_MDI_TOPLEVEL) ||
					(mdi->mode == MATE_MDI_MODAL) )
					mate_mdi_get_app_from_view(view)->contents = NULL;

				gtk_container_remove(GTK_CONTAINER(view->parent), view);
			}

			view_node = view_node->next;

			/* if we are to change mode to MODAL, destroy all views except
			   the active one */
			/* if( (mode == MATE_MDI_MODAL) && (view != mdi->active_view) )
			   mate_mdi_child_remove_view(child, view); */
		}
		child_node = child_node->next;
	}

	/* remove all MateApps but the active one */
	app_node = mdi->windows;
	while(app_node) {
		if(MATE_APP(app_node->data) != mdi->active_window)
			gtk_widget_destroy(GTK_WIDGET(app_node->data));
		app_node = app_node->next;
	}

	if(mdi->windows)
		g_list_free(mdi->windows);

	if(mdi->active_window) {
		if(mdi->mode == MATE_MDI_NOTEBOOK)
			gtk_container_remove(GTK_CONTAINER(mdi->active_window->dock),
					     MATECOMPONENT_DOCK(mdi->active_window->dock)->client_area);

		mdi->active_window->contents = NULL;

		if( (mdi->mode == MATE_MDI_TOPLEVEL) || (mdi->mode == MATE_MDI_MODAL))
			g_signal_handlers_disconnect_by_func(mdi->active_window, G_CALLBACK(app_close_top), mdi);
		else if(mdi->mode == MATE_MDI_NOTEBOOK)
			g_signal_handlers_disconnect_by_func(mdi->active_window, G_CALLBACK(app_close_book), mdi);

		if( (mode == MATE_MDI_TOPLEVEL) || (mode == MATE_MDI_MODAL))
			g_signal_connect(mdi->active_window, "delete_event",
							 G_CALLBACK(app_close_top), mdi);
		else if(mode == MATE_MDI_NOTEBOOK)
			g_signal_connect(mdi->active_window, "delete_event",
							 G_CALLBACK(app_close_book), mdi);

		mdi->windows = g_list_append(NULL, mdi->active_window);

		if(mode == MATE_MDI_NOTEBOOK)
			book_create(mdi);
	}

	mdi->mode = mode;

	/* re-implant views in proper containers */
	child_node = mdi->children;
	while(child_node) {
		child = MATE_MDI_CHILD(child_node->data);
		view_node = child->views;
		while(view_node) {
			view = GTK_WIDGET(view_node->data);

			if(width != 0)
				gtk_widget_set_size_request (view, width, height);

			if(mdi->mode == MATE_MDI_NOTEBOOK)
				book_add_view(GTK_NOTEBOOK(mdi->active_window->contents), view);
			else if(mdi->mode == MATE_MDI_TOPLEVEL)
				/* add a new toplevel unless the remaining one is empty */
				top_add_view(mdi, child, view);
			else if(mdi->mode == MATE_MDI_MODAL) {
				/* replace the existing view if there is one */
				if(mdi->active_window->contents) {
					mate_mdi_remove_view(mdi, mdi->active_window->contents, TRUE);
					mdi->active_window->contents = NULL;
				}
				mate_app_set_contents(mdi->active_window, view);
				app_set_view(mdi, mdi->active_window, view);

				mdi->active_view = view;
			}

			view_node = view_node->next;

			gtk_widget_show(GTK_WIDGET(mdi->active_window));
		}
		child_node = child_node->next;
	}

	if(windows && !mdi->active_window)
		mate_mdi_open_toplevel(mdi);
}

/**
 * mate_mdi_get_active_child:
 * @mdi: A pointer to a MateMDI object.
 *
 * Description:
 * Returns a pointer to the active MateMDIChild object.
 *
 * Return value:
 * A pointer to the active MateMDIChild object. %NULL, if there is none.
 **/
MateMDIChild *mate_mdi_get_active_child (MateMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(MATE_IS_MDI(mdi), NULL);

	if(mdi->active_view)
		return(mate_mdi_get_child_from_view(mdi->active_view));

	return NULL;
}

/**
 * mate_mdi_get_active_view:
 * @mdi: A pointer to a MateMDI object.
 *
 * Description:
 * Returns a pointer to the active view (the one with the focus).
 *
 * Return value:
 * A pointer to a GtkWidget *.
 **/
GtkWidget *mate_mdi_get_active_view(MateMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(MATE_IS_MDI(mdi), NULL);

	return mdi->active_view;
}

/**
 * mate_mdi_get_active_window:
 * @mdi: A pointer to a MateMDI object.
 *
 * Description:
 * Returns a pointer to the toplevel window containing the active view.
 *
 * Return value:
 * A pointer to a MateApp that has the focus.
 **/
MateApp *mate_mdi_get_active_window (MateMDI *mdi)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(MATE_IS_MDI(mdi), NULL);

	return mdi->active_window;
}

/**
 * mate_mdi_set_menubar_template:
 * @mdi: A pointer to a MateMDI object.
 * @menu_tmpl: A MateUIInfo array describing the menu.
 *
 * Description:
 * This function sets the template for menus that appear in each toplevel
 * window to menu_template. For each new toplevel window created by the MDI,
 * this structure is copied, the menus are created with
 * mate_app_create_menus_with_data() function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new
 * toplevel window (a MateApp widget) and can be obtained by calling
 * &mate_mdi_get_menubar_info.
 **/
void mate_mdi_set_menubar_template (MateMDI *mdi, MateUIInfo *menu_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	mdi->menu_template = menu_tmpl;
}

/**
 * mate_mdi_set_toolbar_template:
 * @mdi: A pointer to a MateMDI object.
 * @tbar_tmpl: A MateUIInfo array describing the toolbar.
 *
 * Description:
 * This function sets the template for toolbar that appears in each toplevel
 * window to toolbar_template. For each new toplevel window created by the MDI,
 * this structure is copied, the toolbar is created with
 * mate_app_create_toolbar_with_data() function with mdi as the callback
 * user data. Finally, the pointer to the copy is assigned to the new toplevel
 * window (a MateApp widget) and can be retrieved with a call to
 * &mate_mdi_get_toolbar_info.
 **/
void mate_mdi_set_toolbar_template (MateMDI *mdi, MateUIInfo *tbar_tmpl)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	mdi->toolbar_template = tbar_tmpl;
}

/**
 * mate_mdi_set_child_menu_path:
 * @mdi: A pointer to a MateMDI object.
 * @path: A menu path where the child menus should be inserted.
 *
 * Description:
 * Sets the desired position of child-specific menus (which are added to and
 * removed from the main menus as views of different children are activated).
 * See mate_app_find_menu_pos for details on menu paths.
 **/
void mate_mdi_set_child_menu_path (MateMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	g_free(mdi->child_menu_path);
	mdi->child_menu_path = g_strdup(path);
}

/**
 * mate_mdi_set_child_list_path:
 * @mdi: A pointer to a MateMDI object.
 * @path: A menu path where the child list menu should be inserted
 *
 * Description:
 * Sets the position for insertion of menu items used to activate the MDI
 * children that were added to the MDI. See mate_app_find_menu_pos for
 * details on menu paths. If the path is not set or set to %NULL, these menu
 * items aren't going to be inserted in the MDI menu structure. Note that if
 * you want all menu items to be inserted in their own submenu, you have to
 * create that submenu (and leave it empty, of course).
 **/
void mate_mdi_set_child_list_path (MateMDI *mdi, const gchar *path)
{
	g_return_if_fail(mdi != NULL);
	g_return_if_fail(MATE_IS_MDI(mdi));

	g_free(mdi->child_list_path);
	mdi->child_list_path = g_strdup(path);
}

/**
 * mate_mdi_register:
 * @mdi: A pointer to a MateMDI object.
 * @object: Object to register.
 *
 * Description:
 * Registers GtkObject @object with MDI.
 * This is mostly intended for applications that open other windows besides
 * those opened by the MDI and want to continue to run even when no MDI
 * windows exist (an example of this would be GIMP's window with tools, if
 * the pictures were MDI children). As long as there is an object registered
 * with the MDI, the MDI will not destroy itself when the last of its windows
 * is closed. If no objects are registered, closing the last MDI window
 * results in MDI being destroyed.
 **/
void mate_mdi_register (MateMDI *mdi, GtkObject *object)
{
	if(!g_slist_find(mdi->registered, object))
		mdi->registered = g_slist_append(mdi->registered, object);
}

/**
 * mate_mdi_unregister:
 * @mdi: A pointer to a MateMDI object.
 * @object: Object to unregister.
 *
 * Description:
 * Removes GtkObject @object from the list of registered objects.
 **/
void mate_mdi_unregister (MateMDI *mdi, GtkObject *object)
{
	mdi->registered = g_slist_remove(mdi->registered, object);
}

/**
 * mate_mdi_get_child_from_view:
 * @view: A pointer to a GtkWidget.
 *
 * Description:
 * Returns a child that @view is a view of.
 *
 * Return value:
 * A pointer to the MateMDIChild the view belongs to.
 **/
MateMDIChild *mate_mdi_get_child_from_view(GtkWidget *view)
{
	return MATE_MDI_CHILD(g_object_get_data(G_OBJECT(view), MATE_MDI_CHILD_KEY));
}

/**
 * mate_mdi_get_app_from_view:
 * @view: A pointer to a GtkWidget.
 *
 * Description:
 * Returns the toplevel window for this view.
 *
 * Return value:
 * A pointer to the MateApp containg the specified view.
 **/
MateApp *mate_mdi_get_app_from_view(GtkWidget *view)
{
	return MATE_APP(gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

/**
 * mate_mdi_get_view_from_window:
 * @mdi: A pointer to a MateMDI object.
 * @app: A pointer to a MateApp widget.
 *
 * Description:
 * Returns the pointer to the view in the MDI toplevel window @app.
 * If the mode is set to %MATE_MDI_NOTEBOOK, the view in the current
 * page is returned.
 *
 * Return value:
 * A pointer to a view.
 **/
GtkWidget *mate_mdi_get_view_from_window (MateMDI *mdi, MateApp *app)
{
	g_return_val_if_fail(mdi != NULL, NULL);
	g_return_val_if_fail(MATE_IS_MDI(mdi), NULL);
	g_return_val_if_fail(app != NULL, NULL);
	g_return_val_if_fail(MATE_IS_APP(app), NULL);

	if((mdi->mode == MATE_MDI_TOPLEVEL) || (mdi->mode == MATE_MDI_MODAL)) {
		return app->contents;
	} else if( (mdi->mode == MATE_MDI_NOTEBOOK) &&
		   GTK_NOTEBOOK(app->contents)->cur_page) {
		int cur_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (app->contents));
		return gtk_notebook_get_nth_page (GTK_NOTEBOOK (app->contents), cur_page);
	} else {
		return NULL;
	}
}

/**
 * mate_mdi_get_menubar_info:
 * @app: A pointer to a MateApp widget created by the MDI.
 *
 * Return value:
 * A MateUIInfo array used for menubar in @app if the menubar has been created with a template.
 * %NULL otherwise.
 **/
MateUIInfo *mate_mdi_get_menubar_info (MateApp *app)
{
	return g_object_get_data(G_OBJECT(app), MATE_MDI_MENUBAR_INFO_KEY);
}

/**
 * mate_mdi_get_toolbar_info:
 * @app: A pointer to a MateApp widget created by the MDI.
 *
 * Return value:
 * A MateUIInfo array used for toolbar in @app if the toolbar has been created with a template.
 * %NULL otherwise.
 **/
MateUIInfo *mate_mdi_get_toolbar_info (MateApp *app)
{
	return g_object_get_data(G_OBJECT(app), MATE_MDI_TOOLBAR_INFO_KEY);
}

/**
 * mate_mdi_get_child_menu_info:
 * @app: A pointer to a MateApp widget created by the MDI.
 *
 * Return value:
 * A MateUIInfo array used for child's menus in @app if they have been created with a template.
 * %NULL otherwise.
 **/
MateUIInfo *mate_mdi_get_child_menu_info (MateApp *app)
{
	return g_object_get_data(G_OBJECT(app), MATE_MDI_CHILD_MENU_INFO_KEY);
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
