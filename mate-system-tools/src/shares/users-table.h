/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* users-table.c: this file is part of shares-admin, a mate-system-tool frontend
 * for shared folders administration.
 *
 * Copyright (C) 2007 Carlos Garnacho
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
 * Authors: Carlos Garnacho <carlosg@mate.org>.
 */

#ifndef __USERS_TABLE_H
#define __USERS_TABLE_H

#include "gst.h"
#include "shares-tool.h"

G_BEGIN_DECLS

void  users_table_create     (GstTool *tool);
void  users_table_set_config (GstSharesTool *tool);


G_END_DECLS

#endif /* __USERS_TABLE_H */
