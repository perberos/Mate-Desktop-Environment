/* logview-utils.h - misc logview utilities
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
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
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 */

#ifndef __LOGVIEW_UTILS_H__
#define __LOGVIEW_UTILS_H__

#include <glib.h>

typedef struct {
  GDate *date;
  int first_line;
  int last_line;
  int timestamp_len;
} Day;

GSList * log_read_dates (const char **buffer_lines, time_t current);
gint     days_compare (gconstpointer a, gconstpointer b);
void     logview_utils_day_free (Day *day);
Day *    logview_utils_day_copy (Day *day);
GSList * logview_utils_day_list_copy (GSList *days);


#endif /* __LOGVIEW_UTILS_H__ */