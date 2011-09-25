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

#ifndef __MDM_SESSION_DIRECT_H
#define __MDM_SESSION_DIRECT_H

#include <glib-object.h>
#include "mdm-session.h"

G_BEGIN_DECLS

#define MDM_TYPE_SESSION_DIRECT (mdm_session_direct_get_type ())
#define MDM_SESSION_DIRECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_SESSION_DIRECT, MdmSessionDirect))
#define MDM_SESSION_DIRECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_SESSION_DIRECT, MdmSessionDirectClass))
#define MDM_IS_SESSION_DIRECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_SESSION_DIRECT))
#define MDM_IS_SESSION_DIRECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_SESSION_DIRECT))
#define MDM_SESSION_DIRECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MDM_TYPE_SESSION_DIRECT, MdmSessionDirectClass))

typedef struct _MdmSessionDirectPrivate MdmSessionDirectPrivate;

typedef struct
{
        GObject                  parent;
        MdmSessionDirectPrivate *priv;
} MdmSessionDirect;

typedef struct
{
        GObjectClass parent_class;
} MdmSessionDirectClass;

GType              mdm_session_direct_get_type                 (void);

MdmSessionDirect * mdm_session_direct_new                      (const char *display_id,
                                                                const char *display_name,
                                                                const char *display_hostname,
                                                                const char *display_device,
                                                                const char *display_x11_authority_file,
                                                                gboolean    display_is_local) G_GNUC_MALLOC;

char             * mdm_session_direct_get_username             (MdmSessionDirect     *session_direct);
char             * mdm_session_direct_get_display_device       (MdmSessionDirect     *session_direct);
gboolean           mdm_session_direct_bypasses_xsession        (MdmSessionDirect     *session_direct);

/* Exported methods */
gboolean           mdm_session_direct_restart                  (MdmSessionDirect     *session_direct,
                                                                GError              **error);
gboolean           mdm_session_direct_stop                     (MdmSessionDirect     *session_direct,
                                                                GError              **error);
gboolean           mdm_session_direct_detach                   (MdmSessionDirect     *session_direct,
                                                                GError              **error);

G_END_DECLS

#endif /* MDM_SESSION_DIRECT_H */
