/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu.h
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __GDU_H
#define __GDU_H

#ifndef GDU_API_IS_SUBJECT_TO_CHANGE
#error  libgdu is unstable API. You must define GDU_API_IS_SUBJECT_TO_CHANGE before including gdu/gdu.h
#endif

#define __GDU_INSIDE_GDU_H 1

#include <gdu/gdu-types.h>
#include <gdu/gdu-linux-md-drive.h>
#include <gdu/gdu-linux-lvm2-volume-group.h>
#include <gdu/gdu-linux-lvm2-volume.h>
#include <gdu/gdu-linux-lvm2-volume-hole.h>
#include <gdu/gdu-device.h>
#include <gdu/gdu-adapter.h>
#include <gdu/gdu-expander.h>
#include <gdu/gdu-port.h>
#include <gdu/gdu-drive.h>
#include <gdu/gdu-error.h>
#include <gdu/gdu-known-filesystem.h>
#include <gdu/gdu-pool.h>
#include <gdu/gdu-presentable.h>
#include <gdu/gdu-process.h>
#include <gdu/gdu-util.h>
#include <gdu/gdu-volume.h>
#include <gdu/gdu-volume-hole.h>
#include <gdu/gdu-hub.h>
#include <gdu/gdu-machine.h>
#include <gdu/gdu-callbacks.h>

#undef __GDU_INSIDE_GDU_H

G_END_DECLS

#endif /* __GDU_H */
