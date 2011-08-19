/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
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
 */

#ifndef __GDM_SESSION_DIRECT_H
#define __GDM_SESSION_DIRECT_H

#include <glib-object.h>
#include "gdm-session.h"

G_BEGIN_DECLS

#define GDM_TYPE_SESSION_DIRECT (gdm_session_direct_get_type ())
#define GDM_SESSION_DIRECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_SESSION_DIRECT, GdmSessionDirect))
#define GDM_SESSION_DIRECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_SESSION_DIRECT, GdmSessionDirectClass))
#define GDM_IS_SESSION_DIRECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_SESSION_DIRECT))
#define GDM_IS_SESSION_DIRECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_SESSION_DIRECT))
#define GDM_SESSION_DIRECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GDM_TYPE_SESSION_DIRECT, GdmSessionDirectClass))

typedef struct _GdmSessionDirectPrivate GdmSessionDirectPrivate;

typedef struct
{
        GObject                  parent;
        GdmSessionDirectPrivate *priv;
} GdmSessionDirect;

typedef struct
{
        GObjectClass parent_class;
} GdmSessionDirectClass;

GType              gdm_session_direct_get_type                 (void);

GdmSessionDirect * gdm_session_direct_new                      (const char *display_id,
                                                                const char *display_name,
                                                                const char *display_hostname,
                                                                const char *display_device,
                                                                const char *display_x11_authority_file,
                                                                gboolean    display_is_local) G_GNUC_MALLOC;

char             * gdm_session_direct_get_username             (GdmSessionDirect     *session_direct);
char             * gdm_session_direct_get_display_device       (GdmSessionDirect     *session_direct);
gboolean           gdm_session_direct_bypasses_xsession        (GdmSessionDirect     *session_direct);

/* Exported methods */
gboolean           gdm_session_direct_restart                  (GdmSessionDirect     *session_direct,
                                                                GError              **error);
gboolean           gdm_session_direct_stop                     (GdmSessionDirect     *session_direct,
                                                                GError              **error);
gboolean           gdm_session_direct_detach                   (GdmSessionDirect     *session_direct,
                                                                GError              **error);

G_END_DECLS

#endif /* GDM_SESSION_DIRECT_H */
