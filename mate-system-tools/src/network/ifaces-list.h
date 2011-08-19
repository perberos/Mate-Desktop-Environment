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

#ifndef __IFACES_LIST_H
#define __IFACES_LIST_H

#include "gst.h"

enum {
  COL_ACTIVE,
  COL_IMAGE,
  COL_DESC,
  COL_OBJECT,
  COL_DEV,
  COL_INCONSISTENT,
  COL_NOT_INCONSISTENT,
  COL_SHOW_IFACE_NAME,
  COL_LAST
};

typedef enum {
  SEARCH_DEV,
  SEARCH_TYPE
} IfaceSearchTerm;

GtkTreeModel* ifaces_model_create                   (void);
void          ifaces_model_add_interface            (OobsIface*, gboolean);
void          ifaces_model_modify_interface_at_iter (GtkTreeIter*);
OobsIface*    ifaces_model_search_iface             (IfaceSearchTerm, const gchar*);
void          ifaces_model_clear                    (void);

GtkTreeModelFilter* gateways_filter_model_create    (GtkTreeModel *model);

GtkTreeView*  ifaces_list_create                   (GstTool *tool);

#endif /* __IFACES_LIST_H */
