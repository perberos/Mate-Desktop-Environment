/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 *
 */

#ifndef __MDM_SIGNAL_HANDLER_H
#define __MDM_SIGNAL_HANDLER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_SIGNAL_HANDLER \
	(mdm_signal_handler_get_type())

#define MDM_SIGNAL_HANDLER(o) \
	(G_TYPE_CHECK_INSTANCE_CAST((o), MDM_TYPE_SIGNAL_HANDLER, MdmSignalHandler))

#define MDM_SIGNAL_HANDLER_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SIGNAL_HANDLER, MdmSignalHandlerClass))

#define MDM_IS_SIGNAL_HANDLER(o) \
	(G_TYPE_CHECK_INSTANCE_TYPE((o), MDM_TYPE_SIGNAL_HANDLER))

#define MDM_IS_SIGNAL_HANDLER_CLASS(k) \
	(G_TYPE_CHECK_CLASS_TYPE((k), MDM_TYPE_SIGNAL_HANDLER))

#define MDM_SIGNAL_HANDLER_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS((o), MDM_TYPE_SIGNAL_HANDLER, MdmSignalHandlerClass))


typedef gboolean (*MdmSignalHandlerFunc)(int signal, gpointer data);

typedef void (*MdmShutdownHandlerFunc)(gpointer data);

typedef struct MdmSignalHandlerPrivate MdmSignalHandlerPrivate;

typedef struct {
	GObject parent;
	MdmSignalHandlerPrivate* priv;
} MdmSignalHandler;

typedef struct {
	GObjectClass parent_class;
} MdmSignalHandlerClass;

GType mdm_signal_handler_get_type(void);

MdmSignalHandler* mdm_signal_handler_new(void);
void mdm_signal_handler_set_fatal_func(MdmSignalHandler* handler, MdmShutdownHandlerFunc func, gpointer user_data);

void mdm_signal_handler_add_fatal(MdmSignalHandler* handler);
guint mdm_signal_handler_add(MdmSignalHandler* handler, int signal_number, MdmSignalHandlerFunc callback, gpointer data);
void mdm_signal_handler_remove(MdmSignalHandler* handler, guint id);
void mdm_signal_handler_remove_func(MdmSignalHandler* handler, guint signal_number, MdmSignalHandlerFunc callback, gpointer data);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_SIGNAL_HANDLER_H */
