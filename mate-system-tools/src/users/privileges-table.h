/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* privileges-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
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
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>
 */

#ifndef __PRIVILEGES_TABLE_H
#define __PRIVILEGES_TABLE_H

#include "user-profiles.h"

void     create_user_privileges_table      (void);
void     create_profile_privileges_table   (void);
void     privileges_table_clear            (void);

void     populate_privileges_table         (GtkWidget*, gchar*);
GList*   user_privileges_get_list          (GList*);

void     privileges_table_add_group        (OobsGroup    *group);

void     privileges_table_set_from_profile (GstUserProfile *profile);
void     privileges_table_set_from_user    (OobsUser *user);

void     privileges_table_save             (OobsUser *user);


#endif /* __PRIVILEGES_TABLE_H */
