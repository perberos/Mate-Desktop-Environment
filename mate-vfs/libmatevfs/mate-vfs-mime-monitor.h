/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   mate-vfs-mime-monitor.h: Class for noticing changes in MIME data.

   Copyright (C) 2000 Eazel, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; see the file COPYING.LIB. If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: John Sullivan <sullivan@eazel.com>
*/

#ifndef MATE_VFS_MIME_MONITOR_H
#define MATE_VFS_MIME_MONITOR_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_MIME_MONITOR_TYPE        (mate_vfs_mime_monitor_get_type ())
#define MATE_VFS_MIME_MONITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_MIME_MONITOR_TYPE, MateVFSMIMEMonitor))
#define MATE_VFS_MIME_MONITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_MIME_MONITOR_TYPE, MateVFSMIMEMonitorClass))
#define MATE_VFS_IS_MIME_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_MIME_MONITOR_TYPE))
#define MATE_VFS_IS_MIME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_MIME_MONITOR_TYPE))

typedef struct _MateVFSMIMEMonitorPrivate MateVFSMIMEMonitorPrivate;

typedef struct {
	/*< private >*/
	GObject object;

	MateVFSMIMEMonitorPrivate *priv;
} MateVFSMIMEMonitor;

typedef struct {
	GObjectClass parent_class;

	/* signals */
	void (*data_changed) (MateVFSMIMEMonitor *monitor);
} MateVFSMIMEMonitorClass;

GType                mate_vfs_mime_monitor_get_type (void);

/* There's a single MateVFSMIMEMonitor object.
 * The only thing you need it for is to connect to its signals.
 */
MateVFSMIMEMonitor *mate_vfs_mime_monitor_get      (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_MONITOR_H */
