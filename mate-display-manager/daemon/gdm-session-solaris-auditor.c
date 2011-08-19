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
#include "gdm-session-solaris-auditor.h"

#include <syslog.h>
#include <security/pam_appl.h>
#include <pwd.h>

#include <fcntl.h>
#include <bsm/adt.h>
#include <bsm/adt_event.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

struct _GdmSessionSolarisAuditorPrivate
{
        adt_session_data_t *audit_session_handle;

        guint password_change_initiated : 1;
        guint password_changed  : 1;
        guint user_accredited  : 1;

        /* cached values to prevent repeated calls
         * to getpwnam
         */
        char               *username;
        uid_t               uid;
        gid_t               gid;
};

static void gdm_session_solaris_auditor_finalize (GObject *object);

G_DEFINE_TYPE (GdmSessionSolarisAuditor, gdm_session_solaris_auditor, GDM_TYPE_SESSION_AUDITOR)

static void
gdm_session_solaris_auditor_report_password_changed (GdmSessionAuditor *auditor)
{
        GdmSessionSolarisAuditor *solaris_auditor;

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);
        solaris_auditor->priv->password_change_initiated = TRUE;
        solaris_auditor->priv->password_changed = TRUE;
}

static void
gdm_session_solaris_auditor_report_password_change_failure (GdmSessionAuditor *auditor)
{
        GdmSessionSolarisAuditor *solaris_auditor;

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);
        solaris_auditor->priv->password_change_initiated = TRUE;
        solaris_auditor->priv->password_changed = FALSE;
}

static void
gdm_session_solaris_auditor_report_user_accredited (GdmSessionAuditor *auditor)
{
        GdmSessionSolarisAuditor *solaris_auditor;

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);
        solaris_auditor->priv->user_accredited = TRUE;
}

static void
gdm_session_solaris_auditor_report_login (GdmSessionAuditor *auditor)
{
       GdmSessionSolarisAuditor *solaris_auditor;
       adt_session_data_t       *adt_ah;  /* Audit session handle */
       adt_event_data_t         *event;   /* Event to generate */

       solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);

       g_return_if_fail (solaris_auditor->priv->username != NULL);

       adt_ah = NULL;
       if (adt_start_session (&adt_ah, NULL, ADT_USE_PROC_DATA) != 0) {
               syslog (LOG_AUTH | LOG_ALERT,
                       "adt_start_session (ADT_login): %m");
               goto cleanup;
       }

       if (adt_set_user (adt_ah, solaris_auditor->priv->uid,
           solaris_auditor->priv->gid, solaris_auditor->priv->uid,
           solaris_auditor->priv->gid, NULL, ADT_USER) != 0) {
               syslog (LOG_AUTH | LOG_ALERT,
                       "adt_set_user (ADT_login, %s): %m",
                       solaris_auditor->priv->username);
       }

       event = adt_alloc_event (adt_ah, ADT_login);
       if (event == NULL) {
               syslog (LOG_AUTH | LOG_ALERT, "adt_alloc_event (ADT_login): %m");
       } else if (adt_put_event (event, ADT_SUCCESS, ADT_SUCCESS) != 0) {
               syslog (LOG_AUTH | LOG_ALERT,
                       "adt_put_event (ADT_login, ADT_SUCCESS): %m");
       }

       if (solaris_auditor->priv->password_changed) {

               g_assert (solaris_auditor->priv->password_change_initiated);

               /* Also audit password change */
               adt_free_event (event);
               event = adt_alloc_event (adt_ah, ADT_passwd);
               if (event == NULL) {
                       syslog (LOG_AUTH | LOG_ALERT,
                               "adt_alloc_event (ADT_passwd): %m");
               } else if (adt_put_event (event, ADT_SUCCESS,
                                         ADT_SUCCESS) != 0) {

                       syslog (LOG_AUTH | LOG_ALERT,
                               "adt_put_event (ADT_passwd, ADT_SUCCESS): %m");
               }
       }

       adt_free_event (event);

cleanup:
       solaris_auditor->priv->audit_session_handle = adt_ah;
}

static void
gdm_session_solaris_auditor_report_login_failure (GdmSessionAuditor *auditor,
                                                  int                pam_error_code,
                                                  const char        *pam_error_string)
{
        GdmSessionSolarisAuditor *solaris_auditor;
        char                     *hostname;
        char                     *display_device;
        adt_session_data_t       *ah;     /* Audit session handle     */
        adt_event_data_t         *event;  /* Event to generate        */
        adt_termid_t             *tid;    /* Terminal ID for failures */

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);
        g_object_get (G_OBJECT (auditor),
                      "hostname", &hostname,
                      "display-device", &display_device, NULL);

        if (solaris_auditor->priv->user_accredited) {
                if (adt_start_session (&ah, NULL, ADT_USE_PROC_DATA) != 0) {
                        syslog (LOG_AUTH | LOG_ALERT,
                                "adt_start_session (ADT_login, ADT_FAILURE): %m");
                        goto cleanup;
                }
        } else {
                if (adt_start_session (&ah, NULL, 0) != 0) {
                        syslog (LOG_AUTH | LOG_ALERT,
                                "adt_start_session (ADT_login, ADT_FAILURE): %m");
                        goto cleanup;
                }

                /* If display is on console or VT */
                if (hostname != NULL && hostname[0] != '\0') {
                        /* Login from a remote host */
                        if (adt_load_hostname (hostname, &tid) != 0) {
                                syslog (LOG_AUTH | LOG_ALERT,
                                        "adt_loadhostname (%s): %m", hostname);
                        }
                } else {
                        /* login from the local host */
                        if (adt_load_ttyname (display_device, &tid) != 0) {
                                syslog (LOG_AUTH | LOG_ALERT,
                                        "adt_loadhostname (localhost): %m");
                        }
                }

                if (adt_set_user (ah,
                                  solaris_auditor->priv->username != NULL ? solaris_auditor->priv->uid : ADT_NO_ATTRIB,
                                  solaris_auditor->priv->username != NULL ? solaris_auditor->priv->gid : ADT_NO_ATTRIB,
                                  solaris_auditor->priv->username != NULL ? solaris_auditor->priv->uid : ADT_NO_ATTRIB,
                                  solaris_auditor->priv->username != NULL ? solaris_auditor->priv->gid : ADT_NO_ATTRIB,
                                  tid, ADT_NEW) != 0) {

                        syslog (LOG_AUTH | LOG_ALERT,
                                "adt_set_user (%s): %m",
                                solaris_auditor->priv->username != NULL ? solaris_auditor->priv->username : "ADT_NO_ATTRIB");
                }
        }

        event = adt_alloc_event (ah, ADT_login);

        if (event == NULL) {
                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_alloc_event (ADT_login, ADT_FAILURE): %m");
                goto done;
        } else if (adt_put_event (event, ADT_FAILURE,
                                  ADT_FAIL_PAM + pam_error_code) != 0) {
                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_put_event (ADT_login (ADT_FAIL, %s): %m",
                        pam_error_string);
        }

        if (solaris_auditor->priv->password_change_initiated) {
                /* Also audit password change */
                adt_free_event (event);

                event = adt_alloc_event (ah, ADT_passwd);
                if (event == NULL) {
                        syslog (LOG_AUTH | LOG_ALERT,
                                "adt_alloc_event (ADT_passwd): %m");
                        goto done;
                }

                if (solaris_auditor->priv->password_changed) {
                        if (adt_put_event (event, ADT_SUCCESS,
                                           ADT_SUCCESS) != 0) {

                                syslog (LOG_AUTH | LOG_ALERT,
                                        "adt_put_event (ADT_passwd, ADT_SUCCESS): "
                                        "%m");
                        }
                } else {
                        if (adt_put_event (event, ADT_FAILURE,
                                           ADT_FAIL_PAM + pam_error_code) != 0) {

                                syslog (LOG_AUTH | LOG_ALERT,
                                        "adt_put_event (ADT_passwd, ADT_FAILURE): "
                                        "%m");
                        }
                }
        }
        adt_free_event (event);

done:
        /* Reset process audit state. this process is being reused.*/
        if ((adt_set_user (ah, ADT_NO_AUDIT, ADT_NO_AUDIT, ADT_NO_AUDIT,
                           ADT_NO_AUDIT, NULL, ADT_NEW) != 0) ||
            (adt_set_proc (ah) != 0)) {

                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_put_event (ADT_login (ADT_FAILURE reset, %m)");
        }
        (void) adt_end_session (ah);

cleanup:
        g_free (hostname);
        g_free (display_device);
}

static void
gdm_session_solaris_auditor_report_logout (GdmSessionAuditor *auditor)
{
        GdmSessionSolarisAuditor *solaris_auditor;
        adt_session_data_t       *adt_ah;  /* Audit session handle */
        adt_event_data_t         *event;   /* Event to generate    */

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (auditor);

        adt_ah = solaris_auditor->priv->audit_session_handle;

        event = adt_alloc_event (adt_ah, ADT_logout);
        if (event == NULL) {
                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_alloc_event (ADT_logout): %m");
        } else if (adt_put_event (event, ADT_SUCCESS, ADT_SUCCESS) != 0) {
                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_put_event (ADT_logout, ADT_SUCCESS): %m");
        }

        adt_free_event (event);

        /* Reset process audit state. this process is being reused. */
        if ((adt_set_user (adt_ah, ADT_NO_AUDIT, ADT_NO_AUDIT, ADT_NO_AUDIT,
                           ADT_NO_AUDIT, NULL, ADT_NEW) != 0) ||
            (adt_set_proc (adt_ah) != 0)) {
                syslog (LOG_AUTH | LOG_ALERT,
                        "adt_set_proc (ADT_logout reset): %m");
        }

        (void) adt_end_session (adt_ah);
        solaris_auditor->priv->audit_session_handle = NULL;
}

static void
gdm_session_solaris_auditor_class_init (GdmSessionSolarisAuditorClass *klass)
{
        GObjectClass           *object_class;
        GdmSessionAuditorClass *auditor_class;

        object_class = G_OBJECT_CLASS (klass);
        auditor_class = GDM_SESSION_AUDITOR_CLASS (klass);

        object_class->finalize = gdm_session_solaris_auditor_finalize;

        auditor_class->report_password_changed = gdm_session_solaris_auditor_report_password_changed;
        auditor_class->report_password_change_failure = gdm_session_solaris_auditor_report_password_change_failure;
        auditor_class->report_user_accredited = gdm_session_solaris_auditor_report_user_accredited;
        auditor_class->report_login = gdm_session_solaris_auditor_report_login;
        auditor_class->report_login_failure = gdm_session_solaris_auditor_report_login_failure;
        auditor_class->report_logout = gdm_session_solaris_auditor_report_logout;

        g_type_class_add_private (auditor_class, sizeof (GdmSessionSolarisAuditorPrivate));
}

static void
on_username_set (GdmSessionSolarisAuditor *auditor)
{
        char          *username;
        struct passwd *passwd_entry;

        g_object_get (G_OBJECT (auditor), "username", &username, NULL);

        gdm_get_pwent_for_name (username, &passwd_entry);

        if (passwd_entry != NULL) {
                auditor->priv->uid = passwd_entry->pw_uid;
                auditor->priv->gid = passwd_entry->pw_gid;
                auditor->priv->username = g_strdup (passwd_entry->pw_name);
        } else {
                g_free (auditor->priv->username);
                auditor->priv->username = NULL;
                auditor->priv->uid = (uid_t) -1;
                auditor->priv->gid = (gid_t) -1;
        }

        g_free (username);
}

static void
gdm_session_solaris_auditor_init (GdmSessionSolarisAuditor *auditor)
{
        auditor->priv = G_TYPE_INSTANCE_GET_PRIVATE (auditor,
                                                     GDM_TYPE_SESSION_SOLARIS_AUDITOR,
                                                     GdmSessionSolarisAuditorPrivate);

        g_signal_connect (G_OBJECT (auditor), "notify::username",
                          G_CALLBACK (on_username_set), NULL);

        auditor->priv->uid = (uid_t) -1;
        auditor->priv->gid = (gid_t) -1;
}

static void
gdm_session_solaris_auditor_finalize (GObject *object)
{
        GdmSessionSolarisAuditor *solaris_auditor;
        GObjectClass *parent_class;

        solaris_auditor = GDM_SESSION_SOLARIS_AUDITOR (object);

        g_free (solaris_auditor->priv->username);
        solaris_auditor->priv->username = NULL;

        parent_class = G_OBJECT_CLASS (gdm_session_solaris_auditor_parent_class);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

GdmSessionAuditor *
gdm_session_solaris_auditor_new (const char *hostname,
                                 const char *display_device)
{
        GObject *auditor;

        auditor = g_object_new (GDM_TYPE_SESSION_SOLARIS_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return GDM_SESSION_AUDITOR (auditor);
}


