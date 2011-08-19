/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-nfs-add-hosts.h: this file is part of shares-admin, a mate-system-tool frontend 
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

#ifndef _SHARE_NFS_ACL_H
#define _SHARE_NFS_ACL_H

#include <gtk/gtk.h>

enum {
	NFS_SHARE_IFACE,
	NFS_SHARE_HOSTNAME,
	NFS_SHARE_ADDRESS,
	NFS_SHARE_NETWORK
};

enum {
	NFS_HOST_COL_PIXBUF,
	NFS_HOST_COL_NAME,
	NFS_HOST_COL_TYPE,
	NFS_HOST_COL_NETWORK,
	NFS_HOST_COL_NETMASK,
	NFS_HOST_COL_LAST
};

void           share_nfs_add_hosts_dialog_setup       (void);
GtkWidget*     share_nfs_add_hosts_dialog_prepare     (void);
gboolean       share_nfs_add_hosts_dialog_get_data    (gchar**, gboolean*);

#endif /* _SHARE_NFS_ACL_H */
