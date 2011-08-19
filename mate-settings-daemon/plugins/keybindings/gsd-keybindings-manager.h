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

#ifndef __GSD_KEYBINDINGS_MANAGER_H
#define __GSD_KEYBINDINGS_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_KEYBINDINGS_MANAGER         (gsd_keybindings_manager_get_type ())
#define GSD_KEYBINDINGS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_KEYBINDINGS_MANAGER, GsdKeybindingsManager))
#define GSD_KEYBINDINGS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_KEYBINDINGS_MANAGER, GsdKeybindingsManagerClass))
#define GSD_IS_KEYBINDINGS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_KEYBINDINGS_MANAGER))
#define GSD_IS_KEYBINDINGS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_KEYBINDINGS_MANAGER))
#define GSD_KEYBINDINGS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_KEYBINDINGS_MANAGER, GsdKeybindingsManagerClass))

typedef struct GsdKeybindingsManagerPrivate GsdKeybindingsManagerPrivate;

typedef struct
{
        GObject                     parent;
        GsdKeybindingsManagerPrivate *priv;
} GsdKeybindingsManager;

typedef struct
{
        GObjectClass   parent_class;
} GsdKeybindingsManagerClass;

GType                   gsd_keybindings_manager_get_type            (void);

GsdKeybindingsManager *       gsd_keybindings_manager_new                 (void);
gboolean                gsd_keybindings_manager_start               (GsdKeybindingsManager *manager,
                                                               GError         **error);
void                    gsd_keybindings_manager_stop                (GsdKeybindingsManager *manager);

G_END_DECLS

#endif /* __GSD_KEYBINDINGS_MANAGER_H */
