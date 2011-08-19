/* -*- mode: C; c-basic-offset: 4 -*- */

#include "pymatevfs-private.h"
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>


PyObject *
pymate_vfs_handle_new(MateVFSHandle *fd)
{
    PyMateVFSHandle *self;

    self = PyObject_NEW(PyMateVFSHandle, &PyMateVFSHandle_Type);
    if (self == NULL) return NULL;

    self->fd = fd;

    return (PyObject *)self;
}

static void
pygvhandle_dealloc(PyMateVFSHandle *self)
{
    if (self->fd) {
	MateVFSResult result;

        pyg_begin_allow_threads;
        result = mate_vfs_close(self->fd);
        pyg_end_allow_threads;

	if (pymate_vfs_result_check(result)) {
	    PyErr_Print();
	    PyErr_Clear();
	}
    }
    PyObject_FREE(self);
}

static int
pygvhandle_init(PyMateVFSHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "open_mode", NULL };
    PyObject *uri;
    MateVFSOpenMode open_mode = MATE_VFS_OPEN_READ;
    MateVFSHandle *handle = NULL;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O|i:matevfs.Handle.__init__",
				     kwlist, &uri, &open_mode))
	return -1;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_open_uri(&handle,
				    pymate_vfs_uri_get(uri),
				    open_mode);
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_open(&handle,
				PyString_AsString(uri),
				open_mode);
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return -1;
    }

    if (pymate_vfs_result_check(result))
	return -1;

    self->fd = handle;

    return 0;
}

static PyObject *
pygvhandle_close(PyMateVFSHandle *self)
{
    if (self->fd) {
	MateVFSResult result = mate_vfs_close(self->fd);

	if (pymate_vfs_result_check(result)) {
	    PyErr_Print();
	    PyErr_Clear();
	}
    }
    self->fd = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvhandle_read(PyMateVFSHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "bytes", NULL };
    glong bytes;
    gchar *buffer;
    MateVFSFileSize bytes_read = 0;
    MateVFSResult result;
    PyObject *pybuffer;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "l:matevfs.Handle.read",
				     kwlist, &bytes))
	return NULL;

    if (bytes < 0) {
	PyErr_SetString(PyExc_ValueError, "bytes must be >= 0");
	return NULL;
    }
    else if(bytes == 0)
	return PyString_FromString("");

    buffer = g_malloc(bytes);
    pyg_begin_allow_threads;
    result = mate_vfs_read(self->fd, buffer, bytes, &bytes_read);
    pyg_end_allow_threads;
    if (pymate_vfs_result_check(result)) {
	g_free(buffer);
	return NULL;
    }
    pybuffer = PyString_FromStringAndSize(buffer, bytes_read);
    g_free(buffer);
    return pybuffer;
}

static PyObject *
pygvhandle_write(PyMateVFSHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "buffer", NULL };
    gchar *buffer;
    Py_ssize_t bytes;
    MateVFSFileSize bytes_written = 0;
    MateVFSResult result;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#:matevfs.Handle.write",
				     kwlist, &buffer, &bytes))
	return NULL;

    pyg_begin_allow_threads;
    result = mate_vfs_write(self->fd, buffer, bytes, &bytes_written);
    pyg_end_allow_threads;
    if (pymate_vfs_result_check(result)) {
	g_free(buffer);
	return NULL;
    }
    return PyInt_FromLong(bytes_written);
}

static PyObject *
pygvhandle_seek(PyMateVFSHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "offset", "whence", NULL };
    PyObject *py_offset;
    MateVFSFileOffset offset;
    MateVFSSeekPosition whence = MATE_VFS_SEEK_START;
    MateVFSResult result;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:matevfs.Handle.seek",
				     kwlist, &py_offset, &whence))
	return NULL;

    offset = PyLong_Check(py_offset) ? PyLong_AsLongLong(py_offset)
	: PyInt_AsLong(py_offset);
    if (PyErr_Occurred()) return NULL;

    result = mate_vfs_seek(self->fd, whence, offset);

    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvhandle_tell(PyMateVFSHandle *self)
{
    MateVFSFileSize offset;
    MateVFSResult result;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }

    result = mate_vfs_tell(self->fd, &offset);

    if (pymate_vfs_result_check(result))
	return NULL;
    return PyLong_FromUnsignedLongLong(offset);
}

static PyObject *
pygvhandle_get_file_info(PyMateVFSHandle *self, PyObject *args,
			 PyObject *kwargs)
{
    static char *kwlist[] = { "options", NULL };
    MateVFSFileInfo *finfo;
    MateVFSFileInfoOptions options = MATE_VFS_FILE_INFO_DEFAULT;
    MateVFSResult result;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "|i:matevfs.Handle.get_file_info",
				     kwlist, &options))
	return NULL;

    finfo = mate_vfs_file_info_new();
    result = mate_vfs_get_file_info_from_handle(self->fd, finfo, options);
    if (pymate_vfs_result_check(result)) {
	mate_vfs_file_info_unref(finfo);
	return NULL;
    }
    return pymate_vfs_file_info_new(finfo);
}

static PyObject *
pygvhandle_truncate(PyMateVFSHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "length", NULL };
    PyObject *py_length;
    MateVFSFileSize length;
    MateVFSResult result;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O:matevfs.Handle.truncate", kwlist,
				     &py_length))
	return NULL;

    length = PyLong_Check(py_length) ? PyLong_AsUnsignedLongLong(py_length)
	: PyInt_AsLong(py_length);
    if (PyErr_Occurred()) return NULL;

    result = mate_vfs_truncate_handle(self->fd, length);

    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvhandle_file_control(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "operation", "operation_data", NULL };
    char *operation;
    PyObject *operation_data_obj, *py_retval;
    PyGVFSOperationData operation_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sOO|O:matevfs.Handle.control",
				     kwlist,
                                     &operation, &operation_data_obj)) {
	return NULL;
    }

    operation_data.magic = PYGVFS_CONTROL_MAGIC_IN;
    Py_INCREF(operation_data_obj);
    operation_data.data = operation_data_obj;

    mate_vfs_file_control(((PyMateVFSHandle*) self)->fd,
                           operation, &operation_data);

    if (operation_data.magic == PYGVFS_CONTROL_MAGIC_OUT) {
        py_retval = operation_data.data;
    } else {
        PyErr_SetString(PyExc_TypeError, "matevfs.Handle.control() can only be used"
                        " on vfs methods implemented in python");
        Py_DECREF(operation_data_obj);
        return NULL;
    }
    operation_data.magic = 0;
    return py_retval;
}

static PyMethodDef pygvhandle_methods[] = {
    { "close", (PyCFunction)pygvhandle_close, METH_NOARGS },
    { "read", (PyCFunction)pygvhandle_read, METH_VARARGS|METH_KEYWORDS },
    { "write", (PyCFunction)pygvhandle_write, METH_VARARGS|METH_KEYWORDS },
    { "seek", (PyCFunction)pygvhandle_seek, METH_VARARGS|METH_KEYWORDS },
    { "tell", (PyCFunction)pygvhandle_tell, METH_NOARGS },
    { "get_file_info", (PyCFunction)pygvhandle_get_file_info,
      METH_VARARGS|METH_KEYWORDS },
    { "truncate", (PyCFunction)pygvhandle_truncate,
      METH_VARARGS|METH_KEYWORDS },
    { "control", (PyCFunction)pygvhandle_file_control, METH_VARARGS|METH_KEYWORDS }, 
    { NULL, NULL, 0 }
};


PyTypeObject PyMateVFSHandle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.Handle",                 /* tp_name */
    sizeof(PyMateVFSHandle),           /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pygvhandle_dealloc,     /* tp_dealloc */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pygvhandle_methods,                 /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pygvhandle_init,          /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};
