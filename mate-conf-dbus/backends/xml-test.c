/* MateConf
 * Copyright (C) 2002 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "xml-entry.h"
#include "xml-dir.h"
#include "xml-cache.h"
#include <mateconf/mateconf-internals.h>
#include <mateconf/mateconf-backend.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/entities.h>
#include <libxml/globals.h>

MateConfBackendVTable* mateconf_backend_get_vtable (void);

int
main (int argc, char **argv)
{
  MateConfBackendVTable *vtable;

  vtable = mateconf_backend_get_vtable ();
  
  xml_test_entry ();
  xml_test_dir ();
  xml_test_cache ();
  
  return 0;
}
