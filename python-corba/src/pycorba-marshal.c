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

size_t  MateCORBA_gather_alloc_info (CORBA_TypeCode tc);

#define alignval(val,align)    *val = ALIGN_ADDRESS(*val, align)
#define getval(val,type)       (*(type *)*val)
#define advanceptr(val,offset) *val = ((guchar *)*val) + offset

static CORBA_TypeCode
get_union_tc(CORBA_TypeCode tc, PyObject *discrim)
{
    CORBA_TypeCode subtc = NULL;
    glong discriminator, i;

    if (PyString_Check(discrim)) {
	if (PyString_Size(discrim) != 1)
	    return NULL;
	discriminator = *(CORBA_octet *)PyString_AsString(discrim);
    } else {
	discriminator = PyInt_AsLong(discrim);
	if (PyErr_Occurred())
	    return NULL;
    }

    for (i = 0; i < tc->sub_parts; i++) {
	if (i == tc->default_index) continue;
	if (tc->sublabels[i] == discriminator) {
	    subtc = tc->subtypes[i];
	    break;
	}
    }
    if (i == tc->sub_parts) {
	if (tc->default_index >= 0)
	    subtc = tc->subtypes[tc->default_index];
	else
	    subtc = TC_null;
    }

    return subtc;
}


static gboolean
marshal_value(CORBA_TypeCode tc, gconstpointer *val, PyObject *value)
{
    gboolean ret = FALSE;
    gint i;

    while (tc->kind == CORBA_tk_alias)
	tc = tc->subtypes[0];

    switch (tc->kind) {
    case CORBA_tk_null:
    case CORBA_tk_void:
	ret = (value == Py_None);
	break;
    case CORBA_tk_short:
	alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	getval(val, CORBA_short) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_short));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_long:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG);
	getval(val, CORBA_long) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_long));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_ushort:
	alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	getval(val, CORBA_unsigned_short) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_unsigned_short));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_ulong:
    case CORBA_tk_enum:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG);
	if (PyLong_Check(value))
	    getval(val, CORBA_unsigned_long) = PyLong_AsUnsignedLong(value);
	else
	    getval(val, CORBA_unsigned_long) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_unsigned_long));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_float:
	alignval(val, MATECORBA_ALIGNOF_CORBA_FLOAT);
	getval(val, CORBA_float) = PyFloat_AsDouble(value);
	advanceptr(val, sizeof(CORBA_float));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_double:
	alignval(val, MATECORBA_ALIGNOF_CORBA_DOUBLE);
	getval(val, CORBA_double) = PyFloat_AsDouble(value);
	advanceptr(val, sizeof(CORBA_double));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_boolean:
	getval(val, CORBA_boolean) = PyObject_IsTrue(value);
	advanceptr(val, sizeof(CORBA_boolean));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_char:
	if (PyString_Check(value) && PyString_Size(value) == 1) {
	    getval(val, CORBA_char) = PyString_AsString(value)[0];
	    advanceptr(val, sizeof(CORBA_char));
	    ret = TRUE;
	}
	break;
    case CORBA_tk_octet:
	getval(val, CORBA_long) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_octet));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_any:
	if (PyObject_TypeCheck(value, &PyCORBA_Any_Type)) {
	    alignval(val, MATECORBA_ALIGNOF_CORBA_ANY);
	    CORBA_any__copy(&getval(val, CORBA_any),
			    &((PyCORBA_Any *)value)->any);
	    advanceptr(val, sizeof(CORBA_any));
	    ret = TRUE;
	}
	break;
    case CORBA_tk_TypeCode:
	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	if (PyObject_TypeCheck(value, &PyCORBA_TypeCode_Type)) {
	    getval(val, CORBA_TypeCode) =
		(CORBA_TypeCode)CORBA_Object_duplicate(
			(CORBA_Object)((PyCORBA_TypeCode *)value)->tc, NULL);
	    ret = TRUE;
	}
	advanceptr(val, sizeof(CORBA_TypeCode));
	break;
    case CORBA_tk_Principal:
	g_warning("can't marshal Principal");
	break;
    case CORBA_tk_objref:
	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	if (value == Py_None) {
	    getval(val, CORBA_Object) = CORBA_OBJECT_NIL;
	    ret = TRUE;
	} else if (PyObject_TypeCheck(value, &PyCORBA_Object_Type)) {
	    CORBA_Object objref = ((PyCORBA_Object *)value)->objref;
	    getval(val, CORBA_Object) = CORBA_Object_duplicate(objref, NULL);
	    advanceptr(val, sizeof(CORBA_Object));
	    ret = TRUE;
	}
	break;
    case CORBA_tk_except:
    case CORBA_tk_struct:
	alignval(val, tc->c_align);
	for (i = 0; i < tc->sub_parts; i++) {
	    gchar *pyname;
	    PyObject *item;
	    gboolean itemret;

	    pyname = _pymatecorba_escape_name(tc->subnames[i]);
	    item = PyObject_GetAttrString(value, pyname);
	    g_free(pyname);
	    if (!item) break;
	    itemret = marshal_value(tc->subtypes[i], val, item);
	    Py_DECREF(item);
	    if (!itemret) break;
	}
	if (i == tc->sub_parts) ret = TRUE; /* no error */
	break;
    case CORBA_tk_union: {
	PyObject *discrim, *subval;
	CORBA_TypeCode subtc;
	gconstpointer body;
	int sz = 0;

	discrim = PyObject_GetAttrString(value, "_d");
	if (!discrim) break;
	subval = PyObject_GetAttrString(value, "_v");
	if (!subval) break;

	alignval(val, MAX(tc->c_align, tc->discriminator->c_align));
	ret = marshal_value(tc->discriminator, val, discrim);
	if (!ret) {
	    Py_DECREF(discrim);
	    Py_DECREF(subval);
	    break;
	}

	/* calculate allocation info */
	for (i = 0; i < tc->sub_parts; i++) {
	    sz = MAX(sz, MateCORBA_gather_alloc_info(tc->subtypes[i]));
	}
	subtc = get_union_tc(tc, discrim);
	Py_DECREF(discrim);
	if (!subtc) {
	    Py_DECREF(subval);
	    break;
	}

	alignval(val, tc->c_align);
	body = *val;

	ret = marshal_value(subtc, &body, subval);
	Py_DECREF(subval);
	advanceptr(val, sz);
	break;
    }
    case CORBA_tk_string:
	if (PyString_Check(value)) {
	    /* error out if python string is longer than the bound
	     * specified in the string typecode. */
	    if (tc->length > 0 && PyString_Size(value) > tc->length)
		break;

	    alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	    getval(val, CORBA_string) = CORBA_string_dup(PyString_AsString(value));
	    advanceptr(val, sizeof(CORBA_char *));
	    ret = TRUE;
	}
	break;
    case CORBA_tk_sequence: 
	if (PySequence_Check(value)) {
	    CORBA_sequence_CORBA_octet *sval;
	    gconstpointer seqval;
	    gint i;

	    /* error out if python sequence is longer than the bound
	     * specified in the sequence typecode. */
	    if (tc->length > 0 && PySequence_Length(value) > tc->length)
		break;

	    alignval(val, MATECORBA_ALIGNOF_CORBA_SEQ);
	    sval = (CORBA_sequence_CORBA_octet *)*val;
	    if (CORBA_TypeCode_equal(tc->subtypes[0], TC_CORBA_octet, NULL)
		&& PyString_Check(value))
	    {
		Py_ssize_t length;
		char *buffer;
		if (PyString_AsStringAndSize(value, &buffer, &length) == -1)
		    ret = FALSE;
		else {
		    sval->_release = CORBA_TRUE;
		    sval->_buffer  = MateCORBA_alloc_tcval(TC_CORBA_octet, length);
		    memcpy(sval->_buffer, buffer, length);
		    sval->_length  = length;
		    ret = TRUE;
		}
	    } else {
		sval->_release = TRUE;
		sval->_length = PySequence_Length(value);
		sval->_buffer = MateCORBA_alloc_tcval(tc->subtypes[0], sval->_length);
		seqval = sval->_buffer;
		for (i = 0; i < sval->_length; i++) {
		    PyObject *item = PySequence_GetItem(value, i);
		    gboolean itemret;
		    
		    if (!item) break;
		    itemret = marshal_value(tc->subtypes[0], &seqval, item);
		    Py_DECREF(item);
		    if (!itemret) break;
		}
		if (i == sval->_length) ret = TRUE; /* no error */
	    }
	    advanceptr(val, sizeof(CORBA_sequence_CORBA_octet));
	}
	break;
    case CORBA_tk_array:
	if (PySequence_Check(value) &&
	    PySequence_Length(value) == tc->length) {
	    gint i;

	    for (i = 0; i < tc->length; i++) {
		PyObject *item = PySequence_GetItem(value, i);
		gboolean itemret;

		if (!item) break;
		itemret = marshal_value(tc->subtypes[0], val, item);
		Py_DECREF(item);
		if (!itemret) break;
	    }
	    if (i == tc->length) ret = TRUE; /* no error */
	}
	break;
    case CORBA_tk_longlong:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG_LONG);
	if (PyLong_Check(value))
	    getval(val, CORBA_long_long) = PyLong_AsLongLong(value);
	else
	    getval(val, CORBA_long_long) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_long_long));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_ulonglong:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG_LONG);
	if (PyLong_Check(value))
	    getval(val, CORBA_unsigned_long_long) =PyLong_AsUnsignedLongLong(value);
	else
	    getval(val, CORBA_unsigned_long_long) = PyInt_AsLong(value);
	advanceptr(val, sizeof(CORBA_unsigned_long_long));
	if (!PyErr_Occurred()) ret = TRUE;
	break;
    case CORBA_tk_longdouble:
	g_warning("can't marshal long double");
	break;
    case CORBA_tk_wchar:
	if (PyUnicode_Check(value) && PyUnicode_GetSize(value) == 1) {
	    alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	    getval(val, CORBA_wchar) = PyUnicode_AsUnicode(value)[0];
	    advanceptr(val, sizeof(CORBA_wchar));
	    ret = TRUE;
	}
	break;
    case CORBA_tk_wstring:
	if (PyUnicode_Check(value)) {
	    int length = PyUnicode_GetSize(value);
	    Py_UNICODE *unicode = PyUnicode_AsUnicode(value);
	    CORBA_wchar *wstr;

	    /* error out if python unicode string is longer than the
	     * bound specified in the wstring typecode. */
	    if (tc->length > 0 && length > tc->length)
		break;

	    alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	    wstr = CORBA_wstring_alloc(length);
	    getval(val, CORBA_wstring) = wstr;
	    for (i = 0; i < length; i++) {
		wstr[i] = unicode[i];
	    }
	    advanceptr(val, sizeof(CORBA_wchar *));
	    ret = TRUE;
	}
	break;
    default:
	g_warning("unhandled typecode: %s (kind=%d)", tc->repo_id, tc->kind);
	break;
    }
    if (!ret)
	PyErr_Clear();
    return ret;
}

gboolean
pymatecorba_marshal_any(CORBA_any *any, PyObject *value)
{
    CORBA_TypeCode tc = any->_type;
    gconstpointer buf = any->_value;

    if (tc == NULL) return FALSE;
    return marshal_value(tc, &buf, value);
}

static PyObject *
demarshal_value(CORBA_TypeCode tc, gconstpointer *val)
{
    PyObject *ret = NULL;

    while (tc->kind == CORBA_tk_alias)
	tc = tc->subtypes[0];

    switch (tc->kind) {
    case CORBA_tk_null:
    case CORBA_tk_void:
	ret = Py_None;
	Py_INCREF(ret);
	break;
    case CORBA_tk_short:
	alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	ret = PyInt_FromLong(getval(val, CORBA_short));
	advanceptr(val, sizeof(CORBA_short));
	break;
    case CORBA_tk_long:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG);
	ret = PyInt_FromLong(getval(val, CORBA_long));
	advanceptr(val, sizeof(CORBA_long));
	break;
    case CORBA_tk_ushort:
	alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	ret = PyInt_FromLong(getval(val, CORBA_unsigned_short));
	advanceptr(val, sizeof(CORBA_unsigned_short));
	break;
    case CORBA_tk_ulong:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG);
	ret = PyLong_FromUnsignedLong(getval(val, CORBA_unsigned_long));
	advanceptr(val, sizeof(CORBA_unsigned_long));
	break;
    case CORBA_tk_float:
	alignval(val, MATECORBA_ALIGNOF_CORBA_FLOAT);
	ret = PyFloat_FromDouble(getval(val, CORBA_float));
	advanceptr(val, sizeof(CORBA_float));
	break;
    case CORBA_tk_double:
	alignval(val, MATECORBA_ALIGNOF_CORBA_DOUBLE);
	ret = PyFloat_FromDouble(getval(val, CORBA_double));
	advanceptr(val, sizeof(CORBA_double));
	break;
    case CORBA_tk_boolean:
	ret = getval(val, CORBA_boolean) ? Py_True : Py_False;
	Py_INCREF(ret);
	advanceptr(val, sizeof(CORBA_boolean));
	break;
    case CORBA_tk_char: {
	char charbuf[2];

	charbuf[0] = getval(val, CORBA_char);
	charbuf[1] = '\0';
	ret = PyString_FromString(charbuf);
	advanceptr(val, sizeof(CORBA_char));
	break;
    }
    case CORBA_tk_octet:
	ret = PyInt_FromLong(getval(val, CORBA_octet));
	advanceptr(val, sizeof(CORBA_octet));
	break;
    case CORBA_tk_any:
	alignval(val, MATECORBA_ALIGNOF_CORBA_ANY);
	ret = pycorba_any_new(&getval(val, CORBA_any));
	advanceptr(val, sizeof(CORBA_any));
	break;
    case CORBA_tk_TypeCode:
	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	ret = pycorba_typecode_new(getval(val, CORBA_TypeCode));
	advanceptr(val, sizeof(CORBA_TypeCode));
	break;
    case CORBA_tk_Principal:
	g_warning("can't demarshal Principal's");
	break;
    case CORBA_tk_objref:
	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	ret = pycorba_object_new_with_type(getval(val, CORBA_Object), tc);
	advanceptr(val, sizeof(CORBA_Object));
	break;
    case CORBA_tk_struct:
    case CORBA_tk_except: {
	PyObject *stub;
	PyObject *instance;
	gint i;

	alignval(val, tc->c_align);
	stub = pymatecorba_get_stub(tc);
	if (!stub) {
	    if (tc->kind == CORBA_tk_struct)
		stub = (PyObject *)&PyBaseObject_Type;
	    else
		stub = pymatecorba_exception;
	}
	instance = PyObject_CallFunction(stub, "()");
	if (!instance)
	    break;
	if (stub == pymatecorba_exception) {
	    PyObject *pytc = pycorba_typecode_new(tc);

	    PyObject_SetAttrString(instance, "__typecode__", pytc);
	    Py_DECREF(pytc);
	}
	for (i = 0; i < tc->sub_parts; i++) {
	    PyObject *item;
	    gchar *pyname;

	    item = demarshal_value(tc->subtypes[i], val);
	    if (!item) {
		Py_DECREF(instance);
		break;
	    }
	    pyname = _pymatecorba_escape_name(tc->subnames[i]);
	    PyObject_SetAttrString(instance, pyname, item);
	    g_free(pyname);
	    Py_DECREF(item);
	}
	if (i == tc->sub_parts) ret = instance;
	break;
    }
    case CORBA_tk_union: {
	PyObject *stub, *instance, *discrim, *subval;
	CORBA_TypeCode subtc;
	gint i, sz = 0;
	gconstpointer body;

	alignval(val, MAX(tc->c_align, tc->discriminator->c_align));
	stub = pymatecorba_get_stub(tc);
	if (!stub) {
	    if (tc->kind == CORBA_tk_struct)
		stub = (PyObject *)&PyBaseObject_Type;
	    else
		stub = PyExc_Exception;
	}
	instance = PyObject_CallFunction(stub, "()");
	if (!instance)
	    break;

	discrim = demarshal_value(tc->discriminator, val);
	if (!discrim) {
	    Py_DECREF(instance);
	    break;
	}
	subtc = get_union_tc(tc, discrim);
	if (!subtc) {
	    Py_DECREF(instance);
	    Py_DECREF(discrim);
	    break;
	}
	PyObject_SetAttrString(instance, "_d", discrim);
	Py_DECREF(discrim);

	for (i = 0; i < tc->sub_parts; i++)
	    sz = MAX(sz, MateCORBA_gather_alloc_info(tc->subtypes[i]));
	alignval(val, tc->c_align);
	body = *val;

	subval = demarshal_value(subtc, &body);
	if (!subval) {
	    Py_DECREF(instance);
	    break;
	}
	PyObject_SetAttrString(instance, "_v", subval);
	Py_DECREF(subval);

	ret = instance;
	advanceptr(val, sz);
	break;
    }
    case CORBA_tk_enum:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG);
	ret = pycorba_enum_from_long(tc, getval(val, CORBA_unsigned_long));
	advanceptr(val, sizeof(CORBA_unsigned_long));
	break;
    case CORBA_tk_string: {
	CORBA_string str;

	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	str = getval(val, CORBA_string);
	advanceptr(val, sizeof(CORBA_string));
	if (str) {
	    ret = PyString_FromString(str);
	} else {
	    Py_INCREF(Py_None);
	    ret = Py_None;
	}
	break;
    }
    case CORBA_tk_sequence: {
	const CORBA_sequence_CORBA_octet *sval;
	gconstpointer seqval;

	alignval(val, MATECORBA_ALIGNOF_CORBA_SEQ);
	sval = (CORBA_sequence_CORBA_octet *)*val;
	advanceptr(val, sizeof(CORBA_sequence_CORBA_octet));
	switch (tc->subtypes[0]->kind) {
	case CORBA_tk_char:
	case CORBA_tk_octet:
	    ret = PyString_FromStringAndSize((char *) sval->_buffer, sval->_length);
	    break;
	default: {
	    PyObject *list;
	    Py_ssize_t i;
            int align;

	    list = PyList_New(sval->_length);
	    align = tc->subtypes[0]->c_align;
	    seqval = sval->_buffer;
	    for (i = 0; i < sval->_length; i++) {
		PyObject *item;

		alignval(&seqval, align);
		item = demarshal_value(tc->subtypes[0], &seqval);
		if (!item) {
		    Py_DECREF(list);
		    break;
		}
		PyList_SetItem(list, i, item);
	    }
	    if (i == sval->_length) ret = list;
	}
	}
	break;
    }
    case CORBA_tk_array:
	switch (tc->subtypes[0]->kind) {
	case CORBA_tk_char:
	case CORBA_tk_octet:
	    ret = PyString_FromStringAndSize(*val, tc->length);
	    advanceptr(val, tc->length);
	    break;
	default: {
	    PyObject *list;
	    gint i, align;

	    list = PyList_New(tc->length);
	    align = tc->subtypes[0]->c_align;
	    for (i = 0; i < tc->length; i++) {
		PyObject *item;

		alignval(val, align);
		item = demarshal_value(tc->subtypes[0], val);
		if (!item) {
		    Py_DECREF(list);
		    break;
		}
		PyList_SetItem(list, i, item);
	    }
	    if (i == tc->length) ret = list;
	}
	}
	break;
    case CORBA_tk_longlong:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG_LONG);
	ret = PyLong_FromLongLong(getval(val, CORBA_long_long));
	advanceptr(val, sizeof(CORBA_long_long));
	break;
    case CORBA_tk_ulonglong:
	alignval(val, MATECORBA_ALIGNOF_CORBA_LONG_LONG);
	ret = PyLong_FromUnsignedLongLong(getval(val, CORBA_unsigned_long_long));
	advanceptr(val, sizeof(CORBA_unsigned_long_long));
	break;
    case CORBA_tk_longdouble:
	g_warning("can't demarshal long doubles");
	break;
    case CORBA_tk_wchar: {
	Py_UNICODE uchar;

	alignval(val, MATECORBA_ALIGNOF_CORBA_SHORT);
	uchar = getval(val, CORBA_wchar);
	ret = PyUnicode_FromUnicode(&uchar, 1);
	advanceptr(val, sizeof(CORBA_wchar));
	break;
    }
    case CORBA_tk_wstring: {
	CORBA_wstring wstr;

	alignval(val, MATECORBA_ALIGNOF_CORBA_POINTER);
	wstr = getval(val, CORBA_wstring);
	advanceptr(val, sizeof(CORBA_wstring));

	if (wstr) {
	    gint i, length = CORBA_wstring_len(wstr);
	    Py_UNICODE *ustr;

	    ustr = g_new(Py_UNICODE, length);
	    for (i = 0; i < length; i++) ustr[i] = wstr[i];
	    ret = PyUnicode_FromUnicode(ustr, length);
	    g_free(ustr);
	} else {
	    Py_INCREF(Py_None);
	    ret = Py_None;
	}
	break;
    }
    default:
	g_warning("unhandled typecode: %s, (kind==%d)", tc->repo_id, tc->kind);
	break;
    }
    return ret;
}

PyObject *
pymatecorba_demarshal_any(CORBA_any *any)
{
    CORBA_TypeCode tc = any->_type;
    gconstpointer buf = any->_value;

    if (tc == NULL)
	return NULL;
    return demarshal_value(tc, &buf);
}
