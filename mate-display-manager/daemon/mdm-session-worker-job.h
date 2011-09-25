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


#ifndef __MDM_SESSION_WORKER_JOB_H
#define __MDM_SESSION_WORKER_JOB_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_SESSION_WORKER_JOB         (mdm_session_worker_job_get_type ())
#define MDM_SESSION_WORKER_JOB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SESSION_WORKER_JOB, MdmSessionWorkerJob))
#define MDM_SESSION_WORKER_JOB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SESSION_WORKER_JOB, MdmSessionWorkerJobClass))
#define MDM_IS_SESSION_WORKER_JOB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SESSION_WORKER_JOB))
#define MDM_IS_SESSION_WORKER_JOB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SESSION_WORKER_JOB))
#define MDM_SESSION_WORKER_JOB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SESSION_WORKER_JOB, MdmSessionWorkerJobClass))

typedef struct MdmSessionWorkerJobPrivate MdmSessionWorkerJobPrivate;

typedef struct
{
        GObject                 parent;
        MdmSessionWorkerJobPrivate *priv;
} MdmSessionWorkerJob;

typedef struct
{
        GObjectClass   parent_class;

        void (* started)           (MdmSessionWorkerJob  *session_worker_job);
        void (* exited)            (MdmSessionWorkerJob  *session_worker_job,
                                    int                   exit_code);

        void (* died)              (MdmSessionWorkerJob  *session_worker_job,
                                    int                   signal_number);
} MdmSessionWorkerJobClass;

GType                   mdm_session_worker_job_get_type           (void);
MdmSessionWorkerJob *   mdm_session_worker_job_new                (void);
void                    mdm_session_worker_job_set_server_address (MdmSessionWorkerJob *session_worker_job,
                                                                   const char          *server_address);
gboolean                mdm_session_worker_job_start              (MdmSessionWorkerJob *session_worker_job);
gboolean                mdm_session_worker_job_stop               (MdmSessionWorkerJob *session_worker_job);

G_END_DECLS

#endif /* __MDM_SESSION_WORKER_JOB_H */
