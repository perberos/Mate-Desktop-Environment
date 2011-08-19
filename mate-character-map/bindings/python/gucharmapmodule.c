#include <Python.h>

#include <config.h>

#include <gucharmap/gucharmap.h>

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>

#include <pygtk/pygtk.h>

void pygucharmap_register_classes(PyObject *d);
void pygucharmap_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pygucharmap_functions[];

DL_EXPORT(void) initgucharmap (void);

DL_EXPORT(void)
initgucharmap (void)
{
	PyObject *m, *d;

	init_pygobject();
	init_pygtk ();

	m = Py_InitModule ("gucharmap", pygucharmap_functions);
	d = PyModule_GetDict (m);

	pygucharmap_register_classes (d);
        pygucharmap_add_constants(m, "GUCHARMAP_");
}
