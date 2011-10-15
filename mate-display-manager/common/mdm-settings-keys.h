/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _MDM_SETTINGS_KEYS_H
#define _MDM_SETTINGS_KEYS_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_KEY_USER "daemon/User"
#define MDM_KEY_GROUP "daemon/Group"
#define MDM_KEY_AUTO_LOGIN_ENABLE "daemon/AutomaticLoginEnable"
#define MDM_KEY_AUTO_LOGIN_USER "daemon/AutomaticLogin"
#define MDM_KEY_TIMED_LOGIN_ENABLE "daemon/TimedLoginEnable"
#define MDM_KEY_TIMED_LOGIN_USER "daemon/TimedLogin"
#define MDM_KEY_TIMED_LOGIN_DELAY "daemon/TimedLoginDelay"

#define MDM_KEY_DEBUG "debug/Enable"

#define MDM_KEY_INCLUDE "greeter/Include"
#define MDM_KEY_EXCLUDE "greeter/Exclude"
#define MDM_KEY_INCLUDE_ALL "greeter/IncludeAll"

#define MDM_KEY_DISALLOW_TCP "security/DisallowTCP"

#define MDM_KEY_XDMCP_ENABLE "xdmcp/Enable"
#define MDM_KEY_MAX_PENDING "xdmcp/MaxPending"
#define MDM_KEY_MAX_SESSIONS "xdmcp/MaxSessions"
#define MDM_KEY_MAX_WAIT "xdmcp/MaxWait"
#define MDM_KEY_DISPLAYS_PER_HOST "xdmcp/DisplaysPerHost"
#define MDM_KEY_UDP_PORT "xdmcp/Port"
#define MDM_KEY_INDIRECT "xdmcp/HonorIndirect"
#define MDM_KEY_MAX_WAIT_INDIRECT "xdmcp/MaxWaitIndirect"
#define MDM_KEY_PING_INTERVAL "xdmcp/PingIntervalSeconds"
#define MDM_KEY_WILLING "xdmcp/Willing"

#define MDM_KEY_MULTICAST "chooser/Multicast"
#define MDM_KEY_MULTICAST_ADDR "chooser/MulticastAddr"

#ifdef __cplusplus
}
#endif

#endif /* _MDM_SETTINGS_KEYS_H */
