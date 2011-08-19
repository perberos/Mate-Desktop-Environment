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

#ifndef __NM_INTEGRATION_H__
#define __NM_INTEGRATION_H__

#include <glib.h>
#include "network-tool.h"

G_BEGIN_DECLS

typedef enum {
  NM_STATE_UNKNOWN = 0,
  NM_STATE_ASLEEP,
  NM_STATE_CONNECTING,
  NM_STATE_CONNECTED,
  NM_STATE_DISCONNECTED
} NMState;

NMState  nm_integration_get_state  (GstNetworkTool *tool);
void     nm_integration_sleep      (GstNetworkTool *tool);
void     nm_integration_wake       (GstNetworkTool *tool);
gboolean nm_integration_iface_supported (OobsIface *iface);

G_END_DECLS

#endif /* __NM_INTEGRATION_H__ */
