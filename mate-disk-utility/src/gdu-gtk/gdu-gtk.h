/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-gtk.h
 *
 * Copyright (C) 2007 David Zeuthen
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

#ifndef GDU_GTK_H
#define GDU_GTK_H

#ifndef GDU_GTK_API_IS_SUBJECT_TO_CHANGE
#error  libgdu-gtk is unstable API. You must define GDU_GTK_API_IS_SUBJECT_TO_CHANGE before including gdu-gtk/gdu-gtk.h
#endif

#define __GDU_GTK_INSIDE_GDU_GTK_H
#include <gdu-gtk/gdu-gtk-types.h>
#include <gdu-gtk/gdu-gtk-enumtypes.h>
#include <gdu-gtk/gdu-time-label.h>
#include <gdu-gtk/gdu-pool-tree-view.h>
#include <gdu-gtk/gdu-pool-tree-model.h>
#include <gdu-gtk/gdu-size-widget.h>
#include <gdu-gtk/gdu-create-linux-md-dialog.h>
#include <gdu-gtk/gdu-ata-smart-dialog.h>
#include <gdu-gtk/gdu-spinner.h>
#include <gdu-gtk/gdu-volume-grid.h>
#include <gdu-gtk/gdu-details-table.h>
#include <gdu-gtk/gdu-details-element.h>
#include <gdu-gtk/gdu-error-dialog.h>
#include <gdu-gtk/gdu-confirmation-dialog.h>
#include <gdu-gtk/gdu-button-element.h>
#include <gdu-gtk/gdu-button-table.h>
#include <gdu-gtk/gdu-dialog.h>
#include <gdu-gtk/gdu-edit-partition-dialog.h>
#include <gdu-gtk/gdu-format-dialog.h>
#include <gdu-gtk/gdu-partition-dialog.h>
#include <gdu-gtk/gdu-create-partition-dialog.h>
#include <gdu-gtk/gdu-edit-name-dialog.h>
#include <gdu-gtk/gdu-disk-selection-widget.h>
#include <gdu-gtk/gdu-add-component-linux-md-dialog.h>
#include <gdu-gtk/gdu-edit-linux-md-dialog.h>
#include <gdu-gtk/gdu-edit-linux-lvm2-dialog.h>
#include <gdu-gtk/gdu-drive-benchmark-dialog.h>
#include <gdu-gtk/gdu-connect-to-server-dialog.h>
#include <gdu-gtk/gdu-create-linux-lvm2-volume-dialog.h>
#include <gdu-gtk/gdu-add-pv-linux-lvm2-dialog.h>
#undef __GDU_GTK_INSIDE_GDU_GTK_H

G_BEGIN_DECLS

gboolean gdu_util_dialog_show_filesystem_busy (GtkWidget *parent_window, GduPresentable *presentable);


char *gdu_util_dialog_ask_for_new_secret (GtkWidget      *parent_window,
                                          gboolean       *save_in_keyring,
                                          gboolean       *save_in_keyring_session);

char *gdu_util_dialog_ask_for_secret (GtkWidget       *parent_window,
                                      GduPresentable  *presentable,
                                      gboolean         bypass_keyring,
                                      gboolean         indicate_wrong_passphrase,
                                      gboolean        *asked_user);

gboolean gdu_util_dialog_change_secret (GtkWidget       *parent_window,
                                        GduPresentable  *presentable,
                                        char           **old_secret,
                                        char           **new_secret,
                                        gboolean        *save_in_keyring,
                                        gboolean        *save_in_keyring_session,
                                        gboolean         bypass_keyring,
                                        gboolean         indicate_wrong_passphrase);

gboolean gdu_util_delete_confirmation_dialog (GtkWidget   *parent_window,
                                              const char  *title,
                                              const char  *primary_text,
                                              const char  *secondary_text,
                                              const char  *affirmative_action_button_mnemonic);

/* ---------------------------------------------------------------------------------------------------- */

GtkWidget *gdu_util_fstype_combo_box_create         (GduPool *pool,
                                                     const char *include_extended_partitions_for_scheme);
void       gdu_util_fstype_combo_box_rebuild        (GtkWidget  *combo_box,
                                                     GduPool *pool,
                                                     const char *include_extended_partitions_for_scheme);
void       gdu_util_fstype_combo_box_set_desc_label (GtkWidget *combo_box, GtkWidget *desc_label);
gboolean   gdu_util_fstype_combo_box_select         (GtkWidget  *combo_box,
                                                     const char *fstype);
char      *gdu_util_fstype_combo_box_get_selected   (GtkWidget  *combo_box);

/* ---------------------------------------------------------------------------------------------------- */

GtkWidget *gdu_util_part_type_combo_box_create       (const char *part_scheme);
void       gdu_util_part_type_combo_box_rebuild      (GtkWidget  *combo_box,
                                                      const char *part_scheme);
gboolean   gdu_util_part_type_combo_box_select       (GtkWidget  *combo_box,
                                                      const char *part_type);
char      *gdu_util_part_type_combo_box_get_selected (GtkWidget  *combo_box);

/* ---------------------------------------------------------------------------------------------------- */

GtkWidget *gdu_util_part_table_type_combo_box_create         (void);
void       gdu_util_part_table_type_combo_box_set_desc_label (GtkWidget *combo_box, GtkWidget *desc_label);
gboolean   gdu_util_part_table_type_combo_box_select         (GtkWidget  *combo_box,
                                                              const char *part_table_type);
char      *gdu_util_part_table_type_combo_box_get_selected   (GtkWidget  *combo_box);

/* ---------------------------------------------------------------------------------------------------- */

GdkPixbuf *gdu_util_get_pixbuf_for_presentable (GduPresentable *presentable, GtkIconSize size);

GdkPixbuf *gdu_util_get_pixbuf_for_presentable_at_pixel_size (GduPresentable *presentable, gint pixel_size);

void       gdu_util_get_mix_color (GtkWidget    *widget,
                                   GtkStateType  state,
                                   gchar        *color_buf,
                                   gsize         color_buf_size);


G_END_DECLS

#endif /* GDU_GTK_H */
