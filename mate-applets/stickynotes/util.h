/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <glib.h>
#include <gtk/gtk.h>

gchar * get_current_date(const gchar *format);
void	xstuff_change_workspace (GtkWindow *window,
			         int        new_space);
int	xstuff_get_current_workspace (GtkWindow *window);

#endif /* #ifndef __UTIL_H__ */
