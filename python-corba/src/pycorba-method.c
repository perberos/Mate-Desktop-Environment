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

typedef struct {
    PyObject_HEAD
    MateCORBA_IMethod *imethod;
    PyObject *meth_class;
} PyCORBA_Method;

typedef struct {
    PyObject_HEAD
    PyCORBA_Method *meth;
    PyObject *meth_self;
} PyCORBA_BoundMethod;

static void
pycorba_method_dealloc(PyCORBA_Method *self)
{
    Py_DECREF(self->meth_class);
    PyObject_DEL(self);
}

static PyObject *
pycorba_method_repr(PyCORBA_Method *self)
{
    return PyString_FromFormat("<unbound CORBA method %s.%s>",
			       ((PyTypeObject *)self->meth_class)->tp_name,
			       self->imethod->name);
}

static inline gboolean
pycorba_call_marshal_args(MateCORBA_IMethod *imethod, PyObject *args,
                          CORBA_TypeCode *pret_tc, gpointer *pret,
                          gpointer **pretptr, gpointer **pargv,
                          gpointer **pargvptr, int *pnum_args, int *p_n_rets)
{
    CORBA_TypeCode ret_tc;
    gpointer ret = NULL, *retptr = NULL, *argv = NULL, *argvptr = NULL;
    PyObject *item;
    gint n_args, n_rets, argpos, num_args, i;
    gboolean has_ret;

    /* unaliased argument types ... */
    ret_tc = imethod->ret;
    while (ret_tc && ret_tc->kind == CORBA_tk_alias)
	ret_tc = ret_tc->subtypes[0];
    has_ret = ret_tc != CORBA_OBJECT_NIL && ret_tc->kind != CORBA_tk_void;
    /* calculate number of in or inout arguments */
    n_args = 0;
    n_rets = 0;
    for (i = 0; i < imethod->arguments._length; i++) {
	if ((imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0)
	    n_args++;
	if ((imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_OUT | MateCORBA_I_ARG_INOUT)) != 0)
	    n_rets++;
    }
    if (PyTuple_Size(args) != n_args + 1) {
	PyErr_Format(PyExc_TypeError, "wrong number of arguments: expected %i, got %i",
                     n_args + 1, (int) PyTuple_Size(args));
        PyObject_Print(args, stderr, 0);
	return FALSE;
    }

    num_args = imethod->arguments._length;
    
    /* set up return value argument, depending on type */
    if (has_ret)
	switch (ret_tc->kind) {
	case CORBA_tk_any:
	case CORBA_tk_sequence:
	case CORBA_tk_array:
            retptr = g_new0(gpointer, 1);
	    ret = retptr;
	    break;
	case CORBA_tk_struct:
	case CORBA_tk_union:
	    /* handle non-fixed size structs, arrays and unions like any/seq */
	    if ((imethod->flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0) {
                retptr = g_new0(gpointer, 1);
		ret = retptr;
		break;
	    }
	    /* else fall through */
	default:
	    ret = MateCORBA_small_alloc(imethod->ret);
	}
    argv = g_new0(gpointer, num_args);
    argvptr = g_new0(gpointer, num_args);
    for (i = 0; i < num_args; i++) {
	gint flags = imethod->arguments._buffer[i].flags;
	CORBA_TypeCode tc = imethod->arguments._buffer[i].tc;

	if ((flags & (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0) {
	    argv[i] = MateCORBA_small_alloc(tc);
	} else { /* OUT */
	    argv[i] = &argvptr[i];
	    /* is it a "variable length" type? */
	    if (tc->kind == CORBA_tk_any || tc->kind == CORBA_tk_sequence ||
		((tc->kind == CORBA_tk_struct || tc->kind == CORBA_tk_union ||
		  tc->kind == CORBA_tk_array) &&
		 (flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0))
		argvptr[i] = NULL;
	    else
		argvptr[i] = MateCORBA_small_alloc(tc);
	}
    }

    argpos = 1;
    for (i = 0; i < num_args; i++) {
	gint flags = imethod->arguments._buffer[i].flags;

	if ((flags & (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0) {
	    CORBA_any any = { NULL, NULL, FALSE };

	    any._type = imethod->arguments._buffer[i].tc;
	    any._value = argv[i];
	    item = PyTuple_GetItem(args, argpos++);
	    if (!pymatecorba_marshal_any(&any, item)) {
		PyErr_Format(PyExc_TypeError, "could not marshal arg '%s'",
			     imethod->arguments._buffer[i].name ?
			     imethod->arguments._buffer[i].name : "<unknown>");
		return FALSE;
	    }
	}
    }
    *pret = ret;
    *pargv = argv;
    *pargvptr = argvptr;
    *pret_tc = ret_tc;
    *pnum_args = num_args;
    *pretptr = retptr;
    *p_n_rets = n_rets;
    return TRUE;
}

static inline PyObject *
pycorba_call_demarshal_retval(MateCORBA_IMethod *imethod, int num_args, CORBA_TypeCode ret_tc,
                              gpointer ret, gpointer *argv, gpointer *argvptr, int n_rets)
{
    gint i, retpos;
    PyObject *pyret = NULL, *item;
    gboolean has_ret = (ret_tc != CORBA_OBJECT_NIL && ret_tc->kind != CORBA_tk_void);

    pyret = PyTuple_New(n_rets + (has_ret ? 1 : 0));
    retpos = 0;
    if (has_ret) {
	CORBA_any any = { NULL, NULL, FALSE };

	any._type = imethod->ret;
	switch (ret_tc->kind) {
	case CORBA_tk_any:
	case CORBA_tk_sequence:
	case CORBA_tk_array:
	    any._value = *(gpointer *)ret;
	    break;
	case CORBA_tk_struct:
	case CORBA_tk_union:
	    /* handle non-fixed size structs, arrays and unions like any/seq */
	    if ((imethod->flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0) {
		any._value = *(gpointer *)ret;
		break;
	    }
	    /* else fall through */
	default:
	    any._value = ret;
	}
	item = pymatecorba_demarshal_any(&any);
	if (!item) {
	    Py_DECREF(pyret);
	    pyret = NULL;
	    PyErr_SetString(PyExc_TypeError,
			    "could not demarshal return value");
	    return NULL;
	}
	PyTuple_SetItem(pyret, retpos++, item);
    }
    for (i = 0; i < num_args; i++) {
	gint flags = imethod->arguments._buffer[i].flags;

	if ((flags & MateCORBA_I_ARG_OUT) != 0) {
	    CORBA_any any = { NULL, NULL, FALSE };

	    any._type = imethod->arguments._buffer[i].tc;
	    any._value = argvptr[i];
	    item = pymatecorba_demarshal_any(&any);
	    if (!item) {
		Py_DECREF(pyret);
		PyErr_Format(PyExc_TypeError,
			     "could not demarshal return value '%s'",
			     imethod->arguments._buffer[i].name ?
			     imethod->arguments._buffer[i].name : "<unknown>");
		return NULL;
	    }
	    PyTuple_SetItem(pyret, retpos++, item);
	} else if ((flags & (MateCORBA_I_ARG_INOUT)) != 0) {
	    CORBA_any any = { NULL, NULL, FALSE };

	    any._type = imethod->arguments._buffer[i].tc;
	    any._value = argv[i];
	    item = pymatecorba_demarshal_any(&any);
	    if (!item) {
		Py_DECREF(pyret);
		PyErr_Format(PyExc_TypeError,
			     "could not demarshal return value '%s'",
			     imethod->arguments._buffer[i].name ?
			     imethod->arguments._buffer[i].name : "<unknown>");
		return NULL;
	    }
	    PyTuple_SetItem(pyret, retpos++, item);
	}
    }
    /* special case certain n_args cases */
    switch (PyTuple_Size(pyret)) {
    case 0:
	Py_DECREF(pyret);
	Py_INCREF(Py_None);
	pyret = Py_None;
	break;
    case 1:
	item = PyTuple_GetItem(pyret, 0);
	Py_INCREF(item);
	Py_DECREF(pyret);
	pyret = item;
	break;
    default:
	break;
    }
    return pyret;
}

static inline void
pycorba_call_cleanup(MateCORBA_IMethod *imethod, int num_args, gpointer ret,
                     gpointer *argv, gpointer *argvptr, gpointer *retptr,
                     CORBA_TypeCode ret_tc)
{
    int i;
    if (ret) {
	switch (ret_tc->kind) {
	case CORBA_tk_any:
	case CORBA_tk_sequence:
	case CORBA_tk_array:
	    CORBA_free(*retptr);
            g_free(retptr);
	    break;
	case CORBA_tk_struct:
	case CORBA_tk_union:
	    /* handle non-fixed size structs, arrays and unions like any/seq */
	    if ((imethod->flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0) {
		CORBA_free(*retptr);
                g_free(retptr);
		break;
	    }
	    /* else fall through */
	default:
	    CORBA_free(ret);
	}
    }
    if (argv) {
	for (i = 0; i < num_args; i++) {
	    gint flags = imethod->arguments._buffer[i].flags;

	    if ((flags & MateCORBA_I_ARG_OUT) != 0)
		CORBA_free(argvptr[i]);
	    else
		CORBA_free(argv[i]);
	}
	g_free(argv);
	g_free(argvptr);
    }
}


/* XXXX handle keyword arguments? */
static PyObject *
pycorba_method_call(PyCORBA_Method *self, PyObject *args, PyObject *kwargs)
{
    CORBA_Object objref;
    CORBA_Environment ev;
    CORBA_TypeCode ret_tc = TC_null;
    gpointer ret = NULL, *retptr = NULL, *argv = NULL, *argvptr = NULL;
    PyObject *obj, *pyret = NULL;
    int num_args = 0, n_rets;

    obj = PyTuple_GetItem(args, 0);
    if (!PyObject_TypeCheck(obj, (PyTypeObject *)self->meth_class)) {
	PyErr_SetString(PyExc_TypeError, "wrong object type as first arg");
	return NULL;
    }


    if (!pycorba_call_marshal_args(self->imethod, args, &ret_tc, &ret,
                                   &retptr, &argv, &argvptr, &num_args, &n_rets))
        goto cleanup;


    objref = ((PyCORBA_Object *)obj)->objref;

    CORBA_exception_init(&ev);
    pymatecorba_begin_allow_threads;
    MateCORBA_small_invoke_stub(objref, self->imethod, ret, argv,
			    CORBA_OBJECT_NIL, &ev);
    pymatecorba_end_allow_threads;
    if (pymatecorba_check_ex(&ev))
	goto cleanup;
    CORBA_exception_free(&ev);

    pyret = pycorba_call_demarshal_retval(self->imethod, num_args, ret_tc,
                                          ret, argv, argvptr, n_rets);
 cleanup:
    pycorba_call_cleanup(self->imethod, num_args, ret, argv, argvptr, retptr, ret_tc);
    return pyret;
}

static PyObject *
pycorba_method_get_doc(PyCORBA_Method *self, void *closure)
{
    GString *string;
    gint i;
    gboolean has_arg;
    PyObject *ret;

    string = g_string_new(NULL);
    g_string_append(string, self->imethod->name);
    g_string_append_c(string, '(');
    has_arg = FALSE;
    for (i = 0; i < self->imethod->arguments._length; i++) {
	if ((self->imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0) {
	    const gchar *argname = self->imethod->arguments._buffer[i].name;
	    g_string_append(string, argname ? argname : "arg");
	    g_string_append(string, ", ");
	    has_arg = TRUE;
	}
    }
    if (has_arg) g_string_truncate(string, string->len - 2); /* ", " */
    g_string_append(string, ") -> ");
    has_arg = FALSE;
    if (self->imethod->ret != CORBA_OBJECT_NIL) {
	g_string_append_c(string, '\'');
	g_string_append(string, self->imethod->ret->repo_id);
	g_string_append(string, "', ");
	has_arg = TRUE;
    }
    for (i = 0; i < self->imethod->arguments._length; i++) {
	if ((self->imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_OUT | MateCORBA_I_ARG_INOUT)) != 0) {
	    g_string_append(string, self->imethod->arguments._buffer[i].name);
	    g_string_append(string, ", ");
	    has_arg = TRUE;
	}
    }
    if (has_arg)
	g_string_truncate(string, string->len - 2); /* ", " */
    else
	g_string_truncate(string, string->len - 4); /* " -> " */

    ret = PyString_FromString(string->str);
    g_string_free(string, TRUE);
    return ret;
}

static PyObject *
pycorba_method_get_name(PyCORBA_Method *self, void *closure)
{
    return PyString_FromString(self->imethod->name);
}

static PyObject *
pycorba_method_get_class(PyCORBA_Method *self, void *closure)
{
    Py_INCREF(self->meth_class);
    return self->meth_class;
}

static PyGetSetDef pycorba_method_getsets[] = {
    { "__doc__",  (getter)pycorba_method_get_doc,   (setter)0 },
    { "__name__", (getter)pycorba_method_get_name,  (setter)0 },
    { "im_name",  (getter)pycorba_method_get_name,  (setter)0 },
    { "im_class", (getter)pycorba_method_get_class, (setter)0 },
    { NULL,       (getter)0,                  (setter)0 }
};

static PyObject *
pycorba_method_descr_get(PyCORBA_Method *self, PyObject *obj, PyObject *type)
{
    PyCORBA_BoundMethod *bmeth;

    if (obj == NULL || obj == Py_None) {
	Py_INCREF(self);
	return (PyObject *)self;
    }

    bmeth = PyObject_NEW(PyCORBA_BoundMethod, &PyCORBA_BoundMethod_Type);
    if (!bmeth)
	return NULL;
    Py_INCREF(self);
    bmeth->meth = self;
    Py_INCREF(obj);
    bmeth->meth_self = obj;
    
    return (PyObject *)bmeth;
}

PyTypeObject PyCORBA_Method_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "MateCORBA.Method",                     /* tp_name */
    sizeof(PyCORBA_Method),             /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_method_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_method_repr,      /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)pycorba_method_call,   /* tp_call */
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
    pycorba_method_getsets,             /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    (descrgetfunc)pycorba_method_descr_get,   /* tp_descr_get */
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
pycorba_bound_method_dealloc(PyCORBA_BoundMethod *self)
{
    Py_DECREF(self->meth);
    Py_DECREF(self->meth_self);
    PyObject_DEL(self);
}

static PyObject *
pycorba_bound_method_repr(PyCORBA_BoundMethod *self)
{
    return PyString_FromFormat("<bound CORBA method %s.%s>",
			       self->meth->meth_class->ob_type->tp_name,
			       self->meth->imethod->name);
}

static PyObject *
pycorba_bound_method_call(PyCORBA_BoundMethod *self, PyObject *args, PyObject *kwargs)
{
    PyObject *selfarg, *ret;

    selfarg = PyTuple_New(1);
    Py_INCREF(self->meth_self);
    PyTuple_SetItem(selfarg, 0, self->meth_self);
    args = PySequence_Concat(selfarg, args);
    Py_DECREF(selfarg);

    ret = pycorba_method_call(self->meth, args, kwargs);
    Py_DECREF(args);

    return ret;
}

typedef struct _PyMateCORBAAsyncData {
    PyObject *callback;
    PyObject *user_data;
    CORBA_TypeCode ret_tc;
    gpointer ret;
    gpointer *retptr;
    gpointer *argv;
    gpointer *argvptr;
    int num_args;
    int n_rets;
} PyMateCORBAAsyncData;

static void
async_callback (CORBA_Object          object,
                MateCORBA_IMethod        *imethod,
                MateCORBAAsyncQueueEntry *aqe,
                gpointer              user_data,
                CORBA_Environment    *ev)
{
    PyMateCORBAAsyncData *async_data = user_data;
    PyObject *py_async_retval = NULL, *pyexc_type = NULL, *pyexc_value = NULL;
    PyObject *pytmp;
    PyGILState_STATE state;

    g_return_if_fail (async_data != NULL);

    state = pymatecorba_gil_state_ensure();
    if (pymatecorba_check_ex(ev)) {
        PyObject *traceback = NULL;
        PyErr_Fetch(&pyexc_type, &pyexc_value, &traceback);
        Py_XDECREF(traceback);
        goto call_python;
    }

    MateCORBA_small_demarshal_async (aqe, async_data->ret, async_data->argv, ev);
    state = pymatecorba_gil_state_ensure();
    if (pymatecorba_check_ex(ev)) {
        PyObject *traceback = NULL;
        PyErr_Fetch(&pyexc_type, &pyexc_value, &traceback);
        Py_XDECREF(traceback);
        goto call_python;
    }

    py_async_retval = pycorba_call_demarshal_retval(
        imethod, async_data->num_args, async_data->ret_tc, async_data->ret,
        async_data->argv, async_data->argvptr, async_data->n_rets);
    pycorba_call_cleanup(imethod, async_data->num_args, async_data->ret,
                         async_data->argv, async_data->argvptr,
                         async_data->retptr, async_data->ret_tc);

    if (pymatecorba_check_ex(ev)) {
        PyObject *traceback = NULL;
        PyErr_Fetch(&pyexc_type, &pyexc_value, &traceback);
        Py_XDECREF(traceback);
        goto call_python;
    }

call_python:
    if (!py_async_retval) {
        Py_INCREF(Py_None);
        py_async_retval = Py_None;
    }
    if (!pyexc_type) {
        Py_INCREF(Py_None);
        pyexc_type = Py_None;
    }
    if (!pyexc_value) {
        Py_INCREF(Py_None);
        pyexc_value = Py_None;
    }
    if (async_data->user_data)
        pytmp = PyObject_CallFunction(async_data->callback, "NNNN", py_async_retval,
                                      pyexc_type, pyexc_value, async_data->user_data);
    else
        pytmp = PyObject_CallFunction(async_data->callback, "NNN", py_async_retval,
                                      pyexc_type, pyexc_value);
    Py_DECREF(pytmp);
    Py_DECREF(async_data->callback);
    g_free(async_data);
}


static PyObject *
pycorba_bound_method_async_call(PyCORBA_BoundMethod *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "args", "callback", "user_data", NULL};
    PyObject *py_args, *py_callback, *py_user_data = NULL;
    PyMateCORBAAsyncData *async_data;
    PyObject *selfarg, *tmp;
    CORBA_Environment ev;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O|O:async", kwlist,
                                     &PyList_Type, &py_args, &py_callback, &py_user_data))
        return NULL;

    if (!PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument not callable");
        return NULL;
    }

    async_data = g_new0(PyMateCORBAAsyncData, 1);
    Py_INCREF(py_callback);
    async_data->callback = py_callback;
    Py_XINCREF(py_user_data);
    async_data->user_data = py_user_data;

    selfarg = PyList_New(1);
    Py_INCREF(self->meth_self);
    PyList_SET_ITEM(selfarg, 0, self->meth_self);
    tmp = PySequence_Concat(selfarg, py_args);
    if (!tmp) {
        PyErr_Print();
        goto error;
    }
    Py_DECREF(selfarg);
    args = PySequence_Tuple(tmp);
    Py_DECREF(tmp);

    if (!pycorba_call_marshal_args(self->meth->imethod, args, &async_data->ret_tc, &async_data->ret,
                                   &async_data->retptr, &async_data->argv, &async_data->argvptr,
                                   &async_data->num_args, &async_data->n_rets))
        goto error;

    

    CORBA_exception_init(&ev);

    MateCORBA_small_invoke_async(((PyCORBA_Object *)self->meth_self)->objref,
                             self->meth->imethod,
                             async_callback, async_data,
                             async_data->argv, CORBA_OBJECT_NIL, &ev);

    if (ev._major != CORBA_NO_EXCEPTION) {
        PyErr_SetString(PyExc_RuntimeError, "async invocation failed");
        CORBA_exception_free(&ev);
        goto error;
    }

    Py_INCREF(Py_None);
    return Py_None;
error:
    Py_DECREF(args);
    pycorba_call_cleanup(self->meth->imethod, async_data->num_args, async_data->ret,
                         async_data->argv, async_data->argvptr, async_data->retptr,
                         async_data->ret_tc);
    return NULL;
}

static PyMethodDef pycorba_bound_method_methods[] = {
    { "async", (PyCFunction)pycorba_bound_method_async_call, METH_KEYWORDS },
    { NULL, NULL, 0 }
};

static PyObject *
pycorba_bound_method_get_doc(PyCORBA_BoundMethod *self, void *closure)
{
    return pycorba_method_get_doc(self->meth, closure);
}

static PyObject *
pycorba_bound_method_get_name(PyCORBA_BoundMethod *self, void *closure)
{
    return PyString_FromString(self->meth->imethod->name);
}

static PyObject *
pycorba_bound_method_get_class(PyCORBA_BoundMethod *self, void *closure)
{
    Py_INCREF(self->meth->meth_class);
    return self->meth->meth_class;
}

static PyObject *
pycorba_bound_method_get_self(PyCORBA_BoundMethod *self, void *closure)
{
    Py_INCREF(self->meth_self);
    return self->meth_self;
}

static PyGetSetDef pycorba_bound_method_getsets[] = {
    { "__doc__",  (getter)pycorba_bound_method_get_doc,   (setter)0 },
    { "__name__", (getter)pycorba_bound_method_get_name,  (setter)0 },
    { "im_name",  (getter)pycorba_bound_method_get_name,  (setter)0 },
    { "im_class", (getter)pycorba_bound_method_get_class, (setter)0 },
    { "im_self",  (getter)pycorba_bound_method_get_self,  (setter)0 },
    { NULL,       (getter)0,                       (setter)0 }
};

PyTypeObject PyCORBA_BoundMethod_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "MateCORBA.BoundMethod",                /* tp_name */
    sizeof(PyCORBA_BoundMethod),        /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pycorba_bound_method_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pycorba_bound_method_repr, /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)pycorba_bound_method_call, /* tp_call */
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
    pycorba_bound_method_methods,       /* tp_methods */
    0,                                  /* tp_members */
    pycorba_bound_method_getsets,       /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    (descrgetfunc)0,                    /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,                        /* tp_init */
    0,                                  /* tp_alloc */
    0,                                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};


void
pymatecorba_add_imethods_to_stub(PyObject *stub, MateCORBA_IMethods *imethods)
{
    PyObject *tp_dict;
    int i;

    g_return_if_fail(PyType_Check(stub) && PyType_IsSubtype((PyTypeObject *)stub, &PyCORBA_Object_Type));

    tp_dict = ((PyTypeObject *)stub)->tp_dict;
    for (i = 0; i < imethods->_length; i++) {
	PyCORBA_Method *meth;
	gchar *pyname;

	meth = PyObject_NEW(PyCORBA_Method, &PyCORBA_Method_Type);
	if (!meth)
	    return;
	Py_INCREF(stub);
	meth->meth_class = stub;
	meth->imethod = &imethods->_buffer[i];
	pyname = _pymatecorba_escape_name(meth->imethod->name);
	PyDict_SetItemString(tp_dict, pyname, (PyObject *)meth);
	g_free(pyname);
	Py_DECREF(meth);
    }

    /* set up property descriptors for interface attributes */
    for (i = 0; i < imethods->_length; i++) {
	MateCORBA_IMethod *imethod = &imethods->_buffer[i];

	if (!strncmp(imethod->name, "_get_", 4)) {
	    PyObject *fget, *fset, *property;
	    gchar *name;

	    fget = PyDict_GetItemString(tp_dict, imethod->name);
	    name = g_strdup(imethod->name);
	    name[1] = 's';
	    fset = PyDict_GetItemString(tp_dict, name);
	    g_free(name);
	    if (!fset)
		PyErr_Clear();

	    name = g_strconcat(&imethod->name[5], ": ", imethod->ret->repo_id,
			       (fset ? "" : " (readonly)"), NULL);
	    property = PyObject_CallFunction((PyObject *)&PyProperty_Type,
					"OOOs", fget, fset ? fset : Py_None,
					Py_None, name);
	    g_free(name);

	    name = _pymatecorba_escape_name(&imethod->name[5]);
	    PyDict_SetItemString(tp_dict, name, property);
	    g_free(name);

	    Py_DECREF(property);
	    Py_DECREF(fget);
	    Py_XDECREF(fset);
	}
    }
}
