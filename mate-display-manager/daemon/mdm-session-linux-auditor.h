/* mdm-linux-session-auditor.h - Object for linux auditing of session login/logout
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
#ifndef MDM_SESSION_LINUX_AUDITOR_H
#define MDM_SESSION_LINUX_AUDITOR_H

#include <glib.h>
#include <glib-object.h>

#include "mdm-session-auditor.h"

G_BEGIN_DECLS
#define MDM_TYPE_SESSION_LINUX_AUDITOR (mdm_session_linux_auditor_get_type ())
#define MDM_SESSION_LINUX_AUDITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_SESSION_LINUX_AUDITOR, MdmSessionLinuxAuditor))
#define MDM_SESSION_LINUX_AUDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_SESSION_LINUX_AUDITOR, MdmSessionLinuxAuditorClass))
#define MDM_IS_SESSION_LINUX_AUDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_SESSION_LINUX_AUDITOR))
#define MDM_IS_SESSION_LINUX_AUDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_SESSION_LINUX_AUDITOR))
#define MDM_SESSION_LINUX_AUDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MDM_TYPE_SESSION_LINUX_AUDITOR, MdmSessionLinuxAuditorClass))
#define MDM_SESSION_LINUX_AUDITOR_ERROR (mdm_session_linux_auditor_error_quark ())
typedef struct _MdmSessionLinuxAuditor        MdmSessionLinuxAuditor;
typedef struct _MdmSessionLinuxAuditorClass   MdmSessionLinuxAuditorClass;
typedef struct _MdmSessionLinuxAuditorPrivate MdmSessionLinuxAuditorPrivate;

struct _MdmSessionLinuxAuditor
{
        MdmSessionAuditor                parent;

        /*< private > */
        MdmSessionLinuxAuditorPrivate *priv;
};

struct _MdmSessionLinuxAuditorClass
{
        MdmSessionAuditorClass parent_class;
};

GType              mdm_session_linux_auditor_get_type                       (void);
MdmSessionAuditor *mdm_session_linux_auditor_new                            (const char *hostname,
                                                                             const char *display_device);
G_END_DECLS
#endif /* MDM_SESSION_LINUX_AUDITOR_H */
