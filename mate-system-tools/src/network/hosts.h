/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#ifndef __HOSTS_H_
#define __HOSTS_H_

#include <gtk/gtk.h>
#include "network-tool.h"

enum {
  COL_HOST_IP,
  COL_HOST_ALIASES,
  COL_HOST_OBJECT,
  COL_HOST_ITER,
  COL_HOST_LAST
};

GtkTreeView*   host_aliases_list_create    (GstTool*);
void           host_aliases_add            (OobsStaticHost*, OobsListIter*);
void           host_aliases_run_dialog     (GstNetworkTool *network_tool,
					    GtkTreeIter    *iter);
void           host_aliases_clear          (void);
void           host_aliases_check_fields   (void);

#endif /* __HOSTS_H_ */
