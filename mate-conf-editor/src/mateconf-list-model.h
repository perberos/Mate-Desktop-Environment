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

#ifndef __MATECONF_LIST_MODEL_H__
#define __MATECONF_LIST_MODEL_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#define MATECONF_TYPE_LIST_MODEL		  (mateconf_list_model_get_type ())
#define MATECONF_LIST_MODEL(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_LIST_MODEL, MateConfListModel))
#define MATECONF_LIST_MODEL_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_LIST_MODEL, MateConfListModelClass))
#define MATECONF_IS_LIST_MODEL(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_LIST_MODEL))
#define MATECONF_IS_LIST_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_LIST_MODEL))
#define MATECONF_LIST_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_LIST_MODEL, MateConfListModelClass))

typedef struct _MateConfListModel MateConfListModel;
typedef struct _MateConfListModelClass MateConfListModelClass;

enum {
	MATECONF_LIST_MODEL_ICON_NAME_COLUMN,
	MATECONF_LIST_MODEL_KEY_NAME_COLUMN,
	MATECONF_LIST_MODEL_KEY_PATH_COLUMN,
	MATECONF_LIST_MODEL_VALUE_COLUMN,
	MATECONF_LIST_MODEL_NUM_COLUMNS
};

struct _MateConfListModel {
	GObject parent_instance;

	gchar *root_path;
	gint stamp;

	MateConfClient *client;
	
	GSList *values;
	gint length;

	guint notify_id;
	GHashTable *key_hash;
};

struct _MateConfListModelClass {
	GObjectClass parent_class;
};

GType mateconf_list_model_get_type (void);
GtkTreeModel *mateconf_list_model_new (void);
void mateconf_list_model_set_root_path (MateConfListModel *model, const gchar *path);
void mateconf_list_model_set_client (MateConfListModel *model, MateConfClient *client);

#endif /* __MATECONF_LIST_MODEL_H__ */
