/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
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

#include <config.h>

#include "mateconf-list-model.h"

#include <string.h>
#include "mateconf-util.h"

#define G_SLIST(x) ((GSList *) x)

static const char *mateconf_value_icon_names[] = {
  "type-undefined",
  "type-string",
  "type-integer",
  "type-float",
  "type-boolean",
  "type-schema",
  "type-list",
  "type-pair"
};

static void mateconf_list_model_class_init       (MateConfListModelClass *klass);
static void mateconf_list_model_init             (MateConfListModel *model);
static void mateconf_list_model_tree_model_init  (GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE (MateConfListModel, mateconf_list_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, mateconf_list_model_tree_model_init));

static void
mateconf_list_model_notify_func (MateConfClient* client, guint cnxn_id, MateConfEntry *entry, gpointer user_data)
{
	GSList *list;
	const gchar *key;
	char *path_str;
	MateConfListModel *list_model = user_data;
	GtkTreeIter iter;
	GtkTreePath *path;

	key = mateconf_entry_get_key (entry);

	path_str = g_path_get_dirname (key);

	if (strcmp (path_str, list_model->root_path) != 0)
	  {
	    g_free (path_str);
	    return;
	  }

	g_free (path_str);

	if (strncmp (key, list_model->root_path, strlen (list_model->root_path)) != 0)
	    return;
	
	if (mateconf_client_dir_exists (client, key, NULL))
		/* this is a directory -- ignore */
		return;

	list = g_hash_table_lookup (list_model->key_hash, key);

	if (list == NULL) {
		/* Create a new entry */
		entry = mateconf_entry_new (mateconf_entry_get_key (entry),
					 mateconf_entry_get_value (entry));

		list = g_slist_append (list, entry);
		list_model->values = g_slist_concat (list_model->values, list);
		g_hash_table_insert (list_model->key_hash, g_strdup (key), list);

		list_model->stamp++;

		iter.stamp = list_model->stamp;
		iter.user_data = list;

		list_model->length++;

		path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_model), &iter);
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (list_model), path, &iter);
		gtk_tree_path_free (path);
	}
	else {
		list_model->stamp++;

		iter.stamp = list_model->stamp;
		iter.user_data = list;

		path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_model), &iter);

		mateconf_entry_unref (list->data);

		if (mateconf_entry_get_value (entry) != NULL) {
			list->data = mateconf_entry_new (mateconf_entry_get_key (entry),
						      mateconf_entry_get_value (entry));
			gtk_tree_model_row_changed (GTK_TREE_MODEL (list_model), path, &iter);
		}
		else {
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (list_model), path);
			list_model->values = g_slist_remove (list_model->values, list->data);
			list_model->length--;
			g_hash_table_remove (list_model->key_hash, key);
		}

		gtk_tree_path_free (path);
	}
}

void
mateconf_list_model_set_root_path (MateConfListModel *model, const gchar *root_path)
{
	GSList *list;
	GSList *values;
	GtkTreeIter iter;
	GtkTreePath *path;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, 0);

	if (model->root_path != NULL) {
		for (list = model->values; list; list = list->next) {
			MateConfEntry *entry = list->data;

			g_hash_table_remove (model->key_hash, mateconf_entry_get_key (entry));
			model->stamp++;
			gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

			mateconf_entry_unref (entry);
		}

		mateconf_client_notify_remove (model->client, model->notify_id);

		mateconf_client_remove_dir  (model->client,
					  model->root_path, NULL);

		g_free (model->root_path);
		g_slist_free (model->values);
		model->values = NULL;
	}
	gtk_tree_path_free (path);

	mateconf_client_add_dir (model->client,
			      root_path,
			      MATECONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	model->notify_id = mateconf_client_notify_add (model->client, root_path,
						    mateconf_list_model_notify_func,
						    model, NULL, NULL);

	model->root_path = g_strdup (root_path);
	values = mateconf_client_all_entries (model->client, root_path, NULL);
	model->length = 0;

	for (list = values; list; list = list->next) {
		MateConfEntry *entry = list->data;

		model->values = g_slist_append (model->values, list->data);
		model->length++;

		model->stamp++;

		iter.stamp = model->stamp;
		iter.user_data = g_slist_last (model->values);

		g_hash_table_insert (model->key_hash, g_strdup (mateconf_entry_get_key (entry)), iter.user_data);

		path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);

		gtk_tree_path_free (path);
	}

	if (values) 
		g_slist_free (values);
}

static GtkTreeModelFlags 
mateconf_list_model_get_flags (GtkTreeModel *tree_model)
{
	return 0;
}

static gint
mateconf_list_model_get_n_columns (GtkTreeModel *tree_model)
{
	return MATECONF_LIST_MODEL_NUM_COLUMNS;
}

static GType
mateconf_list_model_get_column_type (GtkTreeModel *tree_model, gint index)
{
	switch (index) {
	case MATECONF_LIST_MODEL_ICON_NAME_COLUMN:
	case MATECONF_LIST_MODEL_KEY_PATH_COLUMN:
	case MATECONF_LIST_MODEL_KEY_NAME_COLUMN:
		return G_TYPE_STRING;
	case MATECONF_LIST_MODEL_VALUE_COLUMN:
		return MATECONF_TYPE_VALUE;
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
mateconf_list_model_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	MateConfListModel *list_model = (MateConfListModel *)tree_model;
	GSList *list;
	gint i;

	g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

	i = gtk_tree_path_get_indices (path)[0];

	if (i >= list_model->length)
		return FALSE;

	list = g_slist_nth (list_model->values, i);

	iter->stamp = list_model->stamp;
	iter->user_data = list;

	return TRUE;
}

static GtkTreePath *
mateconf_list_model_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GSList *list;
	GtkTreePath *tree_path;
	gint i = 0;

	g_return_val_if_fail (iter->stamp == MATECONF_LIST_MODEL (tree_model)->stamp, NULL);

	for (list = G_SLIST (MATECONF_LIST_MODEL (tree_model)->values); list; list = list->next) {
		if (list == G_SLIST (iter->user_data))
			break;
		i++;
	}
	if (list == NULL)
		return NULL;

	tree_path = gtk_tree_path_new ();
	gtk_tree_path_append_index (tree_path, i);

	return tree_path;
}

static void
mateconf_list_model_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	MateConfEntry *entry;
	
	g_return_if_fail (iter->stamp == MATECONF_LIST_MODEL (tree_model)->stamp);

	entry = G_SLIST (iter->user_data)->data;
	
	switch (column) {
	case MATECONF_LIST_MODEL_KEY_PATH_COLUMN:
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, mateconf_entry_get_key (entry));
		break;
		
	case MATECONF_LIST_MODEL_KEY_NAME_COLUMN:
		g_value_init (value, G_TYPE_STRING);
		g_value_take_string (value, mateconf_get_key_name_from_path (mateconf_entry_get_key (entry)));
		break;
		
	case MATECONF_LIST_MODEL_ICON_NAME_COLUMN: {
                MateConfValue *mateconf_value;
                MateConfValueType value_type;

                mateconf_value = mateconf_entry_get_value (entry);
                if (mateconf_value) {
                        value_type = mateconf_value->type;
                } else {
                        value_type = MATECONF_VALUE_INVALID;
                }

                g_value_init (value, G_TYPE_STRING);
                g_value_set_static_string (value, mateconf_value_icon_names[value_type]);
		
		break;
        }
		
	case MATECONF_LIST_MODEL_VALUE_COLUMN:
		g_value_init (value, MATECONF_TYPE_VALUE);
		g_value_set_boxed (value, mateconf_entry_get_value (entry));
		break;

	default:
		break;
	}
}

static gboolean
mateconf_list_model_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	g_return_val_if_fail (iter->stamp == MATECONF_LIST_MODEL (tree_model)->stamp, FALSE);

	iter->user_data = G_SLIST (iter->user_data)->next;
	
	return (iter->user_data != NULL);
}

static gboolean
mateconf_list_model_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	return FALSE;
}

static gint
mateconf_list_model_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	/* it should ask for the root node, because we're a list */
	if (!iter)
		return g_slist_length (MATECONF_LIST_MODEL (tree_model)->values);

	return -1;
}

static gboolean
mateconf_list_model_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	GSList *child;

	if (parent)
		return FALSE;
	
	child = g_slist_nth (MATECONF_LIST_MODEL (tree_model)->values, n);

	if (child) {
		iter->stamp = MATECONF_LIST_MODEL (tree_model)->stamp;
		iter->user_data = child;
		return TRUE;
	}
	else
		return FALSE;
}

static void
mateconf_list_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags = mateconf_list_model_get_flags;
	iface->get_n_columns = mateconf_list_model_get_n_columns;
	iface->get_column_type = mateconf_list_model_get_column_type;
	iface->get_iter = mateconf_list_model_get_iter;
	iface->get_path = mateconf_list_model_get_path;
	iface->get_value = mateconf_list_model_get_value;
	iface->iter_next = mateconf_list_model_iter_next;
	iface->iter_has_child = mateconf_list_model_iter_has_child;
	iface->iter_n_children = mateconf_list_model_iter_n_children;
	iface->iter_nth_child = mateconf_list_model_iter_nth_child;
}

static void
mateconf_list_model_finalize (GObject *object)
{
	MateConfListModel *list_model;

	list_model = (MateConfListModel *)object;
	
	g_hash_table_destroy (list_model->key_hash);

	if (list_model->client && list_model->notify_id > 0) {
		mateconf_client_notify_remove (list_model->client, list_model->notify_id);
	}

	if (list_model->client && list_model->root_path) {
		mateconf_client_remove_dir  (list_model->client,
					  list_model->root_path, NULL);
	}

        if (list_model->client) {
                g_object_unref (list_model->client);
		list_model->client = NULL;
        }

	if (list_model->values) {
		g_slist_foreach (list_model->values, (GFunc) mateconf_entry_unref, NULL);
		g_slist_free (list_model->values);
		list_model->values = NULL;
	}

	g_free (list_model->root_path);

	G_OBJECT_CLASS (mateconf_list_model_parent_class)->finalize (object);
}

static void
mateconf_list_model_class_init (MateConfListModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = mateconf_list_model_finalize;
}

static void
mateconf_list_model_init (MateConfListModel *model)
{
	model->stamp = g_random_int ();
	model->key_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						 g_free, NULL);
	model->client = mateconf_client_get_default ();
}

void
mateconf_list_model_set_client (MateConfListModel *model, MateConfClient *client)
{
	if (model->client != NULL) {
		g_object_unref (model->client);
	}

	model->client = g_object_ref (client);
}

GtkTreeModel *
mateconf_list_model_new (void)
{
	return GTK_TREE_MODEL (g_object_new (MATECONF_TYPE_LIST_MODEL, NULL));
}
