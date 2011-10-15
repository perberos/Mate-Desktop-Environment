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

#ifndef __MATE_XSETTINGS_MANAGER_H
#define __MATE_XSETTINGS_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_XSETTINGS_MANAGER         (mate_xsettings_manager_get_type ())
#define MATE_XSETTINGS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_TYPE_XSETTINGS_MANAGER, MateXSettingsManager))
#define MATE_XSETTINGS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MATE_TYPE_XSETTINGS_MANAGER, MateXSettingsManagerClass))
#define MATE_IS_XSETTINGS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_TYPE_XSETTINGS_MANAGER))
#define MATE_IS_XSETTINGS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_TYPE_XSETTINGS_MANAGER))
#define MATE_XSETTINGS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MATE_TYPE_XSETTINGS_MANAGER, MateXSettingsManagerClass))

typedef struct MateXSettingsManagerPrivate MateXSettingsManagerPrivate;

typedef struct
{
        GObject                     parent;
        MateXSettingsManagerPrivate *priv;
} MateXSettingsManager;

typedef struct
{
        GObjectClass   parent_class;
} MateXSettingsManagerClass;

GType                   mate_xsettings_manager_get_type            (void);

MateXSettingsManager * mate_xsettings_manager_new                 (void);
gboolean                mate_xsettings_manager_start               (MateXSettingsManager *manager,
                                                                     GError               **error);
void                    mate_xsettings_manager_stop                (MateXSettingsManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __MATE_XSETTINGS_MANAGER_H */
