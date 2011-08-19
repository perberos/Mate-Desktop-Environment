/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Fernando Herrera <fherrera@onirica.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>

#include "mateconf-search.h"
#include "mateconf-search-dialog.h"
#include "mateconf-editor-window.h"
#include "mateconf-tree-model.h"
#include "mateconf-list-model.h"

typedef struct _Node Node;

struct _Node {
	gint ref_count;
	gint offset;
	gchar *path;

	Node *parent;
	Node *children;
	Node *next;
	Node *prev;
};	

static
gboolean
mateconf_tree_model_search_iter_foreach (GtkTreeModel *model, GtkTreePath *path,
				      GtkTreeIter *iter, gpointer data)
{
	Node    *node;
	SearchIter *st;
	gchar *found;
	GSList *values, *list;

	st = (SearchIter *) data;

	if (st->searching == NULL) {
		return TRUE;
	}

	if (st->res >= 1) {
		gtk_widget_show (GTK_WIDGET (st->output_window));
	}
	while (gtk_events_pending ())
		gtk_main_iteration ();

	node = iter->user_data;
	found = g_strrstr ((char*) node->path, (char*) st->pattern);

	if (found != NULL) {
		/* We found the pattern in the tree */
		gchar *key = mateconf_tree_model_get_mateconf_path (MATECONF_TREE_MODEL (model), iter);
		gedit_output_window_append_line (st->output_window, key, FALSE);
		g_free (key);
		st->res++;
		return FALSE;
	}

	if (!st->search_keys && !st->search_values) {
		return FALSE;
	}
	values = mateconf_client_all_entries (MATECONF_TREE_MODEL (model)->client, (const char*) node->path , NULL);
	for (list = values; list; list = list->next) {
		const gchar *key;
		MateConfEntry *entry = list->data;
		key = mateconf_entry_get_key (entry);
		/* Search in the key names */
		if (st->search_keys) {
			found = g_strrstr (key, (char*) st->pattern);
			if (found != NULL) {
				/* We found the pattern in the final key name */
				gedit_output_window_append_line (st->output_window, key, FALSE);
				st->res++;
				mateconf_entry_unref (entry);
				/* After finding an entry continue the list to find other matches */
				continue;
			}
		}

		/* Search in the values */
		if (st->search_values) {
			const char *mateconf_string;
			MateConfValue *mateconf_value = mateconf_entry_get_value (entry);

			/* FIXME: We are only looking into strings... should we do in
			 * int's? */
			if (mateconf_value != NULL && mateconf_value->type == MATECONF_VALUE_STRING)
				mateconf_string = mateconf_value_get_string (mateconf_value);
			else {
				mateconf_entry_unref (entry);
				continue;
			}

                	found = g_strrstr (mateconf_string, (char*) st->pattern);
			if (found != NULL) {
				/* We found the pattern in the key value */
				gedit_output_window_append_line (st->output_window, key, FALSE);
				st->res++;
				mateconf_entry_unref (entry);
				continue;
			}
		}
		mateconf_entry_unref (entry);
	}

	return FALSE;
}

int
mateconf_tree_model_build_match_list (MateConfTreeModel *tree_model, GeditOutputWindow *output_window,
				   const char *pattern, gboolean search_keys, gboolean search_values,
				   GObject *dialog)
{
	GtkTreeIter iter_root;
	int res;
	SearchIter *st;

	st = g_new0 (SearchIter, 1);
	st->pattern = pattern;
	st->search_keys = search_keys;
	st->search_values = search_values;
	st->output_window = output_window; 
	st->res = 0;
	st->searching = dialog;

	g_object_add_weak_pointer (st->searching, (gpointer)&st->searching);

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree_model), &iter_root)) {

		/* Ugh, something is terribly wrong */
		return 0;
	}

	gtk_tree_model_foreach (GTK_TREE_MODEL (tree_model),
				mateconf_tree_model_search_iter_foreach, st);

	res = st->res;
#if 0
	g_free (st); /* This causes invalid memory access according to valgrind */
#endif		     /* FIXME: This introduces a small leak (24 bytes) */
	return res;

}

