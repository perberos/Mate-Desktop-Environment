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
pycorba_fixed_dealloc(PyCORBA_fixed *self)
{
    PyObject_DEL(self);
}

static PyObject *
pycorba_fixed_repr(PyCORBA_fixed *self)
{
    gchar *value;
    gint digits, scale, i, pos;
    gboolean have_leading_digit;
    PyObject *str;

    digits = self->fixed._digits;
    scale = self->fixed._scale;
    value = g_malloc(digits + 4); /* extra for '-', '.' and '\0' */
    pos = 0;
    have_leading_digit = FALSE;

    if ((self->fixed._value[(digits / 2)] & 0x0f) == 0xD)
	value[pos++] = '-';

    if (scale == digits) {
	value[pos++] = '0';
	value[pos++] = '.';
	have_leading_digit = TRUE;
    }

    for (i = digits - 1; i >= 0; i--) {
	gchar digit;

	if (i % 2 == 0)
	    digit = self->fixed._value[(digits - i) / 2] >> 4;
	else
	    digit = self->fixed._value[(digits - i - 1) / 2] & 0x0f;
	if (have_leading_digit || digit != 0) {
	    value[pos++] = '0' + digit;
	    have_leading_digit = TRUE;
	}
	if (i == scale) {
	    if (!have_leading_digit) value[pos++] = '0';
	    value[pos++] = '.';
	    have_leading_digit = TRUE;
	}
    }
    value[pos] = '\0'; /* null termination */
    /* trim trailing digits */
    while (pos > 0 && value[pos-1] == '0')
	value[--pos] = '\0';
    if (value[pos-1] == '.')
	value[--pos] = '\0';

    str = PyString_FromString(value);
    g_free(value);

    return str;
}

static gint
get_digit(PyObject **val_p)
{
    static PyObject *ten = NULL;
    PyObject *val = *val_p;
    gint remainder = -1;

    if (!ten) ten = PyInt_FromLong(10);

    if (PyInt_Check(val)) {
	gint ival = PyInt_AsLong(val);

	remainder = ival % 10;
	Py_DECREF(val);
	*val_p = PyInt_FromLong(ival / 10);
    } else {
	PyObject *tuple = PyNumber_Divmod(val, ten), *div, *mod;

	if (!tuple) {
	    PyErr_Clear();
	    return -1;
	}
	div = PyTuple_GetItem(tuple, 0);
	if (!div) {
	    PyErr_Clear();
	    Py_DECREF(tuple);
	    return -1;
	}
	mod = PyTuple_GetItem(tuple, 1);
	if (!mod) {
	    PyErr_Clear();
	    Py_DECREF(tuple);
	    return -1;
	}

	Py_DECREF(val);
	*val_p = div;
	Py_INCREF(div);

	remainder = PyInt_AsLong(mod);
	Py_DECREF(tuple);
	if (PyErr_Occurred()) {
	    PyErr_Clear();
	    return -1;
	}
    }
    return remainder;
}

static PyObject *
pycorba_fixed_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "digits", "scale", "value", NULL };
    unsigned short digits;
    short scale = 0;
    gint result;
    PyObject *obvalue, *value;
    PyCORBA_fixed *self;
    gint i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "HhO", kwlist,
				     &digits, &scale, &obvalue))
	return NULL;

    
    self = (PyCORBA_fixed *)type->tp_alloc(type, digits);
    self->fixed._digits = digits;
    self->fixed._scale = scale;

    if (PyObject_Cmp(obvalue, Py_False, &result) < 0) {
	Py_DECREF(self);
	return NULL;
    }
    if (result < 0) { /* negative */
	self->fixed._value[digits / 2] = 0xD;
	value = PyNumber_Negative(obvalue);
	if (!value) {
	    Py_DECREF(self);
	    return NULL;
	}
    } else {
	self->fixed._value[digits / 2] = 0xC;
	value = obvalue;
	Py_INCREF(value);
    }

    result = 1;
    i = 0;
    while (result != 0) {
	gint remainder;

	if (i >= digits) {
	    Py_DECREF(value);
	    Py_DECREF(self);
	    PyErr_SetString(PyExc_ValueError, "value out of range for fixed");
	    return NULL;
	}

	remainder = get_digit(&value);
	if (remainder < 0) {
	    Py_DECREF(value);
	    Py_DECREF(self);
	    return NULL;
	}
	if (i % 2 == 0) {
	    self->fixed._value[(digits - i) / 2] |= remainder << 4;
	} else {
	    self->fixed._value[(digits - i - 1) / 2] = remainder;
	}

	if (PyObject_Cmp(value, Py_False, &result) < 0) {
	    Py_DECREF(value);
	    Py_DECREF(self);
	    return NULL;
	}
	i++;
    }
    Py_DECREF(value);

    return (PyObject *)self;
}

static PyObject *
pycorba_fixed_value(PyCORBA_fixed *self)
{
    static PyObject *ten = NULL;
    PyObject *ret;
    gint i, digits;

    if (!ten) ten = PyInt_FromLong(10);

    digits = self->fixed._digits;
    ret = PyInt_FromLong(0);
    for (i = digits - 1; i >= 0; i--) {
	gchar digit;
	PyObject *tmp, *pydigit;

	if (i % 2 == 0)
	    digit = self->fixed._value[(digits - i) / 2] >> 4;
	else
	    digit = self->fixed._value[(digits - i - 1) / 2] & 0x0f;

	tmp = PyNumber_Multiply(ret, ten);
	Py_DECREF(ret);
	pydigit = PyInt_FromLong(digit);
	ret = PyNumber_Add(tmp, pydigit);;
	Py_DECREF(tmp);
	Py_DECREF(pydigit);
    }

    if ((self->fixed._value[digits / 2] & 0x0f) == 0xD) {
	PyObject *tmp;

	tmp = PyNumber_Negative(ret);
	Py_DECREF(ret);
	ret = tmp;
    }

    return ret;
}

static PyObject *
pycorba_fixed_precision(PyCORBA_fixed *self)
{
    return PyInt_FromLong(self->fixed._digits);
}

static PyObject *
pycorba_fixed_decimals(PyCORBA_fixed *self)
{
    return PyInt_FromLong(self->fixed._scale);
}

static PyMethodDef pycorba_fixed_methods[] = {
    { "value",     (PyCFunction)pycorba_fixed_value,     METH_NOARGS },
    { "precision", (PyCFunction)pycorba_fixed_precision, METH_NOARGS },
    { "decimals",  (PyCFunction)pycorba_fixed_decimals,  METH_NOARGS },
    { NULL, NULL, 0 }
};

PyTypeObject PyCORBA_fixed_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "CORBA.fixed",                      /* tp_name */
    sizeof(PyCORBA_fixed) - sizeof(signed char), /* tp_basicsize */
    sizeof(signed char),                /* tp_itemsize */
    /* methods */
    (destructor)pycorba_fixed_dealloc,   /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_fixed_repr,        /* tp_repr */
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
    pycorba_fixed_methods,              /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)pycorba_fixed_new,          /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};
