/* gdm-session-settings.h - Object for auditing session login/logout
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
#ifndef GDM_SESSION_SETTINGS_H
#define GDM_SESSION_SETTINGS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS
#define GDM_TYPE_SESSION_SETTINGS (gdm_session_settings_get_type ())
#define GDM_SESSION_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_SESSION_SETTINGS, GdmSessionSettings))
#define GDM_SESSION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_SESSION_SETTINGS, GdmSessionSettingsClass))
#define GDM_IS_SESSION_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_SESSION_SETTINGS))
#define GDM_IS_SESSION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_SESSION_SETTINGS))
#define GDM_SESSION_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GDM_TYPE_SESSION_SETTINGS, GdmSessionSettingsClass))
#define GDM_SESSION_SETTINGS_ERROR (gdm_session_settings_error_quark ())
typedef struct _GdmSessionSettings GdmSessionSettings;
typedef struct _GdmSessionSettingsClass GdmSessionSettingsClass;
typedef struct _GdmSessionSettingsPrivate GdmSessionSettingsPrivate;

struct _GdmSessionSettings
{
        GObject                   parent;

        /*< private > */
        GdmSessionSettingsPrivate *priv;
};

struct _GdmSessionSettingsClass
{
        GObjectClass        parent_class;
};

GType               gdm_session_settings_get_type           (void);
GdmSessionSettings *gdm_session_settings_new                (void);

gboolean            gdm_session_settings_load               (GdmSessionSettings  *settings,
                                                             const char          *username,
                                                             GError             **error);
gboolean            gdm_session_settings_save               (GdmSessionSettings  *settings,
                                                             const char          *home_directory,
                                                             GError             **error);
gboolean            gdm_session_settings_is_loaded          (GdmSessionSettings  *settings);
char               *gdm_session_settings_get_language_name  (GdmSessionSettings *settings);
char               *gdm_session_settings_get_layout_name    (GdmSessionSettings *settings);
char               *gdm_session_settings_get_session_name   (GdmSessionSettings *settings);
void                gdm_session_settings_set_language_name  (GdmSessionSettings *settings,
                                                             const char         *language_name);
void                gdm_session_settings_set_layout_name    (GdmSessionSettings *settings,
                                                             const char         *layout_name);
void                gdm_session_settings_set_session_name   (GdmSessionSettings *settings,
                                                             const char         *session_name);

G_END_DECLS
#endif /* GDM_SESSION_SETTINGS_H */
