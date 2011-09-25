/* mdm-session-auditor.c - Object for auditing session login/logout
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
#include "mdm-session-auditor.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

struct _MdmSessionAuditorPrivate
{
        char *username;
        char *hostname;
        char *display_device;
};

static void mdm_session_auditor_finalize (GObject *object);
static void mdm_session_auditor_class_install_properties (MdmSessionAuditorClass *
                                              auditor_class);

static void mdm_session_auditor_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void mdm_session_auditor_get_property (GObject      *object,
                                              guint         prop_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

enum {
        PROP_0 = 0,
        PROP_USERNAME,
        PROP_HOSTNAME,
        PROP_DISPLAY_DEVICE
};

G_DEFINE_TYPE (MdmSessionAuditor, mdm_session_auditor, G_TYPE_OBJECT)

static void
mdm_session_auditor_class_init (MdmSessionAuditorClass *auditor_class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (auditor_class);

        object_class->finalize = mdm_session_auditor_finalize;

        mdm_session_auditor_class_install_properties (auditor_class);

        g_type_class_add_private (auditor_class, sizeof (MdmSessionAuditorPrivate));
}

static void
mdm_session_auditor_class_install_properties (MdmSessionAuditorClass *auditor_class)
{
        GObjectClass *object_class;
        GParamSpec   *param_spec;

        object_class = G_OBJECT_CLASS (auditor_class);
        object_class->set_property = mdm_session_auditor_set_property;
        object_class->get_property = mdm_session_auditor_get_property;

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
mdm_session_auditor_init (MdmSessionAuditor *auditor)
{
        auditor->priv = G_TYPE_INSTANCE_GET_PRIVATE (auditor,
                                                     MDM_TYPE_SESSION_AUDITOR,
                                                     MdmSessionAuditorPrivate);

}

static void
mdm_session_auditor_finalize (GObject *object)
{
        MdmSessionAuditor *auditor;
        GObjectClass *parent_class;

        auditor = MDM_SESSION_AUDITOR (object);

        g_free (auditor->priv->username);
        g_free (auditor->priv->hostname);
        g_free (auditor->priv->display_device);

        parent_class = G_OBJECT_CLASS (mdm_session_auditor_parent_class);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

void
mdm_session_auditor_set_username (MdmSessionAuditor *auditor,
                                  const char        *username)
{
        g_return_if_fail (MDM_IS_SESSION_AUDITOR (auditor));

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
mdm_session_auditor_set_hostname (MdmSessionAuditor *auditor,
                                  const char        *hostname)
{
        g_return_if_fail (MDM_IS_SESSION_AUDITOR (auditor));
        auditor->priv->hostname = g_strdup (hostname);
}

static void
mdm_session_auditor_set_display_device (MdmSessionAuditor *auditor,
                                        const char        *display_device)
{
        g_return_if_fail (MDM_IS_SESSION_AUDITOR (auditor));
        auditor->priv->display_device = g_strdup (display_device);
}

static char *
mdm_session_auditor_get_username (MdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->username);
}

static char *
mdm_session_auditor_get_hostname (MdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->hostname);
}

static char *
mdm_session_auditor_get_display_device (MdmSessionAuditor *auditor)
{
        return g_strdup (auditor->priv->display_device);
}

static void
mdm_session_auditor_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        MdmSessionAuditor *auditor;

        auditor = MDM_SESSION_AUDITOR (object);

        switch (prop_id) {
                case PROP_USERNAME:
                        mdm_session_auditor_set_username (auditor, g_value_get_string (value));
                break;

                case PROP_HOSTNAME:
                        mdm_session_auditor_set_hostname (auditor, g_value_get_string (value));
                break;

                case PROP_DISPLAY_DEVICE:
                        mdm_session_auditor_set_display_device (auditor, g_value_get_string (value));
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
mdm_session_auditor_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        MdmSessionAuditor *auditor;

        auditor = MDM_SESSION_AUDITOR (object);

        switch (prop_id) {
                case PROP_USERNAME:
                        g_value_take_string (value, mdm_session_auditor_get_username (auditor));
                break;

                case PROP_HOSTNAME:
                        g_value_take_string (value, mdm_session_auditor_get_hostname (auditor));
                break;

                case PROP_DISPLAY_DEVICE:
                        g_value_take_string (value, mdm_session_auditor_get_display_device (auditor));
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

MdmSessionAuditor *
mdm_session_auditor_new (const char *hostname,
                         const char *display_device)
{
        MdmSessionAuditor *auditor;

        auditor = g_object_new (MDM_TYPE_SESSION_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return auditor;
}

void
mdm_session_auditor_report_password_changed (MdmSessionAuditor *auditor)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_changed != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_changed (auditor);
        }
}

void
mdm_session_auditor_report_password_change_failure (MdmSessionAuditor *auditor)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_change_failure != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_password_change_failure (auditor);
        }
}

void
mdm_session_auditor_report_user_accredited (MdmSessionAuditor *auditor)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_user_accredited != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_user_accredited (auditor);
        }
}

void
mdm_session_auditor_report_login (MdmSessionAuditor *auditor)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login (auditor);
        }
}

void
mdm_session_auditor_report_login_failure (MdmSessionAuditor *auditor,
                                          int                error_code,
                                          const char        *error_message)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login_failure != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_login_failure (auditor, error_code, error_message);
        }
}

void
mdm_session_auditor_report_logout (MdmSessionAuditor *auditor)
{
        if (MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_logout != NULL) {
                MDM_SESSION_AUDITOR_GET_CLASS (auditor)->report_logout (auditor);
        }
}
