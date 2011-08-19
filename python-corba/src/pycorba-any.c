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

static void
pycorba_any_dealloc(PyCORBA_Any *self)
{
    if (self->any._type)
	CORBA_Object_release((CORBA_Object)self->any._type, NULL);
    CORBA_free(self->any._value);
    PyObject_DEL(self);
}

static int
pycorba_any_cmp(PyCORBA_Any *self, PyCORBA_Any *other)
{
    CORBA_Environment ev;
    gboolean equal;

    CORBA_exception_init(&ev);
    equal = MateCORBA_any_equivalent(&self->any, &other->any, &ev);
    if (pymatecorba_check_ex(&ev))
	return -1;

    if (equal) return 0;
    if (self < other) return -1;
    return 1;
}

static PyObject *
pycorba_any_repr(PyCORBA_Any *self)
{
    const gchar *repo_id = NULL;

    if (self->any._type) repo_id = self->any._type->repo_id;
    return PyString_FromFormat("<CORBA.any of type '%s'>",
			       repo_id ? repo_id : "(null)");
}

static int
pycorba_any_init(PyCORBA_Any *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "typecode", "value", NULL };
    PyCORBA_TypeCode *pytc;
    PyObject *value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O:CORBA.any.__init__",
				     kwlist, &PyCORBA_TypeCode_Type, &pytc,
				     &value))
	return -1;

    self->any._type = (CORBA_TypeCode)CORBA_Object_duplicate(
					(CORBA_Object)pytc->tc, NULL);
    self->any._value = MateCORBA_small_alloc(self->any._type);

    if (!pymatecorba_marshal_any(&self->any, value)) {
	CORBA_Object_release((CORBA_Object)self->any._type, NULL);
	self->any._type = NULL;
	CORBA_free(self->any._value);
	self->any._value = NULL;
	PyErr_SetString(PyExc_TypeError, "could not marshal value");
	return -1;
    }

    return 0;
}

static PyObject *
pycorba_any_typecode(PyCORBA_Any *self, void *closure)
{
    return pycorba_typecode_new(self->any._type);
}

static PyObject *
pycorba_any_value(PyCORBA_Any *self, void *closure)
{
    PyObject *ret;

    if (!self->any._value) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    ret = pymatecorba_demarshal_any(&self->any);
    if (!ret)
	PyErr_SetString(PyExc_TypeError, "could not demarshal any value");
    return ret;
}

static PyMethodDef pycorba_any_methods[] = {
    { "typecode", (PyCFunction)pycorba_any_typecode, METH_NOARGS,
      "typecode() -> CORBA.TypeCode" },
    { "value",    (PyCFunction)pycorba_any_value,    METH_NOARGS,
      "value() -> any value" },
    { NULL, 0, 0 }
};

PyTypeObject PyCORBA_Any_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Any",                        /* tp_name */
    sizeof(PyCORBA_Any),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_any_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pycorba_any_cmp,           /* tp_compare */
    (reprfunc)pycorba_any_repr,         /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pycorba_any_methods,                /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pycorba_any_init,         /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

PyObject *
pycorba_any_new(CORBA_any *any)
{
    PyCORBA_Any *self;

    self =  PyObject_NEW(PyCORBA_Any, &PyCORBA_Any_Type);
    if (!self)
	return NULL;
    self->any._type = (CORBA_TypeCode)CORBA_Object_duplicate((CORBA_Object)any->_type, NULL);
    self->any._value = MateCORBA_copy_value(any->_value, any->_type);
    self->any._release = CORBA_FALSE;

    return (PyObject *)self;
}
