/* -*- mode: C; c-basic-offset: 4 -*- */

#include "pymatevfs-private.h"
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>



static int
pygvvolume_monitor_init(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     ":matevfs.VolumeMonitor.__init__",
                                     kwlist))
        return -1;
    self->obj = (GObject *) mate_vfs_get_volume_monitor();
    g_object_ref(self->obj);
    return 0;
}

static PyObject *
pygvvolume_monitor_get_mounted_volumes(PyGObject *self)
{
    GList *volumes, *l;
    PyObject *pyvolumes = PyList_New(0);
    
    for (volumes = l = mate_vfs_volume_monitor_get_mounted_volumes(MATE_VFS_VOLUME_MONITOR(self->obj));
         l; l = l->next)
    {
        MateVFSVolume *volume = MATE_VFS_VOLUME(l->data);
        PyObject *pyvol = pygobject_new((GObject *) volume);
        PyList_Append(pyvolumes, pyvol);
        Py_DECREF(pyvol);
    }
    mate_vfs_drive_volume_list_free(volumes);
    return pyvolumes;
}

static PyObject *
pygvvolume_monitor_get_connected_drives(PyGObject *self)
{
    GList *drives, *l;
    PyObject *pydrives = PyList_New(0);
    
    for (drives = l = mate_vfs_volume_monitor_get_connected_drives(MATE_VFS_VOLUME_MONITOR(self->obj));
         l; l = l->next)
    {
        MateVFSDrive *drive = MATE_VFS_DRIVE(l->data);
        PyObject *pydrive = pygobject_new((GObject *) drive);
        g_object_unref(drive);
        PyList_Append(pydrives, pydrive);
        Py_DECREF(pydrive);
    }
    g_list_free(drives);
    return pydrives;
}

static PyObject *
pygvvolume_monitor_get_volume_for_path(PyGObject *self,
                                       PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "path", NULL };
    char *path;
    MateVFSVolume *vol;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "s:matevfs.VolumeMonitor.get_volume_for_path",
                                     kwlist, &path))
        return NULL;
    vol = mate_vfs_volume_monitor_get_volume_for_path(MATE_VFS_VOLUME_MONITOR(self->obj), path);
    if (vol) {
        retval = pygobject_new((GObject *) vol);
        g_object_unref(vol);
        return retval;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvvolume_monitor_get_volume_by_id(PyGObject *self,
                                       PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "id", NULL };
    gulong id;
    MateVFSVolume *vol;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "k:matevfs.VolumeMonitor.get_volume_by_id",
                                     kwlist, &id))
        return NULL;
    vol = mate_vfs_volume_monitor_get_volume_by_id(MATE_VFS_VOLUME_MONITOR(self->obj), id);
    if (vol) {
        retval = pygobject_new((GObject *) vol);
        g_object_unref(vol);
        return retval;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvvolume_monitor_get_drive_by_id(PyGObject *self,
                                   PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "id", NULL };
    gulong id;
    MateVFSDrive *drive;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "k:matevfs.VolumeMonitor.get_drive_by_id",
                                     kwlist, &id))
        return NULL;
    drive = mate_vfs_volume_monitor_get_drive_by_id(MATE_VFS_VOLUME_MONITOR(self->obj), id);
    if (drive) {
        retval = pygobject_new((GObject *) drive);
        g_object_unref(drive);
        return retval;
    }
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef pygvvolume_monitor_methods[] = {
    { "get_mounted_volumes", (PyCFunction)pygvvolume_monitor_get_mounted_volumes, METH_NOARGS },
    { "get_connected_drives", (PyCFunction)pygvvolume_monitor_get_connected_drives, METH_NOARGS },
    { "get_volume_for_path", (PyCFunction)pygvvolume_monitor_get_volume_for_path,
      METH_VARARGS|METH_KEYWORDS },
    { "get_volume_by_id", (PyCFunction)pygvvolume_monitor_get_volume_by_id,
      METH_VARARGS|METH_KEYWORDS },
    { "get_drive_by_id", (PyCFunction)pygvvolume_monitor_get_drive_by_id,
      METH_VARARGS|METH_KEYWORDS },
    { NULL, NULL, 0 }
};



PyTypeObject PyMateVFSVolumeMonitor_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.VolumeMonitor",           /* tp_name */
    sizeof(PyGObject),           	/* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)0,     			/* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,        			/* tp_compare */
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
    pygvvolume_monitor_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pygvvolume_monitor_init,	/* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};


