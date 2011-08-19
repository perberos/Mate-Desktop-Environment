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

#ifndef __MATECONF_TREE_MODEL_H__
#define __MATECONF_TREE_MODEL_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#define MATECONF_TYPE_TREE_MODEL		  (mateconf_tree_model_get_type ())
#define MATECONF_TREE_MODEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_TREE_MODEL, MateConfTreeModel))
#define MATECONF_TREE_MODEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_TREE_MODEL, MateConfTreeModelClass))
#define MATECONF_IS_TREE_MODEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_TREE_MODEL))
#define MATECONF_IS_TREE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_TREE_MODEL))
#define MATECONF_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_TREE_MODEL, MateConfTreeModelClass))

typedef struct _MateConfTreeModel MateConfTreeModel;
typedef struct _MateConfTreeModelClass MateConfTreeModelClass;

enum {
	MATECONF_TREE_MODEL_NAME_COLUMN,
	MATECONF_TREE_MODEL_NUM_COLUMNS
};

struct _MateConfTreeModel {
	GObject parent_instance;

	gpointer root;
	gint stamp;

	MateConfClient *client;
};

struct _MateConfTreeModelClass {
	GObjectClass parent_class;
};


	
GType mateconf_tree_model_get_type (void);
GtkTreeModel *mateconf_tree_model_new (void);
gchar *mateconf_tree_model_get_mateconf_name (MateConfTreeModel *tree_model, GtkTreeIter *iter);
gchar *mateconf_tree_model_get_mateconf_path (MateConfTreeModel *tree_model, GtkTreeIter *iter);

GtkTreePath *mateconf_tree_model_get_tree_path_from_mateconf_path (MateConfTreeModel *tree_model, const char *path);
void mateconf_tree_model_set_client (MateConfTreeModel *model, MateConfClient *client);

#endif /* __MATECONF_TREE_MODEL_H__ */
