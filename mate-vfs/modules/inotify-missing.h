/* inotify-helper.h - MATE VFS Monitor using inotify

   Copyright (C) 2006 John McCutchan <john@johnmccutchan.com>

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

   Author: John McCutchan <ttb@tentacle.dhs.org>
*/


#ifndef __INOTIFY_MISSING_H
#define __INOTIFY_MISSING_H

#include "inotify-sub.h"

void im_startup (void (*missing_cb)(ih_sub_t *sub));
void im_add (ih_sub_t *sub);
void im_rm (ih_sub_t *sub);
void im_diag_dump (GIOChannel *ioc);

#endif /* __INOTIFY_MISSING_H */
