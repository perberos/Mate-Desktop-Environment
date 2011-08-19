/* -*- mode: C; c-basic-offset: 4 -*- */
#ifndef __PYMATEVFS_PRIVATE_H_
#define __PYMATEVFS_PRIVATE_H_

#ifdef _PYMATEVFS_H_
#  error "include pymatevfs.h or pymatevfs-private.h, but not both"
#endif

#define _INSIDE_PYMATEVFS_
#include "pymatevfs.h"
#include <libmatevfs/mate-vfs-volume.h>
#include <libmatevfs/mate-vfs-volume-monitor.h>

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
typedef inquiry lenfunc;
typedef intargfunc ssizeargfunc;
typedef intobjargproc ssizeobjargproc;
#endif


G_BEGIN_DECLS

/* vfsmodule.c */
extern struct _PyMateVFS_Functions pymatevfs_api_functions;

MateVFSResult pymate_vfs_exception_check(void);
gboolean pymate_vfs_result_check(MateVFSResult result);

/* vfs-contexti.c */
extern PyTypeObject PyMateVFSContext_Type;

PyObject *pymate_vfs_context_new(MateVFSContext *context);

/* vfs-uri.c */
extern PyTypeObject PyMateVFSURI_Type;

/* takes ownership of URI */
PyObject *pymate_vfs_uri_new(MateVFSURI *uri);

/* vfs-file-info.c */
extern PyTypeObject PyMateVFSFileInfo_Type;

PyObject *pymate_vfs_file_info_new(MateVFSFileInfo *finfo);


/* vfs-dir-handle.c */
typedef struct {
    PyObject_HEAD
    MateVFSDirectoryHandle *dir;
} PyMateVFSDirectoryHandle;
extern PyTypeObject PyMateVFSDirectoryHandle_Type;

#define pymate_vfs_directory_handle_get(v) (((PyMateVFSDirectoryHandle *)(v))->dir)
PyObject *pymate_vfs_directory_handle_new(MateVFSDirectoryHandle *dir);


/* vfs-handle.c */
typedef struct {
    PyObject_HEAD
    MateVFSHandle *fd;
} PyMateVFSHandle;
extern PyTypeObject PyMateVFSHandle_Type;

#define pymate_vfs_handle_get(v) (((PyMateVFSHandle *)(v))->fd)
PyObject *pymate_vfs_handle_new(MateVFSHandle *fd);

/* vfs-async-handle.c */
PyObject *pygvfs_async_module_init (void);

typedef struct {
    PyObject *func, *data;
} PyGVFSCustomNotify;


/* vfs-xfer-progress-info.c */
typedef struct {
    PyObject_HEAD
    MateVFSXferProgressInfo *info; /* not owned */
} PyMateVFSXferProgressInfo;

extern PyTypeObject PyMateVFSXferProgressInfo_Type;
PyObject *pymate_vfs_xfer_progress_info_new(MateVFSXferProgressInfo *info);
#define pymate_vfs_xfer_progress_info_set(self, info_)\
       ((PyMateVFSXferProgressInfo *) (self))->info = info_


/* vfs-volume.c (PyGObject) */
extern PyTypeObject PyMateVFSVolume_Type;
/* vfs-drive.c (PyGObject) */
extern PyTypeObject PyMateVFSDrive_Type;
/* vfs-drive-monitor.c (PyGObject) */
extern PyTypeObject PyMateVFSVolumeMonitor_Type;

typedef struct {
    PyObject *callback;
    PyObject *user_data;
} PyMateVFSVolumeOpCallback;

void wrap_matevfs_volume_op_callback(gboolean succeeded,
                                      char *error,
                                      char *detailed_error,
                                      gpointer data);

G_END_DECLS

#endif /* __PYMATEVFS_PRIVATE_H_ */
