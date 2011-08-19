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


#ifndef __GDM_SIGNAL_HANDLER_H
#define __GDM_SIGNAL_HANDLER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SIGNAL_HANDLER         (gdm_signal_handler_get_type ())
#define GDM_SIGNAL_HANDLER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SIGNAL_HANDLER, GdmSignalHandler))
#define GDM_SIGNAL_HANDLER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SIGNAL_HANDLER, GdmSignalHandlerClass))
#define GDM_IS_SIGNAL_HANDLER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SIGNAL_HANDLER))
#define GDM_IS_SIGNAL_HANDLER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SIGNAL_HANDLER))
#define GDM_SIGNAL_HANDLER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SIGNAL_HANDLER, GdmSignalHandlerClass))


typedef gboolean (*GdmSignalHandlerFunc) (int           signal,
                                          gpointer      data);

typedef void     (*GdmShutdownHandlerFunc) (gpointer    data);

typedef struct GdmSignalHandlerPrivate GdmSignalHandlerPrivate;

typedef struct
{
        GObject                  parent;
        GdmSignalHandlerPrivate *priv;
} GdmSignalHandler;

typedef struct
{
        GObjectClass   parent_class;
} GdmSignalHandlerClass;

GType               gdm_signal_handler_get_type                (void);

GdmSignalHandler *  gdm_signal_handler_new                     (void);
void                gdm_signal_handler_set_fatal_func          (GdmSignalHandler       *handler,
                                                                GdmShutdownHandlerFunc  func,
                                                                gpointer                user_data);

void                gdm_signal_handler_add_fatal               (GdmSignalHandler    *handler);
guint               gdm_signal_handler_add                     (GdmSignalHandler    *handler,
                                                                int                  signal_number,
                                                                GdmSignalHandlerFunc callback,
                                                                gpointer             data);
void                gdm_signal_handler_remove                  (GdmSignalHandler    *handler,
                                                                guint                id);
void                gdm_signal_handler_remove_func             (GdmSignalHandler    *handler,
                                                                guint                signal_number,
                                                                GdmSignalHandlerFunc callback,
                                                                gpointer             data);


G_END_DECLS

#endif /* __GDM_SIGNAL_HANDLER_H */
