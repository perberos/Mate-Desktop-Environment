/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef _NOTIFY_STACK_H_
#define _NOTIFY_STACK_H_

#include <gtk/gtk.h>

#include "daemon.h"

typedef enum
{
        NOTIFY_STACK_LOCATION_UNKNOWN = -1,
        NOTIFY_STACK_LOCATION_TOP_LEFT,
        NOTIFY_STACK_LOCATION_TOP_RIGHT,
        NOTIFY_STACK_LOCATION_BOTTOM_LEFT,
        NOTIFY_STACK_LOCATION_BOTTOM_RIGHT,
        NOTIFY_STACK_LOCATION_DEFAULT = NOTIFY_STACK_LOCATION_BOTTOM_RIGHT
} NotifyStackLocation;

typedef struct _NotifyStack NotifyStack;

NotifyStack    *notify_stack_new           (NotifyDaemon       *daemon,
                                            GdkScreen          *screen,
                                            guint               monitor,
                                            NotifyStackLocation stack_location);
void            notify_stack_destroy       (NotifyStack        *stack);

void            notify_stack_set_location  (NotifyStack        *stack,
                                            NotifyStackLocation location);
void            notify_stack_add_window    (NotifyStack        *stack,
                                            GtkWindow          *nw,
                                            gboolean            new_notification);
void            notify_stack_remove_window (NotifyStack        *stack,
                                            GtkWindow          *nw);
GList *         notify_stack_get_windows   (NotifyStack        *stack);
void            notify_stack_queue_update_position (NotifyStack        *stack);

#endif /* _NOTIFY_STACK_H_ */
