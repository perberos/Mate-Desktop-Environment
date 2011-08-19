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
pycorba_orb_dealloc(PyCORBA_ORB *self)
{
    CORBA_Object_release((CORBA_Object)self->orb, NULL);
    PyObject_DEL(self);
}

static PyObject *
pycorba_orb_repr(PyCORBA_ORB *self)
{
    return PyString_FromString("<CORBA.ORB>");
}

static PyObject *
pycorba_orb_object_to_string(PyCORBA_ORB *self, PyObject *args)
{
    CORBA_string ret;
    CORBA_Environment ev;
    PyCORBA_Object *obj;
    PyObject *pyret;

    if (!PyArg_ParseTuple(args, "O!:CORBA.ORB.object_to_string",
			  &PyCORBA_Object_Type, &obj))
	return NULL;

    CORBA_exception_init(&ev);
    ret = CORBA_ORB_object_to_string(self->orb, obj->objref, &ev);

    if (pymatecorba_check_ex(&ev))
	return NULL;

    pyret = PyString_FromString(ret);
    CORBA_free(ret);
    return pyret;
}

static PyObject *
pycorba_orb_string_to_object(PyCORBA_ORB *self, PyObject *args)
{
    CORBA_Object objref;
    CORBA_Environment ev;
    gchar *ior;
    PyObject *py_objref;

    if (!PyArg_ParseTuple(args, "s:CORBA.ORB.string_to_object", &ior))
	return NULL;
    
    CORBA_exception_init(&ev);
    objref = CORBA_ORB_string_to_object(self->orb, ior, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_objref = pycorba_object_new(objref);
    CORBA_Object_release(objref, NULL);
    return py_objref;
}

static PyObject *
pycorba_orb_list_initial_services(PyCORBA_ORB *self)
{
    CORBA_Environment ev;
    CORBA_ORB_ObjectIdList *ret;
    PyObject *pyret;
    Py_ssize_t i;

    CORBA_exception_init(&ev);
    ret = CORBA_ORB_list_initial_services(self->orb, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    pyret = PyList_New(ret->_length);
    for (i = 0; i < ret->_length; i++) {
	PyObject *item = PyString_FromString(ret->_buffer[i]);
	PyList_SetItem(pyret, i, item);
    }
    CORBA_free(ret);
    return pyret;
}

static PyObject *
pycorba_orb_resolve_initial_references(PyCORBA_ORB *self, PyObject *args)
{
    CORBA_Object objref;
    CORBA_Environment ev;
    gchar *identifier;
    PyObject *py_objref;

    if (!PyArg_ParseTuple(args, "s:CORBA.ORB.resolve_initial_references",
			  &identifier))
	return NULL;
    
    CORBA_exception_init(&ev);
    objref = CORBA_ORB_resolve_initial_references(self->orb, identifier, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    /* PortableServer::POA isn't a real CORBA object */
    if (!strcmp(identifier, "RootPOA")) {
	return pymatecorba_poa_new((PortableServer_POA)objref);
    }
    py_objref = pycorba_object_new(objref);
    CORBA_Object_release(objref, NULL);
    return py_objref;
}

static PyObject *
pycorba_orb_work_pending(PyCORBA_ORB *self)
{
    CORBA_Environment ev;
    PyObject *ret;

    CORBA_exception_init(&ev);
    ret = CORBA_ORB_work_pending(self->orb, &ev) ? Py_True : Py_False;
    if (pymatecorba_check_ex(&ev))
	return NULL;
    
    Py_INCREF(ret);
    return ret;
}

static PyObject *
pycorba_orb_perform_work(PyCORBA_ORB *self)
{
    CORBA_Environment ev;

    CORBA_exception_init(&ev);
    CORBA_ORB_perform_work(self->orb, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pycorba_orb_run(PyCORBA_ORB *self)
{
    CORBA_Environment ev;

    CORBA_exception_init(&ev);
    pymatecorba_begin_allow_threads;
    CORBA_ORB_run(self->orb, &ev);
    pymatecorba_end_allow_threads;
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pycorba_orb_shutdown(PyCORBA_ORB *self, PyObject *args)
{
    gboolean wait_for_completion = TRUE;
    CORBA_Environment ev;

    if (!PyArg_ParseTuple(args, "|i:CORBA.ORB.shutdown", &wait_for_completion))
	return NULL;
    CORBA_exception_init(&ev);
    CORBA_ORB_shutdown(self->orb, wait_for_completion, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pycorba_orb_methods[] = {
    { "object_to_string", (PyCFunction)pycorba_orb_object_to_string, METH_VARARGS },
    { "string_to_object", (PyCFunction)pycorba_orb_string_to_object, METH_VARARGS },
    { "list_initial_services", (PyCFunction)pycorba_orb_list_initial_services, METH_NOARGS },
    { "resolve_initial_references", (PyCFunction)pycorba_orb_resolve_initial_references, METH_VARARGS },
    { "work_pending", (PyCFunction)pycorba_orb_work_pending, METH_NOARGS },
    { "perform_work", (PyCFunction)pycorba_orb_perform_work, METH_VARARGS },
    { "run", (PyCFunction)pycorba_orb_run, METH_NOARGS },
    { "shutdown", (PyCFunction)pycorba_orb_shutdown, METH_VARARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyCORBA_ORB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.ORB",                        /* tp_name */
    sizeof(PyCORBA_ORB),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_orb_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_orb_repr,         /* tp_repr */
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
    pycorba_orb_methods,                /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

PyObject *
pycorba_orb_new(CORBA_ORB orb)
{
    PyCORBA_ORB *self;

    self =  PyObject_NEW(PyCORBA_ORB, &PyCORBA_ORB_Type);
    if (!self)
	return NULL;

    self->orb = (CORBA_ORB)CORBA_Object_duplicate((CORBA_Object)orb, NULL);
    return (PyObject *)self;
    
}
