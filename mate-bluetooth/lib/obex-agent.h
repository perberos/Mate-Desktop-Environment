/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __OBEX_AGENT_H
#define __OBEX_AGENT_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define OBEX_TYPE_AGENT (obex_agent_get_type())
#define OBEX_AGENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
						OBEX_TYPE_AGENT, ObexAgent))
#define OBEX_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					OBEX_TYPE_AGENT, ObexAgentClass))
#define OBEX_IS_AGENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							OBEX_TYPE_AGENT))
#define OBEX_IS_AGENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							OBEX_TYPE_AGENT))
#define OBEX_GET_AGENT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					OBEX_TYPE_AGENT, ObexAgentClass))

typedef struct _ObexAgent ObexAgent;
typedef struct _ObexAgentClass ObexAgentClass;

struct _ObexAgent {
	GObject parent;
};

struct _ObexAgentClass {
	GObjectClass parent_class;
};

GType obex_agent_get_type(void);

ObexAgent *obex_agent_new(void);

gboolean obex_agent_setup(ObexAgent *agent, const char *path);

typedef gboolean (*ObexAgentReleaseFunc) (DBusGMethodInvocation *context,
								gpointer data);
typedef gboolean (*ObexAgentRequestFunc) (DBusGMethodInvocation *context,
					DBusGProxy *transfer, gpointer data);
typedef gboolean (*ObexAgentProgressFunc) (DBusGMethodInvocation *context,
					DBusGProxy *transfer,
					guint64 transferred, gpointer data);
typedef gboolean (*ObexAgentCompleteFunc) (DBusGMethodInvocation *context,
					DBusGProxy *transfer, gpointer data);
typedef gboolean (*ObexAgentErrorFunc) (DBusGMethodInvocation *context,
					DBusGProxy *transfer,
					const char *message,
					gpointer data);

void obex_agent_set_release_func(ObexAgent *agent,
				ObexAgentReleaseFunc func, gpointer data);
void obex_agent_set_request_func(ObexAgent *agent,
				ObexAgentRequestFunc func, gpointer data);
void obex_agent_set_progress_func(ObexAgent *agent,
				ObexAgentProgressFunc func, gpointer data);
void obex_agent_set_complete_func(ObexAgent *agent,
				ObexAgentCompleteFunc func, gpointer data);
void obex_agent_set_error_func(ObexAgent *agent,
			       ObexAgentErrorFunc func, gpointer data);
G_END_DECLS

#endif /* __OBEX_AGENT_H */
