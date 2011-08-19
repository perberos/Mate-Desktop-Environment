/* -*- Mode: C; c-basic-offset: 4 -*-
 *
 * mateweathermodule.c: module wrapping mateweather.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

void pymateweather_register_classes (PyObject *d);
void pymateweather_add_constants(PyObject *module, const gchar *strip_prefix);
void _pymateweather_register_boxed_types(void);	

extern PyMethodDef pymateweather_functions[];

DL_EXPORT(void)
initmateweather(void)
{
    PyObject *m, *d;

    init_pygobject ();
    g_thread_init (NULL);

    m = Py_InitModule ("mateweather", pymateweather_functions);
    d = PyModule_GetDict (m);
    pymateweather_register_classes (d);
    pymateweather_add_constants(m, "MATEWEATHER_");
}
