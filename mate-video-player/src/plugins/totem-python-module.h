/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Heavily based on code from Rhythmbox and Gedit.
 *
 * Copyright (C) 2005 Raphael Slinckx
 * Copyright (C) 2007 Philip Withnall
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
 * Saturday 19th May 2007: Philip Withnall: Add exception clause.
 * See license_change file for details.
 */

#ifndef TOTEM_PYTHON_MODULE_H
#define TOTEM_PYTHON_MODULE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_PYTHON_MODULE		(totem_python_module_get_type ())
#define TOTEM_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_PYTHON_MODULE, TotemPythonModule))
#define TOTEM_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_PYTHON_MODULE, TotemPythonModuleClass))
#define TOTEM_IS_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_PYTHON_MODULE))
#define TOTEM_IS_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TOTEM_TYPE_PYTHON_MODULE))
#define TOTEM_PYTHON_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), TOTEM_TYPE_PYTHON_MODULE, TotemPythonModuleClass))

typedef struct
{
	GTypeModuleClass parent_class;
} TotemPythonModuleClass;

typedef struct
{
	GTypeModule parent_instance;
} TotemPythonModule;

GType			totem_python_module_get_type		(void);
TotemPythonModule	*totem_python_module_new		(const gchar* path, const gchar *module);
GObject			*totem_python_module_new_object		(TotemPythonModule *module);

/* --- python utils --- */
void			totem_python_garbage_collect		(void);
void			totem_python_shutdown			(void);

G_END_DECLS

#endif
