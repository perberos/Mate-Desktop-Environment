/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
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

#include <config.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include "drw-monitor.h"

struct _DrwMonitorPriv {
	XScreenSaverInfo *ss_info;
	guint             timeout_id;
	unsigned long     last_idle;

	time_t            last_activity;
};

/* Signals */
enum {
	ACTIVITY,
	LAST_SIGNAL
};


static void     drw_monitor_class_init    (DrwMonitorClass *klass);
static void     drw_monitor_init          (DrwMonitor      *monitor);
static void     drw_monitor_finalize      (GObject         *object);
static gboolean drw_monitor_setup         (DrwMonitor      *monitor);

static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];


GType
drw_monitor_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (DrwMonitorClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) drw_monitor_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (DrwMonitor),
			0,              /* n_preallocs */
			(GInstanceInitFunc) drw_monitor_init,
		};

		object_type = g_type_register_static (G_TYPE_OBJECT,
                                                      "DrwMonitor", 
                                                      &object_info, 0);
	}

	return object_type;
}

static void
drw_monitor_class_init (DrwMonitorClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        
        object_class->finalize = drw_monitor_finalize;

	signals[ACTIVITY] = 
		g_signal_new ("activity",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
drw_monitor_init (DrwMonitor *monitor)
{
        DrwMonitorPriv *priv;
        
        priv = g_new0 (DrwMonitorPriv, 1);
        monitor->priv = priv;

	drw_monitor_setup (monitor);
}

static void
drw_monitor_finalize (GObject *object)
{
        DrwMonitor     *monitor = DRW_MONITOR (object);
        DrwMonitorPriv *priv;
        
        priv = monitor->priv;

	g_source_remove (priv->timeout_id);
	priv->timeout_id = 0;

	if (priv->ss_info) {
		XFree (priv->ss_info);
	}
	
	g_free (priv);
	monitor->priv = NULL;

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static gboolean
drw_monitor_timeout (DrwMonitor *monitor)
{
	DrwMonitorPriv *priv;
 	time_t          now;

	priv = monitor->priv;
	
	if (XScreenSaverQueryInfo (GDK_DISPLAY (), DefaultRootWindow (GDK_DISPLAY ()), priv->ss_info) != 0) {
		if (priv->ss_info->idle < priv->last_idle) {
 			now = time (NULL);
 
 			if (now - priv->last_activity < 25) {
 				g_signal_emit (monitor, signals[ACTIVITY], 0, NULL);
 			}
 			
 			priv->last_activity = now;
		}

		priv->last_idle = priv->ss_info->idle;
	}

	return TRUE;
}

static gboolean
drw_monitor_setup (DrwMonitor *monitor)
{
	DrwMonitorPriv *priv;
	int             event_base;
	int             error_base;

	priv = monitor->priv;

	if (!XScreenSaverQueryExtension (GDK_DISPLAY (), &event_base, &error_base)) {
		return FALSE;
	}

	priv->ss_info = XScreenSaverAllocInfo ();

	priv->last_activity = time (NULL);
	
	priv->timeout_id = g_timeout_add_seconds (3, (GSourceFunc) drw_monitor_timeout, monitor);
	
	return TRUE;
}

DrwMonitor *
drw_monitor_new (void)
{
	return g_object_new (DRW_TYPE_MONITOR, NULL);
}

