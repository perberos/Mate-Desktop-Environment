/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Michael J. Chudobiak <mjc@avtechpulse.com>
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

#ifndef __GSD_HOUSEKEEPING_MANAGER_H
#define __GSD_HOUSEKEEPING_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_HOUSEKEEPING_MANAGER         (gsd_housekeeping_manager_get_type ())
#define GSD_HOUSEKEEPING_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_HOUSEKEEPING_MANAGER, GsdHousekeepingManager))
#define GSD_HOUSEKEEPING_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_HOUSEKEEPING_MANAGER, GsdHousekeepingManagerClass))
#define GSD_IS_HOUSEKEEPING_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_HOUSEKEEPING_MANAGER))
#define GSD_IS_HOUSEKEEPING_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_HOUSEKEEPING_MANAGER))
#define GSD_HOUSEKEEPING_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_HOUSEKEEPING_MANAGER, GsdHousekeepingManagerClass))

typedef struct GsdHousekeepingManagerPrivate GsdHousekeepingManagerPrivate;

typedef struct {
        GObject                        parent;
        GsdHousekeepingManagerPrivate *priv;
} GsdHousekeepingManager;

typedef struct {
        GObjectClass   parent_class;
} GsdHousekeepingManagerClass;

GType                    gsd_housekeeping_manager_get_type      (void);

GsdHousekeepingManager * gsd_housekeeping_manager_new           (void);
gboolean                 gsd_housekeeping_manager_start         (GsdHousekeepingManager  *manager,
                                                                 GError                 **error);
void                     gsd_housekeeping_manager_stop          (GsdHousekeepingManager  *manager);

G_END_DECLS

#endif /* __GSD_HOUSEKEEPING_MANAGER_H */
