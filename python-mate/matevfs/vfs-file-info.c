/* -*- mode: C; c-basic-offset: 4 -*- */

#include "pymatevfs-private.h"

#define PYMATE_VFS_ACCESS_BITS (MATE_VFS_PERM_ACCESS_READABLE         \
                                 |MATE_VFS_PERM_ACCESS_WRITABLE        \
                                 |MATE_VFS_PERM_ACCESS_EXECUTABLE)

PyObject *
pymate_vfs_file_info_new(MateVFSFileInfo *finfo)
{
    PyMateVFSFileInfo *self;

    self = PyObject_NEW(PyMateVFSFileInfo, &PyMateVFSFileInfo_Type);
    if (self == NULL) return NULL;

    self->finfo = finfo;

    return (PyObject *)self;
}

static void
pygvfinfo_dealloc(PyMateVFSFileInfo *self)
{
    if (self->finfo)
	mate_vfs_file_info_unref(self->finfo);
    PyObject_FREE(self);
}

static PyObject *
pygvfinfo_repr(PyMateVFSFileInfo *self)
{
    return PyString_FromFormat("<matevfs.FileInfo '%s'>",
			       self->finfo->name? self->finfo->name: "(null)");
}

static PyObject *
pygvfinfo_getattr(PyMateVFSFileInfo *self, const gchar *attr)
{
    MateVFSFileInfo *finfo;

    finfo = self->finfo;
    if (!strcmp(attr, "__members__")) {
	return Py_BuildValue("[ssssssssssssssssss]", "atime", "block_count",
			     "ctime", "device", "flags", "gid", "inode",
			     "io_block_size", "link_count", "mime_type",
			     "mtime", "name", "permissions", "access", "size",
			     "symlink_name", "type", "uid", "valid_fields");
    } else if (!strcmp(attr, "name")) {
	if (finfo->name)
	    return PyString_FromString(finfo->name);
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "valid_fields")) {
	return PyInt_FromLong(finfo->valid_fields);
    } else if (!strcmp(attr, "type")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE)) {
	    PyErr_SetString(PyExc_ValueError, "type field has no valid value");
	    return NULL;
	}
	return PyInt_FromLong(finfo->type);
    } else if (!strcmp(attr, "permissions")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS)) {
	    PyErr_SetString(PyExc_ValueError,
			    "permissions field has no valid value");
	    return NULL;
	}
	return PyInt_FromLong(finfo->permissions & (~PYMATE_VFS_ACCESS_BITS));
    } else if (!strcmp(attr, "access")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_ACCESS)) {
	    PyErr_SetString(PyExc_ValueError,
			    "access field has no valid value");
	    return NULL;
	}
	return PyInt_FromLong(finfo->permissions & PYMATE_VFS_ACCESS_BITS);
    } else if (!strcmp(attr, "flags")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_FLAGS)) {
	    PyErr_SetString(PyExc_ValueError,"flags field has no valid value");
	    return NULL;
	}
	return PyInt_FromLong(finfo->flags);
    } else if (!strcmp(attr, "device")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_DEVICE)) {
	    PyErr_SetString(PyExc_ValueError,
			    "device field has no valid value");
	    return NULL;
	}
	if (finfo->device <= G_MAXLONG)
	    return PyInt_FromLong(finfo->device);
	else
	    return PyLong_FromUnsignedLongLong(finfo->device);
    } else if (!strcmp(attr, "inode")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_INODE)) {
	    PyErr_SetString(PyExc_ValueError,"inode field has no valid value");
	    return NULL;
	}
	if (finfo->inode <= G_MAXLONG)
	    return PyInt_FromLong(finfo->inode);
	else
	    return PyLong_FromUnsignedLongLong(finfo->inode);
    } else if (!strcmp(attr, "link_count")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT)) {
	    PyErr_SetString(PyExc_ValueError,
			    "link_count field has no valid value");
	    return NULL;
	}
	if (finfo->link_count < G_MAXLONG)
	    return PyInt_FromLong(finfo->link_count);
	else
	    return PyLong_FromUnsignedLong(finfo->link_count);
    } else if (!strcmp(attr, "uid")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_IDS)) {
	    PyErr_SetString(PyExc_ValueError, "uid field has no valid value");
	    return NULL;
	}
	if (finfo->uid < G_MAXLONG)
	    return PyInt_FromLong(finfo->uid);
	else
	    return PyLong_FromUnsignedLong(finfo->uid);
    } else if (!strcmp(attr, "gid")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_IDS)) {
	    PyErr_SetString(PyExc_ValueError, "gid field has no valid value");
	    return NULL;
	}
	if (finfo->gid < G_MAXLONG)
	    return PyInt_FromLong(finfo->gid);
	else
	    return PyLong_FromUnsignedLong(finfo->gid);
    } else if (!strcmp(attr, "size")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SIZE)) {
	    PyErr_SetString(PyExc_ValueError, "size field has no valid value");
	    return NULL;
	}
	if (finfo->size <= G_MAXLONG)
	    return PyInt_FromLong(finfo->size);
	else
	    return PyLong_FromUnsignedLongLong(finfo->size);
    } else if (!strcmp(attr, "block_count")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT)) {
	    PyErr_SetString(PyExc_ValueError,
			    "block_count field has no valid value");
	    return NULL;
	}
	if (finfo->block_count <= G_MAXLONG)
	    return PyInt_FromLong(finfo->block_count);
	else
	    return PyLong_FromUnsignedLongLong(finfo->block_count);
    } else if (!strcmp(attr, "io_block_size")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE)){
	    PyErr_SetString(PyExc_ValueError,
			    "io_block_size field has no valid value");
	    return NULL;
	}
	if (finfo->io_block_size < G_MAXLONG)
	    return PyInt_FromLong(finfo->io_block_size);
	else
	    return PyLong_FromUnsignedLong(finfo->io_block_size);
    } else if (!strcmp(attr, "atime")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_ATIME)) {
	    PyErr_SetString(PyExc_ValueError,"atime field has no valid value");
	    return NULL;
	}
	return PyLong_FromLongLong(finfo->atime);
    } else if (!strcmp(attr, "mtime")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_MTIME)) {
	    PyErr_SetString(PyExc_ValueError,"mtime field has no valid value");
	    return NULL;
	}
	return PyLong_FromLongLong(finfo->mtime);
    } else if (!strcmp(attr, "ctime")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_CTIME)) {
	    PyErr_SetString(PyExc_ValueError,"ctime field has no valid value");
	    return NULL;
	}
	return PyLong_FromLongLong(finfo->ctime);
    } else if (!strcmp(attr, "symlink_name")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME)) {
	    PyErr_SetString(PyExc_ValueError,
			    "link_name field has no valid value");
	    return NULL;
	}
	if (finfo->symlink_name)
	    return PyString_FromString(finfo->symlink_name);
	Py_INCREF(Py_None);
	return Py_None;
    } else if (!strcmp(attr, "mime_type")) {
	if (!(finfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE)) {
	    PyErr_SetString(PyExc_ValueError,
			    "mime_type field has no valid value");
	    return NULL;
	}
	if (finfo->mime_type)
	    return PyString_FromString(finfo->mime_type);
	Py_INCREF(Py_None);
	return Py_None;
    } else {
	PyObject *name = PyString_FromString(attr);
	PyObject *ret = PyObject_GenericGetAttr((PyObject *)self, name);

	Py_DECREF(name);
	return ret;
    }
}

static int
pygvfinfo_setattr(PyMateVFSFileInfo *self, const gchar *attr, PyObject *value)
{
    MateVFSFileInfo *finfo;

    if (!self->finfo)
	self->finfo = mate_vfs_file_info_new();
    finfo = self->finfo;

    if (!strcmp(attr, "__members__")) {
	PyErr_SetString(PyExc_TypeError, "readonly attribute");
	return -1;

    } else if (!strcmp(attr, "name")) {
        if (!PyString_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'name' attribute must be a string");
                return -1;
        }
	if (finfo->name) g_free(finfo->name);
        finfo->name = g_strdup(PyString_AsString(value));
	return 0;

    } else if (!strcmp(attr, "valid_fields")) {
        if (!PyInt_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'valid_fields' attribute must be an int");
                return -1;
        }
	finfo->valid_fields = PyInt_AsLong(value);
        return 0;

    } else if (!strcmp(attr, "type")) {
        if (!PyInt_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'type' attribute must be an int");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;
	finfo->type = PyInt_AsLong(value);
        return 0;

    } else if (!strcmp(attr, "permissions")) {
        if (!PyInt_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'permissions' attribute must be an int");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	finfo->permissions = PyInt_AsLong(value);
        return 0;

    } else if (!strcmp(attr, "access")) {
        if (!PyInt_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'access' attribute must be an int");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ACCESS;
	finfo->permissions |= PyInt_AsLong(value);
        return 0;

    } else if (!strcmp(attr, "flags")) {
        if (!PyInt_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'flags' attribute must be an int");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_FLAGS;
	finfo->flags = PyInt_AsLong(value);
        return 0;

    } else if (!strcmp(attr, "device")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'device' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_DEVICE;
	if (PyInt_Check(value))
	    finfo->device = PyInt_AsLong(value);
	else
	    finfo->device = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "inode")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'inode' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_INODE;
	if (PyInt_Check(value))
	    finfo->inode = PyInt_AsLong(value);
	else
	    finfo->inode = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "link_count")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'link_count' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_LINK_COUNT;
	if (PyInt_Check(value))
	    finfo->link_count = PyInt_AsLong(value);
	else
	    finfo->link_count = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "uid")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'uid' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IDS;
	if (PyInt_Check(value))
	    finfo->uid = PyInt_AsLong(value);
	else
	    finfo->uid = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "gid")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'gid' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IDS;
	if (PyInt_Check(value))
	    finfo->gid = PyInt_AsLong(value);
	else
	    finfo->gid = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "size")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'size' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
	if (PyInt_Check(value))
	    finfo->size = PyInt_AsLong(value);
	else
	    finfo->size = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "block_count")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'block_count' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_BLOCK_COUNT;
	if (PyInt_Check(value))
	    finfo->block_count = PyInt_AsLong(value);
	else
	    finfo->block_count = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "io_block_size")) {
        if (!(PyInt_Check(value) || PyLong_Check(value))) {
            	PyErr_SetString(PyExc_TypeError,
                                "'io_block_size' attribute must be an int or long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
	if (PyInt_Check(value))
	    finfo->io_block_size = PyInt_AsLong(value);
	else
	    finfo->io_block_size = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "atime")) {
        if (!PyLong_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'atime' attribute must be a long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ATIME;
	finfo->atime = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "mtime")) {
        if (!PyLong_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'mtime' attribute must be a long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MTIME;
	finfo->mtime = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "ctime")) {
        if (!PyLong_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'ctime' attribute must be a long");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_CTIME;
	finfo->ctime = PyLong_AsUnsignedLongLong(value);
        return 0;

    } else if (!strcmp(attr, "symlink_name")) {
        if (!PyString_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'symlink_name' attribute must be a string");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME;
	if (finfo->symlink_name) g_free(finfo->symlink_name);
        finfo->symlink_name = g_strdup(PyString_AsString(value));
	return 0;

    } else if (!strcmp(attr, "mime_type")) {
        if (!PyString_Check(value)) {
            	PyErr_SetString(PyExc_TypeError,
                                "'mime_type' attribute must be a string");
                return -1;
        }
	finfo->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	if (finfo->mime_type) g_free(finfo->mime_type);
        finfo->mime_type = g_strdup(PyString_AsString(value));
	return 0;

    } else {
	PyObject *name = PyString_FromString(attr);
	int ret = PyObject_GenericSetAttr((PyObject *)self, name, value);

	Py_DECREF(name);
	return ret;
    }
}

static int
pygvfinfo_init(PyMateVFSFileInfo *self, PyObject *args, PyObject *kwargs)
{
    if (kwargs != NULL) {
	PyErr_SetString(PyExc_TypeError,
		"matevfs.FileInfo.__init__ takes no keyword arguments");
	return -1;
    }
    if (!PyArg_ParseTuple(args, ":matevfs.FileInfo.__init__"))
	return -1;

    self->finfo = mate_vfs_file_info_new();
    if (!self->finfo) {
	PyErr_SetString(PyExc_TypeError, "could not create FileInfo object");
	return -1;
    }

    return 0;
}


static PyMethodDef pygvfinfo_methods[] = {
    { NULL, NULL, 0 }
};


PyTypeObject PyMateVFSFileInfo_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.FileInfo",               /* tp_name */
    sizeof(PyMateVFSFileInfo),         /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pygvfinfo_dealloc,      /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)pygvfinfo_getattr,     /* tp_getattr */
    (setattrfunc)pygvfinfo_setattr,     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pygvfinfo_repr,           /* tp_repr */
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
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pygvfinfo_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pygvfinfo_init,           /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};
