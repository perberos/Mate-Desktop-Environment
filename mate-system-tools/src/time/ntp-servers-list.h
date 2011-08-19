/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2005 Carlos Garnacho.
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

#ifndef __NTP_SERVERS_LIST_H__
#define __NTP_SERVERS_LIST_H__

#include <glib.h>
#include "time-tool.h"

GtkWidget*  ntp_servers_list_get   (GstTimeTool *tool);
void        ntp_servers_list_check (GtkWidget     *list,
				    OobsListIter  *iter,
				    OobsNTPServer *server);
void        on_ntp_addserver       (GtkWidget *widget,
				    GstDialog *dialog);

#endif /* __NTP_SERVERS_LIST_H__ */
