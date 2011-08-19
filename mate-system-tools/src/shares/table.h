/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of shares-admin, a mate-system-tool frontend 
 * for shared folders administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#ifndef __TABLE_H__
#define __TABLE_H__

#include "gst-tool.h"

enum {
	   COL_PIXBUF,
	   COL_PATH,
	   COL_SHARE,
	   COL_ITER,
	   COL_LAST
};

enum {
	SHARES_DND_URI_LIST
};

void      table_create               (GstTool*);
void      table_clear                (void);
void      table_add_share            (OobsShare*, OobsListIter*);
gboolean  table_get_iter_with_path   (const gchar*, GtkTreeIter*);
OobsShare *table_get_share_at_iter    (GtkTreeIter*, OobsListIter**);
void      table_modify_share_at_iter (GtkTreeIter*, OobsShare*, OobsListIter*);
void      table_delete_share_at_iter (GtkTreeIter*);

#endif /* __TABLE_H__ */
