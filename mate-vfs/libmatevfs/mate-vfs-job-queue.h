/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-job-queue.h - Job queue for asynchronous operation of the MATE
   Virtual File System (version for POSIX threads).

   Copyright (C) 2001 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: László Péter <laca@ireland.sun.com>
*/

#ifndef MATE_VFS_JOB_QUEUE_H
#define MATE_VFS_JOB_QUEUE_H

#include "mate-vfs-job.h"

G_GNUC_INTERNAL void          _mate_vfs_job_queue_init       (void);
G_GNUC_INTERNAL void          _mate_vfs_job_queue_shutdown   (void);
G_GNUC_INTERNAL gboolean      _mate_vfs_job_schedule         (MateVFSJob       *job);

#endif /* MATE_VFS_JOB_QUEUE_H */
