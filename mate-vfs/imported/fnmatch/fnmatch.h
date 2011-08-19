/* Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __FNMATCH_H__
#define __FNMATCH_H__

/* Stripped-down UTF-8 fnmatch() lifted from GTK+ */

int fnmatch_utf8 (const char *pattern,
		  const char *string,
		  int         flags);

#define fnmatch(pattern, string ,flags) fnmatch_utf8 (pattern, string, flags)

#endif	/* __FNMATCH_H__ */
