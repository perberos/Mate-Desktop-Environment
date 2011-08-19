/* -*- mode: C; c-basic-offset: 4 -*- */
#ifndef __PYMATEVFS_H_
#define __PYMATEVFS_H_

#include <Python.h>

#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-directory.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-mime-handlers.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-xfer.h>


G_BEGIN_DECLS

typedef struct {
    PyObject_HEAD
    MateVFSURI *uri;
} PyMateVFSURI;

typedef struct {
    PyObject_HEAD
    MateVFSFileInfo *finfo;
} PyMateVFSFileInfo;

typedef struct {
    PyObject_HEAD
    MateVFSContext *context;
} PyMateVFSContext;
    
#define pymate_vfs_uri_get(v) (((PyMateVFSURI *)(v))->uri)
#define pymate_vfs_uri_check(v) ((v)->ob_type == _PyMateVFS_API->uri_type)

#define pymate_vfs_file_info_get(v) (((PyMateVFSFileInfo *)(v))->finfo)
#define pymate_vfs_file_info_check(v) ((v)->ob_type == _PyMateVFS_API->file_info_type)

#define pymate_vfs_context_get(v) (((PyMateVFSURI *)(v))->context)
#define pymate_vfs_context_check(v) ((v)->ob_type == _PyMateVFS_API->context_type)

struct _PyMateVFS_Functions {
    MateVFSResult (* exception_check)(void);
    PyObject *(* uri_new)(MateVFSURI *uri);
    PyTypeObject *uri_type;
    PyObject *(* file_info_new)(MateVFSFileInfo *finfo);
    PyTypeObject *file_info_type;
    PyObject *(* context_new)(MateVFSContext *context);
    PyTypeObject *context_type;
};

#ifndef _INSIDE_PYMATEVFS_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYMATEVFS)
extern struct _PyMateVFS_Functions *_PyMateVFS_API;
#else
struct _PyMateVFS_Functions *_PyMateVFS_API;
#endif

#define pymate_vfs_exception_check (_PyMateVFS_API->exception_check)
#define pymate_vfs_uri_new         (_PyMateVFS_API->uri_new)
#define PyMateVFSURI_Type          (*_PyMateVFS_API->uri_type)
#define pymate_vfs_file_info_new   (_PyMateVFS_API->file_info_new)
#define PyMateVFSFileInfo_Type     (*_PyMateVFS_API->file_info_type)
#define pymate_vfs_context_new     (_PyMateVFS_API->context_new)
#define PyMateVFSContext_Type      (*_PyMateVFS_API->context_type)

static inline PyObject *
pymatevfs_init(void)
{
    PyObject *module = PyImport_ImportModule("matevfs");
    if (module != NULL) {
        PyObject *mdict = PyModule_GetDict(module);
        PyObject *cobject = PyDict_GetItemString(mdict, "_PyMateVFS_API");
        if (PyCObject_Check(cobject))
            _PyMateVFS_API = (struct _PyMateVFS_Functions *)PyCObject_AsVoidPtr(cobject);
        else {
	    Py_FatalError("could not find _PyMateVFS_API object");
        }
    } else {
        Py_FatalError("could not import matevfs");
    }
    return module;
}

#define init_pymatevfs() pymatevfs_init();


#endif /* !_INSIDE_PYMATEVFS_ */


#define PYGVFS_CONTROL_MAGIC_IN 0xa346a943U
#define PYGVFS_CONTROL_MAGIC_OUT 0xb49535dcU

typedef struct {
    guint magic;
    PyObject *data;
} PyGVFSOperationData;

G_END_DECLS

#endif /* __PYMATEVFS_H_ */
