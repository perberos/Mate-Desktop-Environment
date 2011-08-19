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

#ifndef __GDM_SESSION_WORKER_H
#define __GDM_SESSION_WORKER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SESSION_WORKER            (gdm_session_worker_get_type ())
#define GDM_SESSION_WORKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_SESSION_WORKER, GdmSessionWorker))
#define GDM_SESSION_WORKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_SESSION_WORKER, GdmSessionWorkerClass))
#define GDM_IS_SESSION_WORKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_SESSION_WORKER))
#define GDM_IS_SESSION_WORKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_SESSION_WORKER))
#define GDM_SESSION_WORKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GDM_TYPE_SESSION_WORKER, GdmSessionWorkerClass))

#define GDM_SESSION_WORKER_ERROR           (gdm_session_worker_error_quark ())

typedef enum _GdmSessionWorkerError {
        GDM_SESSION_WORKER_ERROR_GENERIC = 0,
        GDM_SESSION_WORKER_ERROR_WITH_SESSION_COMMAND,
        GDM_SESSION_WORKER_ERROR_FORKING,
        GDM_SESSION_WORKER_ERROR_OPENING_MESSAGE_PIPE,
        GDM_SESSION_WORKER_ERROR_COMMUNICATING,
        GDM_SESSION_WORKER_ERROR_WORKER_DIED,
        GDM_SESSION_WORKER_ERROR_AUTHENTICATING,
        GDM_SESSION_WORKER_ERROR_AUTHORIZING,
        GDM_SESSION_WORKER_ERROR_OPENING_LOG_FILE,
        GDM_SESSION_WORKER_ERROR_OPENING_SESSION,
        GDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS
} GdmSessionWorkerError;

typedef struct GdmSessionWorkerPrivate GdmSessionWorkerPrivate;

typedef struct
{
        GObject                  parent;
        GdmSessionWorkerPrivate *priv;
} GdmSessionWorker;

typedef struct
{
        GObjectClass parent_class;
} GdmSessionWorkerClass;

GType              gdm_session_worker_get_type                 (void);
GQuark             gdm_session_worker_error_quark              (void);

GdmSessionWorker * gdm_session_worker_new                      (const char *server_address) G_GNUC_MALLOC;

G_END_DECLS

#endif /* GDM_SESSION_WORKER_H */
