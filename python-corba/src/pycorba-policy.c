/* -*- mode: C; c-basic-offset: 4 -*-
 * pymatecorba - a Python language mapping for the MateCORBA2 CORBA ORB
 * Copyright (C) 205  Gustavo Carneiro <gjc@inescporto.pt>
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
#include "pymatecorba-private.h"

static PyObject *
pycorba_policy_repr(PyCORBA_Policy *self)
{
    return PyString_FromFormat("<%s at %p>", self->ob_type->tp_name, self->objref);
}

static void
pycorba_policy_dealloc(PyCORBA_Object *self)
{
    if (self->objref)
	CORBA_Object_release(self->objref, NULL);
    self->objref = NULL;

    if (self->ob_type->tp_free)
	self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}

PyTypeObject PyCORBA_Policy_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Policy",                     /* tp_name */
    sizeof(PyCORBA_Policy), 		/* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_policy_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_policy_repr,      /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,   			/* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,
};


PyObject *
pycorba_policy_new(CORBA_Object policy)
{
    PyCORBA_Policy *self;

    self = PyObject_NEW(PyCORBA_Policy, &PyCORBA_Policy_Type);
    if (!self)
	return NULL;

    self->objref = CORBA_Object_duplicate(policy, NULL);
    return (PyObject *)self;
}


