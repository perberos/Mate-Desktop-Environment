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

PyObject *pymatecorba_exception;
PyObject *pymatecorba_system_exception;
PyObject *pymatecorba_user_exception;

gboolean
pymatecorba_check_ex(CORBA_Environment *ev)
{
    gboolean ret;
    if (ev->_major != CORBA_NO_EXCEPTION) {
	CORBA_any any;
	PyObject *instance;

	any = ev->_any;
	any._type = pymatecorba_lookup_typecode(ev->_id);
	instance = pymatecorba_demarshal_any(&any);
	any._type = NULL;
	if (instance) {
	    PyObject *stub, *attr;

	    attr = PyString_FromString(ev->_id);
	    PyObject_SetAttrString(instance, "_id", attr);
	    Py_DECREF(attr);
	
	    attr = PyInt_FromLong(ev->_major);
	    PyObject_SetAttrString(instance, "_major", attr);
	    Py_DECREF(attr);

	    stub = PyObject_GetAttrString(instance, "__class__");
	    PyErr_SetObject(stub, instance);
	    Py_DECREF(stub);
	    Py_DECREF(instance);
	} else {
	    PyObject *stub, *instance, *arg;

	    if (ev->_major == CORBA_SYSTEM_EXCEPTION)
		stub = pymatecorba_system_exception;
	    else
		stub = pymatecorba_user_exception;
	    instance = PyObject_CallFunction(stub, "()");
	    arg = PyString_FromString(ev->_id ? ev->_id : "(null)");
	    PyObject_SetAttrString(instance, "args", arg);
	    Py_DECREF(arg);

	    PyErr_SetObject(stub, instance);
	    Py_DECREF(instance);
	}
    }

    ret = (ev->_major != CORBA_NO_EXCEPTION);
    CORBA_exception_free(ev);
    return ret;
}

gboolean
pymatecorba_check_python_ex(CORBA_Environment *ev)
{
    if (PyErr_Occurred()) {
	PyObject *type = NULL, *val = NULL, *tb = NULL;
	PyObject *pytc;

	PyErr_Fetch(&type, &val, &tb);
	pytc = PyObject_GetAttrString(type, "__typecode__");
	if (pytc && PyObject_TypeCheck(pytc, &PyCORBA_TypeCode_Type)
	    && PyObject_IsSubclass(type, pymatecorba_exception)) {
	    CORBA_TypeCode tc = ((PyCORBA_TypeCode *)pytc)->tc;
	    CORBA_any any = { NULL, NULL, FALSE };

	    any._type = tc;
	    any._value = MateCORBA_small_alloc(tc);
	    if (pymatecorba_marshal_any(&any, val)) {
		CORBA_exception_type major;

		major = PyObject_IsSubclass(type, pymatecorba_system_exception)
		    ? CORBA_SYSTEM_EXCEPTION : CORBA_USER_EXCEPTION;
		CORBA_exception_set(ev, major, tc->repo_id, any._value);
	    } else {
		/* could not marshal exception */
		CORBA_free(any._value);
		CORBA_exception_set_system(ev, ex_CORBA_UNKNOWN,
					   CORBA_COMPLETED_MAYBE);
	    }
	} else {
	    Py_XDECREF(pytc);
	    PyErr_Restore(type, val, tb);
	    PyErr_Print();
	    type = val = tb = NULL;
	    CORBA_exception_set_system(ev, ex_CORBA_UNKNOWN,
				       CORBA_COMPLETED_MAYBE);
	}
	Py_XDECREF(type);
	Py_XDECREF(val);
	Py_XDECREF(tb);
	PyErr_Clear();
	return TRUE;
    }
    return FALSE;
}

static PyObject *
create_system_exception(CORBA_TypeCode tc, PyObject *corbamod)
{
    PyObject *exc;
    gchar *exc_name;

    exc_name = g_strconcat("CORBA.", tc->name, NULL);
    exc = PyErr_NewException(exc_name, pymatecorba_system_exception, NULL);
    g_free(exc_name);

    pymatecorba_register_stub(tc, exc);
    PyModule_AddObject(corbamod, tc->name, exc);

    return exc;
}

static PyObject *
pymatecorba_exception_init(PyObject *s, PyObject *args)
{
    PyObject *self;
    Py_ssize_t len, i;
    PyObject *pytc, *obj;
    CORBA_TypeCode tc;

    /* for the zero argument case, don't do anything */
    len = PyTuple_Size(args);
    if (len == 0) {
	PyErr_SetString(PyExc_TypeError, "required argument 'self' missing");
	return NULL;
    }
    self = PyTuple_GetItem(args, 0);

    /* make sure the exception has an "args" attribute */
    obj = PyTuple_New(0);
    PyObject_SetAttrString(self, "args", obj);
    Py_DECREF(obj);

    if (len == 1) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    pytc = PyObject_GetAttrString(self, "__typecode__");
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

    if (tc->sub_parts != len - 1) {
	PyErr_Format(PyExc_TypeError, "expected %d arguments, got %d",
		     tc->sub_parts, len);
	return NULL;
    }

    for (i = 1; i < len; i++) {
	PyObject *item  = PyTuple_GetItem(args, i);

	PyObject_SetAttrString(self, tc->subnames[i-1], item);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pymatecorba_exception_init_methoddef = {
    "__init__", pymatecorba_exception_init, METH_VARARGS
};

void
pymatecorba_register_exceptions(PyObject *corbamod)
{
    PyObject *init_f, *init_m;

    /* base exception classes */
    pymatecorba_exception = PyErr_NewException("CORBA.Exception",
					   PyExc_RuntimeError, NULL);
    /* exception constructor */
    init_f = PyCFunction_New(&pymatecorba_exception_init_methoddef, NULL);
    init_m = PyMethod_New(init_f, NULL, pymatecorba_exception);
    Py_DECREF(init_f);
    PyObject_SetAttrString(pymatecorba_exception, "__init__", init_m);
    Py_DECREF(init_m);

    PyModule_AddObject(corbamod, "Exception", pymatecorba_exception);
    pymatecorba_system_exception = PyErr_NewException("CORBA.SystemException",
						  pymatecorba_exception, NULL);
    PyModule_AddObject(corbamod, "SystemException", pymatecorba_system_exception);
    pymatecorba_user_exception = PyErr_NewException("CORBA.UserException",
						pymatecorba_exception, NULL);
    PyModule_AddObject(corbamod, "UserException", pymatecorba_user_exception);

    create_system_exception(TC_CORBA_UNKNOWN, corbamod);
    create_system_exception(TC_CORBA_BAD_PARAM, corbamod);
    create_system_exception(TC_CORBA_NO_MEMORY, corbamod);
    create_system_exception(TC_CORBA_IMP_LIMIT, corbamod);
    create_system_exception(TC_CORBA_COMM_FAILURE, corbamod);
    create_system_exception(TC_CORBA_INV_OBJREF, corbamod);
    create_system_exception(TC_CORBA_NO_PERMISSION, corbamod);
    create_system_exception(TC_CORBA_INTERNAL, corbamod);
    create_system_exception(TC_CORBA_MARSHAL, corbamod);
    create_system_exception(TC_CORBA_INITIALIZE, corbamod);
    create_system_exception(TC_CORBA_NO_IMPLEMENT, corbamod);
    create_system_exception(TC_CORBA_BAD_TYPECODE, corbamod);
    create_system_exception(TC_CORBA_BAD_OPERATION, corbamod);
    create_system_exception(TC_CORBA_NO_RESOURCES, corbamod);
    create_system_exception(TC_CORBA_NO_RESPONSE, corbamod);
    create_system_exception(TC_CORBA_PERSIST_STORE, corbamod);
    create_system_exception(TC_CORBA_BAD_INV_ORDER, corbamod);
    create_system_exception(TC_CORBA_TRANSIENT, corbamod);
    create_system_exception(TC_CORBA_FREE_MEM, corbamod);
    create_system_exception(TC_CORBA_INV_IDENT, corbamod);
    create_system_exception(TC_CORBA_INV_FLAG, corbamod);
    create_system_exception(TC_CORBA_INTF_REPOS, corbamod);
    create_system_exception(TC_CORBA_BAD_CONTEXT, corbamod);
    create_system_exception(TC_CORBA_OBJ_ADAPTER, corbamod);
    create_system_exception(TC_CORBA_DATA_CONVERSION, corbamod);
    create_system_exception(TC_CORBA_OBJECT_NOT_EXIST, corbamod);
    create_system_exception(TC_CORBA_TRANSACTION_REQUIRED, corbamod);
    create_system_exception(TC_CORBA_TRANSACTION_ROLLEDBACK, corbamod);
    create_system_exception(TC_CORBA_INVALID_TRANSACTION, corbamod);
    create_system_exception(TC_CORBA_INV_POLICY, corbamod);
    create_system_exception(TC_CORBA_CODESET_INCOMPATIBLE, corbamod);
}
