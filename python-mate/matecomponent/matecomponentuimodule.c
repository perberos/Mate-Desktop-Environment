/* -*- Mode: C; c-basic-offset: 4 -*- */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <matecomponent/matecomponent-ui-main.h>

/* include any extra headers needed here */

void pymatecomponentui_register_classes(PyObject *d);
void pymatecomponentui_add_constants(PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pymatecomponentui_functions[];

DL_EXPORT(void)
initui(void)
{
    PyObject *m, *d;

    init_pygobject();

    m = Py_InitModule("matecomponent.ui", pymatecomponentui_functions);
    d = PyModule_GetDict(m);

    init_pygtk();

    /* we don't call matecomponent_ui_init() here, as all it does is call
     * matecomponent_init() (done by the matecomponent module we import),
     * mate_program_init() (meant to be done by the user), and
     * gtk_init() (done by the gtk module we import), and call
     * matecomponent_setup_x_error_handler().  This last call is all that is
     * left. */
    matecomponent_setup_x_error_handler();


    pymatecomponentui_register_classes(d);

    /* add anything else to the module dictionary (such as constants) */
    pymatecomponentui_add_constants(m, "MATECOMPONENT_");
}
