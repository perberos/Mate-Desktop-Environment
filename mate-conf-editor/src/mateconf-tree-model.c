/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
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

#include <string.h>
#include "mateconf-tree-model.h"

typedef struct _Node Node;

#define NODE(x) ((Node *)(x))

static void mateconf_tree_model_class_init       (MateConfTreeModelClass *klass);
static void mateconf_tree_model_init             (MateConfTreeModel *model);
static void mateconf_tree_model_tree_model_init  (GtkTreeModelIface *iface);
static void mateconf_tree_model_clear_node       (GtkTreeModel *tree_model, Node *node);

G_DEFINE_TYPE_WITH_CODE (MateConfTreeModel, mateconf_tree_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, mateconf_tree_model_tree_model_init));

struct _Node {
	gint ref_count;
	gint offset;
	gchar *path;

	Node *parent;
	Node *children;
	Node *next;
	Node *prev;
};


GtkTreePath *
mateconf_tree_model_get_tree_path_from_mateconf_path (MateConfTreeModel *tree_model, const char *key)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter, child_iter, found_iter;
	gchar *tmp_str;
	gchar **key_array;
	int i;
	gboolean found;

	g_assert (key[0] == '/');

	/* special case root node */
	if (strlen (key) == 1 && key[0] == '/')
		return gtk_tree_path_new_from_string ("0");

	key_array = g_strsplit (key + 1, "/", 0);

	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (tree_model), &iter)) {

		/* Ugh, something is terribly wrong */
		g_strfreev (key_array);
		return NULL;
	}

	for (i = 0; key_array[i] != NULL; i++) {
		/* FIXME: this will build the level if it isn't there. But,
		 * the level can also be there, possibly incomplete. This
		 * code isn't handling those incomplete levels yet (that
		 * needs some current level/mateconf directory comparing code)
		 */
		if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (tree_model), &child_iter, &iter))
			break;

		found = FALSE;
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (tree_model), &child_iter,
					    MATECONF_TREE_MODEL_NAME_COLUMN, &tmp_str,
					    -1);

			if (strcmp (tmp_str, key_array[i]) == 0) {
				found_iter = child_iter;
				found = TRUE;
				g_free (tmp_str);
				break;
			} else
				g_free (tmp_str);
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (tree_model), &child_iter));

		if (!found)
			break;
		iter = child_iter;
	}

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_model), &found_iter);

	g_strfreev (key_array);

	return path;
}


gchar *
mateconf_tree_model_get_mateconf_path (MateConfTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;

	return g_strdup (node->path);
}

gchar *
mateconf_tree_model_get_mateconf_name (MateConfTreeModel *tree_model, GtkTreeIter *iter)
{
	gchar *ptr;
	Node *node = iter->user_data;

	ptr = node->path + strlen (node->path);

	while (ptr[-1] != '/')
		ptr--;

	return g_strdup (ptr);
}

static gboolean
mateconf_tree_model_build_level (MateConfTreeModel *model, Node *parent_node, gboolean emit_signals)
{
	GSList *list, *tmp;
	Node *tmp_node = NULL;
	gint i = 0;

	if (parent_node->children)
		return FALSE;

	list = mateconf_client_all_dirs (model->client, parent_node->path, NULL);

	if (!list)
		return FALSE;

	for (tmp = list; tmp; tmp = tmp->next, i++) {
		Node *node;

		node = g_new0 (Node, 1);
		node->offset = i;
		node->parent = parent_node;
		node->path = tmp->data;

		if (tmp_node) {
			tmp_node->next = node;
			node->prev = tmp_node;
		} else {
			/* set parent node's children */
			parent_node->children = node;
		}

		tmp_node = node;

		/* let the model know things have changed */
		if (emit_signals) {
			GtkTreeIter tmp_iter;
			GtkTreePath *tmp_path;

			model->stamp++;
			tmp_iter.stamp = model->stamp;
			tmp_iter.user_data = tmp_node;
			tmp_path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &tmp_iter);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), tmp_path, &tmp_iter);
			gtk_tree_path_free (tmp_path);
		}
	}

	g_slist_free (list);

	return TRUE;
}

static gint
mateconf_tree_model_get_n_columns (GtkTreeModel *tree_model)
{
	return MATECONF_TREE_MODEL_NUM_COLUMNS;
}

static GType
mateconf_tree_model_get_column_type (GtkTreeModel *tree_model, gint index)
{
	switch (index) {
	case MATECONF_TREE_MODEL_NAME_COLUMN:
		return G_TYPE_STRING;
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
mateconf_tree_model_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	gint *indices;
	gint depth, i;
	GtkTreeIter parent;
	MateConfTreeModel *model;

	model = (MateConfTreeModel *)tree_model;

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	parent.stamp = model->stamp;
	parent.user_data = NULL;

	if (!gtk_tree_model_iter_nth_child (tree_model, iter, NULL, indices[0]))
		return FALSE;

	for (i = 1; i < depth; i++) {
		parent = *iter;

		if (!gtk_tree_model_iter_nth_child (tree_model, iter, &parent, indices[i]))
			return FALSE;
	}

	return TRUE;
}

static GtkTreePath *
mateconf_tree_model_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	MateConfTreeModel *model = (MateConfTreeModel *)tree_model;
	Node *tmp_node;
	GtkTreePath *tree_path;
	gint i = 0;

	tmp_node = iter->user_data;

	if (NODE (iter->user_data)->parent == NULL) {
		tree_path = gtk_tree_path_new ();
		tmp_node = model->root;
	}
	else {
		GtkTreeIter tmp_iter = *iter;

		tmp_iter.user_data = NODE (iter->user_data)->parent;

		tree_path = mateconf_tree_model_get_path (tree_model, &tmp_iter);

		tmp_node = NODE (iter->user_data)->parent->children;
	}

	for (; tmp_node; tmp_node = tmp_node->next) {
		if (tmp_node == NODE (iter->user_data))
			break;

		i++;
	}

	gtk_tree_path_append_index (tree_path, i);

	return tree_path;
}


static void
mateconf_tree_model_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	Node *node = iter->user_data;

	switch (column) {
	case MATECONF_TREE_MODEL_NAME_COLUMN:
		g_value_init (value, G_TYPE_STRING);

		if (node == NULL || node->parent == NULL)
			g_value_set_string (value, "/");
		else {
			gchar *ptr;

			ptr = node->path + strlen (node->path);

			while (ptr[-1] != '/')
				ptr--;

			g_value_set_string (value, ptr);
		}
		break;
	default:
		break;
	}
}

static gboolean
mateconf_tree_model_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	MateConfTreeModel *model;
	Node *node;
	Node *parent_node = NULL;

	model = (MateConfTreeModel *)tree_model;

	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);
	g_return_val_if_fail (parent == NULL || parent->stamp == model->stamp, FALSE);

	if (parent == NULL)
		node = model->root;
	else {
		parent_node = (Node *)parent->user_data;
		node = parent_node->children;
	}

	if (!node && parent && mateconf_tree_model_build_level (model, parent_node, FALSE)) {
		node = parent_node->children;
	}

	for (; node != NULL; node = node->next)
		if (node->offset == n) {
			iter->stamp = model->stamp;
			iter->user_data = node;

			return TRUE;
		}

	iter->stamp = 0;

	return FALSE;
}

static gboolean
mateconf_tree_model_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	if (NODE (iter->user_data)->next != NULL) {
		iter->user_data = NODE (iter->user_data)->next;

		return TRUE;
	}
	else
		return FALSE;
}

static gboolean
mateconf_tree_model_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	MateConfTreeModel *model = (MateConfTreeModel *)tree_model;
	Node *parent_node = parent->user_data;

	if (parent_node->children != NULL) {
		iter->stamp = model->stamp;
		iter->user_data = parent_node->children;

		return TRUE;
	}

	if (!mateconf_tree_model_build_level (model, parent_node, TRUE)) {
		iter->stamp = 0;
		iter->user_data = NULL;

		return FALSE;
	}

	iter->stamp = model->stamp;
	iter->user_data = parent_node->children;

	return TRUE;
}

static gint
mateconf_tree_model_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	MateConfTreeModel *model = (MateConfTreeModel *)tree_model;
	Node *node;
	GtkTreeIter tmp;
	gint i = 0;

	g_return_val_if_fail (MATECONF_IS_TREE_MODEL (tree_model), 0);
	if (iter) g_return_val_if_fail (model->stamp == iter->stamp, 0);

	if (iter == NULL)
		node = model->root;
	else
		node = ((Node *)iter->user_data)->children;

	if (!node && iter && mateconf_tree_model_iter_children (tree_model, &tmp, iter)) {
		g_return_val_if_fail (tmp.stamp == model->stamp, 0);
		node = ((Node *)tmp.user_data);
	}

	if (!node)
		return 0;

	for (; node != NULL; node = node->next)
		i++;

	return i;
}

static gboolean
mateconf_tree_model_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	Node *child_node = child->user_data;

	if (child_node->parent == NULL)
		return FALSE;

	iter->stamp = ((MateConfTreeModel *)tree_model)->stamp;
	iter->user_data = child_node->parent;

	return TRUE;
}

static gboolean
mateconf_tree_model_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	MateConfTreeModel *model = (MateConfTreeModel *)tree_model;
	Node *node = iter->user_data;
	GSList *list;

	list = mateconf_client_all_dirs (model->client, node->path, NULL);

	if (list == NULL)
		return FALSE;
	else {
		g_slist_foreach (list, (GFunc) g_free, NULL);
		g_slist_free (list);

		return TRUE;
	}
}

static void
mateconf_tree_model_ref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;

	node->ref_count += 1;
}

static void
mateconf_tree_model_free_node (GtkTreeModel *tree_model, Node *node)
{
	mateconf_tree_model_clear_node (tree_model, node);
	if (node->path)
		g_free (node->path);
	g_free (node);
}

static void
mateconf_tree_model_clear_node (GtkTreeModel *tree_model, Node *node)
{
	Node *children;

	children = node->children;
	node->children = NULL;
	while (children) {
		Node *next = children->next;
		mateconf_tree_model_free_node (tree_model, children);
		children = next;
	}
}

static void
mateconf_tree_model_unref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;
	node->ref_count -= 1;

	if (node->ref_count == 0)
		mateconf_tree_model_clear_node (tree_model, node);
}

static void
mateconf_tree_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_n_columns = mateconf_tree_model_get_n_columns;
	iface->get_column_type = mateconf_tree_model_get_column_type;
	iface->get_iter = mateconf_tree_model_get_iter;
	iface->get_path = mateconf_tree_model_get_path;
	iface->get_value = mateconf_tree_model_get_value;
	iface->iter_nth_child = mateconf_tree_model_iter_nth_child;
	iface->iter_next = mateconf_tree_model_iter_next;
	iface->iter_has_child = mateconf_tree_model_iter_has_child;
	iface->iter_n_children = mateconf_tree_model_iter_n_children;
	iface->iter_children = mateconf_tree_model_iter_children;
	iface->iter_parent = mateconf_tree_model_iter_parent;
	iface->ref_node = mateconf_tree_model_ref_node;
	iface->unref_node = mateconf_tree_model_unref_node;
}

static void
mateconf_tree_model_finalize (GObject *obj)
{
	MateConfTreeModel *model = MATECONF_TREE_MODEL (obj);

	if (model->client) {
		g_object_unref (model->client);
		model->client = NULL;
	}

	G_OBJECT_CLASS (mateconf_tree_model_parent_class)->finalize (obj);
}

static void
mateconf_tree_model_class_init (MateConfTreeModelClass *klass)
{
	G_OBJECT_CLASS (klass)->finalize = mateconf_tree_model_finalize;
}

static void
mateconf_tree_model_init (MateConfTreeModel *model)
{
	Node *root;

	root = g_new0 (Node, 1);
	root->path = g_strdup ("/");
	root->offset = 0;

	model->root = root;
	model->client = mateconf_client_get_default ();
}

void
mateconf_tree_model_set_client (MateConfTreeModel *model, MateConfClient *client)
{
	if (model->client != NULL) {
		g_object_unref (model->client);
	}

	model->client = g_object_ref (client);
}

GtkTreeModel *
mateconf_tree_model_new (void)
{
	return GTK_TREE_MODEL (g_object_new (MATECONF_TYPE_TREE_MODEL, NULL));
}
