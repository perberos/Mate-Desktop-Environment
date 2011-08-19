/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2001, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   Copyright (C) 2002 Red Hat, Inc. (trivial port to GLib)

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef G_CANONICALIZE_H
#define G_CANONICALIZE_H

#include <glib.h>

G_BEGIN_DECLS

char* menu_canonicalize_file_name (const char *name,
                                   gboolean    allow_missing_basename);

G_END_DECLS

#endif /* G_CANONICALIZE_H */
