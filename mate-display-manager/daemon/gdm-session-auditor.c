/* gdm-session-auditor.c - Object for auditing session login/logout
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
#include "gdm-session-auditor.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

struct _GdmSessionAuditorPrivate
{
        char *username;
        char *hostname;
        char *display_device;
};

static void gdm_session_auditor_finalize (GObject *object);
static void gdm_session_auditor_class_install_properties (GdmSessionAuditorClass *
                                              auditor_class);

static void gdm_session_auditor_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void gdm_session_auditor_get_property (GObject      *object,
                                              guint         prop_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

enum {
        PROP_0 = 0,
        PROP_USERNAME,
        PROP_HOSTNAME,
        PROP_DISPLAY_DEVICE
};

G_DEFINE_TYPE (GdmSessionAuditor, gdm_session_auditor, G_TYPE_OBJECT)

static void
gdm_session_auditor_class_init (GdmSessionAuditorClass *auditor_class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (auditor_class);

        object_class->finalize = gdm_session_auditor_finalize;

        gdm_session_auditor_class_install_properties (auditor_class);

        g_type_class_add_private (auditor_class, sizeof (GdmSessionAuditorPrivate));
}

static void
gdm_session_auditor_class_install_properties (GdmSessionAuditorClass *auditor_class)
{
        GObjectClass *object_class;
        GParamSpec   *param_spec;

        object_class = G_OBJECT_CLASS (auditor_class);
        object_class->set_property = gdm_session_auditor_set_property;
        object_class->get_property = gdm_session_auditor_get_property;

        param_spec = g_param_spec_string ("username", _("Username"),
                                        _("The username"),
                                        NULL, G_PARAM_READWRITE);
        g_object_class_install_property (object_class, PROP_USERNAME, param_spec);

        param_spec = g_param_spec_string ("hostname", _("Hostname"),
                                        _("The hostname"),
                                        NULL,
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_HOSTNAME, param_spec);

        param_spec = g_param_spec_string ("display-device", _("Display Device"),
                                        _("The display device"),
                                        NULL,
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_DISPLAY_DEVICE, param_spec);
}

static void
gdm_session_auditor_init (GdmSessionAuditor *auditor)
{
        auditor->priv = G_TYPE_INSTANCE_GET_PRIVATE (auditor,
                                                     GDM_TYPE_SESSION_AUDITOR,
                                                     GdmSessionAuditorPrivate);

}

static void
gdm_session_auditor_finalize (GObject *object)
{
        GdmSessionAuditor *auditor;
        GObjectClass *parent_class;

        auditor = GDM_SESSION_AUDITOR (object);

        g_free (auditor->priv->username);
        g_free (auditor->priv->hostname);
        g_free (auditor->priv->display_device);

        parent_class = G_OBJECT_CLASS (gdm_session_auditor_parent_class);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

void
gdm_session_auditor_set_username (GdmSessionAuditor *auditor,
                                  const char        *username)
{
        g_return_if_fail (GDM_IS_SESSION_AUDITOR (auditor));

        if (username == auditor->priv->username) {
                return;
        }

        if ((username == NULL || auditor->priv->username == NULL) ||
            strcmp (username, auditor->priv->username) != 0) {
                auditor->priv->username = g_strdup (username);
                g_object_notify (G_OBJECT (auditor), "username");
        }
}

static void
gdm_session_auditor_set_hostname (GdmSessionAuditor *auditor,
                                  const char        *hostname)
{
        g_return_if_fail (GDM_IS_SESSION_AUDITOR (auditor));
        auditor->priv->hostname = g_strdup (hostname);
}

static void
gdm_session_auditor_set_display_device (GdmSessionAuditor *auditor,
                                        const char        *display_device)
{
        g_return_if_fail (GDM_IS_SESSION_AUDITOR (auditor));
        auditor->priv->display_device = g_strdup (display_device);
}

static char *
gdm_session_auditor_get_username (GdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->username);
}

static char *
gdm_session_auditor_get_hostname (GdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->hostname);
}

static char *
gdm_session_auditor_get_display_device (GdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->display_device);
}

static void
gdm_session_auditor_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        GdmSessionAuditor *auditor;

        auditor = GDM_SESSION_AUDITOR (object);

        switch (prop_id) {
                case PROP_USERNAME:
                        gdm_session_auditor_set_username (auditor, g_value_get_string (value));
                break;

                case PROP_HOSTNAME:
                        gdm_session_auditor_set_hostname (auditor, g_value_get_string (value));
                break;

                case PROP_DISPLAY_DEVICE:
                        gdm_session_auditor_set_display_device (auditor, g_value_get_string (value));
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
gdm_session_auditor_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        GdmSessionAuditor *auditor;

        auditor = GDM_SESSION_AUDITOR (object);

        switch (prop_id) {
                case PROP_USERNAME:
                        g_value_take_string (value, gdm_session_auditor_get_username (auditor));
                break;

                case PROP_HOSTNAME:
                        g_value_take_string (value, gdm_session_auditor_get_hostname (auditor));
                break;

                case PROP_DISPLAY_DEVICE:
                        g_value_take_string (value, gdm_session_auditor_get_display_device (auditor));
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GdmSessionAuditor *
gdm_session_auditor_new (const char *hostname,
                         const char *display_device)
{
        GdmSessionAuditor *auditor;

        auditor = g_object_new (GDM_TYPE_SESSION_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return auditor;
}

void
gdm_session_auditor_report_password_changed (GdmSessionAuditor *auditor)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_changed != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_changed (auditor);
        }
}

void
gdm_session_auditor_report_password_change_failure (GdmSessionAuditor *auditor)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_change_failure != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_change_failure (auditor);
        }
}

void
gdm_session_auditor_report_user_accredited (GdmSessionAuditor *auditor)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_user_accredited != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_user_accredited (auditor);
        }
}

void
gdm_session_auditor_report_login (GdmSessionAuditor *auditor)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login (auditor);
        }
}

void
gdm_session_auditor_report_login_failure (GdmSessionAuditor *auditor,
                                          int                error_code,
                                          const char        *error_message)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login_failure != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login_failure (auditor, error_code, error_message);
        }
}

void
gdm_session_auditor_report_logout (GdmSessionAuditor *auditor)
{
        if (GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_logout != NULL) {
                GDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_logout (auditor);
        }
}
