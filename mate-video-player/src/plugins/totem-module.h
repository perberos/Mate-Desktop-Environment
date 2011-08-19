/*
 * This is a based on rb-module.h from Rhythmbox, which is based on 
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

#ifndef TOTEM_MODULE_H
#define TOTEM_MODULE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_MODULE		(totem_module_get_type ())
#define TOTEM_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_MODULE, TotemModule))
#define TOTEM_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_MODULE, TotemModuleClass))
#define TOTEM_IS_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_MODULE))
#define TOTEM_IS_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TOTEM_TYPE_MODULE))
#define TOTEM_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), TOTEM_TYPE_MODULE, TotemModuleClass))

typedef struct _TotemModule	TotemModule;

GType		 totem_module_get_type		(void) G_GNUC_CONST;;

TotemModule	*totem_module_new		(const gchar *path, const char *module);

const gchar	*totem_module_get_path		(TotemModule *module);

GObject		*totem_module_new_object	(TotemModule *module);

G_END_DECLS

#endif
