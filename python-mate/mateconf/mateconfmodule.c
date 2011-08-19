/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

#include "mateconf-types.h"

void pymateconf_register_classes (PyObject *d);
void pymateconf_add_constants(PyObject *module, const gchar *strip_prefix);
		
extern PyMethodDef pymateconf_functions[];
extern PyTypeObject PyMateConfEngine_Type;

DL_EXPORT(void)
initmateconf (void)
{
	PyObject *m, *d;

	init_pygobject ();

	m = Py_InitModule ("mateconf", pymateconf_functions);
	d = PyModule_GetDict (m);

	pymateconf_register_classes (d);
	pymateconf_add_constants (m, "MATECONF_");
	pymateconf_register_engine_type (m);

        PyModule_AddObject(m, "Engine", (PyObject *) &PyMateConfEngine_Type);
}
