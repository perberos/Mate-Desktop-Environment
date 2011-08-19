/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
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
#ifndef __SOUND_THEME_FILE_UTILS_HH__
#define __SOUND_THEME_FILE_UTILS_HH__

#include <gio/gio.h>

char *custom_theme_dir_path (const char *child);
gboolean custom_theme_dir_is_empty (void);
void create_custom_theme (const char *parent);

void delete_custom_theme_dir (void);
void delete_old_files (const char **sounds);
void delete_disabled_files (const char **sounds);

void add_disabled_file (const char **sounds);
void add_custom_file (const char **sounds, const char *filename);

void custom_theme_update_time (void);

#endif /* __SOUND_THEME_FILE_UTILS_HH__ */
