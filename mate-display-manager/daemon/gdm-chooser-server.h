/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __GDM_CHOOSER_SERVER_H
#define __GDM_CHOOSER_SERVER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_SERVER         (gdm_chooser_server_get_type ())
#define GDM_CHOOSER_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CHOOSER_SERVER, GdmChooserServer))
#define GDM_CHOOSER_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CHOOSER_SERVER, GdmChooserServerClass))
#define GDM_IS_CHOOSER_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CHOOSER_SERVER))
#define GDM_IS_CHOOSER_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CHOOSER_SERVER))
#define GDM_CHOOSER_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CHOOSER_SERVER, GdmChooserServerClass))

typedef struct GdmChooserServerPrivate GdmChooserServerPrivate;

typedef struct
{
        GObject                  parent;
        GdmChooserServerPrivate *priv;
} GdmChooserServer;

typedef struct
{
        GObjectClass   parent_class;

        void (* hostname_selected)          (GdmChooserServer  *chooser_server,
                                             const char        *hostname);
        void (* connected)                  (GdmChooserServer  *chooser_server);
        void (* disconnected)               (GdmChooserServer  *chooser_server);
} GdmChooserServerClass;

GType               gdm_chooser_server_get_type              (void);
GdmChooserServer *  gdm_chooser_server_new                   (const char       *display_id);

gboolean            gdm_chooser_server_start                 (GdmChooserServer *chooser_server);
gboolean            gdm_chooser_server_stop                  (GdmChooserServer *chooser_server);
char *              gdm_chooser_server_get_address           (GdmChooserServer *chooser_server);

G_END_DECLS

#endif /* __GDM_CHOOSER_SERVER_H */
