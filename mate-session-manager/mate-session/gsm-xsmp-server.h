/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __GSM_XSMP_SERVER_H
#define __GSM_XSMP_SERVER_H

#include <glib-object.h>

#include "gsm-store.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSM_TYPE_XSMP_SERVER         (gsm_xsmp_server_get_type ())
#define GSM_XSMP_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSM_TYPE_XSMP_SERVER, GsmXsmpServer))
#define GSM_XSMP_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSM_TYPE_XSMP_SERVER, GsmXsmpServerClass))
#define GSM_IS_XSMP_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSM_TYPE_XSMP_SERVER))
#define GSM_IS_XSMP_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSM_TYPE_XSMP_SERVER))
#define GSM_XSMP_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSM_TYPE_XSMP_SERVER, GsmXsmpServerClass))

typedef struct GsmXsmpServerPrivate GsmXsmpServerPrivate;

typedef struct
{
        GObject            parent;
        GsmXsmpServerPrivate *priv;
} GsmXsmpServer;

typedef struct
{
        GObjectClass   parent_class;
} GsmXsmpServerClass;

GType               gsm_xsmp_server_get_type                       (void);

GsmXsmpServer *     gsm_xsmp_server_new                            (GsmStore      *client_store);
void                gsm_xsmp_server_start                          (GsmXsmpServer *server);

#ifdef __cplusplus
}
#endif

#endif /* __GSM_XSMP_SERVER_H */
