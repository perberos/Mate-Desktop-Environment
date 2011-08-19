/* -*- Mode: C; c-basic-offset: 4 -*- */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <Python.h>
#include <signal.h>

#include <pygobject.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-property-bag.h>
#include <matecomponent/matecomponent-application.h>

/* include any extra headers needed here */

void _pymatecomponent_register_boxed_types(PyObject *moddict);
void pymatecomponent_register_classes(PyObject *d);
extern PyMethodDef pymatecomponent_functions[];


static void
register_constants(PyObject *module)
{
#define regconst(x) PyModule_AddIntConstant(module, #x, MATECOMPONENT_##x)
    regconst(PROPERTY_READABLE);
    regconst(PROPERTY_WRITEABLE);
    regconst(PROPERTY_NO_LISTENING);
    regconst(PROPERTY_NO_AUTONOTIFY);
}


DL_EXPORT(void)
init_matecomponent(void)
{
    PyObject *m, *d;

    struct sigaction sa;
    PyObject *av;
    int argc, i;
    char **argv;

    /* perform any initialisation required by the library here */
    init_pygobject();

    /* make sure MateCORBA2-python is ready? */

    av = PySys_GetObject("argv");
    if (av != NULL) {
	argc = PyList_Size(av);

	argv = g_new(char *, argc);
	for (i = 0; i < argc; i++)
	    argv[i] = g_strdup(PyString_AsString(PyList_GetItem(av, i)));
    } else {
	argc = 0;
	argv = NULL;
    }
    memset(&sa, 0, sizeof(sa));
    sigaction(SIGCHLD, NULL, &sa);

    if (!matecomponent_init(&argc, argv)) {
	if (argv != NULL) {
	    for (i = 0; i < argc; i++)
		g_free(argv[i]);
	    g_free(argv);
	}
	sigaction(SIGCHLD, &sa, NULL);
	PyErr_SetString(PyExc_RuntimeError, "could not initialise MateComponent");
	return;
    }
    sigaction(SIGCHLD, &sa, NULL);

    if (argv != NULL) {
	PySys_SetArgv(argc, argv);
	for (i = 0; i < argc; i++)
	    g_free(argv[i]);
	g_free(argv);
    }

    /* now initialise the module */
    m = Py_InitModule("matecomponent._matecomponent", pymatecomponent_functions);
    d = PyModule_GetDict(m);

    register_constants(m);
    _pymatecomponent_register_boxed_types(d);
    pymatecomponent_register_classes(d);

    /* add anything else to the module dictionary (such as constants) */
}
