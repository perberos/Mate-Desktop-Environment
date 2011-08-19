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
pycorba_object_dealloc(PyCORBA_Object *self)
{
	if (self->in_weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) self);

    if (self->objref)
        CORBA_Object_release(self->objref, NULL);
    self->objref = NULL;

    if (self->ob_type->tp_free)
        self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}

static int
pycorba_object_cmp(PyCORBA_Object *self, PyCORBA_Object *other)
{
    CORBA_boolean ret;
    CORBA_Environment ev;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_is_equivalent(self->objref, other->objref, &ev);
    if (pymatecorba_check_ex(&ev))
	return -1;

    if (ret) return 0;
    if (self->objref < other->objref) return -1;
    return 1;
}

static PyObject *
pycorba_object_repr(PyCORBA_Object *self)
{
    CORBA_char *repo_id;
    PyObject *ret;
    PyCORBA_TypeCode *pytc;

    pytc = (PyCORBA_TypeCode *) PyObject_GetAttrString((PyObject *)self, "__typecode__");
    if (!pytc || !PyObject_IsInstance((PyObject *) pytc, (PyObject *) &PyCORBA_TypeCode_Type)) {
        PyErr_SetString(PyExc_TypeError, "__typecode__ of object is missing or of wrong type");
        Py_XDECREF(pytc);
        return NULL;
    }
    repo_id = pytc->tc->repo_id ? pytc->tc->repo_id : "(null)";
    Py_DECREF(pytc);
    ret = PyString_FromFormat("<CORBA.Object '%s' at %p>",
			      repo_id ? repo_id : "(null)", self->objref);
    return ret;
}

static long
pycorba_object_tp_hash(PyCORBA_Object *self)
{
    CORBA_unsigned_long ret;
    CORBA_Environment ev;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_hash(self->objref, G_MAXLONG, &ev);

    if (pymatecorba_check_ex(&ev))
	return -1;
    return ret;
}

static int
pycorba_object_init(PyCORBA_Object *self, PyObject *args, PyObject *kwargs)
{
    PyErr_SetString(PyExc_NotImplementedError,
		    "can not construct objecre references");
    return -1;
}

static PyObject *
pycorba_object__is_nil(PyCORBA_Object *self)
{
    CORBA_boolean ret;
    CORBA_Environment ev;
    PyObject *py_ret;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_is_nil(self->objref, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_ret = ret ? Py_True : Py_False;
    Py_INCREF(py_ret);
    return py_ret;
}

static PyObject *
pycorba_object__duplicate(PyCORBA_Object *self)
{
    /* pycorba_object_new() duplicates the objref ... */
    return pycorba_object_new(self->objref);
}

static PyObject *
pycorba_object__is_a(PyCORBA_Object *self, PyObject *args)
{
    CORBA_boolean ret;
    CORBA_Environment ev;
    gchar *type_id;
    PyObject *py_ret;

    if (!PyArg_ParseTuple(args, "s:CORBA.Object._is_a", &type_id))
	return NULL;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_is_a(self->objref, type_id, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_ret = ret ? Py_True : Py_False;
    Py_INCREF(py_ret);
    return py_ret;
}

static PyObject *
pycorba_object__non_existent(PyCORBA_Object *self)
{
    CORBA_boolean ret;
    CORBA_Environment ev;
    PyObject *py_ret;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_non_existent(self->objref, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_ret = ret ? Py_True : Py_False;
    Py_INCREF(py_ret);
    return py_ret;
}

static PyObject *
pycorba_object__is_equivalent(PyCORBA_Object *self, PyObject *args)
{
    CORBA_boolean ret;
    CORBA_Environment ev;
    PyCORBA_Object *other;
    PyObject *py_ret;

    if (!PyArg_ParseTuple(args, "O!:CORBA.Object._is_equivalent",
			  &PyCORBA_Object_Type, &other))
	return NULL;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_is_equivalent(self->objref, other->objref, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_ret = ret ? Py_True : Py_False;
    Py_INCREF(py_ret);
    return py_ret;
}

static PyObject *
pycorba_object__hash(PyCORBA_Object *self, PyObject *args)
{
    CORBA_unsigned_long ret;
    CORBA_Environment ev;
    int max;

    if (!PyArg_ParseTuple(args, "i:CORBA.Object._hash", &max))
	return NULL;

    CORBA_exception_init(&ev);
    ret = CORBA_Object_hash(self->objref, max, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;
    return PyLong_FromUnsignedLong(ret);
}

static PyObject *
pycorba_object__narrow(PyCORBA_Object *self, PyObject *args)
{
    PyTypeObject *stub;
    PyObject *pytc;
    const gchar *repo_id;
    gboolean type_matches;
    CORBA_Environment ev;
    PyCORBA_Object *narrowed;

    if (!PyArg_ParseTuple(args, "O!:CORBA.Object._narrow",
			  &PyType_Type, &stub))
	return NULL;
    if (!PyType_IsSubtype(stub, &PyCORBA_Object_Type)) {
	PyErr_SetString(PyExc_TypeError,
			"argument must be a CORBA.Object subclass");
	return NULL;
    }

    /* check to see if the object conforms to the type */
    pytc = PyObject_GetAttrString((PyObject *)stub, "__typecode__");
    if (!pytc) {
	PyErr_Clear();
	PyErr_SetString(PyExc_TypeError, "could not get typecode for stub");
	return NULL;
    }
    if (!PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)) {
	PyErr_SetString(PyExc_TypeError, "could not get typecode for stub");
	Py_DECREF(pytc);
	return NULL;
    }
    repo_id = ((PyCORBA_TypeCode *)pytc)->tc->repo_id;
    CORBA_exception_init(&ev);
    type_matches = CORBA_Object_is_a(self->objref, repo_id, &ev);
    Py_DECREF(pytc);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    if (!type_matches) {
	PyErr_SetString(PyExc_TypeError, "type does not match");
	return NULL;
    }

    /* create the narrowed instance */
    args = PyTuple_New(0);
    narrowed = (PyCORBA_Object *)stub->tp_new(stub, args, NULL);
    Py_DECREF(args);
    if (!narrowed)
	return NULL;
    narrowed->objref = CORBA_Object_duplicate(self->objref, NULL);
    return (PyObject *)narrowed;
    
}

static PyObject *
pycorba_object__get_matecorba_object_refcount(PyCORBA_Object *self)
{
    MateCORBA_RootObject robj = (MateCORBA_RootObject)self->objref;
    return Py_BuildValue("i", robj->refs);
}

static PyMethodDef pycorba_object_methods[] = {
    { "_is_nil", (PyCFunction)pycorba_object__is_nil, METH_NOARGS,
      "_is_nil() -> boolean" },
    { "_duplicate", (PyCFunction)pycorba_object__duplicate, METH_NOARGS,
      "_duplicate() -> CORBA.Object" },
    { "_is_a", (PyCFunction)pycorba_object__is_a, METH_VARARGS,
      "_is_a(type_id) -> boolean" },
    { "_non_existent", (PyCFunction)pycorba_object__non_existent, METH_NOARGS,
      "_non_existent() -> boolean" },
    { "_is_equivalent", (PyCFunction)pycorba_object__is_equivalent,
      METH_VARARGS, "_is_equivalent(other) -> boolean" },
    { "_hash", (PyCFunction)pycorba_object__hash, METH_VARARGS,
      "_hash(maximum) -> int" },
    { "_narrow", (PyCFunction)pycorba_object__narrow, METH_VARARGS,
      "_narrow(stub) -> object" },
    { "_get_matecorba_object_refcount", 
      (PyCFunction)pycorba_object__get_matecorba_object_refcount, METH_NOARGS, 
      "_get_matecorba_object_refcount() -> integer"},
    { NULL, NULL, 0 }
};

PyTypeObject PyCORBA_Object_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Object",                     /* tp_name */
    sizeof(PyCORBA_Object),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_object_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pycorba_object_cmp,        /* tp_compare */
    (reprfunc)pycorba_object_repr,      /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)pycorba_object_tp_hash,   /* tp_hash */
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
    offsetof(PyCORBA_Object, in_weakreflist), /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pycorba_object_methods,             /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pycorba_object_init,      /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

PyObject *
pycorba_object_new_with_type(CORBA_Object objref, CORBA_TypeCode tc)
{
    PyTypeObject *stub = NULL;
    PyCORBA_Object *self;
    PyObject *args;

    if (objref == CORBA_OBJECT_NIL) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    // get stub for MateCORBA object
    stub = (PyTypeObject *)pymatecorba_get_stub_from_objref(objref);
    if (!stub && tc != TC_null) {
        // get stub from typecode
        stub = (PyTypeObject *)pymatecorba_get_stub(tc);
    }
    if (!stub) stub = &PyCORBA_Object_Type; /* fall back */

    args = PyTuple_New(0);
    self = (PyCORBA_Object *)stub->tp_new(stub, args, NULL);
    self->in_weakreflist = NULL;
    Py_DECREF(args);
    if (!self)
	return NULL;

    self->objref = objref;
    CORBA_Object_duplicate(self->objref, NULL);
    return (PyObject *)self;
}

PyObject *
pycorba_object_new(CORBA_Object objref)
{
    return pycorba_object_new_with_type(objref, TC_null);
}
