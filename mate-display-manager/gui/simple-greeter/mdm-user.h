/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2005 James M. Cape <jcape@ignore-your.tv>.
 * Copyright (C) 2007-2008 William Jon McCann <mccann@jhu.edu>
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
 * Facade object for user data, owned by MdmUserManager
 */

#ifndef __MDM_USER_H__
#define __MDM_USER_H__

#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define MDM_TYPE_USER (mdm_user_get_type ())
#define MDM_USER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MDM_TYPE_USER, MdmUser))
#define MDM_IS_USER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MDM_TYPE_USER))

typedef struct _MdmUser MdmUser;
typedef struct _MdmUserClass MdmUserClass;

GType                 mdm_user_get_type            (void) G_GNUC_CONST;

MdmUser              *mdm_user_new_from_object_path (const char *path);
const char           *mdm_user_get_object_path      (MdmUser *user);

gulong                mdm_user_get_uid             (MdmUser   *user);
const char           *mdm_user_get_user_name       (MdmUser   *user);
const char           *mdm_user_get_real_name       (MdmUser   *user);
guint                 mdm_user_get_num_sessions    (MdmUser   *user);
gboolean              mdm_user_is_logged_in        (MdmUser   *user);
gulong                mdm_user_get_login_frequency (MdmUser   *user);
const char           *mdm_user_get_icon_file       (MdmUser   *user);
const char           *mdm_user_get_primary_session_id (MdmUser *user);

GdkPixbuf            *mdm_user_render_icon         (MdmUser   *user,
                                                    gint       icon_size);

gint                  mdm_user_collate             (MdmUser   *user1,
                                                    MdmUser   *user2);
gboolean              mdm_user_is_loaded           (MdmUser *user);

G_END_DECLS

#endif
