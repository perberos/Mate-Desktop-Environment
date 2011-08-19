/* -*- Mode: C; c-basic-offset: 4 -*- 
 * Copyright (C) 2001 Johan Dahlin <zilch.am@home.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Johan Dahlin
 */

#include <Python.h>
#include <sysmodule.h>

#include <signal.h>

#include <matecomponent-activation/matecomponent-activation.h>
#include <pymatecorba.h>

#ifdef WITH_PYPOPT
#include <pypopt/pypopt.h>
#endif /* WITH_PYPOPT */

#include <pygobject.h>


static PyObject *
wrap_ba_orb_get (PyObject *self, PyObject *args)
{
    CORBA_ORB orb;

    if (!PyArg_ParseTuple(args, ":matecomponent.activation.orb_get"))
	return NULL;

    orb = matecomponent_activation_orb_get();
    if (orb == NULL) {
	Py_INCREF (Py_None);
	return Py_None;
    }

    return pycorba_orb_new(orb);
}

static PyObject *
wrap_ba_get_popt_table_name (PyObject *self, PyObject *args)
{
    char *name;
    
    if (!PyArg_ParseTuple (args, ":matecomponent.activation.get_popt_table_name"))
	return NULL;
	
    name = matecomponent_activation_get_popt_table_name ();
    if (name == NULL) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    
    return PyString_FromString(name);
}

static PyObject *
wrap_ba_query (PyObject *self,
	       PyObject *args)
{
    PyObject              *pysort = NULL;
    guint                  i;
    gchar                 *query;
    guint                  len;
    gchar                **sort;
    CORBA_Environment      ev;
    MateComponent_ServerInfoList *infolist;
    CORBA_any              retany = { NULL, NULL, FALSE };
    PyObject              *pyinfolist;
    
    pysort = NULL;
    i = 0;

    if (!PyArg_ParseTuple (args, "s|O!:matecomponent.activation.query",
			   &query, &PyList_Type, &pysort))
	return NULL;

    /* Create sortlist */
    if (pysort != NULL) {
	len = PyList_Size(pysort);
	sort = g_new(gchar *, len+1);
	for (i = 0; i < len; i++) {
	    PyObject *item = PyList_GetItem(pysort, i);

	    if (!PyString_Check(item)) {
		PyErr_SetString(PyExc_TypeError, "sort list items must be strings");
		g_free(sort);
		return NULL;
	    }
	    sort[i] = PyString_AsString(item);
	}
	sort[i] = NULL;
    } else {
	sort = NULL;
    }

    CORBA_exception_init (&ev);

    infolist = matecomponent_activation_query (query, sort, &ev);
    g_free(sort);

    if (pymatecorba_check_ex(&ev))
	return NULL;

    retany._type = TC_MateComponent_ServerInfoList;
    retany._value = infolist;
    pyinfolist = pymatecorba_demarshal_any(&retany);
    CORBA_free(infolist);

    if (!pyinfolist) {
	PyErr_SetString(PyExc_ValueError, "could not demarshal query results");
	return NULL;
    }

    return pyinfolist;
}

static PyObject *
wrap_ba_activate(PyObject *self, PyObject *args)
{
    gchar              *requirements;
    PyObject           *pysort;
    glong               flags;
    gchar              *ret_aid;
    guint               len;
    char              **sort;
    guint               i;
    CORBA_Environment   ev;
    CORBA_Object        corba_object;

    pysort = NULL;
    flags = 0L;
    ret_aid = NULL;
	
    if (!PyArg_ParseTuple(args, "z|O!ls:matecomponent.activation.activate",
			  &requirements, &PyList_Type, &pysort, &flags,
			  &ret_aid))
	return NULL;
	
    if (pysort != NULL) {
	len = PyList_Size(pysort);
	sort = g_new(gchar *, len+1);
	for (i = 0; i < len; i++) {
	    PyObject *item = PyList_GetItem(pysort, i);

	    if (!PyString_Check(item)) {
		PyErr_SetString(PyExc_TypeError, "sort list items must be strings");
		g_free(sort);
		return NULL;
	    }
	    sort[i] = PyString_AsString(item);
	}
	sort[i] = NULL;
    } else {
	sort = NULL;
    }
	
    CORBA_exception_init(&ev);
    corba_object = matecomponent_activation_activate(requirements, sort, flags,
					      (MateComponent_ActivationID *)ret_aid,
					      &ev);

    g_free(sort);

    if (pymatecorba_check_ex(&ev))
	return NULL;
	
    return pycorba_object_new(corba_object);
}

static PyObject *
wrap_ba_activate_from_id(PyObject *self, PyObject *args)
{
    CORBA_Object         object = NULL;
    CORBA_Environment    ev;
    char                *activation_id;
    PyObject            *py_do_ret_aid = NULL;
    gboolean             do_ret_aid;
    MateComponent_ActivationID  ret_aid = NULL;
    long                 flags = 0;
    PyObject            *retval;
	
    if (!PyArg_ParseTuple(args, "s|lO:matecomponent.activation.activate_from_id",
			   &activation_id, &flags, &py_do_ret_aid))
	return NULL;

    do_ret_aid = !py_do_ret_aid || PyObject_IsTrue(py_do_ret_aid);

    CORBA_exception_init (&ev);

    object = matecomponent_activation_activate_from_id (activation_id,
						 flags, 
						 do_ret_aid? &ret_aid : NULL,
						 &ev);
    
    if (pymatecorba_check_ex(&ev)) {
        if (ret_aid) g_free(ret_aid);
	return NULL;
    }
    if (do_ret_aid) {
        retval = Py_BuildValue("Os", pycorba_object_new(object), ret_aid);
        g_free(ret_aid);
    } else
        retval = pycorba_object_new(object);
    return retval;
}

static PyObject *
wrap_ba_active_server_register(PyObject *self, PyObject *args)
{
    char                     *iid;
    PyCORBA_Object           *obj;
    MateComponent_RegistrationResult retval;
	        
    if (!PyArg_ParseTuple (args, "sO!:matecomponent.activation.active_server_register",
			   &iid, &PyCORBA_Object_Type, &obj))
		return NULL;
	
    retval = matecomponent_activation_active_server_register(iid, obj->objref);
	
    return PyInt_FromLong(retval);
}

typedef struct {
    PyObject *callback;
    PyObject *user_data;
} WrapBAActAsyncData;

static void
_wrap_MateComponentActivationCallback(CORBA_Object activated_object,
                               const char *error_reason,
                               gpointer user_data)
{
    PyGILState_STATE state;
    WrapBAActAsyncData *data = (WrapBAActAsyncData *) user_data;
    PyObject *py_activated_object = pycorba_object_new(activated_object);
    PyObject *retobj;
    
    state = pyg_gil_state_ensure();

    if (data->user_data)
        retobj = PyEval_CallFunction(data->callback, "(OsO)",
                                     py_activated_object,
                                     error_reason,
                                     data->user_data);
    else
        retobj = PyEval_CallFunction(data->callback, "(Os)",
                                     py_activated_object,
                                     error_reason);
    Py_DECREF(data->callback);
    Py_XDECREF(data->user_data);
    g_free(data);
    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }
    Py_DECREF(py_activated_object);
    Py_XDECREF(retobj);

    pyg_gil_state_release(state);
}

static PyObject *
wrap_ba_activate_async(PyObject *self, PyObject *args)
{
    gchar              *requirements;
    PyObject           *pysort = NULL;
    glong               flags = 0L;
    guint               len;
    char              **sort;
    guint               i;
    CORBA_Environment   ev;
    PyObject           *callback, *user_data = NULL;
    WrapBAActAsyncData *data;

    if (!PyArg_ParseTuple(args, "zO|OO!l:matecomponent.activation.activate_async",
			  &requirements, &callback, &user_data,
                          &PyList_Type, &pysort, &flags))
	return NULL;
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "second argument must be callable");
        return NULL;
    }
    if (pysort != NULL) {
	len = PyList_Size(pysort);
	sort = g_new(gchar *, len+1);
	for (i = 0; i < len; i++) {
	    PyObject *item = PyList_GetItem(pysort, i);

	    if (!PyString_Check(item)) {
		PyErr_SetString(PyExc_TypeError, "sort list items must be strings");
		g_free(sort);
		return NULL;
	    }
	    sort[i] = PyString_AsString(item);
	}
	sort[i] = NULL;
    } else {
	sort = NULL;
    }

    data = g_new0(WrapBAActAsyncData, 1);
    data->callback = callback;
    Py_INCREF(callback);
    data->user_data = user_data;
    Py_XINCREF(user_data);

    CORBA_exception_init(&ev);
    matecomponent_activation_activate_async(requirements, sort, flags,
                                     _wrap_MateComponentActivationCallback, data,
                                     &ev);

    g_free(sort);
    
    if (pymatecorba_check_ex(&ev))
	return NULL;
    
    Py_INCREF(Py_None);
    return Py_None;
}



static PyMethodDef activation_functions[] =
{
    { "orb_get",                  wrap_ba_orb_get,                   METH_VARARGS },
    { "get_popt_table_name",      wrap_ba_get_popt_table_name,       METH_VARARGS },
    { "query",                    wrap_ba_query,                     METH_VARARGS },
    { "activate",                 wrap_ba_activate,                  METH_VARARGS },
    { "activate_from_id",         wrap_ba_activate_from_id,          METH_VARARGS },
    { "active_server_register",   wrap_ba_active_server_register,    METH_VARARGS },
    { "activate_async",           wrap_ba_activate_async,            METH_VARARGS },
#if 0
    { "active_server_unregister", wrap_oaf_active_server_unregister, METH_VARARGS },
    { "name_service_get",         wrap_oaf_name_service_get,         METH_VARARGS },
    { "activation_context_get",   wrap_oaf_activation_context_get,   METH_VARARGS },
    { "hostname_get",             wrap_oaf_hostname_get,             METH_VARARGS },
    { "session_name_get",         wrap_oaf_session_name_get,         METH_VARARGS },
    { "domain_get",               wrap_oaf_domain_get,               METH_VARARGS },
#endif	
	{ NULL, NULL }
};

void
initactivation (void)
{
    PyObject  *av;
    int        argc;
    int        i;
    char     **argv;
    struct sigaction sa;

    PyObject   *mod;

	
#ifdef WITH_PYPOPT
    init_pypopt ();
#endif /* WITH_PYPOPT */
	
    init_pymatecorba ();
    init_pygobject();
		
    mod = Py_InitModule("matecomponent.activation", activation_functions);

    av = PySys_GetObject("argv");
    if (av != NULL) {
	argc = PyList_Size(av);

	argv = g_new(gchar *, argc);
       for (i = 0; i < argc; i++) {
           argv[i] = g_strdup(PyString_AsString(PyList_GetItem(av, i)));
       }
    } else {
       argc = 0;
       argv = NULL;
    }
	
    memset(&sa, 0, sizeof(sa));
    sigaction(SIGCHLD, NULL, &sa);

    if (!matecomponent_activation_is_initialized ())
	matecomponent_activation_init(argc, argv);

    sigaction(SIGCHLD, &sa, NULL);
	
    if (argv != NULL) {
	PySys_SetArgv(argc, argv);
	for (i = 0; i < argc; i++)
	    g_free(argv[i]);
	g_free(argv);
    }
	
#ifdef WITH_PYPOPT
    PyModule_AddObject(mod, "popt_options",
		       PyPoptOption_New(matecomponent_activation_popt_options));
#endif /* WITH_PYPOPT */
}
