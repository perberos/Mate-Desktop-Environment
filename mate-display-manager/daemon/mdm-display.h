/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __MDM_DISPLAY_H
#define __MDM_DISPLAY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

#define MDM_TYPE_DISPLAY         (mdm_display_get_type ())
#define MDM_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_DISPLAY, MdmDisplay))
#define MDM_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_DISPLAY, MdmDisplayClass))
#define MDM_IS_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_DISPLAY))
#define MDM_IS_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_DISPLAY))
#define MDM_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_DISPLAY, MdmDisplayClass))

typedef struct MdmDisplayPrivate MdmDisplayPrivate;

typedef enum {
        MDM_DISPLAY_UNMANAGED = 0,
        MDM_DISPLAY_PREPARED,
        MDM_DISPLAY_MANAGED,
        MDM_DISPLAY_FINISHED,
        MDM_DISPLAY_FAILED,
} MdmDisplayStatus;

typedef struct
{
        GObject            parent;
        MdmDisplayPrivate *priv;
} MdmDisplay;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*create_authority)          (MdmDisplay *display);
        gboolean (*add_user_authorization)    (MdmDisplay *display,
                                               const char *username,
                                               char      **filename,
                                               GError    **error);
        gboolean (*remove_user_authorization) (MdmDisplay *display,
                                               const char *username,
                                               GError    **error);
        gboolean (*set_slave_bus_name)        (MdmDisplay *display,
                                               const char *name,
                                               GError    **error);
        gboolean (*prepare)                   (MdmDisplay *display);
        gboolean (*manage)                    (MdmDisplay *display);
        gboolean (*finish)                    (MdmDisplay *display);
        gboolean (*unmanage)                  (MdmDisplay *display);
        void     (*get_timed_login_details)   (MdmDisplay *display,
                                               gboolean   *enabled,
                                               char      **username,
                                               int        *delay);
} MdmDisplayClass;

typedef enum
{
         MDM_DISPLAY_ERROR_GENERAL,
         MDM_DISPLAY_ERROR_GETTING_USER_INFO
} MdmDisplayError;

#define MDM_DISPLAY_ERROR mdm_display_error_quark ()

GQuark              mdm_display_error_quark                    (void);
GType               mdm_display_get_type                       (void);

int                 mdm_display_get_status                     (MdmDisplay *display);
time_t              mdm_display_get_creation_time              (MdmDisplay *display);
char *              mdm_display_get_user_auth                  (MdmDisplay *display);

gboolean            mdm_display_create_authority               (MdmDisplay *display);
gboolean            mdm_display_prepare                        (MdmDisplay *display);
gboolean            mdm_display_manage                         (MdmDisplay *display);
gboolean            mdm_display_finish                         (MdmDisplay *display);
gboolean            mdm_display_unmanage                       (MdmDisplay *display);


/* exported to bus */
gboolean            mdm_display_get_id                         (MdmDisplay *display,
                                                                char      **id,
                                                                GError    **error);
gboolean            mdm_display_get_remote_hostname            (MdmDisplay *display,
                                                                char      **hostname,
                                                                GError    **error);
gboolean            mdm_display_get_x11_display_number         (MdmDisplay *display,
                                                                int        *number,
                                                                GError    **error);
gboolean            mdm_display_get_x11_display_name           (MdmDisplay *display,
                                                                char      **x11_display,
                                                                GError    **error);
gboolean            mdm_display_get_seat_id                    (MdmDisplay *display,
                                                                char      **seat_id,
                                                                GError    **error);
gboolean            mdm_display_is_local                       (MdmDisplay *display,
                                                                gboolean   *local,
                                                                GError    **error);
gboolean            mdm_display_get_timed_login_details        (MdmDisplay *display,
                                                                gboolean   *enabled,
                                                                char      **username,
                                                                int        *delay,
                                                                GError    **error);

/* exported but protected */
gboolean            mdm_display_get_x11_cookie                 (MdmDisplay *display,
                                                                GArray     **x11_cookie,
                                                                GError    **error);
gboolean            mdm_display_get_x11_authority_file         (MdmDisplay *display,
                                                                char      **filename,
                                                                GError    **error);
gboolean            mdm_display_add_user_authorization         (MdmDisplay *display,
                                                                const char *username,
                                                                char      **filename,
                                                                GError    **error);
gboolean            mdm_display_remove_user_authorization      (MdmDisplay *display,
                                                                const char *username,
                                                                GError    **error);
gboolean            mdm_display_set_slave_bus_name             (MdmDisplay *display,
                                                                const char *name,
                                                                GError    **error);


G_END_DECLS

#endif /* __MDM_DISPLAY_H */
