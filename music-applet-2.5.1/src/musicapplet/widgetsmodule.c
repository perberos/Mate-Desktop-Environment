/*
 * Music Applet
 * Copyright (C) 2006 Paul Kuliniewicz <paul@kuliniewicz.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#include <pygobject.h>

void widgets_register_classes (PyObject *d);
extern PyMethodDef widgets_functions[];

DL_EXPORT (void)
initwidgets (void)
{
	PyObject *m;
	PyObject *d;

	init_pygobject ();

	m = Py_InitModule ("widgets", widgets_functions);
	d = PyModule_GetDict (m);

	widgets_register_classes (d);

	if (PyErr_Occurred ())
	{
		Py_FatalError ("can't initialize module musicapplet.widgets");
	}
}
