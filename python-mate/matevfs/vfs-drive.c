/* -*- mode: C; c-basic-offset: 4 -*- */

#include "pymatevfs-private.h"
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>


static PyObject *
pygvdrive_get_id(PyGObject *self)
{
    return PyLong_FromUnsignedLong(mate_vfs_drive_get_id(MATE_VFS_DRIVE(self->obj)));
}

static PyObject *
pygvdrive_get_device_type(PyGObject *self)
{
    return PyInt_FromLong(mate_vfs_drive_get_device_type(MATE_VFS_DRIVE(self->obj)));
}

static PyObject *
pygvdrive_get_mounted_volumes(PyGObject *self)
{
    GList *volumes, *l;
    PyObject *pyvolumes = PyList_New(0);
    
    for (volumes = l = mate_vfs_drive_get_mounted_volumes(MATE_VFS_DRIVE(self->obj));
         l; l = l->next)
    {
        MateVFSVolume *volume = MATE_VFS_VOLUME(l->data);
        PyObject *pyvol = pygobject_new((GObject *)volume);
        PyList_Append(pyvolumes, pyvol);
        Py_DECREF(pyvol);
    }
    mate_vfs_drive_volume_list_free(volumes);
    return pyvolumes;
}

static PyObject *
pygvdrive_get_device_path(PyGObject *self)
{
    char *retval;

    retval = mate_vfs_drive_get_device_path(MATE_VFS_DRIVE(self->obj));
    if (retval)
        return PyString_FromString(retval);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *
pygvdrive_get_activation_uri(PyGObject *self)
{
    char *retval;

    retval = mate_vfs_drive_get_activation_uri(MATE_VFS_DRIVE(self->obj));
    if (retval)
        return PyString_FromString(retval);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}


static PyObject *
pygvdrive_get_display_name(PyGObject *self)
{
    char *retval;

    retval = mate_vfs_drive_get_display_name(MATE_VFS_DRIVE(self->obj));
    if (retval)
        return PyString_FromString(retval);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *
pygvdrive_get_icon(PyGObject *self)
{
    char *retval;

    retval = mate_vfs_drive_get_icon(MATE_VFS_DRIVE(self->obj));
    if (retval)
        return PyString_FromString(retval);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *
pygvdrive_get_hal_udi(PyGObject *self)
{
    char *retval;

    retval = mate_vfs_drive_get_hal_udi(MATE_VFS_DRIVE(self->obj));
    if (retval)
        return PyString_FromString(retval);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *
pygvdrive_is_user_visible(PyGObject *self)
{
    gboolean retval;

    retval = mate_vfs_drive_is_user_visible(MATE_VFS_DRIVE(self->obj));
    if (retval) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyObject *
pygvdrive_is_connected(PyGObject *self)
{
    gboolean retval;

    retval = mate_vfs_drive_is_connected(MATE_VFS_DRIVE(self->obj));
    if (retval) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyObject *
pygvdrive_is_mounted(PyGObject *self)
{
    gboolean retval;

    retval = mate_vfs_drive_is_mounted(MATE_VFS_DRIVE(self->obj));
    if (retval) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

void
wrap_matevfs_volume_op_callback(gboolean succeeded,
                                 char *error,
                                 char *detailed_error,
                                 gpointer data)
{
    PyMateVFSVolumeOpCallback *context;
    PyObject *retval;
    PyGILState_STATE gil;

    context = (PyMateVFSVolumeOpCallback *) data;
    gil = pyg_gil_state_ensure();
    
    if (context->user_data)
	retval = PyEval_CallFunction(context->callback, "(ssO)",
                                     error, detailed_error,
                                     context->user_data);
    else
	retval = PyEval_CallFunction(context->callback, "(ss)",
                                     error, detailed_error);
    if (!retval) {
	PyErr_Print();
	PyErr_Clear();
    }
    Py_XDECREF(retval);

    Py_DECREF(context->callback);
    Py_XDECREF(context->user_data);
    g_free(data);

    pyg_gil_state_release(gil);
}


static PyObject *
pygvdrive_mount(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "callback", "user_data", NULL };
    PyObject *py_callback, *py_user_data = NULL;
    PyMateVFSVolumeOpCallback *callback_context;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|O:matevfs.Drive.mount",
                                     kwlist, &py_callback, py_user_data))
        return NULL;

    if (!PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    callback_context = g_new(PyMateVFSVolumeOpCallback, 1);
    callback_context->callback = py_callback;
    Py_INCREF(py_callback);
    callback_context->user_data = py_user_data;
    Py_XINCREF(py_user_data);

    mate_vfs_drive_mount(MATE_VFS_DRIVE(self->obj),
                          wrap_matevfs_volume_op_callback,
                          callback_context);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
pygvdrive_unmount(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "callback", "user_data", NULL };
    PyObject *py_callback, *py_user_data = NULL;
    PyMateVFSVolumeOpCallback *callback_context;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|O:matevfs.Drive.unmount",
                                     kwlist, &py_callback, py_user_data))
        return NULL;

    if (!PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    callback_context = g_new(PyMateVFSVolumeOpCallback, 1);
    callback_context->callback = py_callback;
    Py_INCREF(py_callback);
    callback_context->user_data = py_user_data;
    Py_XINCREF(py_user_data);

    mate_vfs_drive_unmount(MATE_VFS_DRIVE(self->obj),
                            wrap_matevfs_volume_op_callback,
                            callback_context);
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
pygvdrive_eject(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "callback", "user_data", NULL };
    PyObject *py_callback, *py_user_data = NULL;
    PyMateVFSVolumeOpCallback *callback_context;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|O:matevfs.Drive.eject",
                                     kwlist, &py_callback, py_user_data))
        return NULL;

    if (!PyCallable_Check(py_callback)) {
        PyErr_SetString(PyExc_TypeError, "first argument must be callable");
        return NULL;
    }

    callback_context = g_new(PyMateVFSVolumeOpCallback, 1);
    callback_context->callback = py_callback;
    Py_INCREF(py_callback);
    callback_context->user_data = py_user_data;
    Py_XINCREF(py_user_data);

    mate_vfs_drive_eject(MATE_VFS_DRIVE(self->obj),
                          wrap_matevfs_volume_op_callback,
                          callback_context);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvdrive_needs_eject(PyGObject *self)
{
    gboolean retval;

    retval = mate_vfs_drive_needs_eject(MATE_VFS_DRIVE(self->obj));
    if (retval) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef pygvdrive_methods[] = {
    { "get_id", (PyCFunction)pygvdrive_get_id, METH_NOARGS },
    { "get_device_type", (PyCFunction)pygvdrive_get_device_type, METH_NOARGS },
    { "get_mounted_volumes", (PyCFunction)pygvdrive_get_mounted_volumes, METH_NOARGS },
    { "get_device_path", (PyCFunction)pygvdrive_get_device_path, METH_NOARGS },
    { "get_activation_uri", (PyCFunction)pygvdrive_get_activation_uri, METH_NOARGS },
    { "get_display_name", (PyCFunction)pygvdrive_get_display_name, METH_NOARGS },
    { "get_icon", (PyCFunction)pygvdrive_get_icon, METH_NOARGS },
    { "get_hal_udi", (PyCFunction)pygvdrive_get_hal_udi, METH_NOARGS },
    { "is_user_visible", (PyCFunction)pygvdrive_is_user_visible, METH_NOARGS },
    { "is_connected", (PyCFunction)pygvdrive_is_connected, METH_NOARGS },
    { "is_mounted", (PyCFunction)pygvdrive_is_mounted, METH_NOARGS },
    { "mount", (PyCFunction)pygvdrive_mount, METH_VARARGS|METH_KEYWORDS },
    { "unmount", (PyCFunction)pygvdrive_unmount, METH_VARARGS|METH_KEYWORDS },
    { "eject", (PyCFunction)pygvdrive_eject, METH_VARARGS|METH_KEYWORDS },
    { "needs_eject", (PyCFunction)pygvdrive_needs_eject, METH_NOARGS },
    { NULL, NULL, 0 }
};


static int
pygvdrive_compare(PyGObject *self, PyGObject *other)
{
    return mate_vfs_drive_compare(MATE_VFS_DRIVE(self->obj),
                                   MATE_VFS_DRIVE(other->obj));
}


PyTypeObject PyMateVFSDrive_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.Drive",                   /* tp_name */
    sizeof(PyGObject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)0,     			/* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)pygvdrive_compare,         /* tp_compare */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 	/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pygvdrive_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)0,          		/* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};


