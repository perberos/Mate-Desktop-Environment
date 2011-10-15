/* mdm-session-settings.h - Object for auditing session login/logout
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Written by: Ray Strode <rstrode@redhat.com>
 */
#ifndef MDM_SESSION_SETTINGS_H
#define MDM_SESSION_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MDM_TYPE_SESSION_SETTINGS (mdm_session_settings_get_type ())
#define MDM_SESSION_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_SESSION_SETTINGS, MdmSessionSettings))
#define MDM_SESSION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_SESSION_SETTINGS, MdmSessionSettingsClass))
#define MDM_IS_SESSION_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_SESSION_SETTINGS))
#define MDM_IS_SESSION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_SESSION_SETTINGS))
#define MDM_SESSION_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MDM_TYPE_SESSION_SETTINGS, MdmSessionSettingsClass))
#define MDM_SESSION_SETTINGS_ERROR (mdm_session_settings_error_quark ())
typedef struct _MdmSessionSettings MdmSessionSettings;
typedef struct _MdmSessionSettingsClass MdmSessionSettingsClass;
typedef struct _MdmSessionSettingsPrivate MdmSessionSettingsPrivate;

struct _MdmSessionSettings
{
        GObject                   parent;

        /*< private > */
        MdmSessionSettingsPrivate *priv;
};

struct _MdmSessionSettingsClass
{
        GObjectClass        parent_class;
};

GType               mdm_session_settings_get_type           (void);
MdmSessionSettings *mdm_session_settings_new                (void);

gboolean            mdm_session_settings_load               (MdmSessionSettings  *settings,
                                                             const char          *username,
                                                             GError             **error);
gboolean            mdm_session_settings_save               (MdmSessionSettings  *settings,
                                                             const char          *home_directory,
                                                             GError             **error);
gboolean            mdm_session_settings_is_loaded          (MdmSessionSettings  *settings);
char               *mdm_session_settings_get_language_name  (MdmSessionSettings *settings);
char               *mdm_session_settings_get_layout_name    (MdmSessionSettings *settings);
char               *mdm_session_settings_get_session_name   (MdmSessionSettings *settings);
void                mdm_session_settings_set_language_name  (MdmSessionSettings *settings,
                                                             const char         *language_name);
void                mdm_session_settings_set_layout_name    (MdmSessionSettings *settings,
                                                             const char         *layout_name);
void                mdm_session_settings_set_session_name   (MdmSessionSettings *settings,
                                                             const char         *session_name);

#ifdef __cplusplus
}
#endif
#endif /* MDM_SESSION_SETTINGS_H */
