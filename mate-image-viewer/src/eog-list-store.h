/* Eye Of Mate - Image Store
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 *
 * Based on code by: Jens Finke <jens@triq.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef EOG_LIST_STORE_H
#define EOG_LIST_STORE_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#ifndef __EOG_IMAGE_DECLR__
#define __EOG_IMAGE_DECLR__
  typedef struct _EogImage EogImage;
#endif

typedef struct _EogListStore EogListStore;
typedef struct _EogListStoreClass EogListStoreClass;
typedef struct _EogListStorePrivate EogListStorePrivate;

#define EOG_TYPE_LIST_STORE            eog_list_store_get_type()
#define EOG_LIST_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_LIST_STORE, EogListStore))
#define EOG_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  EOG_TYPE_LIST_STORE, EogListStoreClass))
#define EOG_IS_LIST_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_LIST_STORE))
#define EOG_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  EOG_TYPE_LIST_STORE))
#define EOG_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  EOG_TYPE_LIST_STORE, EogListStoreClass))

#define EOG_LIST_STORE_THUMB_SIZE 90

typedef enum {
	EOG_LIST_STORE_THUMBNAIL = 0,
	EOG_LIST_STORE_THUMB_SET,
	EOG_LIST_STORE_EOG_IMAGE,
	EOG_LIST_STORE_EOG_JOB,
	EOG_LIST_STORE_NUM_COLUMNS
} EogListStoreColumn;

struct _EogListStore {
        GtkListStore parent;
	EogListStorePrivate *priv;
};

struct _EogListStoreClass {
        GtkListStoreClass parent_class;

	/* Padding for future expansion */
	void (* _eog_reserved1) (void);
	void (* _eog_reserved2) (void);
	void (* _eog_reserved3) (void);
	void (* _eog_reserved4) (void);
};

GType           eog_list_store_get_type 	     (void) G_GNUC_CONST;

GtkListStore   *eog_list_store_new 		     (void);

GtkListStore   *eog_list_store_new_from_glist 	     (GList *list);

void            eog_list_store_append_image 	     (EogListStore *store,
						      EogImage     *image);

void            eog_list_store_add_files 	     (EogListStore *store,
						      GList        *file_list);

void            eog_list_store_remove_image 	     (EogListStore *store,
						      EogImage     *image);

gint            eog_list_store_get_pos_by_image      (EogListStore *store,
						      EogImage     *image);

EogImage       *eog_list_store_get_image_by_pos      (EogListStore *store,
						      gint   pos);

gint            eog_list_store_get_pos_by_iter 	     (EogListStore *store,
						      GtkTreeIter  *iter);

gint            eog_list_store_length                (EogListStore *store);

gint            eog_list_store_get_initial_pos 	     (EogListStore *store);

void            eog_list_store_thumbnail_set         (EogListStore *store,
						      GtkTreeIter *iter);

void            eog_list_store_thumbnail_unset       (EogListStore *store,
						      GtkTreeIter *iter);

void            eog_list_store_thumbnail_refresh     (EogListStore *store,
						      GtkTreeIter *iter);

G_END_DECLS

#endif
