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
#include <structmember.h>

static void
pycorba_struct_dealloc(PyObject *self)
{
    if (self->ob_type->tp_free)
	self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}

static int
pycorba_struct_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t len, i;
    PyObject *pytc;
    CORBA_TypeCode tc;

    /* for the zero argument case, don't do anything */
    len = PyTuple_Size(args);
    if (len == 0 && kwargs == NULL) {
	return 0;
    }

    pytc = PyObject_GetAttrString(self, "__typecode__");
    if (!pytc)
	return -1;
    if (!PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__typecode__ attribute not a typecode");
	return -1;
    }
    tc = ((PyCORBA_TypeCode *)pytc)->tc;
    Py_DECREF(pytc);

    if (tc->sub_parts != len) {
	PyErr_Format(PyExc_TypeError, "expected %d arguments, got %d",
		     tc->sub_parts, len);
	return -1;
    }

    for (i = 0; i < len; i++) {
	PyObject *item  = PyTuple_GetItem(args, i);

	PyObject_SetAttrString(self, tc->subnames[i], item);
    }
    return 0;
}

PyTypeObject PyCORBA_Struct_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Struct",                     /* tp_name */
    sizeof(PyObject),                   /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_struct_dealloc, /* tp_dealloc */
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
    (initproc)pycorba_struct_init,      /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

typedef struct {
    PyObject_HEAD

    PyObject *_d;
    PyObject *_v;
} PyCORBA_Union;

static void
pycorba_union_dealloc(PyCORBA_Union *self)
{
    Py_XDECREF(self->_d);
    Py_XDECREF(self->_v);
    if (self->ob_type->tp_free)
	self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}

static int
pycorba_union_init(PyCORBA_Union *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t len;
    PyObject *pytc;
    CORBA_TypeCode tc;

    /* for the zero argument case, don't do anything */
    len = PyTuple_Size(args);
    if (len == 0 && kwargs == NULL) {
	return 0;
    }

    pytc = PyObject_GetAttrString((PyObject *)self, "__typecode__");
    if (!pytc)
	return -1;
    if (!PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__typecode__ attribute not a typecode");
	return -1;
    }
    tc = ((PyCORBA_TypeCode *)pytc)->tc;
    Py_DECREF(pytc);

    if (len > 0 && kwargs == NULL) {
	PyObject *discriminator, *value;

	if (!PyArg_ParseTuple(args, "OO", &discriminator, &value))
	    return -1;
	Py_XDECREF(self->_d);
	self->_d = discriminator;
	Py_INCREF(self->_d);
	Py_XDECREF(self->_v);
	self->_v = value;
	Py_INCREF(self->_v);
    } else if (len == 0 && PyDict_Size(kwargs) == 1) {
	Py_ssize_t pos = 0;
	PyObject *key, *val, *discriminator;
	const gchar *keyname;

	/* we know there is one value in the dict ... */
	PyDict_Next(kwargs, &pos, &key, &val);
	keyname = PyString_AsString(key);

	for (pos = 0; pos < tc->sub_parts; pos++) {
	    if (!strcmp(keyname, tc->subnames[pos]))
		break;
	}
	if (pos == tc->sub_parts) {
	    PyErr_Format(PyExc_TypeError, "union does not have member '%s'",
			 keyname);
	    return -1;
	}
	if (pos == tc->default_index) {
	    PyErr_SetString(PyExc_TypeError,
			    "can not deduce discriminator for default case");
	    return -1;
	}

	switch (tc->discriminator->kind) {
	case CORBA_tk_boolean:
	    discriminator = tc->sublabels[pos] ? Py_True : Py_False;
	    Py_INCREF(discriminator);
	    break;
	case CORBA_tk_char: {
	    char s[2] = { 0, '\0' };

	    s[0] = (CORBA_char)tc->sublabels[pos];
	    discriminator = PyString_FromString(s);
	    break;
	}
	case CORBA_tk_octet:
	case CORBA_tk_short:
	case CORBA_tk_long:
	case CORBA_tk_longlong:
	case CORBA_tk_ushort:
	case CORBA_tk_ulong:
	case CORBA_tk_ulonglong:
	    discriminator = PyInt_FromLong(tc->sublabels[pos]);
	    break;
	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled discriminator type");
	    return -1;
	}

	Py_XDECREF(self->_d);
	self->_d = discriminator;
	Py_XDECREF(self->_v);
	self->_v = val;
	Py_INCREF(self->_v);
    } else {
	PyErr_SetString(PyExc_TypeError, "expected two arguments, or one keyword argument");
	return -1;
    }

    return 0;
}

static PyMemberDef pycorba_union_members[] = {
    { "_d", T_OBJECT, offsetof(PyCORBA_Union, _d), 0,
      "union discriminator" },
    { "_v", T_OBJECT, offsetof(PyCORBA_Union, _v), 0,
      "union value" },
    { NULL, 0, 0, 0 }
};

PyTypeObject PyCORBA_Union_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.Union",                      /* tp_name */
    sizeof(PyCORBA_Union),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_union_dealloc,  /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    0,                                  /* tp_methods */
    pycorba_union_members,              /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pycorba_union_init,       /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

typedef struct {
    PyObject_HEAD
    const char *subname;
} PyCORBA_UnionMember;

static gboolean
branch_matches(PyCORBA_UnionMember *self, PyCORBA_Union *obj)
{
    PyObject *pytc;
    CORBA_TypeCode tc;
    CORBA_long discrim, pos;

    pytc = PyObject_GetAttrString((PyObject *)obj, "__typecode__");
    if (!pytc)
	return FALSE;
    if (!PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)) {
	Py_DECREF(pytc);
	PyErr_SetString(PyExc_TypeError,
			"__typecode__ attribute not a typecode");
	return FALSE;
    }
    tc = ((PyCORBA_TypeCode *)pytc)->tc;
    Py_DECREF(pytc);

    if (!obj->_d) {
	PyErr_Clear();
	PyErr_SetString(PyExc_AttributeError, "could not read discriminator");
	return FALSE;
    }
    if (PyString_Check(obj->_d)) {
	if (PyString_Size(obj->_d) != 1) {
	    PyErr_SetString(PyExc_ValueError,
			"string discriminators must be one character long");
	    return FALSE;
	}
	discrim = *(CORBA_octet *)PyString_AsString(obj->_d);
    } else {
	discrim = PyInt_AsLong(obj->_d);
	if (PyErr_Occurred()) {
	    PyErr_Clear();
	    PyErr_SetString(PyExc_ValueError,
			    "could not read discriminator as an integer");
	    return FALSE;
	}
    }

    for (pos = 0; pos < tc->sub_parts; pos++) {
	if (pos == tc->default_index) continue;
	if (tc->sublabels[pos] == discrim)
	    break;
    }
    if (pos == tc->sub_parts) {
	if (tc->default_index < 0) {
	    PyErr_SetString(PyExc_ValueError,
		"discriminator value doesn't match any union branches");
	    return FALSE;
	}
	pos = tc->default_index;
    }

    if (strcmp(self->subname, tc->subnames[pos]) != 0) {
	PyErr_Format(PyExc_ValueError, "union branch %s is not active",
		     self->subname);
	return FALSE;
    }
    return TRUE;
}

static PyObject *
pycorba_union_member_descr_get(PyCORBA_UnionMember *self, PyCORBA_Union *obj,
			       PyObject *type)
{
    if (!obj) {
	Py_INCREF(self);
	return (PyObject *)self;
    }
    if (!PyObject_TypeCheck(obj, &PyCORBA_Union_Type)) {
	PyErr_SetString(PyExc_TypeError,
			"this descriptor can only be used with union objects");
	return NULL;
    }

    if (!branch_matches(self, obj))
	return NULL;

    if (obj->_v) {
	Py_INCREF(obj->_v);
	return obj->_v;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int
pycorba_union_member_descr_set(PyCORBA_UnionMember *self, PyCORBA_Union *obj,
			       PyObject *value)
{
    if (!PyObject_TypeCheck(obj, &PyCORBA_Union_Type)) {
	PyErr_SetString(PyExc_TypeError,
			"this descriptor can only be used with union objects");
	return -1;
    }

    if (!branch_matches(self, obj))
	return -1;

    Py_XDECREF(obj->_v);
    obj->_v = value;
    Py_INCREF(obj->_v);
    return 0;
}

PyTypeObject PyCORBA_UnionMember_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.UnionMember",                /* tp_name */
    sizeof(PyCORBA_UnionMember),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_struct_dealloc, /* tp_dealloc */
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
    (descrgetfunc)pycorba_union_member_descr_get,  /* tp_descr_get */
    (descrsetfunc)pycorba_union_member_descr_set,  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

void
pymatecorba_add_union_members_to_stub(PyObject *stub, CORBA_TypeCode tc)
{
    PyObject *tp_dict;
    int i;

    g_return_if_fail(PyType_Check(stub) && PyType_IsSubtype((PyTypeObject *)stub, &PyCORBA_Union_Type));

    tp_dict = ((PyTypeObject *)stub)->tp_dict;
    for (i = 0; i < tc->sub_parts; i++) {
	PyCORBA_UnionMember *member;
	gchar *pyname;

	member = PyObject_NEW(PyCORBA_UnionMember, &PyCORBA_UnionMember_Type);
	if (!member)
	    return;
	member->subname = tc->subnames[i];

	pyname = _pymatecorba_escape_name(tc->subnames[i]);
	PyDict_SetItemString(tp_dict, pyname, (PyObject *)member);
	g_free(pyname);
	Py_DECREF(member);
    }
}
