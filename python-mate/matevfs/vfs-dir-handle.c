/* -*- mode: C; c-basic-offset: 4 -*- */

#include "pymatevfs-private.h"

PyObject *
pymate_vfs_directory_handle_new(MateVFSDirectoryHandle *dir)
{
    PyMateVFSDirectoryHandle *self;

    self = PyObject_NEW(PyMateVFSDirectoryHandle, &PyMateVFSDirectoryHandle_Type);
    if (self == NULL) return NULL;

    self->dir = dir;

    return (PyObject *)self;
}

static void
pygvdir_dealloc(PyMateVFSDirectoryHandle *self)
{
    /* XXXX: unblock threads here */

    if (self->dir)
	mate_vfs_directory_close(self->dir);
    PyObject_FREE(self);
}

static PyObject *
pygvdir_iter(PyMateVFSDirectoryHandle *self)
{
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
pygvdir_iternext(PyMateVFSDirectoryHandle *self)
{
    MateVFSFileInfo *finfo;
    MateVFSResult result;

    /* XXXX: unblock threads here */

    finfo = mate_vfs_file_info_new();
    result = mate_vfs_directory_read_next(self->dir, finfo);

    if (result == MATE_VFS_ERROR_EOF) {
	PyErr_SetNone(PyExc_StopIteration);
	mate_vfs_file_info_unref(finfo);
	return NULL;
    }
    if (pymate_vfs_result_check(result)) {
	mate_vfs_file_info_unref(finfo);
	return NULL;
    }
    return pymate_vfs_file_info_new(finfo);
}

static int
pygvdir_init(PyMateVFSDirectoryHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "options", NULL };
    PyObject *uri;
    MateVFSDirectoryVisitOptions options = MATE_VFS_DIRECTORY_VISIT_DEFAULT;
    MateVFSDirectoryHandle *handle = NULL;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O|i:matevfs.DirectoryHandle.__init__",
				     kwlist, &uri, &options))
	return -1;

    /* XXXX: unblock threads here */

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type))
	result = mate_vfs_directory_open_from_uri(&handle,
						   pymate_vfs_uri_get(uri),
						   options);
    else if (PyString_Check(uri))
	result = mate_vfs_directory_open(&handle,
					  PyString_AsString(uri),
					  options);
    else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return -1;
    }

    if (pymate_vfs_result_check(result))
	return -1;

    self->dir = handle;

    return 0;
}


static PyMethodDef pygvdir_methods[] = {
    { NULL, NULL, 0 }
};


PyTypeObject PyMateVFSDirectoryHandle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.DirectoryHandle",        /* tp_name */
    sizeof(PyMateVFSDirectoryHandle),  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pygvdir_dealloc,        /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
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
    Py_TPFLAGS_DEFAULT,   		/* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)pygvdir_iter,          /* tp_iter */
    (iternextfunc)pygvdir_iternext,     /* tp_iternext */
    pygvdir_methods,                    /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pygvdir_init,             /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};
