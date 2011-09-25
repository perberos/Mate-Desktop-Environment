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

#ifndef __MDM_SESSION_WORKER_H
#define __MDM_SESSION_WORKER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_SESSION_WORKER            (mdm_session_worker_get_type ())
#define MDM_SESSION_WORKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_SESSION_WORKER, MdmSessionWorker))
#define MDM_SESSION_WORKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_SESSION_WORKER, MdmSessionWorkerClass))
#define MDM_IS_SESSION_WORKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_SESSION_WORKER))
#define MDM_IS_SESSION_WORKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_SESSION_WORKER))
#define MDM_SESSION_WORKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MDM_TYPE_SESSION_WORKER, MdmSessionWorkerClass))

#define MDM_SESSION_WORKER_ERROR           (mdm_session_worker_error_quark ())

typedef enum _MdmSessionWorkerError {
        MDM_SESSION_WORKER_ERROR_GENERIC = 0,
        MDM_SESSION_WORKER_ERROR_WITH_SESSION_COMMAND,
        MDM_SESSION_WORKER_ERROR_FORKING,
        MDM_SESSION_WORKER_ERROR_OPENING_MESSAGE_PIPE,
        MDM_SESSION_WORKER_ERROR_COMMUNICATING,
        MDM_SESSION_WORKER_ERROR_WORKER_DIED,
        MDM_SESSION_WORKER_ERROR_AUTHENTICATING,
        MDM_SESSION_WORKER_ERROR_AUTHORIZING,
        MDM_SESSION_WORKER_ERROR_OPENING_LOG_FILE,
        MDM_SESSION_WORKER_ERROR_OPENING_SESSION,
        MDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS
} MdmSessionWorkerError;

typedef struct MdmSessionWorkerPrivate MdmSessionWorkerPrivate;

typedef struct
{
        GObject                  parent;
        MdmSessionWorkerPrivate *priv;
} MdmSessionWorker;

typedef struct
{
        GObjectClass parent_class;
} MdmSessionWorkerClass;

GType              mdm_session_worker_get_type                 (void);
GQuark             mdm_session_worker_error_quark              (void);

MdmSessionWorker * mdm_session_worker_new                      (const char *server_address) G_GNUC_MALLOC;

G_END_DECLS

#endif /* MDM_SESSION_WORKER_H */
