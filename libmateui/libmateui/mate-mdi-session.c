/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <matecomponent/matecomponent-dock-layout.h>
#include <libmate/mate-config.h>
#include "mate-mdi-session.h"


static GPtrArray *	config_get_list		(const gchar *);
static void		config_set_list		(const gchar *, GList *,
						 gpointer (*)(gpointer));
static void		restore_window_child	(MateMDI *, GHashTable *,
						 GHashTable *, GHashTable *,
						 GHashTable *, GHashTable *,
						 glong, glong, gboolean *,
						 gint, gint, gint, gint);
static void		restore_window		(MateMDI *, const gchar *,
						 GPtrArray *, GHashTable *,
						 GHashTable *, GHashTable *,
						 GHashTable *, GHashTable *,
						 glong);
static void		set_active_window	(MateMDI *, GHashTable *, glong);


static gpointer		view_window_func	(gpointer);

static gchar *
mate_mdi_child_get_config_string (MateMDIChild *child)
{
	if(MATE_MDI_CHILD_GET_CLASS(child)->get_config_string)
		return MATE_MDI_CHILD_GET_CLASS(child)->get_config_string(child, NULL);

	return NULL;
}

static GPtrArray *
config_get_list (const gchar *key)
{
	GPtrArray *array;
	gchar *string, *pos;

	string = mate_config_get_string (key);
	if (!string) return NULL;

	array = g_ptr_array_new ();

	pos = string;
	while (*pos) {
		glong value;
		gchar *tmp;

		tmp = strchr (pos, ':');
		if (tmp) *tmp = '\0';

		if (sscanf (pos, "%lx", &value) == 1)
			g_ptr_array_add (array, (gpointer) value);

		if (tmp)
			pos = tmp+1;
		else
			break;
	}

	g_free (string);
	return array;
}

static void
config_set_list (const gchar *key, GList *list, gpointer (*func)(gpointer))
{
	gchar value [BUFSIZ];

	value [0] = '\0';
	while (list) {
		char buffer [BUFSIZ];
		gpointer data;

		data = func ? func (list->data) : list->data;
		g_snprintf (buffer, sizeof(buffer), "%lx", (glong) data);
		if (*value) strcat (value, ":");
		strcat (value, buffer);

		list = g_list_next (list);
	}

	mate_config_set_string (key, value);
}

static void
restore_window_child (MateMDI *mdi, GHashTable *child_hash,
		      GHashTable *child_windows, GHashTable *child_views,
		      GHashTable *view_hash, GHashTable *window_hash,
		      glong window, glong child, gboolean *init,
		      gint x, gint y, gint width, gint height)
{
	GPtrArray *windows, *views;
	MateMDIChild *mdi_child;
	guint k;

	windows = g_hash_table_lookup (child_windows, (gpointer) child);
	if (!windows) return;

	views = g_hash_table_lookup (child_views, (gpointer) child);
	if (!views) return;
		
	mdi_child = g_hash_table_lookup (child_hash, (gpointer) child);
	if (!mdi_child) return;
		
	for (k = 0; k < windows->len; k++) {
		if (windows->pdata [k] != (gpointer) window)
			continue;

		if (*init)
			mate_mdi_add_view (mdi, mdi_child);
		else {
			mate_mdi_add_toplevel_view (mdi, mdi_child);
			
			gtk_window_set_default_size (GTK_WINDOW (mdi->active_window), width, height);
			gtk_window_move (GTK_WINDOW (mdi->active_window), x, y);
			
			*init = TRUE;

			g_hash_table_insert (window_hash,
					     (gpointer) window,
					     mdi->active_window);
		}

		g_hash_table_insert (view_hash,
				     views->pdata [k],
				     mdi->active_view);
	}
}

static void
restore_window (MateMDI *mdi, const gchar *section, GPtrArray *child_list,
		GHashTable *child_hash, GHashTable *child_windows,
		GHashTable *child_views, GHashTable *view_hash,
		GHashTable *window_hash, glong window)
{
	gboolean init = FALSE;
	gchar key [BUFSIZ], *string;
	gint ret, x, y, w, h;
	guint j;

	g_snprintf (key, sizeof(key), "%s/mdi_window_%lx", section, window);
	string = mate_config_get_string (key);
	if (!string) return;

	ret = sscanf (string, "%d/%d/%d/%d", &x, &y, &w, &h);
	g_free (string);
	if (ret != 4) return;

	if(child_list->len == 0) {	
		mate_mdi_open_toplevel (mdi);

		gtk_window_set_default_size (GTK_WINDOW (mdi->active_window), w, h);
		gtk_window_move (GTK_WINDOW (mdi->active_window), x, y);

		g_hash_table_insert (window_hash, (gpointer) window,
				     mdi->active_window);
 	}
	else
		for (j = 0; j < child_list->len; j++)
			restore_window_child (mdi, child_hash, child_windows,
					      child_views, view_hash,
					      window_hash, window,
					      (glong) child_list->pdata [j],
					      &init, x, y, w, h);

	g_snprintf (key, sizeof(key), "%s/mdi_window_layout_%lx", section, window);
	string = mate_config_get_string (key);
	if (!string) return;

#if 0
	{
		MateApp *app = mdi->active_window;
		MateComponentDockLayout *layout;

		printf("app->layout == %08lx\n", app->layout);

		/* this should be a nasty hack before dock-layout gets a bit better
		   don't even know if it works, though ;) */
		layout = matecomponent_dock_get_layout(MATECOMPONENT_DOCK(app->dock));
		matecomponent_dock_layout_parse_string(mdi->active_window->layout, string);
		gtk_container_forall(GTK_CONTAINER(app->dock), remove_items, app->dock);
		matecomponent_dock_add_from_layout(MATECOMPONENT_DOCK(app->dock), layout);
		g_object_unref (G_OBJECT(layout));
	}
#endif
}

static void
set_active_window (MateMDI *mdi, GHashTable *window_hash, glong active_window)
{
	MateApp *app;
	GtkWidget *view;

	app = g_hash_table_lookup (window_hash, (gpointer) active_window);
	if (!app) return;

	view = mate_mdi_get_view_from_window (mdi, app);
	if (!view) return;

	mate_mdi_set_active_view (mdi, view);
}

/**
 * mate_mdi_restore_state:
 * @mdi: A pointer to a MateMDI object.
 * @section: Name of the section to restore MDI state from.
 * @create_child_func: A function that recreates a child from its config string.
 * 
 * Description:
 * Restores the MDI state. Children are recreated with @create_child_func that
 * restores information about a child from a config string that was provided
 * during saving state by the child. 
 * 
 * Return value:
 * TRUE if state was successfully restored, FALSE otherwise.
 **/
gboolean
mate_mdi_restore_state (MateMDI *mdi, const gchar *section,
			 MateMDIChildCreator create_child_func)
{
	gchar key [BUFSIZ], *string;
	GPtrArray *window_list, *child_list;
	GHashTable *child_hash, *child_windows;
	GHashTable *child_views, *view_hash, *window_hash;
	glong active_view = 0, active_window = 0;
	guint i;
	gint mode;

	g_snprintf (key, sizeof(key), "%s/mdi_mode=-1", section);
	mode = mate_config_get_int (key);
	if (mate_config_get_int (key) == -1)
		return FALSE;

	mate_mdi_set_mode (mdi, mode);

	child_hash = g_hash_table_new (NULL, NULL);
	child_windows = g_hash_table_new (NULL, NULL);
	child_views = g_hash_table_new (NULL, NULL);
	view_hash = g_hash_table_new (NULL, NULL);
	window_hash = g_hash_table_new (NULL, NULL);

	/* Walk the list of windows. */

	g_snprintf (key, sizeof(key), "%s/mdi_windows", section);
	window_list = config_get_list (key);

	/* Walk the list of children. */

	g_snprintf (key, sizeof(key), "%s/mdi_children", section);
	child_list = config_get_list (key);

	/* Get the active view. */

	g_snprintf (key, sizeof(key), "%s/mdi_active_view", section);
	string = mate_config_get_string (key);

	if (string) {
		sscanf (string, "%lx", &active_view);
		g_free (string);
	}

	/* Get the active window. */

	g_snprintf (key, sizeof(key), "%s/mdi_active_window", section);
	string = mate_config_get_string (key);

	if (string) {
		sscanf (string, "%lx", &active_window);
		g_free (string);
	}

	/* Read child descriptions. */

	for (i = 0; i < child_list->len; i++) {
		glong child = (glong) child_list->pdata [i];
		GPtrArray *windows, *views;
		MateMDIChild *mdi_child;

		g_snprintf (key, sizeof(key), "%s/mdi_child_config_%lx",
			    section, child);
		string = mate_config_get_string (key);
		if (!string) continue;

		mdi_child = create_child_func (string);
		g_free (string);

		mate_mdi_add_child (mdi, mdi_child);

		g_hash_table_insert (child_hash, (gpointer)child, mdi_child);
		
		g_snprintf (key, sizeof(key), "%s/mdi_child_windows_%lx",
			    section, child);
		windows = config_get_list (key);

		g_hash_table_insert (child_windows, (gpointer) child, windows);

		g_snprintf (key, sizeof(key), "%s/mdi_child_views_%lx",
			    section, child);
		views = config_get_list (key);

		g_hash_table_insert (child_views, (gpointer) child, views);
	}

	/* Read window descriptions. */
	
	for (i = 0; i < window_list->len; i++)
		restore_window (mdi, section, child_list,
				child_hash, child_windows,
				child_views, view_hash, window_hash,
				(glong) window_list->pdata [i]);

	/* For each window, set its active view. */

	for (i = 0; i < window_list->len; i++) {
		GtkWidget *real_view;
		glong view;
		gint ret;

		g_snprintf (key, sizeof(key), "%s/mdi_window_view_%lx",
			    section, (long) window_list->pdata [i]);
		string = mate_config_get_string (key);
		if (!string) continue;

		ret = sscanf (string, "%lx", &view);
		g_free (string);
		if (ret != 1) continue;

		real_view = g_hash_table_lookup (view_hash, (gpointer) view);
		if (!real_view) continue;

		mate_mdi_set_active_view (mdi, real_view);
	}

	/* Finally, set the active window. */

	set_active_window (mdi, window_hash, active_window);

	/* Free allocated memory. */

	for (i = 0; i < child_list->len; i++) {
		glong child = (glong) child_list->pdata [i];
		GPtrArray *windows, *views;

		windows = g_hash_table_lookup (child_windows, (gpointer) child);
		if (windows) g_ptr_array_free (windows, FALSE);

		views = g_hash_table_lookup (child_views, (gpointer) child);
		if (views) g_ptr_array_free (views, FALSE);
	}

	g_hash_table_destroy (child_hash);
	g_hash_table_destroy (child_windows);
	g_hash_table_destroy (child_views);
	g_hash_table_destroy (view_hash);
	g_hash_table_destroy (window_hash);

	return TRUE;
}

static gpointer
view_window_func (gpointer data)
{
	if (GTK_WIDGET_REALIZED (GTK_WIDGET (data)))
		return mate_mdi_get_app_from_view(GTK_WIDGET(data));
	else
		return NULL;
}

/**
 * mate_mdi_save_state:
 * @mdi: A pointer to a MateMDI object.
 * @section: Name of the section that the MDI config should be saved to.
 * 
 * Description:
 * Saves MDI state to the application's config file in section @section.
 **/
void
mate_mdi_save_state (MateMDI *mdi, const gchar *section)
{
	gchar key [BUFSIZ], value [BUFSIZ];
	GList *child, *window;
	gint x, y, w, h;

	mate_config_clean_section (section);

	/* save MDI mode */
	g_snprintf (key, sizeof(key), "%s/mdi_mode", section);
	mate_config_set_int (key, mdi->mode);

	/* Write list of children. */

	g_snprintf (key, sizeof(key), "%s/mdi_children", section);
	config_set_list (key, mdi->children, NULL);

	/* Write list of windows. */

	g_snprintf (key, sizeof(key), "%s/mdi_windows", section);
	config_set_list (key, mdi->windows, NULL);

	/* Save active window. */

	g_snprintf (key, sizeof(key), "%s/mdi_active_window", section);
	g_snprintf (value, sizeof(value), "%lx", (glong) mdi->active_window);
	mate_config_set_string (key, value);

	/* Save active view. */

	g_snprintf (key, sizeof(key), "%s/mdi_active_view", section);
	g_snprintf (value, sizeof(value), "%lx", (glong) mdi->active_view);
	mate_config_set_string (key, value);

	/* Walk list of children. */

	child = mdi->children;
	while (child) {
		MateMDIChild *mdi_child;
		gchar *string;

		mdi_child = MATE_MDI_CHILD (child->data);

		/* Save child configuration. */

		string = mate_mdi_child_get_config_string(mdi_child);

		if (string) {
			g_snprintf (key, sizeof(key), "%s/mdi_child_config_%lx",
				    section, (long) mdi_child);
			mate_config_set_string (key, string);
			g_free (string);
		}

		/* Save list of views this child has. */

		g_snprintf (key, sizeof(key), "%s/mdi_child_windows_%lx",
			    section, (long) mdi_child);
		config_set_list (key, mdi_child->views, view_window_func);

		g_snprintf (key, sizeof(key), "%s/mdi_child_views_%lx",
			    section, (long) mdi_child);
		config_set_list (key, mdi_child->views, NULL);

		child = g_list_next (child);
	}

	/* Save list of toplevel windows. */

	window = mdi->windows;
	while (window) {
		MateApp *app;
		MateComponentDockLayout *layout;
		GtkWidget *view;
		gchar *string;

		app = MATE_APP (window->data);

		gdk_window_get_geometry (GTK_WIDGET (app)->window,
					 &x, &y, &w, &h, NULL);

		gdk_window_get_origin (GTK_WIDGET (app)->window, &x, &y);

		g_snprintf (key, sizeof(key), "%s/mdi_window_%lx",
			    section, (long) app);
		g_snprintf (value, sizeof(value), "%d/%d/%d/%d", x, y, w, h);

		mate_config_set_string (key, value);

		view = mate_mdi_get_view_from_window (mdi, app);

		g_snprintf (key, sizeof(key), "%s/mdi_window_view_%lx",
			    section, (long) app);
		g_snprintf (value, sizeof(value), "%lx", (long) view);

		mate_config_set_string (key, value);

		g_snprintf(key, sizeof(key), "%s/mdi_window_layout_%lx",
			   section, (long) app);

		layout = matecomponent_dock_get_layout (MATECOMPONENT_DOCK (app->dock));
		string = matecomponent_dock_layout_create_string (layout);
		g_object_unref (G_OBJECT (layout));
		mate_config_set_string(key, string);
		g_free(string);

		window = g_list_next (window);
	}

	mate_config_sync ();
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
