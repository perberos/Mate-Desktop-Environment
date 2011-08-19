/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-settings.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#ifndef __USER_SETTINGS_H
#define __USER_SETTINGS_H

#include <gtk/gtk.h>
#include "gst-tool.h"
#include "users-tool.h"
#include "user-profiles.h"

#define ADMIN_GROUP "admin"


gboolean        user_delete                      (GtkTreeModel *model,
						  GtkTreePath *path);
void            user_settings_show               (OobsUser *user);
gint            user_settings_dialog_run         (GtkWidget *dialog);

OobsUser *      user_settings_dialog_get_data    (GtkWidget *dialog);
GdkPixbuf *     user_settings_get_user_face      (OobsUser *user, int size);
uid_t           user_settings_find_new_uid       (gint uid_min,
                                                  gint uid_max);
gboolean        user_settings_is_user_in_group   (OobsUser  *user,
                                                  OobsGroup *group);

gboolean        user_settings_check_revoke_admin_rights ();

void            on_user_new                      (GtkButton *button, gpointer user_data);


#endif /* USER_SETTINGS_H */
