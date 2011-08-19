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


#ifndef __GDM_SERVER_H
#define __GDM_SERVER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SERVER         (gdm_server_get_type ())
#define GDM_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SERVER, GdmServer))
#define GDM_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SERVER, GdmServerClass))
#define GDM_IS_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SERVER))
#define GDM_IS_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SERVER))
#define GDM_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SERVER, GdmServerClass))

typedef struct GdmServerPrivate GdmServerPrivate;

typedef struct
{
        GObject           parent;
        GdmServerPrivate *priv;
} GdmServer;

typedef struct
{
        GObjectClass   parent_class;

        void (* ready)             (GdmServer *server);
        void (* exited)            (GdmServer *server,
                                    int        exit_code);
        void (* died)              (GdmServer *server,
                                    int        signal_number);
} GdmServerClass;

GType               gdm_server_get_type  (void);
GdmServer *         gdm_server_new       (const char *display_id,
                                          const char *auth_file);
gboolean            gdm_server_start     (GdmServer   *server);
gboolean            gdm_server_stop      (GdmServer   *server);
char *              gdm_server_get_display_device (GdmServer *server);

G_END_DECLS

#endif /* __GDM_SERVER_H */
