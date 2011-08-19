/*
 * Copyright (C) 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MENU_MONITOR_H__
#define __MENU_MONITOR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct MenuMonitor MenuMonitor;

typedef enum
{
  MENU_MONITOR_EVENT_INVALID = 0,
  MENU_MONITOR_EVENT_CREATED = 1,
  MENU_MONITOR_EVENT_DELETED = 2,
  MENU_MONITOR_EVENT_CHANGED = 3
} MenuMonitorEvent;

typedef void (*MenuMonitorNotifyFunc) (MenuMonitor      *monitor,
				       MenuMonitorEvent  event,
				       const char       *path,
				       gpointer          user_data);


MenuMonitor *menu_get_file_monitor      (const char *path);
MenuMonitor *menu_get_directory_monitor (const char *path);

MenuMonitor *menu_monitor_ref   (MenuMonitor *monitor);
void         menu_monitor_unref (MenuMonitor *monitor);

void menu_monitor_add_notify    (MenuMonitor           *monitor,
				 MenuMonitorNotifyFunc  notify_func,
				 gpointer               user_data);
void menu_monitor_remove_notify (MenuMonitor           *monitor,
				 MenuMonitorNotifyFunc  notify_func,
				 gpointer               user_data);

G_END_DECLS

#endif /* __MENU_MONITOR_H__ */
