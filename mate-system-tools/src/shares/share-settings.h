/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-settings.h: this file is part of shares-admin, a mate-system-tool frontend 
 * for shared folders administration.
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
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#ifndef _SHARE_SETTINGS_H
#define _SHARE_SETTINGS_H

#include <gtk/gtk.h>

enum {
	SHARE_DONT_SHARE,
	SHARE_THROUGH_SMB,
	SHARE_THROUGH_NFS
};

void          share_settings_set_name_from_folder (const gchar*);
void          share_settings_dialog_run           (const gchar*,
						   gboolean);
void          share_settings_dialog_run_for_iter  (const gchar*,
						   GtkTreeIter*,
						   gboolean);
void          share_settings_create_combo      (void);
gboolean      share_settings_validate          (void);

void          smb_settings_prepare_dialog      (void);
void          smb_settings_save                (void);


#endif /* _SHARE_SETTINGS_H */
