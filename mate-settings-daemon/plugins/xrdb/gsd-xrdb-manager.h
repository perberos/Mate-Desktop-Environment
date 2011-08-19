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

#ifndef __GSD_XRDB_MANAGER_H
#define __GSD_XRDB_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_XRDB_MANAGER         (gsd_xrdb_manager_get_type ())
#define GSD_XRDB_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_XRDB_MANAGER, GsdXrdbManager))
#define GSD_XRDB_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_XRDB_MANAGER, GsdXrdbManagerClass))
#define GSD_IS_XRDB_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_XRDB_MANAGER))
#define GSD_IS_XRDB_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_XRDB_MANAGER))
#define GSD_XRDB_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_XRDB_MANAGER, GsdXrdbManagerClass))

typedef struct GsdXrdbManagerPrivate GsdXrdbManagerPrivate;

typedef struct
{
        GObject                     parent;
        GsdXrdbManagerPrivate *priv;
} GsdXrdbManager;

typedef struct
{
        GObjectClass   parent_class;
} GsdXrdbManagerClass;

GType                   gsd_xrdb_manager_get_type            (void);

GsdXrdbManager *        gsd_xrdb_manager_new                 (void);
gboolean                gsd_xrdb_manager_start               (GsdXrdbManager *manager,
                                                              GError        **error);
void                    gsd_xrdb_manager_stop                (GsdXrdbManager *manager);

G_END_DECLS

#endif /* __GSD_XRDB_MANAGER_H */
