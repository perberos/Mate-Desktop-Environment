/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* groups-table.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>
 */

#ifndef _GROUPS_TABLE_H
#define _GROUPS_TABLE_H

enum {
	COL_GROUP_NAME,
	COL_GROUP_ID,
	COL_GROUP_OBJECT,
	COL_GROUP_LAST
};

typedef struct 
{
	gchar *name;
	gboolean advanced_state_showable;
	gboolean basic_state_showable;
} GroupsTableConfig;

typedef struct GroupTreeItem_ GroupTreeItem;
	
struct GroupTreeItem_
{
	const gchar *group;
	guint GID;
	
	GroupTreeItem *children;
};

void          groups_table_clear               (void);
void	      create_groups_table              (void);
void          populate_groups_table            (void);
void	      group_table_update_content       (void);

GtkTreeModel *groups_table_get_model           ();
void          groups_table_set_group           (OobsGroup    *group,
                                                GtkTreeIter  *iter);
void          groups_table_add_group           (OobsGroup    *group);

GList        *groups_table_get_row_references  ();

#endif /* _GROUPS_TABLE_H */
