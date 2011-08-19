/*
 *  caja-python.c - Caja Python extension
 * 
 *  Copyright (C) 2004 Johan Dahlin
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef CAJA_PYTHON_H
#define CAJA_PYTHON_H

#include <glib-object.h>
#include <glib/gprintf.h>
#include <Python.h>

#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

typedef enum {
    CAJA_PYTHON_DEBUG_MISC = 1 << 0,
} CajaPythonDebug;

extern CajaPythonDebug caja_python_debug;

#define debug(x) { if (caja_python_debug & CAJA_PYTHON_DEBUG_MISC) \
                       g_printf( "caja-python:" x "\n"); }
#define debug_enter()  { if (caja_python_debug & CAJA_PYTHON_DEBUG_MISC) \
                             g_printf("%s: entered\n", __FUNCTION__); }
#define debug_enter_args(x, y) { if (caja_python_debug & CAJA_PYTHON_DEBUG_MISC) \
                                     g_printf("%s: entered " x "\n", __FUNCTION__, y); }


PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)

PyTypeObject *_PyCajaColumn_Type;
#define PyCajaColumn_Type (*_PyCajaColumn_Type)

PyTypeObject *_PyCajaColumnProvider_Type;
#define PyCajaColumnProvider_Type (*_PyCajaColumnProvider_Type)

PyTypeObject *_PyCajaInfoProvider_Type;
#define PyCajaInfoProvider_Type (*_PyCajaInfoProvider_Type)

PyTypeObject *_PyCajaLocationWidgetProvider_Type;
#define PyCajaLocationWidgetProvider_Type (*_PyCajaLocationWidgetProvider_Type)

PyTypeObject *_PyCajaMenu_Type;
#define PyCajaMenu_Type (*_PyCajaMenu_Type)

PyTypeObject *_PyCajaMenuItem_Type;
#define PyCajaMenuItem_Type (*_PyCajaMenuItem_Type)

PyTypeObject *_PyCajaMenuProvider_Type;
#define PyCajaMenuProvider_Type (*_PyCajaMenuProvider_Type)

PyTypeObject *_PyCajaPropertyPage_Type;
#define PyCajaPropertyPage_Type (*_PyCajaPropertyPage_Type)

PyTypeObject *_PyCajaPropertyPageProvider_Type;
#define PyCajaPropertyPageProvider_Type (*_PyCajaPropertyPageProvider_Type)

#endif /* CAJA_PYTHON_H */
