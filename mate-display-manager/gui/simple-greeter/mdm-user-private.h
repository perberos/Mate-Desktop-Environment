/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2005 James M. Cape <jcape@ignore-your.tv>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Private interfaces to the MdmUser object
 */

#ifndef __MDM_USER_PRIVATE_H_
#define __MDM_USER_PRIVATE_H_

#include <pwd.h>

#include "mdm-user.h"

G_BEGIN_DECLS

void _mdm_user_update_from_object_path (MdmUser    *user,
                                        const char *object_path);

void _mdm_user_update_from_pwent (MdmUser             *user,
                                  const struct passwd *pwent);

void _mdm_user_update_login_frequency (MdmUser *user,
                                       guint64  login_frequency);

void _mdm_user_add_session      (MdmUser             *user,
                                 const char          *session_id);
void _mdm_user_remove_session   (MdmUser             *user,
                                 const char          *session_id);

G_END_DECLS

#endif /* !__MDM_USER_PRIVATE__ */
