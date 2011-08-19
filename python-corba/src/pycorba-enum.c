/* -*- mode: C; c-basic-offset: 4 -*-
 * pymatecorba - a Python language mapping for the MateCORBA2 CORBA ORB
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define PY_SIZE_T_CLEAN

#include "pymatecorba-private.h"

typedef struct {
    PyIntObject parent;
    PyObject *name;
} PyCORBA_Enum;

static void
pycorba_enum_dealloc(PyObject *self)
{
    PyCORBA_Enum *enumobj = (PyCORBA_Enum *)self;

    Py_DECREF(enumobj->name);

    if (self->ob_type->tp_free)
	self->ob_type->tp_free(self);
    else
	PyObject_DEL(self);
}

static PyObject *
pycorba_enum_repr(PyCORBA_Enum *self)
{
    Py_INCREF(self->name);
    return self->name;
}

static PyObject *
pycorba_enum_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "value", NULL };
    long value;
    PyObject *pytc, *values, *ret;
    CORBA_TypeCode tc;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "l", kwlist, &value))
	return NULL;
    pytc = PyObject_GetAttrString((PyObject *)type, "__typecode__");
    if (!pytc)
	return NULL;
    if (!PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__typecode__ attribute not a typecode");
	return NULL;
    }
    tc = ((PyCORBA_TypeCode *)pytc)->tc;
    Py_DECREF(pytc);

    if (value < 0 || value > tc->sub_parts) {
	PyErr_SetString(PyExc_ValueError, "value out of range");
	return NULL;
    }
    values = PyObject_GetAttrString((PyObject *)type, "__enum_values__");
    if (!values)
	return NULL;
    if (!PyTuple_Check(values) || PyTuple_Size(values) != tc->sub_parts) {
	Py_DECREF(values);
	PyErr_SetString(PyExc_TypeError, "__enum_values__ badly formed");
	return NULL;
    }

    ret = PyTuple_GetItem(values, value);
    Py_INCREF(ret);
    Py_DECREF(values);
    return ret;
}

PyTypeObject PyCORBA_Enum_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Enum",                       /* tp_name */
    sizeof(PyCORBA_Enum),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_enum_dealloc,   /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_enum_repr,        /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)pycorba_enum_new,          /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

PyObject *
_pymatecorba_generate_enum(CORBA_TypeCode tc, PyObject **values_p)
{
    PyObject *stub, *instance_dict, *values;
    long i;

    g_return_val_if_fail(tc->kind == CORBA_tk_enum, NULL);

    instance_dict = PyDict_New();
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
                                 tc->name, (PyObject *)&PyCORBA_Enum_Type,
                                 instance_dict);
    Py_DECREF(instance_dict);

    values = PyTuple_New(tc->sub_parts);
    for (i = 0; i < tc->sub_parts; i++) {
	PyObject *item;

	item = ((PyTypeObject *)stub)->tp_alloc((PyTypeObject *)stub, 0);
	((PyCORBA_Enum *)item)->parent.ob_ival = i;
	((PyCORBA_Enum *)item)->name = PyString_FromString(tc->subnames[i]);

	PyTuple_SetItem(values, i, item);
    }
    PyDict_SetItemString(((PyTypeObject *)stub)->tp_dict,
			 "__enum_values__", values);
    Py_DECREF(values);

    *values_p = values;

    return stub;
}

PyObject *
pycorba_enum_from_long(CORBA_TypeCode tc, long value)
{
    PyObject *stub, *values, *ret;

    stub = pymatecorba_get_stub(tc);
    g_return_val_if_fail(stub != NULL, NULL);

    if (value < 0 || value > tc->sub_parts) {
	PyErr_SetString(PyExc_ValueError, "value out of range");
	return NULL;
    }
    values = PyObject_GetAttrString(stub, "__enum_values__");
    if (!values)
	return NULL;
    if (!PyTuple_Check(values) || PyTuple_Size(values) != tc->sub_parts) {
	Py_DECREF(values);
	PyErr_SetString(PyExc_TypeError, "__enum_values__ badly formed");
	return NULL;
    }

    ret = PyTuple_GetItem(values, value);
    Py_INCREF(ret);
    Py_DECREF(values);
    return ret;
}
