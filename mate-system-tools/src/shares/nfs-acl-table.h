/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* nfs-acl-table.h: this file is part of shares-admin, a mate-system-tool frontend 
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

#ifndef _NFS_ACL_TABLE_H
#define _NFS_ACL_TABLE_H

#include <oobs/oobs.h>

enum {
	NFS_COL_PATTERN,
	NFS_COL_READ_ONLY,
	NFS_COL_LAST
};

void    nfs_acl_table_create          (void);
void    nfs_acl_table_add_element     (OobsShareAclElement* element);
void    nfs_acl_table_insert_elements (OobsShareNFS* share);

#endif /* _NFS_ACL_TABLE_H */
