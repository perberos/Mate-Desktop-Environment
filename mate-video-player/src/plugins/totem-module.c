/*
 * This is a based on rb-module.c from Rhythmbox, which is based on 
 * gedit-module.h from gedit, which is based on Epiphany source code.
 *
 * Copyright (C) 2003 Marco Pesenti Gritti
 * Copyright (C) 2003, 2004 Christian Persch
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2007 - Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#include "config.h"

#include "totem-module.h"

#include <gmodule.h>

typedef struct _TotemModuleClass TotemModuleClass;

struct _TotemModuleClass
{
	GTypeModuleClass parent_class;
};

struct _TotemModule
{
	GTypeModule parent_instance;

	GModule *library;

	gchar *path;
	gchar *name;
	GType type;
};

typedef GType (*TotemModuleRegisterFunc) (GTypeModule *);

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (TotemModule, totem_module, G_TYPE_TYPE_MODULE)

static gboolean
totem_module_load (GTypeModule *gmodule)
{
	TotemModule *module = TOTEM_MODULE (gmodule);
	TotemModuleRegisterFunc register_func;

	module->library = g_module_open (module->path, 0);

	if (module->library == NULL) {
		g_warning ("%s", g_module_error());
		return FALSE;
	}

	/* extract symbols from the lib */
	if (!g_module_symbol (module->library, "register_totem_plugin", (void *)&register_func)) {
		g_warning ("%s", g_module_error ());
		g_module_close (module->library);
		return FALSE;
	}

	g_assert (register_func);

	module->type = register_func (gmodule);
	if (module->type == 0) {
		g_warning ("Invalid totem plugin contained by module %s", module->path);
		return FALSE;
	}

	return TRUE;
}

static void
totem_module_unload (GTypeModule *gmodule)
{
	TotemModule *module = TOTEM_MODULE (gmodule);

	g_module_close (module->library);

	module->library = NULL;
	module->type = 0;
}

const gchar *
totem_module_get_path (TotemModule *module)
{
	g_return_val_if_fail (TOTEM_IS_MODULE (module), NULL);

	return module->path;
}

GObject *
totem_module_new_object (TotemModule *module)
{
	GObject *obj;

	if (module->type == 0) {
		return NULL;
	}

	obj = g_object_new (module->type,
			    "name", module->name,
			    NULL);
	return obj;
}

static void
totem_module_init (TotemModule *module)
{

}

static void
totem_module_finalize (GObject *object)
{
	TotemModule *module = TOTEM_MODULE (object);

	g_free (module->path);
	g_free (module->name);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
totem_module_class_init (TotemModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	parent_class = (GObjectClass *) g_type_class_peek_parent (class);

	object_class->finalize = totem_module_finalize;

	module_class->load = totem_module_load;
	module_class->unload = totem_module_unload;
}

TotemModule *
totem_module_new (const gchar *path, const char *module)
{
	TotemModule *result;

	if (path == NULL || path[0] == '\0') {
		return NULL;
	}

	result = g_object_new (TOTEM_TYPE_MODULE, NULL);

	g_type_module_set_name (G_TYPE_MODULE (result), path);
	result->path = g_strdup (path);
	result->name = g_strdup (module);

	return result;
}

