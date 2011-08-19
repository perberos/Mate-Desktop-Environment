/*
 * Mini-Commander Applet
 * Copyright (C) 1998, 1999 Oliver Maruhn <oliver@maruhn.com>
 *
 * Author: Oliver Maruhn <oliver@maruhn.com>
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

#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <glib.h>

G_BEGIN_DECLS

#include "mini-commander_applet.h"

void       mc_create_command_entry       (MCData    *mc);
int        mc_show_history               (GtkWidget *widget,
				          MCData    *mc);
int        mc_show_file_browser          (GtkWidget *widget,
				          MCData    *mc);
void       mc_command_update_entry_color (MCData    *mc);
void       mc_command_update_entry_size  (MCData    *mc);

G_END_DECLS

#endif /* __COMMAND_LINE_H__ */
