/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
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

#ifndef __TABLE_H
#define __TABLE_H

#include "gst.h"
#include "users-tool.h"

enum {
	TABLE_USERS,
	TABLE_GROUPS,
};

GtkWidget*  popup_menu_create          (GtkWidget *wigdet, gint table);
void	    create_tables	       (GstUsersTool *tool);
GList*      table_get_row_references   (gint table, GtkTreeModel **model);
void        table_populate_profiles    (GstUsersTool *tool, GList *names);
void        table_set_default_profile  (GstUsersTool *tool);


#endif /* __TABLE_H */
