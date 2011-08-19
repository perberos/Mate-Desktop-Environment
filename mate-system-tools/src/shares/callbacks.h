/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.h: this file is part of shares-admin, a mate-system-tool frontend 
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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

gboolean  on_shares_table_button_press        (GtkWidget*, GdkEventButton*, gpointer);
gboolean  on_shares_table_popup_menu          (GtkWidget*, GtkWidget*);
void      on_shares_table_selection_changed   (GtkTreeSelection*, gpointer);
void      on_add_share_clicked                (GtkWidget*, gpointer);
void      on_edit_share_clicked               (GtkWidget*, gpointer);
void      on_delete_share_clicked             (GtkWidget*, gpointer);

void      on_share_type_changed               (GtkWidget*, gpointer);
void      on_share_nfs_delete_clicked         (GtkWidget*, gpointer);
void      on_share_nfs_add_clicked            (GtkWidget*, gpointer);
void      on_share_nfs_host_type_changed      (GtkWidget*, gpointer);
void      on_share_smb_settings_clicked       (GtkWidget*, gpointer);
void      on_dialog_validate                  (GtkWidget*, gpointer);
void      on_shared_folder_changed            (GtkWidget *widget, gpointer data);
void      on_share_smb_name_modified          (GtkWidget *widget, gpointer data);

void      on_shares_dragged_folder            (GtkWidget *widget, GdkDragContext *context,
					       gint x, gint y, GtkSelectionData *selection_data,
					       guint info, guint time, gpointer gdata);

void      on_is_wins_toggled                  (GtkWidget *widget, gpointer data);
gboolean  on_workgroup_focus_out              (GtkWidget *widget, GdkEvent *event, gpointer data);
gboolean  on_wins_server_focus_out            (GtkWidget *widget, GdkEvent *event, gpointer data);


#endif /* __CALLBACKS_H */
