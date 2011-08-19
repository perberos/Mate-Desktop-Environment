/* -*- mode: C; c-basic-offset: 4 -*- */
#include <pygobject.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-address.h>
#include <libmatevfs/mate-vfs-resolve.h>
#include <libmatevfs/mate-vfs-dns-sd.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <pymatecorba.h>
#define NO_IMPORT_PYMATEVFSMATECOMPONENT
#include "pymatevfsmatecomponent.h"

#include <matecomponent-activation/MateComponent_Activation_types.h>


static PyObject *
pygvfs_mime_component_action_new(MateVFSMimeAction *action)
{
    CORBA_any any;
    PyObject *component;

    g_return_val_if_fail(action->action_type == MATE_VFS_MIME_ACTION_TYPE_COMPONENT, NULL);

    any._type = TC_MateComponent_ServerInfo;
    any._value = action->action.application;
    component = pymatecorba_demarshal_any(&any);
    if (!component) {
        PyErr_SetString(PyExc_TypeError, "unable to convert MateComponent_ServerInfo of component");
        return NULL;
    }
    return Py_BuildValue("(iN)", action->action_type, component);
}

static PyObject *
pygvfs_mime_get_default_component(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    MateComponent_ServerInfo *component;
    PyObject *py_component;
    CORBA_any any;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:matevfs.mime_get_default_component",
				     kwlist, &mime_type))
	return NULL;
    if (!(component = mate_vfs_mime_get_default_component(mime_type))) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    any._type = TC_MateComponent_ServerInfo;
    any._value = component;
    py_component = pymatecorba_demarshal_any(&any);
    if (!component) {
	PyErr_SetString(PyExc_TypeError, "unable to convert MateComponent_ServerInfo of component");
	return NULL;
    }
    CORBA_free(component);
    return py_component;
}

static PyObject *
pygvfs_mime_components_list_new(GList *list)
{
    PyObject *retval;
    MateComponent_ServerInfo *component;
    PyObject *py_component;
    CORBA_any any;
    guint i, len = g_list_length(list);
    
    retval = PyList_New(len);
    for (i = 0; list; ++i, list = list->next) {
	g_assert(i < len);
	component = (MateComponent_ServerInfo *) list->data;
	any._value = component;
	py_component = pymatecorba_demarshal_any(&any);
	if (!component) {
	    PyErr_SetString(PyExc_TypeError, "unable to convert MateComponent_ServerInfo of component");
	    Py_DECREF(retval);
	    return NULL;
	}
	PyList_SET_ITEM(retval, i, py_component);
    }
    return retval;
}

static PyObject *
pygvfs_mime_get_short_list_components(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    GList *list;
    PyObject *py_list;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:matevfs.mime_get_short_list_components",
				     kwlist, &mime_type))
	return NULL;
    list = mate_vfs_mime_get_short_list_components(mime_type);
    py_list = pygvfs_mime_components_list_new(list);
    mate_vfs_mime_component_list_free(list);
    return py_list;
}

static PyObject *
pygvfs_mime_get_all_components(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    GList *list;
    PyObject *py_list;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:matevfs.mime_get_all_components",
				     kwlist, &mime_type))
	return NULL;
    list = mate_vfs_mime_get_all_components(mime_type);
    py_list = pygvfs_mime_components_list_new(list);
    mate_vfs_mime_component_list_free(list);
    return py_list;
}


static PyMethodDef pymatevfs_matecomponent_functions[] = {
    { "mime_get_default_component", (PyCFunction)pygvfs_mime_get_default_component,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_get_short_list_components", (PyCFunction)pygvfs_mime_get_short_list_components,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_get_all_components", (PyCFunction)pygvfs_mime_get_all_components,
      METH_VARARGS|METH_KEYWORDS },

    { NULL, NULL, 0 }
};


struct _PyMateVFSMateComponent_Functions pymatevfs_matecomponent_api_functions = {
    pygvfs_mime_component_action_new,

};


DL_EXPORT(void)
initmatevfsmatecomponent(void)
{
    PyObject *m, *d, *o;

    init_pygobject();
    init_pymatecorba();
    if (!mate_vfs_init()) {
	PyErr_SetString(PyExc_RuntimeError, "could not initialise matevfs");
	return;
    }

    m = Py_InitModule("matevfs.matevfsmatecomponent", pymatevfs_matecomponent_functions);
    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "_PyMateVFSMateComponent_API",
			 o=PyCObject_FromVoidPtr(&pymatevfs_matecomponent_api_functions,NULL));
    Py_DECREF(o);
}

