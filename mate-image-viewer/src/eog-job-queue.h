/* Eye Of Mate - Jobs Queue
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-job-queue.h) by:
 * 	- Martin Kretzschmar <martink@mate.org>
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
 */

#ifndef __EOG_JOB_QUEUE_H__
#define __EOG_JOB_QUEUE_H__

#include "eog-jobs.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

void     eog_job_queue_init       (void);

void     eog_job_queue_add_job    (EogJob    *job);

gboolean eog_job_queue_remove_job (EogJob    *job);

G_END_DECLS

#endif /* __EOG_JOB_QUEUE_H__ */
