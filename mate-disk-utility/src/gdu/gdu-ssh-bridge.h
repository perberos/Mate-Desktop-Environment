/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#if defined (__GDU_INSIDE_GDU_H)
#error "Can't include a private header in the public header file."
#endif


#ifndef __GDU_SSH_BRIDGE_H
#define __GDU_SSH_BRIDGE_H

#include <dbus/dbus-glib.h>
#include "gdu-types.h"

DBusGConnection * _gdu_ssh_bridge_connect (const gchar      *ssh_user_name,
                                           const gchar      *ssh_address,
                                           GPid             *out_pid,
                                           GError          **error);

#endif /* __GDU_SSH_BRIDGE_H */
