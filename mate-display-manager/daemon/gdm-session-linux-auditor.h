/* gdm-linux-session-auditor.h - Object for linux auditing of session login/logout
 *
 * Copyright (C) 2004, 2008 Sun Microsystems, Inc.
 * Copyright (C) 2005, 2008 Red Hat, Inc.
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
 *
 * Written by: Brian A. Cameron <Brian.Cameron@sun.com>
 *             Gary Winiger <Gary.Winiger@sun.com>
 *             Ray Strode <rstrode@redhat.com>
 *             Steve Grubb <sgrubb@redhat.com>
 */
#ifndef GDM_SESSION_LINUX_AUDITOR_H
#define GDM_SESSION_LINUX_AUDITOR_H

#include <glib.h>
#include <glib-object.h>

#include "gdm-session-auditor.h"

G_BEGIN_DECLS
#define GDM_TYPE_SESSION_LINUX_AUDITOR (gdm_session_linux_auditor_get_type ())
#define GDM_SESSION_LINUX_AUDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_SESSION_LINUX_AUDITOR, GdmSessionLinuxAuditor))
#define GDM_SESSION_LINUX_AUDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_SESSION_LINUX_AUDITOR, GdmSessionLinuxAuditorClass))
#define GDM_IS_SESSION_LINUX_AUDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_SESSION_LINUX_AUDITOR))
#define GDM_IS_SESSION_LINUX_AUDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_SESSION_LINUX_AUDITOR))
#define GDM_SESSION_LINUX_AUDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GDM_TYPE_SESSION_LINUX_AUDITOR, GdmSessionLinuxAuditorClass))
#define GDM_SESSION_LINUX_AUDITOR_ERROR (gdm_session_linux_auditor_error_quark ())
typedef struct _GdmSessionLinuxAuditor        GdmSessionLinuxAuditor;
typedef struct _GdmSessionLinuxAuditorClass   GdmSessionLinuxAuditorClass;
typedef struct _GdmSessionLinuxAuditorPrivate GdmSessionLinuxAuditorPrivate;

struct _GdmSessionLinuxAuditor
{
        GdmSessionAuditor                parent;

        /*< private > */
        GdmSessionLinuxAuditorPrivate *priv;
};

struct _GdmSessionLinuxAuditorClass
{
        GdmSessionAuditorClass parent_class;
};

GType              gdm_session_linux_auditor_get_type                       (void);
GdmSessionAuditor *gdm_session_linux_auditor_new                            (const char *hostname,
                                                                             const char *display_device);
G_END_DECLS
#endif /* GDM_SESSION_LINUX_AUDITOR_H */
