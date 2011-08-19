/* -*- Mode: C; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <libmatecanvas/libmatecanvas.h>

static PyObject *
matecanvaspoints_from_value(const GValue *value)
{
    MateCanvasPoints *points = (MateCanvasPoints *)g_value_get_boxed(value);
    PyObject *list;
    gint i;

    list = PyList_New(0);
    if (points)
	for (i = 0; i < points->num_points * 2; i++) {
	    PyObject *item = PyFloat_FromDouble(points->coords[i]);

	    PyList_Append(list, item);
	    Py_DECREF(item);
	}
    return list;
}

static int
matecanvaspoints_to_value(GValue *value, PyObject *object)
{
    MateCanvasPoints *points;
    gint i, len;

    if (!PySequence_Check(object))
	return -1;
    len = PySequence_Length(object);
    if (len % 2 != 0)
	return -1;

    points = mate_canvas_points_new(len / 2);
    for (i = 0; i < len; i++) {
	PyObject *item = PySequence_GetItem(object, i);

	points->coords[i] = PyFloat_AsDouble(item);
	if (PyErr_Occurred()) {
	    mate_canvas_points_unref(points);
	    PyErr_Clear();
	    Py_DECREF(item);
	    return -1;
	}
	Py_DECREF(item);
    }
    g_value_set_boxed(value, points);
    mate_canvas_points_unref(points);
    return 0;
}


void pycanvas_register_classes (PyObject *d);

extern PyMethodDef pycanvas_functions[];


DL_EXPORT(void)
initmatecanvas (void)
{
    PyObject *m, *d;
	
    init_pygobject ();
    init_pygtk ();

    pyg_register_boxed_custom(MATE_TYPE_CANVAS_POINTS,
			      matecanvaspoints_from_value,
			      matecanvaspoints_to_value);

    m = Py_InitModule ("matecanvas", pycanvas_functions);
    d = PyModule_GetDict (m);

      /* ArtPathcode */
#define add_item(short)\
    PyModule_AddIntConstant (m, #short, ART_##short)
    add_item(MOVETO);
    add_item(MOVETO_OPEN);
    add_item(CURVETO);
    add_item(LINETO);
    add_item(END);
#undef add_item
	
    pycanvas_register_classes (d);
    /* pycanvas_add_constants (d, "MATE_CANVAS_"); */
}
