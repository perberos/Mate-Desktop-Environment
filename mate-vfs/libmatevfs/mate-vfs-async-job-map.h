/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-async-job-map.h:

	The async job map, maps MateVFSAsyncHandles to MateVFSJobs. Many
   async operations, keep the same 'MateVFSJob' over the course of several
   operations. 

   Copyright (C) 2001 Eazel Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Pavel Cisler <pavel@eazel.com> */

#ifndef MATE_VFS_ASYNC_JOB_MAP_H
#define MATE_VFS_ASYNC_JOB_MAP_H

#include "mate-vfs-job.h"

/* async job map calls */
void		 _mate_vfs_async_job_map_init		  	(void);
void	     	 _mate_vfs_async_job_map_shutdown	  	(void);
gboolean     	 _mate_vfs_async_job_completed 	  	  	(MateVFSAsyncHandle		*handle);
void 		 _mate_vfs_async_job_map_add_job  	  	(MateVFSJob			*job);
void 		 _mate_vfs_async_job_map_remove_job  	  	(MateVFSJob			*job);
MateVFSJob	*_mate_vfs_async_job_map_get_job	  	(const MateVFSAsyncHandle	*handle);

void		 _mate_vfs_async_job_map_assert_locked	  	(void);
void		 _mate_vfs_async_job_map_lock	  	  	(void);
void		 _mate_vfs_async_job_map_unlock	  	  	(void);

/* async job callback map calls */
void		 _mate_vfs_async_job_callback_valid		(guint				 callback_id,
								 gboolean			*valid,
								 gboolean			*cancelled);
gboolean	 _mate_vfs_async_job_add_callback		(MateVFSJob			*job,
							  	 MateVFSNotifyResult		*notify_result);
void		 _mate_vfs_async_job_remove_callback		(guint			 	 callback_id);
void		 _mate_vfs_async_job_cancel_job_and_callbacks	(MateVFSAsyncHandle		*job_handle,
								 MateVFSJob 			*job);

#endif
