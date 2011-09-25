/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#ifndef _MDM_COMMON_H
#define _MDM_COMMON_H

#include <glib.h>
#include <pwd.h>

#include "mdm-common-unknown-origin.h"

#define MDM_CUSTOM_SESSION  "custom"

G_BEGIN_DECLS

gboolean       mdm_is_version_unstable            (void);
void           mdm_set_fatal_warnings_if_unstable (void);

int            mdm_wait_on_pid           (int pid);
int            mdm_signal_pid            (int pid,
                                          int signal);
gboolean       mdm_get_pwent_for_name    (const char     *name,
                                          struct passwd **pwentp);

const char *   mdm_make_temp_dir         (char    *template);

gboolean       mdm_string_hex_encode     (const GString *source,
                                          int            start,
                                          GString       *dest,
                                          int            insert_at);
gboolean       mdm_string_hex_decode     (const GString *source,
                                          int            start,
                                          int           *end_return,
                                          GString       *dest,
                                          int            insert_at);
char          *mdm_generate_random_bytes (gsize          size,
                                          GError       **error);

G_END_DECLS

#endif /* _MDM_COMMON_H */
