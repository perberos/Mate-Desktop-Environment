/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __DBUSPROXY_H
#define __DBUSPROXY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define EGG_TYPE_DBUS_PROXY		(egg_dbus_proxy_get_type ())
#define EGG_DBUS_PROXY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EGG_TYPE_DBUS_PROXY, EggDbusProxy))
#define EGG_DBUS_PROXY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), EGG_TYPE_DBUS_PROXY, EggDbusProxyClass))
#define EGG_IS_DBUS_PROXY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), EGG_TYPE_DBUS_PROXY))
#define EGG_IS_DBUS_PROXY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), EGG_TYPE_DBUS_PROXY))
#define EGG_DBUS_PROXY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), EGG_TYPE_DBUS_PROXY, EggDbusProxyClass))

typedef struct EggDbusProxyPrivate EggDbusProxyPrivate;

typedef struct
{
	GObject		 parent;
	EggDbusProxyPrivate *priv;
} EggDbusProxy;

typedef struct
{
	GObjectClass	parent_class;
	void		(* proxy_status)	(EggDbusProxy	*proxy,
						 gboolean	 status);
} EggDbusProxyClass;

GType		 egg_dbus_proxy_get_type		(void);
EggDbusProxy	*egg_dbus_proxy_new			(void);

DBusGProxy	*egg_dbus_proxy_assign		(EggDbusProxy		*dbus_proxy,
						 DBusGConnection	*connection,
						 const gchar		*service,
						 const gchar		*path,
						 const gchar		*interface);
DBusGProxy	*egg_dbus_proxy_get_proxy	(EggDbusProxy		*egg_dbus_proxy);
gboolean	 egg_dbus_proxy_is_connected	(EggDbusProxy		*egg_dbus_proxy);

G_END_DECLS

#endif	/* __DBUSPROXY_H */

