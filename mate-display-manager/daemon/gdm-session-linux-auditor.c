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
#include "gdm-session-linux-auditor.h"

#include <fcntl.h>
#include <pwd.h>
#include <syslog.h>
#include <unistd.h>

#include <libaudit.h>

#include <glib.h>

#include "gdm-common.h"

struct _GdmSessionLinuxAuditorPrivate
{
        int audit_fd;
};

static void gdm_session_linux_auditor_finalize (GObject *object);

G_DEFINE_TYPE (GdmSessionLinuxAuditor, gdm_session_linux_auditor, GDM_TYPE_SESSION_AUDITOR)

static void
log_user_message (GdmSessionAuditor *auditor,
                  gint               type,
                  gint               result)
{
        GdmSessionLinuxAuditor   *linux_auditor;
        char                      buf[512];
        char                     *username;
        char                     *hostname;
        char                     *display_device;
        struct passwd            *pw;

        linux_auditor = GDM_SESSION_LINUX_AUDITOR (auditor);

        g_object_get (G_OBJECT (auditor), "username", &username, NULL);
        g_object_get (G_OBJECT (auditor), "hostname", &hostname, NULL);
        g_object_get (G_OBJECT (auditor), "display-device", &display_device, NULL);

        if (username != NULL) {
                gdm_get_pwent_for_name (username, &pw);
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
gdm_session_linux_auditor_report_login (GdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 1);
}

static void
gdm_session_linux_auditor_report_login_failure (GdmSessionAuditor *auditor,
                                                  int                pam_error_code,
                                                  const char        *pam_error_string)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 0);
}

static void
gdm_session_linux_auditor_report_logout (GdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGOUT, 1);
}

static void
gdm_session_linux_auditor_class_init (GdmSessionLinuxAuditorClass *klass)
{
        GObjectClass           *object_class;
        GdmSessionAuditorClass *auditor_class;

        object_class = G_OBJECT_CLASS (klass);
        auditor_class = GDM_SESSION_AUDITOR_CLASS (klass);

        object_class->finalize = gdm_session_linux_auditor_finalize;

        auditor_class->report_login = gdm_session_linux_auditor_report_login;
        auditor_class->report_login_failure = gdm_session_linux_auditor_report_login_failure;
        auditor_class->report_logout = gdm_session_linux_auditor_report_logout;

        g_type_class_add_private (auditor_class, sizeof (GdmSessionLinuxAuditorPrivate));
}

static void
gdm_session_linux_auditor_init (GdmSessionLinuxAuditor *auditor)
{
        auditor->priv = G_TYPE_INSTANCE_GET_PRIVATE (auditor,
                                                     GDM_TYPE_SESSION_LINUX_AUDITOR,
                                                     GdmSessionLinuxAuditorPrivate);

        auditor->priv->audit_fd = audit_open ();
}

static void
gdm_session_linux_auditor_finalize (GObject *object)
{
        GdmSessionLinuxAuditor *linux_auditor;
        GObjectClass *parent_class;

        linux_auditor = GDM_SESSION_LINUX_AUDITOR (object);

        close (linux_auditor->priv->audit_fd);

        parent_class = G_OBJECT_CLASS (gdm_session_linux_auditor_parent_class);
        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}


GdmSessionAuditor *
gdm_session_linux_auditor_new (const char *hostname,
                               const char *display_device)
{
        GObject *auditor;

        auditor = g_object_new (GDM_TYPE_SESSION_LINUX_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return GDM_SESSION_AUDITOR (auditor);
}


