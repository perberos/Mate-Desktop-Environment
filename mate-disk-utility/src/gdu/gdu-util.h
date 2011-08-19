/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-util.h
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

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_UTIL_H
#define __GDU_UTIL_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

gchar *gdu_util_get_size_for_display (guint64  size,
                                      gboolean use_pow2,
                                      gboolean long_string);

char *gdu_util_get_fstype_for_display (const char *fstype, const char *fsversion, gboolean long_string);

char *gdu_util_fstype_get_description (char *fstype);

char *gdu_util_part_table_type_get_description (char *part_type);

char *gdu_util_get_speed_for_display (guint64 size);

char *gdu_util_get_connection_for_display (const char *connection_interface, guint64 connection_speed);

char *gdu_util_get_desc_for_part_type (const char *part_scheme, const char *part_type);

char *gdu_get_job_description (const char *job_id);

char *gdu_linux_md_get_raid_level_for_display (const gchar *linux_md_raid_level,
                                               gboolean long_string);

char *gdu_linux_md_get_raid_level_description (const gchar *linux_md_raid_level);

gchar *gdu_util_ata_smart_status_to_desc (const gchar  *status,
                                          gboolean     *out_highlight,
                                          gchar       **out_action_text,
                                          GIcon       **out_icon);

typedef void (*GduUtilPartTypeForeachFunc) (const char *scheme,
                                            const char *type,
                                            const char *name,
                                            gpointer user_data);

void gdu_util_part_type_foreach (GduUtilPartTypeForeachFunc callback, gpointer user_data);

/* ---------------------------------------------------------------------------------------------------- */

char      *gdu_util_get_default_part_type_for_scheme_and_fstype (const char *scheme, const char *fstype, guint64 size);

/* ---------------------------------------------------------------------------------------------------- */

gboolean gdu_util_save_secret (GduDevice      *device,
                               const char     *secret,
                               gboolean        save_in_keyring_session);

gboolean gdu_util_delete_secret (GduDevice *device);

gboolean gdu_util_have_secret (GduDevice *device);

gchar *gdu_util_get_secret (GduDevice *device);

/* ---------------------------------------------------------------------------------------------------- */

GIcon *gdu_util_get_emblemed_icon (const gchar *name,
                                   const gchar *emblem_name);

G_END_DECLS

#endif /* __GDU_UTIL_H */
