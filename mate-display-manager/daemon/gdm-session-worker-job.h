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


#ifndef __GDM_SESSION_WORKER_JOB_H
#define __GDM_SESSION_WORKER_JOB_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SESSION_WORKER_JOB         (gdm_session_worker_job_get_type ())
#define GDM_SESSION_WORKER_JOB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SESSION_WORKER_JOB, GdmSessionWorkerJob))
#define GDM_SESSION_WORKER_JOB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SESSION_WORKER_JOB, GdmSessionWorkerJobClass))
#define GDM_IS_SESSION_WORKER_JOB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SESSION_WORKER_JOB))
#define GDM_IS_SESSION_WORKER_JOB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SESSION_WORKER_JOB))
#define GDM_SESSION_WORKER_JOB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SESSION_WORKER_JOB, GdmSessionWorkerJobClass))

typedef struct GdmSessionWorkerJobPrivate GdmSessionWorkerJobPrivate;

typedef struct
{
        GObject                 parent;
        GdmSessionWorkerJobPrivate *priv;
} GdmSessionWorkerJob;

typedef struct
{
        GObjectClass   parent_class;

        void (* started)           (GdmSessionWorkerJob  *session_worker_job);
        void (* exited)            (GdmSessionWorkerJob  *session_worker_job,
                                    int                   exit_code);

        void (* died)              (GdmSessionWorkerJob  *session_worker_job,
                                    int                   signal_number);
} GdmSessionWorkerJobClass;

GType                   gdm_session_worker_job_get_type           (void);
GdmSessionWorkerJob *   gdm_session_worker_job_new                (void);
void                    gdm_session_worker_job_set_server_address (GdmSessionWorkerJob *session_worker_job,
                                                                   const char          *server_address);
gboolean                gdm_session_worker_job_start              (GdmSessionWorkerJob *session_worker_job);
gboolean                gdm_session_worker_job_stop               (GdmSessionWorkerJob *session_worker_job);

G_END_DECLS

#endif /* __GDM_SESSION_WORKER_JOB_H */
