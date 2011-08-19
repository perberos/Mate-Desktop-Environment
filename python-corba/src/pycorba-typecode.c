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
pycorba_typecode_dealloc(PyCORBA_TypeCode *self)
{
    if (self->tc)
	CORBA_Object_release((CORBA_Object)self->tc, NULL);
    PyObject_DEL(self);
}

static int
pycorba_typecode_cmp(PyCORBA_TypeCode *self, PyCORBA_TypeCode *other)
{
    CORBA_Environment ev;
    gboolean equal;

    CORBA_exception_init(&ev);
    equal = self->tc == other->tc ||
	CORBA_TypeCode_equal(self->tc, other->tc, &ev);
    if (pymatecorba_check_ex(&ev))
	return -1;

    if (equal) return 0;
    if (self->tc < other->tc) return -1;
    return 1;
}

static long
_typecode_hash(gconstpointer v)
{
    CORBA_TypeCode tc = (CORBA_TypeCode)v;
    glong hash, i;

    hash = tc->kind;

    switch (tc->kind) {
    case CORBA_tk_wstring:
    case CORBA_tk_string:
	hash = (hash * 1000003) ^ tc->length;
	break;
    case CORBA_tk_objref:
	hash = (hash * 1000003) ^ g_str_hash(tc->repo_id);
	break;
    case CORBA_tk_except:
    case CORBA_tk_struct:
	hash = (hash * 1000003) ^ g_str_hash(tc->repo_id);
	hash = (hash * 1000003) ^ tc->sub_parts;
	for (i = 0; i < tc->sub_parts; i++)
	    hash = (hash * 1000003) ^ _typecode_hash(tc->subtypes[i]);
	break;
    case CORBA_tk_union:
	hash = (hash * 1000003) ^ g_str_hash(tc->repo_id);
	hash = (hash * 1000003) ^ tc->sub_parts;
	hash = (hash * 1000003) ^ _typecode_hash(tc->discriminator);
	hash = (hash * 1000003) ^ tc->default_index;
	for (i = 0; i < tc->sub_parts; i++) {
	    hash = (hash * 1000003) ^ _typecode_hash(tc->subtypes[i]);
	    hash = (hash * 1000003) ^ tc->sublabels[i];
	}
	break;
    case CORBA_tk_enum:
	hash = (hash * 1000003) ^ g_str_hash(tc->repo_id);
	hash = (hash * 1000003) ^ tc->sub_parts;
	for (i = 0; i < tc->sub_parts; i++)
	    hash = (hash * 1000003) ^ g_str_hash(tc->subnames[i]);
	break;
    case CORBA_tk_sequence:
    case CORBA_tk_array:
	hash = (hash * 1000003) ^ tc->length;
	hash = (hash * 1000003) ^ _typecode_hash(tc->subtypes[0]);
	break;
    case CORBA_tk_alias:
	hash = (hash * 1000003) ^ g_str_hash(tc->repo_id);
	hash = (hash * 1000003) ^ _typecode_hash(tc->subtypes[0]);
	break;
    case CORBA_tk_recursive:
	hash = (hash * 1000003) ^ tc->recurse_depth;
	break;
    case CORBA_tk_fixed:
	hash = (hash * 1000003) ^ tc->digits;
	hash = (hash * 1000003) ^ tc->scale;
	break;
    default:
	break;
    }
    return hash;
}

static long
pycorba_typecode_hash(PyCORBA_TypeCode *self)
{
    if (self->tc)
	return _typecode_hash(self->tc);
    return 0;
}

static PyObject *
pycorba_typecode_repr(PyCORBA_TypeCode *self)
{
    return PyString_FromFormat("<CORBA.TypeCode '%s'>",
	self->tc->repo_id ? self->tc->repo_id : "(null)");
}

static PyObject *
pycorba_typecode_get_kind(PyCORBA_TypeCode *self, void *closure)
{
    return pycorba_enum_from_long(TC_CORBA_TCKind, self->tc->kind);
}

static PyObject *
pycorba_typecode_get_flags(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->flags < G_MAXLONG)
	return PyInt_FromLong(self->tc->flags);
    return PyLong_FromUnsignedLong(self->tc->flags);
}

static PyObject *
pycorba_typecode_get_length(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->length < G_MAXLONG)
	return PyInt_FromLong(self->tc->length);
    return PyLong_FromUnsignedLong(self->tc->length);
}

static PyObject *
pycorba_typecode_get_sub_parts(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->sub_parts < G_MAXLONG)
	return PyInt_FromLong(self->tc->sub_parts);
    return PyLong_FromUnsignedLong(self->tc->sub_parts);
}

static PyObject *
pycorba_typecode_get_subtypes(PyCORBA_TypeCode *self, void *closure)
{
    PyObject *ret;
    Py_ssize_t i;

    if (self->tc->kind != CORBA_tk_struct &&
	self->tc->kind != CORBA_tk_except &&
	self->tc->kind != CORBA_tk_union &&
	self->tc->kind != CORBA_tk_alias &&
	self->tc->kind != CORBA_tk_array &&
	self->tc->kind != CORBA_tk_sequence) {
	PyErr_SetString(PyExc_TypeError,
			"subtypes not available for this type");
	return NULL;
    }
    ret = PyList_New(self->tc->sub_parts);
    for (i = 0; i < self->tc->sub_parts; i++) {
	PyObject *item = pycorba_typecode_new(self->tc->subtypes[i]);

	PyList_SetItem(ret, i, item);
    }
    return ret;
}

static PyObject *
pycorba_typecode_get_discriminator(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->kind != CORBA_tk_union) {
	PyErr_SetString(PyExc_TypeError,
			"discriminator not available for this type");
	return NULL;
    }
    return pycorba_typecode_new(self->tc->discriminator);
}

static PyObject *
pycorba_typecode_get_name(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->name)
	return PyString_FromString(self->tc->name);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pycorba_typecode_get_repo_id(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->repo_id)
	return PyString_FromString(self->tc->repo_id);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pycorba_typecode_get_subnames(PyCORBA_TypeCode *self, void *closure)
{
    PyObject *ret;
    Py_ssize_t i;

    if (self->tc->kind != CORBA_tk_struct &&
	self->tc->kind != CORBA_tk_except &&
	self->tc->kind != CORBA_tk_union &&
	self->tc->kind != CORBA_tk_enum) {
	PyErr_SetString(PyExc_TypeError,
			"subtypes not available for this type");
	return NULL;
    }
    ret = PyList_New(self->tc->sub_parts);
    for (i = 0; i < self->tc->sub_parts; i++) {
	PyObject *item = PyString_FromString(self->tc->subnames[i]);

	PyList_SetItem(ret, i, item);
    }
    return ret;
}

static PyObject *
pycorba_typecode_get_sublabels(PyCORBA_TypeCode *self, void *closure)
{
    PyObject *ret;
    gint i;

    if (self->tc->kind != CORBA_tk_union) {
	PyErr_SetString(PyExc_TypeError,
			"sublabels not available for this type");
	return NULL;
    }
    ret = PyList_New(self->tc->sub_parts);
    for (i = 0; i < self->tc->sub_parts; i++) {
	PyObject *item = PyInt_FromLong(self->tc->sublabels[i]);

	PyList_SetItem(ret, i, item);
    }
    return ret;
}

static PyObject *
pycorba_typecode_get_default_index(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->kind != CORBA_tk_union) {
	PyErr_SetString(PyExc_TypeError,
			"default_index not available for this type");
	return NULL;
    }
    return PyInt_FromLong(self->tc->default_index);
}

static PyObject *
pycorba_typecode_get_recurse_depth(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->kind != CORBA_tk_sequence) {
	PyErr_SetString(PyExc_TypeError,
			"recurse_depth not available for this type");
	return NULL;
    }
    return PyLong_FromUnsignedLong(self->tc->recurse_depth);
}

static PyObject *
pycorba_typecode_get_digits(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->kind != CORBA_tk_fixed) {
	PyErr_SetString(PyExc_TypeError,
			"digits not available for this type");
	return NULL;
    }
    return PyInt_FromLong(self->tc->digits);
}

static PyObject *
pycorba_typecode_get_scale(PyCORBA_TypeCode *self, void *closure)
{
    if (self->tc->kind != CORBA_tk_fixed) {
	PyErr_SetString(PyExc_TypeError,
			"scale not available for this type");
	return NULL;
    }
    return PyInt_FromLong(self->tc->scale);
}

static PyGetSetDef pycorba_typecode_getsets[] = {
    { "kind",          (getter)pycorba_typecode_get_kind,          (setter)0 },
    { "flags",         (getter)pycorba_typecode_get_flags,         (setter)0 },
    { "length",        (getter)pycorba_typecode_get_length,        (setter)0 },
    { "sub_parts",     (getter)pycorba_typecode_get_sub_parts,     (setter)0 },
    { "subtypes",      (getter)pycorba_typecode_get_subtypes,      (setter)0 },
    { "discriminator", (getter)pycorba_typecode_get_discriminator, (setter)0 },
    { "name",          (getter)pycorba_typecode_get_name,          (setter)0 },
    { "repo_id",       (getter)pycorba_typecode_get_repo_id,       (setter)0 },
    { "subnames",      (getter)pycorba_typecode_get_subnames,      (setter)0 },
    { "sublabels",     (getter)pycorba_typecode_get_sublabels,     (setter)0 },
    { "default_index", (getter)pycorba_typecode_get_default_index, (setter)0 },
    { "recurse_depth", (getter)pycorba_typecode_get_recurse_depth, (setter)0 },
    { "digits",        (getter)pycorba_typecode_get_digits,        (setter)0 },
    { "scale",         (getter)pycorba_typecode_get_scale,         (setter)0 },
    { NULL, (getter)0, (setter)0 }
};

static int
pycorba_typecode_init(PyCORBA_TypeCode *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "repo_id", NULL };
    const gchar *repo_id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:CORBA.TypeCode.__init__",
				     kwlist, &repo_id))
	return -1;

    self->tc = pymatecorba_lookup_typecode(repo_id);
    if (!self->tc) {
	PyErr_SetString(PyExc_ValueError, "could not look up typecode");
	return -1;
    }

    return 0;
}

PyTypeObject PyCORBA_TypeCode_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.TypeCode",                   /* tp_name */
    sizeof(PyCORBA_TypeCode),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_typecode_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pycorba_typecode_cmp,      /* tp_compare */
    (reprfunc)pycorba_typecode_repr,    /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)pycorba_typecode_hash,    /* tp_hash */
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
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    pycorba_typecode_getsets,           /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pycorba_typecode_init,    /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};

PyObject *
pycorba_typecode_new(CORBA_TypeCode tc)
{
    PyCORBA_TypeCode *self;

    if (tc == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    self =  PyObject_NEW(PyCORBA_TypeCode, &PyCORBA_TypeCode_Type);
    if (!self)
	return NULL;

    self->tc = (CORBA_TypeCode)CORBA_Object_duplicate((CORBA_Object)tc, NULL);
    return (PyObject *)self;
}
