/* Eye Of Mate - EOG Plugin Manager
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@mate.org>
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

#ifndef __EOG_PLUGIN_MANAGER_H__
#define __EOG_PLUGIN_MANAGER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EogPluginManager EogPluginManager;
typedef struct _EogPluginManagerClass EogPluginManagerClass;
typedef struct _EogPluginManagerPrivate EogPluginManagerPrivate;

#define EOG_TYPE_PLUGIN_MANAGER              (eog_plugin_manager_get_type())
#define EOG_PLUGIN_MANAGER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_PLUGIN_MANAGER, EogPluginManager))
#define EOG_PLUGIN_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_PLUGIN_MANAGER, EogPluginManagerClass))
#define EOG_IS_PLUGIN_MANAGER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_PLUGIN_MANAGER))
#define EOG_IS_PLUGIN_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_PLUGIN_MANAGER))
#define EOG_PLUGIN_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_PLUGIN_MANAGER, EogPluginManagerClass))

struct _EogPluginManager {
	GtkVBox vbox;

	EogPluginManagerPrivate *priv;
};

struct _EogPluginManagerClass {
	GtkVBoxClass parent_class;
};

G_GNUC_INTERNAL
GType		 eog_plugin_manager_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*eog_plugin_manager_new		(void);

G_END_DECLS

#endif  /* __EOG_PLUGIN_MANAGER_H__  */
