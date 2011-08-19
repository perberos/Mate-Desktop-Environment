/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendi.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DRW_MONITOR_H__
#define __DRW_MONITOR_H__

#include <glib-object.h>

#define DRW_TYPE_MONITOR         (drw_monitor_get_type ())
#define DRW_MONITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DRW_TYPE_MONITOR, DrwMonitor))
#define DRW_MONITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), DRW_TYPE_MONITOR, DrwMonitorClass))
#define DRW_IS_MONITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DRW_TYPE_MONITOR))
#define DRW_IS_MONITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), DRW_TYPE_MONITOR))
#define DRW_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DRW_TYPE_MONITOR, DrwMonitorClass))

typedef struct _DrwMonitor      DrwMonitor;
typedef struct _DrwMonitorClass DrwMonitorClass;
typedef struct _DrwMonitorPriv  DrwMonitorPriv;

struct _DrwMonitor {
        GObject        parent;

        DrwMonitorPriv *priv;
};

struct _DrwMonitorClass {
        GObjectClass parent_class;
};

GType        drw_monitor_get_type         (void) G_GNUC_CONST;
DrwMonitor  *drw_monitor_new              (void);

#endif /* __DRW_MONITOR_H__ */
