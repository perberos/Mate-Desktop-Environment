/* Eye Of Mate - Python Module
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-python-module.h) by:
 * 	- Raphael Slinckx <raphael@slinckx.net>
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

#ifndef __EOG_PYTHON_MODULE_H__
#define __EOG_PYTHON_MODULE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EOG_TYPE_PYTHON_MODULE		  (eog_python_module_get_type ())
#define EOG_PYTHON_MODULE(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_PYTHON_MODULE, EogPythonModule))
#define EOG_PYTHON_MODULE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), EOG_TYPE_PYTHON_MODULE, EogPythonModuleClass))
#define EOG_IS_PYTHON_MODULE(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_PYTHON_MODULE))
#define EOG_IS_PYTHON_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), EOG_TYPE_PYTHON_MODULE))
#define EOG_PYTHON_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), EOG_TYPE_PYTHON_MODULE, EogPythonModuleClass))

typedef struct _EogPythonModule	EogPythonModule;
typedef struct _EogPythonModuleClass EogPythonModuleClass;
typedef struct _EogPythonModulePrivate EogPythonModulePrivate;

struct _EogPythonModuleClass {
	GTypeModuleClass parent_class;
};

struct _EogPythonModule {
	GTypeModule parent_instance;
};

G_GNUC_INTERNAL
GType			 eog_python_module_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
EogPythonModule		*eog_python_module_new			(const gchar* path,
								 const gchar *module);

G_GNUC_INTERNAL
GObject			*eog_python_module_new_object		(EogPythonModule *module);

G_GNUC_INTERNAL
gboolean		eog_python_init				(void);

G_GNUC_INTERNAL
void			eog_python_shutdown			(void);

G_GNUC_INTERNAL
void			eog_python_garbage_collect		(void);

G_END_DECLS

#endif /* __EOG_PYTHON_MODULE_H__ */
