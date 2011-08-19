/* Eye Of Mate - Main Window
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@mate.org>
 *      - Marco Pesenti Gritti <marco@mate.org>
 *      - Christian Persch <chpe@mate.org>
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

#ifndef EOG_MODULE_H
#define EOG_MODULE_H

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

typedef struct _EogModule EogModule;
typedef struct _EogModuleClass EogModuleClass;
typedef struct _EogModulePrivate EogModulePrivate;

#define EOG_TYPE_MODULE            (eog_module_get_type ())
#define EOG_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_MODULE, EogModule))
#define EOG_MODULE_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass),  EOG_TYPE_MODULE, EogModuleClass))
#define EOG_IS_MODULE(obj)	   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_MODULE))
#define EOG_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj),    EOG_TYPE_MODULE))
#define EOG_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),   EOG_TYPE_MODULE, EogModuleClass))

struct _EogModule {
	GTypeModule parent_instance;

	GModule *library;
	gchar   *path;
	GType    type;
};

struct _EogModuleClass {
	GTypeModuleClass parent_class;
};

G_GNUC_INTERNAL
GType		 eog_module_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
EogModule	*eog_module_new		(const gchar *path);

G_GNUC_INTERNAL
const gchar	*eog_module_get_path	(EogModule *module);

G_GNUC_INTERNAL
GObject		*eog_module_new_object	(EogModule *module);

G_END_DECLS

#endif
