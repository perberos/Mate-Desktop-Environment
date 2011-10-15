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


#ifndef __MDM_SERVER_H
#define __MDM_SERVER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_SERVER         (mdm_server_get_type ())
#define MDM_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SERVER, MdmServer))
#define MDM_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SERVER, MdmServerClass))
#define MDM_IS_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SERVER))
#define MDM_IS_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SERVER))
#define MDM_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SERVER, MdmServerClass))

typedef struct MdmServerPrivate MdmServerPrivate;

typedef struct
{
        GObject           parent;
        MdmServerPrivate *priv;
} MdmServer;

typedef struct
{
        GObjectClass   parent_class;

        void (* ready)             (MdmServer *server);
        void (* exited)            (MdmServer *server,
                                    int        exit_code);
        void (* died)              (MdmServer *server,
                                    int        signal_number);
} MdmServerClass;

GType               mdm_server_get_type  (void);
MdmServer *         mdm_server_new       (const char *display_id,
                                          const char *auth_file);
gboolean            mdm_server_start     (MdmServer   *server);
gboolean            mdm_server_stop      (MdmServer   *server);
char *              mdm_server_get_display_device (MdmServer *server);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_SERVER_H */
