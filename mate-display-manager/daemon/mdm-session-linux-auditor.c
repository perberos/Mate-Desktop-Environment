/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
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
#include "config.h"
#include "mdm-session-linux-auditor.h"

#include <fcntl.h>
#include <pwd.h>
#include <syslog.h>
#include <unistd.h>

#include <libaudit.h>

#include <glib.h>

#include "mdm-common.h"

struct _MdmSessionLinuxAuditorPrivate
{
        int audit_fd;
};

static void mdm_session_linux_auditor_finalize (GObject *object);

G_DEFINE_TYPE (MdmSessionLinuxAuditor, mdm_session_linux_auditor, MDM_TYPE_SESSION_AUDITOR)

static void
log_user_message (MdmSessionAuditor *auditor,
                  gint               type,
                  gint               result)
{
        MdmSessionLinuxAuditor   *linux_auditor;
        char                      buf[512];
        char                     *username;
        char                     *hostname;
        char                     *display_device;
        struct passwd            *pw;

        linux_auditor = MDM_SESSION_LINUX_AUDITOR (auditor);

        g_object_get (G_OBJECT (auditor), "username", &username, NULL);
        g_object_get (G_OBJECT (auditor), "hostname", &hostname, NULL);
        g_object_get (G_OBJECT (auditor), "display-device", &display_device, NULL);

        if (username != NULL) {
                mdm_get_pwent_for_name (username, &pw);
        } else {
                username = g_strdup ("unknown");
                pw = NULL;
        }

        if (pw != NULL) {
                g_snprintf (buf, sizeof (buf), "uid=%d", pw->pw_uid);
                audit_log_user_message (linux_auditor->priv->audit_fd, type,
                                        buf, hostname, NULL, display_device,
                                        result);
        } else {
                g_snprintf (buf, sizeof (buf), "acct=%s", username);
                audit_log_user_message (linux_auditor->priv->audit_fd, type,
                                        buf, hostname, NULL, display_device,
                                        result);
        }

        g_free (username);
        g_free (hostname);
        g_free (display_device);
}

static void
mdm_session_linux_auditor_report_login (MdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 1);
}

static void
mdm_session_linux_auditor_report_login_failure (MdmSessionAuditor *auditor,
                                                  int                pam_error_code,
                                                  const char        *pam_error_string)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 0);
}

static void
mdm_session_linux_auditor_report_logout (MdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGOUT, 1);
}

static void
mdm_session_linux_auditor_class_init (MdmSessionLinuxAuditorClass *klass)
{
        GObjectClass           *object_class;
        MdmSessionAuditorClass *auditor_class;

        object_class = G_OBJECT_CLASS (klass);
        auditor_class = MDM_SESSION_AUDITOR_CLASS (klass);

        object_class->finalize = mdm_session_linux_auditor_finalize;

        auditor_class->report_login = mdm_session_linux_auditor_report_login;
        auditor_class->report_login_failure = mdm_session_linux_auditor_report_login_failure;
        auditor_class->report_logout = mdm_session_linux_auditor_report_logout;

        g_type_class_add_private (auditor_class, sizeof (MdmSessionLinuxAuditorPrivate));
}

static void
mdm_session_linux_auditor_init (MdmSessionLinuxAuditor *auditor)
{
        auditor->priv = G_TYPE_INSTANCE_GET_PRIVATE (auditor,
                                                     MDM_TYPE_SESSION_LINUX_AUDITOR,
                                                     MdmSessionLinuxAuditorPrivate);

        auditor->priv->audit_fd = audit_open ();
}

static void
mdm_session_linux_auditor_finalize (GObject *object)
{
        MdmSessionLinuxAuditor *linux_auditor;
        GObjectClass *parent_class;

        linux_auditor = MDM_SESSION_LINUX_AUDITOR (object);

        close (linux_auditor->priv->audit_fd);

        parent_class = G_OBJECT_CLASS (mdm_session_linux_auditor_parent_class);
        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}


MdmSessionAuditor *
mdm_session_linux_auditor_new (const char *hostname,
                               const char *display_device)
{
        GObject *auditor;

        auditor = g_object_new (MDM_TYPE_SESSION_LINUX_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return MDM_SESSION_AUDITOR (auditor);
}


