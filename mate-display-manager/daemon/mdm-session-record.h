/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __MDM_SESSION_RECORD_H
#define __MDM_SESSION_RECORD_H

#include <glib.h>

G_BEGIN_DECLS

void
mdm_session_record_login  (GPid                  session_pid,
                           const char           *user_name,
                           const char           *host_name,
                           const char           *x11_display_name,
                           const char           *display_device);
void
mdm_session_record_logout (GPid                  session_pid,
                           const char           *user_name,
                           const char           *host_name,
                           const char           *x11_display_name,
                           const char           *display_device);
void
mdm_session_record_failed (GPid                  session_pid,
                           const char           *user_name,
                           const char           *host_name,
                           const char           *x11_display_name,
                           const char           *display_device);


G_END_DECLS

#endif /* MDM_SESSION_RECORD_H */
