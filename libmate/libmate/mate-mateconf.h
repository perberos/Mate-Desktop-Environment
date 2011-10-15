/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* MATE Library - mate-mateconf.h
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef MATE_MATECONF_H
#define MATE_MATECONF_H

#include <libmate/mate-program.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get keys relative to the mate-libs internal per-app directory and the
   application author per-app directory */
gchar* mate_mateconf_get_mate_libs_settings_relative(const gchar* subkey);
gchar* mate_mateconf_get_app_settings_relative(MateProgram* program, const gchar* subkey);
#ifdef __cplusplus
}
#endif

#endif /* MATE_MATECONF_H */
