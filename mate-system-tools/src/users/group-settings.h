/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* group_settings.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#ifndef __GROUP_SETTINGS_H
#define __GROUP_SETTINGS_H

#include "gst.h"

gboolean     group_delete                       (GtkTreeModel *model,
						 GtkTreePath  *path);

GtkWidget*   group_settings_dialog_new          (OobsGroup    *group);
gboolean     group_settings_dialog_group_is_new (void);

gint         group_settings_dialog_run          (GtkWidget    *dialog,
						 OobsGroup    *group);

gid_t        group_settings_find_new_gid        (void);
gboolean     group_settings_group_exists	(const gchar *name);
OobsGroup *  group_settings_get_group_from_name (const gchar *name);
void         group_settings_dialog_get_data     (OobsGroup    *group);
OobsGroup *  group_settings_dialog_get_group    (void);


#endif /* __GROUP_SETTINGS_H */

