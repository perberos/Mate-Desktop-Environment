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
#include <matecorba/poa/poa-types.h>


static PyObject *
pymatecorba_object_adaptor_set_thread_hint (PyCORBA_Object *self, PyObject *args)
{
    MateCORBAThreadHint hint;

    if (!PyArg_ParseTuple(args, "i:ObjectAdaptor.set_thread_hint", &hint))
	return NULL;
    MateCORBA_ObjectAdaptor_set_thread_hint((MateCORBA_ObjectAdaptor)self->objref, hint);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_object_adaptor_get_thread_hint(PyCORBA_Object *self)
{
    MateCORBAThreadHint hint;

    hint = MateCORBA_ObjectAdaptor_get_thread_hint((MateCORBA_ObjectAdaptor)self->objref);
    return PyInt_FromLong(hint);
}

static PyObject *
pymatecorba_object_adaptor_object_bind_to_current_thread(PyObject *self)
{
    if (!PyObject_TypeCheck(self, &PyCORBA_Object_Type)) {
        PyErr_SetString(PyExc_TypeError,
                        "object_bind_to_current_thread: argument must be a corba object");
        return NULL;
    }
    MateCORBA_ObjectAdaptor_object_bind_to_current_thread(((PyCORBA_Object *) self)->objref);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pymatecorba_object_adaptor_methods[] = {
    { "set_thread_hint", (PyCFunction)pymatecorba_object_adaptor_set_thread_hint,
      METH_VARARGS, NULL },
    { "get_thread_hint", (PyCFunction)pymatecorba_object_adaptor_get_thread_hint,
      METH_NOARGS, NULL },
    { "object_bind_to_current_thread",
      (PyCFunction)pymatecorba_object_adaptor_object_bind_to_current_thread,
      METH_O|METH_STATIC, NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject PyMateCORBA_ObjectAdaptor_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "MateCORBA.ObjectAdaptor",              /* tp_name */
    sizeof(PyCORBA_Object),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)0,    			/* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)0,                        /* tp_repr */
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
    pymatecorba_object_adaptor_methods,     /* tp_methods */
    0,                                  /* tp_members */
    0,                			/* tp_getset */
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

static void
pymatecorba_poa_dealloc(PyCORBA_Object *self)
{
    if (self->objref)
	CORBA_Object_release(self->objref, NULL);
    self->objref = NULL;

    if (self->ob_type->tp_free)
	self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}


static PyObject *
pymatecorba_poa_activate_object(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyPortableServer_Servant *pyservant;
    PortableServer_ServantBase *servant;
    PortableServer_ObjectId *id;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "O!:POA.activate_object",
			  &PyPortableServer_Servant_Type, &pyservant))
	return NULL;
    servant = PYSERVANT_TO_SERVANT(pyservant);

    CORBA_exception_init(&ev);
    id = PortableServer_POA_activate_object((PortableServer_POA)self->objref,
					    servant, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    ret = PyString_FromStringAndSize((char *) id->_buffer, id->_length);
    CORBA_free(id);
    return ret;
}

static PyObject *
pymatecorba_poa_activate_object_with_id(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyPortableServer_Servant *pyservant;
    PortableServer_ServantBase *servant;
    PortableServer_ObjectId *id;
    Py_ssize_t id_length;

    id = (PortableServer_ObjectId *)CORBA_sequence_CORBA_octet__alloc();
    id->_release = CORBA_FALSE;
    if (!PyArg_ParseTuple(args, "s#O!:POA.activate_object_with_id",
			  &id->_buffer, &id_length,
			  &PyPortableServer_Servant_Type, &pyservant)) {
	CORBA_free(id);
	return NULL;
    }
    id->_length = id_length;
    id->_length++; /* account for nul termination */
    servant = PYSERVANT_TO_SERVANT(pyservant);

    CORBA_exception_init(&ev);
    PortableServer_POA_activate_object_with_id((PortableServer_POA)self->objref,
					       id, servant, &ev);
    CORBA_free(id);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poa_deactivate_object(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PortableServer_ObjectId *id;
    Py_ssize_t id_length;
    id = (PortableServer_ObjectId *)CORBA_sequence_CORBA_octet__alloc();
    id->_release = CORBA_FALSE;
    if (!PyArg_ParseTuple(args, "s#:POA.deactivate_object",
			  &id->_buffer, &id_length)) {
	CORBA_free(id);
	return NULL;
    }
    id->_length = id_length;
    id->_length++; /* account for nul termination */

    CORBA_exception_init(&ev);
    PortableServer_POA_deactivate_object((PortableServer_POA)self->objref,
					 id, &ev);
    CORBA_free(id);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poa_servant_to_id(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyPortableServer_Servant *pyservant;
    PortableServer_ServantBase *servant;
    PortableServer_ObjectId *id;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "O!:POA.activate_object",
			  &PyPortableServer_Servant_Type, &pyservant))
	return NULL;
    servant = PYSERVANT_TO_SERVANT(pyservant);

    CORBA_exception_init(&ev);
    id = PortableServer_POA_servant_to_id((PortableServer_POA)self->objref,
					  servant, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    ret = PyString_FromStringAndSize((char *) id->_buffer, id->_length);
    CORBA_free(id);
    return ret;
}

static PyObject *
pymatecorba_poa_servant_to_reference(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyPortableServer_Servant *pyservant;
    PortableServer_ServantBase *servant;
    CORBA_Object reference;
    PyObject *py_reference;

    if (!PyArg_ParseTuple(args, "O!:POA.servant_to_reference",
			  &PyPortableServer_Servant_Type, &pyservant))
	return NULL;
    servant = PYSERVANT_TO_SERVANT(pyservant);

    CORBA_exception_init(&ev);
    reference = PortableServer_POA_servant_to_reference((PortableServer_POA)self->objref,
							servant, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    py_reference = pycorba_object_new(reference);
    CORBA_Object_release(reference, NULL);
    return py_reference;
}

static PyObject *
pymatecorba_poa_reference_to_id(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyCORBA_Object *reference;
    PortableServer_ObjectId *id;
    PyObject *ret;

    if (!PyArg_ParseTuple(args, "O!:POA.reference_to_id",
			  &PyCORBA_Object_Type, &reference))
	return NULL;

    CORBA_exception_init(&ev);
    id = PortableServer_POA_reference_to_id((PortableServer_POA)self->objref,
					    reference->objref, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    ret = PyString_FromStringAndSize((char *) id->_buffer, id->_length);
    CORBA_free(id);
    return ret;
}

static PyObject *
pymatecorba_poa_get_the_poamanager(PyCORBA_Object *self)
{
    CORBA_Environment ev;
    PortableServer_POAManager poamanager;
    PyObject *ret;

    CORBA_exception_init(&ev);
    poamanager = PortableServer_POA__get_the_POAManager((PortableServer_POA)self->objref, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    ret = pymatecorba_poamanager_new(poamanager);
    return ret;
}

static PyObject *
pymatecorba_poa_create_thread_policy(PyCORBA_Object *self, PyObject *args)
{
    CORBA_Environment ev;
    PyObject *py_policy;
    PortableServer_ThreadPolicyValue value;
    PortableServer_ThreadPolicy policy;

    if (!PyArg_ParseTuple(args, "i:POA.create_thread_policy", &value))
	return NULL;
    CORBA_exception_init(&ev);
    policy = PortableServer_POA_create_thread_policy((PortableServer_POA)self->objref, value, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_policy = pycorba_policy_new((CORBA_Object) policy);
    CORBA_Object_release((CORBA_Object) policy, &ev);
    return py_policy;
}

  /* 
        POA create_POA(         in string       adapter_name,
                                in POAManager   a_POAManager,
                                in CORBA::PolicyList policies)
                                    raises (AdapterAlreadyExists, InvalidPolicy);
 */
static PyObject *
pymatecorba_poa_create_POA(PyCORBA_Object *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "adapter_name", "a_POAManager", "policies", NULL};
    CORBA_Environment ev;
    CORBA_PolicyList policies;
    char *adapter_name;
    PyObject *py_POAManager, *py_policies, *py_new_poa;
    PortableServer_POA new_poa;
    PortableServer_POAManager manager;
    int i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO!O!:CORBA.ORB_init", kwlist,
                                     &adapter_name,
                                     &PyPortableServer_POAManager_Type, &py_POAManager,
                                     &PyList_Type, &py_policies))
	return NULL;

    policies._length = PyList_Size(py_policies);
    policies._maximum = policies._length;
    policies._buffer = CORBA_PolicyList_allocbuf(policies._length);
    for (i = 0; i < policies._length; ++i) {
        PyObject *item = PyList_GET_ITEM(py_policies, i);
        if (!PyObject_TypeCheck(item, &PyCORBA_Policy_Type)) {
            CORBA_free(policies._buffer);
            PyErr_SetString(PyExc_TypeError, "policies must be a list of CORBA.Policy objects");
            return NULL;
        }
        policies._buffer[i] = (CORBA_Policy) ((PyCORBA_Policy *) item)->objref;
    }

    manager = (PortableServer_POAManager) ((PyCORBA_Object *) py_POAManager)->objref;
    
    CORBA_exception_init(&ev);
    new_poa = PortableServer_POA_create_POA((PortableServer_POA)self->objref,
                                            adapter_name, manager, &policies, &ev);
    CORBA_free(policies._buffer);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    py_new_poa = pymatecorba_poa_new(new_poa);
    CORBA_Object_release((CORBA_Object) new_poa, &ev);
    return py_new_poa;
}


static PyMethodDef pymatecorba_poa_methods[] = {
    { "activate_object", (PyCFunction)pymatecorba_poa_activate_object,
      METH_VARARGS },
    { "activate_object_with_id",
      (PyCFunction)pymatecorba_poa_activate_object_with_id, METH_VARARGS },
    { "deactivate_object", (PyCFunction)pymatecorba_poa_deactivate_object,
      METH_VARARGS },

    { "servant_to_id", (PyCFunction)pymatecorba_poa_servant_to_id, METH_VARARGS },
    { "servant_to_reference", (PyCFunction)pymatecorba_poa_servant_to_reference,
      METH_VARARGS },
    /* { "reference_to_servant", (PyCFunction)pymatecorba_poa_reference_to_servant,
       METH_VARARGS }, */
    { "reference_to_id", (PyCFunction)pymatecorba_poa_reference_to_id,
      METH_VARARGS },
    /* { "id_to_servant", (PyCFunction)pymatecorba_poa_id_to_servant,
       METH_VARARGS },
     { "id_to_reference", (PyCFunction)pymatecorba_poa_id_to_reference,
       METH_VARARGS }, */
    { "_get_the_POAManager", (PyCFunction)pymatecorba_poa_get_the_poamanager,
      METH_NOARGS },
    { "create_thread_policy", (PyCFunction)pymatecorba_poa_create_thread_policy,
      METH_VARARGS },
    { "create_POA", (PyCFunction)pymatecorba_poa_create_POA,
      METH_KEYWORDS },

    { NULL, NULL, 0 }
};

static PyGetSetDef pymatecorba_poa_getsets[] = {
    { "the_POAManager", (getter)pymatecorba_poa_get_the_poamanager, (setter)0 },
    { NULL, (getter)0, (setter)0 }
};

PyTypeObject PyPortableServer_POA_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "PortableServer.POA",               /* tp_name */
    sizeof(PyCORBA_Object),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pymatecorba_poa_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)0,                        /* tp_repr */
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
    pymatecorba_poa_methods,                /* tp_methods */
    0,                                  /* tp_members */
    pymatecorba_poa_getsets,                /* tp_getset */
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
pymatecorba_poa_new(PortableServer_POA poa)
{
    PyCORBA_Object *self;
    PyTypeObject *type;
    PyObject *args;

    if (!poa) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    type = &PyPortableServer_POA_Type;
    args = PyTuple_New(0);
    self = (PyCORBA_Object *)type->tp_new(type, args, NULL);
    Py_DECREF(args);
    if (!self) return NULL;
    self->objref = (CORBA_Object)poa;
    return (PyObject *)self;
}

static PyObject *
pymatecorba_poamanager_activate(PyCORBA_Object *self)
{
    CORBA_Environment ev;

    CORBA_exception_init(&ev);
    PortableServer_POAManager_activate((PortableServer_POAManager)self->objref,
				       &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poamanager_hold_requests(PyCORBA_Object *self, PyObject *args)
{
    gboolean wait_for_completion;
    CORBA_Environment ev;

    if (!PyArg_ParseTuple(args, "i:POAManager.hold_requests",
			  &wait_for_completion))
	return NULL;
    CORBA_exception_init(&ev);
    PortableServer_POAManager_hold_requests((PortableServer_POAManager)self->objref,
					    wait_for_completion, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poamanager_discard_requests(PyCORBA_Object *self, PyObject *args)
{
    gboolean wait_for_completion;
    CORBA_Environment ev;

    if (!PyArg_ParseTuple(args, "i:POAManager.discard_requests",
			  &wait_for_completion))
	return NULL;
    CORBA_exception_init(&ev);
    PortableServer_POAManager_discard_requests((PortableServer_POAManager)self->objref,
					       wait_for_completion, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poamanager_deactivate(PyCORBA_Object *self, PyObject *args)
{
    gboolean etherealize_objects, wait_for_completion;
    CORBA_Environment ev;

    if (!PyArg_ParseTuple(args, "ii:POAManager.deactivate",
			  &etherealize_objects, &wait_for_completion))
	return NULL;
    CORBA_exception_init(&ev);
    PortableServer_POAManager_deactivate((PortableServer_POAManager)self->objref,
					 etherealize_objects,
					 wait_for_completion, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_poamanager_get_state(PyCORBA_Object *self)
{
    CORBA_Environment ev;
    PortableServer_POAManager_State ret;

    CORBA_exception_init(&ev);
    ret = PortableServer_POAManager_get_state((PortableServer_POAManager)self->objref, &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    return PyInt_FromLong(ret);
}

static PyMethodDef pymatecorba_poamanager_methods[] = {
    { "activate", (PyCFunction)pymatecorba_poamanager_activate, METH_NOARGS },
    { "hold_requests", (PyCFunction)pymatecorba_poamanager_hold_requests, METH_VARARGS },
    { "discard_requests", (PyCFunction)pymatecorba_poamanager_discard_requests, METH_VARARGS },
    { "deactivate", (PyCFunction)pymatecorba_poamanager_deactivate, METH_VARARGS },
    { "get_state", (PyCFunction)pymatecorba_poamanager_get_state, METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyPortableServer_POAManager_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "PortableServer.POAManager",        /* tp_name */
    sizeof(PyCORBA_Object),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pymatecorba_poa_dealloc,    /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)0,                        /* tp_repr */
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
    pymatecorba_poamanager_methods,         /* tp_methods */
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
pymatecorba_poamanager_new(PortableServer_POAManager poamanager)
{
    PyCORBA_Object *self;
    PyTypeObject *type;
    PyObject *args;

    if (!poamanager) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    type = &PyPortableServer_POAManager_Type;
    args = PyTuple_New(0);
    self = (PyCORBA_Object *)type->tp_new(type, args, NULL);
    Py_DECREF(args);
    if (!self) return NULL;
    self->objref = (CORBA_Object)poamanager;
    return (PyObject *)self;
}
