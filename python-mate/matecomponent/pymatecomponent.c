/* -*- Mode: C; c-basic-offset: 4 -*- */
#include "pymatecomponent.h"
#define NO_IMPORT_PYGOBJECT
#define NO_IMPORT_PYMATECORBA
#include <pygobject.h>
#include <pymatecorba.h>
#include <libmatecomponent.h>

static void
pymatecomponent_closure_invalidate(gpointer data, GClosure *closure)
{
    PyGClosure *pc = (PyGClosure *)closure;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    Py_XDECREF(pc->callback);
    Py_XDECREF(pc->extra_args);
    Py_XDECREF(pc->swap_data);
    pyg_gil_state_release(state);

    pc->callback = NULL;
    pc->extra_args = NULL;
    pc->swap_data = NULL;
}

static void
pymatecomponent_closure_marshal(GClosure *closure,
                         GValue *return_value,
                         guint n_param_values,
                         const GValue *param_values,
                         gpointer invocation_hint,
                         gpointer marshal_data)
{
    PyGILState_STATE state;
    PyGClosure *pc = (PyGClosure *)closure;
    PyObject *params, *ret;
    guint i;
    const CORBA_Environment *ev;

    if (MATECOMPONENT_VALUE_HOLDS_CORBA_EXCEPTION(param_values + n_param_values - 1)) {
        ev = matecomponent_value_get_corba_exception(param_values + n_param_values - 1);
        n_param_values--; /* we don't pass the corba exception in python code */
    } else {
        g_warning("Used pymatecomponent_closure_new where pyg_closure_new"
                  " should have been used instead.");
        ev = NULL;
    }

    state = pyg_gil_state_ensure();

      /* construct Python tuple for the parameter values */
    params = PyTuple_New(n_param_values);
    for (i = 0; i < n_param_values; i++) {
          /* swap in a different initial data for connect_object() */
	if (i == 0 && G_CCLOSURE_SWAP_DATA(closure)) {
	    g_return_if_fail(pc->swap_data != NULL);
	    Py_INCREF(pc->swap_data);
	    PyTuple_SetItem(params, 0, pc->swap_data);
	} else {
	    PyObject *item = pyg_value_as_pyobject(&param_values[i], FALSE);

              /* error condition */
	    if (!item) {
		goto out;
	    }
	    PyTuple_SetItem(params, i, item);
	}
    }
      /* params passed to function may have extra arguments */
    if (pc->extra_args) {
	PyObject *tuple = params;
	params = PySequence_Concat(tuple, pc->extra_args);
	Py_DECREF(tuple);
    }
    ret = PyObject_CallObject(pc->callback, params);
    if (ev) {
        if (pymatecorba_check_python_ex((CORBA_Environment *) ev)) {
            Py_XDECREF(ret);
            ret = NULL;
            goto out;
        }
    } else {
        if (ret == NULL) {
            PyErr_Print();
            goto out;
        }
    }

    if (ret == NULL) {
	PyErr_Print();
	goto out;
    }
    if (return_value)
	pyg_value_from_pyobject(return_value, ret);
    Py_DECREF(ret);
    
out:
    Py_DECREF(params);
    
    pyg_gil_state_release(state);
}

/**
 * pymatecomponent_closure_new:
 * callback: a Python callable object
 * extra_args: a tuple of extra arguments, or None/NULL.
 * swap_data: an alternative python object to pass first.
 *
 * Creates a GClosure wrapping a Python callable and optionally a set
 * of additional function arguments.  This should only be used for
 * some matecomponent code that passes a CORBA_Environment as last value.  It
 * automatically maps python exceptions into the CORBA exception,
 * which is not even passed into the python callback.
 *
 * Returns: the new closure.
 */
GClosure *
pymatecomponent_closure_new(PyObject *callback, PyObject *extra_args, PyObject *swap_data)
{
    GClosure *closure;

    g_return_val_if_fail(callback != NULL, NULL);
    closure = g_closure_new_simple(sizeof(PyGClosure), NULL);
    g_closure_add_invalidate_notifier(closure, NULL, pymatecomponent_closure_invalidate);
    g_closure_set_marshal(closure, pymatecomponent_closure_marshal);
    Py_INCREF(callback);
    ((PyGClosure *)closure)->callback = callback;
    if (extra_args && extra_args != Py_None) {
	Py_INCREF(extra_args);
	if (!PyTuple_Check(extra_args)) {
	    PyObject *tmp = PyTuple_New(1);
	    PyTuple_SetItem(tmp, 0, extra_args);
	    extra_args = tmp;
	}
	((PyGClosure *)closure)->extra_args = extra_args;
    }
    if (swap_data) {
	Py_INCREF(swap_data);
	((PyGClosure *)closure)->swap_data = swap_data;
	closure->derivative_flag = TRUE;
    }
    return closure;
}

