/*
 * Copyright (C) 2005 Red Hat, Inc.
 *
 *   mateconf-types.h: wrappers for some specialised MateConf types.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 *
 * Author:
 *     Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __MATECONF_TYPES_H__
#define __MATECONF_TYPES_H__

#include <Python.h>
#include <mateconf/mateconf.h>

void         pymateconf_register_engine_type (PyObject    *moddict);
PyObject    *pymateconf_engine_new           (MateConfEngine *engine);
MateConfEngine *pymateconf_engine_from_pyobject (PyObject    *object);
     
#endif /* __MATECONF_TYPES_H__ */
