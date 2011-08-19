/* -*- mode: C; c-basic-offset: 4 -*- */
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>
#include "pymatevfs-private.h"
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-job-limit.h>

typedef struct {
    PyObject_HEAD;
    MateVFSAsyncHandle *fd;
} PyMateVFSAsyncHandle;
extern PyTypeObject PyMateVFSAsyncHandle_Type;

static PyObject *
pymate_vfs_async_handle_new(MateVFSAsyncHandle *fd)
{
    PyMateVFSAsyncHandle *self;

    self = PyObject_NEW(PyMateVFSAsyncHandle, &PyMateVFSAsyncHandle_Type);
    if (self == NULL) return NULL;

    self->fd = fd;

    return(PyObject *)self;
}

  /* Get the exception in case any error has occured. Return 1 in case
     any error happened. */
static PyObject *
fetch_exception(MateVFSResult result, gboolean *error_happened)
{
    PyObject *retval;

    if (pymate_vfs_result_check(result)) {
	retval = PyErr_Occurred();
	if (error_happened)
	    *error_happened = TRUE;
    } else {
	retval = Py_None;
	if (error_happened)
	    *error_happened = FALSE;
    }

    Py_INCREF(retval);
    PyErr_Clear();

    return retval;
}

static void
pygvhandle_dealloc(PyMateVFSAsyncHandle *self)
{
      /* Following code removed by Gustavo Carneiro on 2004-12-26.
       Rationale: Automatically closing the async operation when the
       python handle object is deallocated is a major source of bugs.
       Even the original example code provided by the author falls
       victim of the bug of ignoring return values, thus causing the
       async operation to be closed almost immediately and
       unintentionally.  The async handle is actually a simple integer
       cast to pointer, thus no free of resources is necessary, and
       the async operation will complete/be freed by itself even if
       not cancelled by the programmer. */
#if 0
    if (self->fd) {
        pyg_begin_allow_threads;
	mate_vfs_async_close(self->fd, 
			      (MateVFSAsyncCloseCallback)async_pass, NULL);
        pyg_end_allow_threads;
    }
#endif

    PyObject_FREE(self);
}

typedef struct {
    PyObject *func, *data;
    PyMateVFSAsyncHandle *self;
    enum {
	ASYNC_NOTIFY_OPEN, 
	ASYNC_NOTIFY_READ, 
	ASYNC_NOTIFY_WRITE, 
	ASYNC_NOTIFY_CLOSE, 
	ASYNC_NOTIFY_G_INFO, 
	ASYNC_NOTIFY_LOAD_DIRECTORY, 
	ASYNC_NOTIFY_CREATE, 
	ASYNC_NOTIFY_CREATE_SYMLINK
    } origin;
    PyObject *extra;
} PyGVFSAsyncNotify;

static PyGVFSAsyncNotify *
async_notify_new(PyObject *func, void *self, 
                 PyObject *data, int origin)
{
    PyGVFSAsyncNotify *result = g_new0(PyGVFSAsyncNotify, 1);

    result->func = func;
    result->self = (PyMateVFSAsyncHandle *)self;
    result->data = data;
    result->origin = origin;

    Py_INCREF(func);
    Py_INCREF((PyMateVFSAsyncHandle*)self);
    Py_XINCREF(data);

    return result;
}

static void
async_notify_free(PyGVFSAsyncNotify *notify)
{
    Py_DECREF(notify->func);
    Py_DECREF(notify->self);
    Py_XDECREF(notify->data);
    Py_XDECREF(notify->extra);

    g_free(notify);
}

    /* Remember to decref the returned object after using it */
static MateVFSURI *
_object_to_uri(const char *name, PyObject *uri)
{
    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
	return mate_vfs_uri_ref(pymate_vfs_uri_get(uri));
    } else if (PyString_Check(uri)) {
	MateVFSURI *c_uri = mate_vfs_uri_new(PyString_AsString(uri));
	if (c_uri == NULL)
	    PyErr_SetString(PyExc_TypeError, "Cannot build a matevfs.URI");
	return c_uri;
    } else {
	gchar * buffer = 
	    g_strdup_printf("'%s' must be a matevfs.URI or a string", 
                            name);
	PyErr_SetString(PyExc_TypeError, buffer);
	g_free(buffer);
    }
    return NULL;
}

#define object_to_uri(OBJECT) _object_to_uri(#OBJECT, OBJECT)

static int
pygvhandle_init(PyMateVFSAsyncHandle *self, PyObject *args, PyObject *kwargs)
{
    return 0;
}

static void
callback_marshal(MateVFSAsyncHandle *handle, 
                 MateVFSResult result, 
                 PyGVFSAsyncNotify *notify)
{
    PyObject *retobj;
    PyObject *exception;
    gboolean error_happened;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    exception = fetch_exception(result, &error_happened);
    if (error_happened && 
	(notify->origin == ASYNC_NOTIFY_OPEN || 
	 notify->origin == ASYNC_NOTIFY_CREATE)) {
	notify->self->fd = NULL;
    }

    if (notify->origin == ASYNC_NOTIFY_CREATE_SYMLINK)
	notify->self->fd = NULL;

    if (notify->data)
	retobj = PyEval_CallFunction(notify->func, "(OOO)", 
                                     notify->self, 
                                     exception, 
                                     notify->data);
    else
	retobj = PyObject_CallFunction(notify->func, "(OO)", 
                                       notify->self, 
                                       exception);

    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }

    Py_XDECREF(retobj);
    Py_DECREF(exception);

    async_notify_free(notify);

    pyg_gil_state_release(state);
}

static PyObject*
pygvfs_async_open(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "callback", "open_mode", "priority", 
			      "data", NULL };
    PyObject *uri;
    MateVFSOpenMode open_mode = MATE_VFS_OPEN_READ;
    PyObject *callback;
    PyObject *data = NULL;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    PyObject *pyself;
    MateVFSURI *c_uri;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OO|iiO:matevfs.async.open", 
				     kwlist, &uri, &callback, &open_mode, 
				     &priority, &data))
	return NULL;

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    c_uri = object_to_uri(uri);
    if (c_uri == NULL)
	return NULL;

    pyself = pymate_vfs_async_handle_new(NULL);

    mate_vfs_async_open_uri(&((PyMateVFSAsyncHandle*)pyself)->fd, 
			     c_uri, 
			     open_mode, 
			     priority, 
			     (MateVFSAsyncOpenCallback)callback_marshal, 
			     async_notify_new(callback, pyself, data, 
                                              ASYNC_NOTIFY_OPEN));

    mate_vfs_uri_unref(c_uri);

    return pyself;
}

static PyObject*
pygvhandle_close(PyMateVFSAsyncHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "callback", "data", NULL };
    PyObject *callback;
    PyObject *data = NULL;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "O|O:matevfs.async.Handle.close", 
				     kwlist, &callback, &data))
	return NULL;

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    mate_vfs_async_close(self->fd, 
			  (MateVFSAsyncCloseCallback)callback_marshal, 
			  async_notify_new(callback, self, data, 
                                           ASYNC_NOTIFY_CLOSE));

    self->fd = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static void
read_write_marshal(MateVFSAsyncHandle *handle, 
                   MateVFSResult result, 
                   gpointer buffer, 
                   MateVFSFileSize requested, 
                   MateVFSFileSize done, 
                   PyGVFSAsyncNotify *notify)
{
    PyObject *retobj;
    PyObject *pyvalue;
    PyObject *exception;
    gboolean error_happened;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    exception = fetch_exception(result, &error_happened);

    if (notify->origin == ASYNC_NOTIFY_READ)
	pyvalue = PyString_FromStringAndSize(buffer, done);
    else
	pyvalue = PyInt_FromLong(done);

#if defined(MATE_VFS_SIZE_IS_UNSIGNED_LONG_LONG)
# define SIZE_FMT "K"
#elif defined(MATE_VFS_SIZE_IS_UNSIGNED_LONG)
# define SIZE_FMT "k"
#else
# error "Unexpected MateVFSFileSize typedef"
#endif

    if (notify->data)
	retobj = PyEval_CallFunction(notify->func, "(OOO"SIZE_FMT"O)",
                                     notify->self,
                                     pyvalue,
                                     exception,
                                     requested,
                                     notify->data);
    else
	retobj = PyObject_CallFunction(notify->func, "(OOO"SIZE_FMT")",
                                       notify->self,
                                       pyvalue,
                                       exception,
                                       requested);

    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }

    Py_XDECREF(retobj);
    Py_DECREF(pyvalue);
    Py_DECREF(exception);

    if (notify->origin == ASYNC_NOTIFY_READ)
	g_free(buffer);

    async_notify_free(notify);

    pyg_gil_state_release(state);
}

static PyObject*
pygvhandle_read(PyMateVFSAsyncHandle *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "bytes", "callback", "data", NULL };
    glong bytes;
    PyObject *data = NULL;
    PyObject *callback;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "lO|O:matevfs.async.Handle.read", 
				     kwlist, &bytes, &callback, &data))
	return NULL;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "third argument not callable");
        return NULL;
    }

    mate_vfs_async_read(self->fd, g_malloc(bytes), bytes, 
                         (MateVFSAsyncReadCallback)read_write_marshal, 
                         async_notify_new(callback, self, data, 
                                          ASYNC_NOTIFY_READ));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
pygvhandle_write(PyMateVFSAsyncHandle *self, 
                 PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "buffer", "callback", "data", NULL };
    PyObject *buffer;
    PyObject *data = NULL;
    PyObject *callback;
    PyGVFSAsyncNotify *notify;

    if (!self->fd) {
	PyErr_SetString(PyExc_ValueError, "I/O operation on closed handle");
	return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OO|O:matevfs.async.Handle.write", 
				     kwlist, &buffer, &callback, &data))
	return NULL;

    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
        return NULL;
    }

    if (!PyString_Check(buffer)) {
	PyErr_SetString(PyExc_TypeError, "'buffer' must be a string object");
	return NULL;
    }

    Py_INCREF(buffer);
    notify = async_notify_new(callback, self, data, ASYNC_NOTIFY_WRITE);
    notify->extra = buffer;
    mate_vfs_async_write(self->fd, PyString_AsString(buffer), 
                          PyString_Size(buffer), 
                          (MateVFSAsyncWriteCallback)read_write_marshal, 
                          notify);

    Py_INCREF(Py_None);
    return Py_None;
}

static void
get_info_marshal(MateVFSAsyncHandle *handle, 
                 GList *results, 
                 PyGVFSAsyncNotify *notify)
{
    PyObject *retobj;
    PyObject *pyresults; /* a list of (uri, exception, info) tuples */
    gint length;
    gint i;
    PyGILState_STATE state;
    
    state = pyg_gil_state_ensure();

    notify->self->fd = NULL;

    length = g_list_length(results);
    pyresults = PyList_New(length);

    for (i = 0; i < length; i++, results = results->next) {
	PyObject *item = PyTuple_New(3);
	MateVFSGetFileInfoResult *r = results->data;
	mate_vfs_uri_ref(r->uri);
	PyTuple_SetItem(item, 0, pymate_vfs_uri_new(r->uri));
	PyTuple_SetItem(item, 1, fetch_exception(r->result, NULL));
	mate_vfs_file_info_ref(r->file_info);
	PyTuple_SetItem(item, 2, pymate_vfs_file_info_new(r->file_info));

	PyList_SetItem(pyresults, i, item);
    }

    if (notify->data)
	retobj = PyEval_CallFunction(notify->func, "(OOO)", 
                                     notify->self, 
                                     pyresults, 
                                     notify->data);
    else
	retobj = PyObject_CallFunction(notify->func, "(OO)", 
                                       notify->self, 
                                       pyresults);

    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }

    Py_XDECREF(retobj);
    Py_DECREF(pyresults);

    async_notify_free(notify);

    pyg_gil_state_release(state);
}

static PyObject *
pygvfs_async_get_file_info(PyObject *self, 
                           PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "urilist", "callback", "options", "priority", 
			      "data", NULL };
    PyObject *py_urilist;
    GList *urilist = NULL;
    MateVFSFileInfoOptions options = MATE_VFS_FILE_INFO_DEFAULT;
    PyObject *callback;
    PyObject *data = NULL;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    int size, i;
    PyObject *pyself;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OO|iiO:matevfs.async.get_file_info", 
				     kwlist, &py_urilist, &callback, &options, 
				     &priority, &data))
	return NULL;

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    if (PyString_Check(py_urilist)) {
	urilist = g_list_append(urilist, 
                                mate_vfs_uri_new(PyString_AsString
                                                  (py_urilist)));
    }
    else if (PyObject_TypeCheck(py_urilist, &PyMateVFSURI_Type)) {
	urilist = g_list_append(urilist, 
                                mate_vfs_uri_ref
                                (pymate_vfs_uri_get(py_urilist)));
    } else if (PySequence_Check(py_urilist)) {
	size = PySequence_Size(py_urilist);
	for (i = 0; i < size; ++i) {
	    PyObject *item = PySequence_GetItem(py_urilist, i);
	    MateVFSURI *uri = NULL;
	    if (PyObject_TypeCheck(item, &PyMateVFSURI_Type))
		uri = mate_vfs_uri_ref(pymate_vfs_uri_get(item));
	    else if (PyString_Check(item)) {
		uri = mate_vfs_uri_new(PyString_AsString(item)); }
	    else {
		PyErr_SetString(PyExc_TypeError, "all items in sequence must be of string type or matevfs.URI");
		return NULL;
	    }
	    urilist = g_list_append(urilist, uri);
	    Py_DECREF(item);
	}
    }
    else {
	PyErr_SetString(PyExc_TypeError, "'urilist' must be either a string, matevfs.URI or a sequence of those");
        return NULL;
    }

    pyself = pymate_vfs_async_handle_new(NULL);
    mate_vfs_async_get_file_info(&((PyMateVFSAsyncHandle*)pyself)->fd, 
				  urilist, 
				  options, 
				  priority, 
				  (MateVFSAsyncGetFileInfoCallback)get_info_marshal, 
				  async_notify_new(callback, pyself, data, 
                                                   ASYNC_NOTIFY_G_INFO));

    while (urilist) {
	mate_vfs_uri_unref((MateVFSURI*)urilist->data);
	urilist = urilist->next;
    }
    g_list_free(urilist);

    return pyself;
}

static PyObject *
pygvhandle_is_open(PyMateVFSAsyncHandle *self)
{
    return PyInt_FromLong(self->fd != NULL);
}

static PyObject *
pygvhandle_cancel(PyMateVFSAsyncHandle *self)
{
    if (self->fd) {
	mate_vfs_async_cancel(self->fd);
	self->fd = NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static void
load_dir_marshal(MateVFSAsyncHandle *handle, 
                 MateVFSResult result, 
                 GList *list, 
                 guint length, 
                 PyGVFSAsyncNotify *notify)
{
    PyObject *retobj;
    PyObject *pyresults; /* a list of matevfs.FileInfo */
    gint i;
    gboolean error_happened;
    PyObject *exception;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    exception = fetch_exception(result, &error_happened);
    if (error_happened && 
	notify->origin == ASYNC_NOTIFY_LOAD_DIRECTORY)
	notify->self->fd = NULL;

    pyresults = PyList_New(length);

    for (i = 0; i < length; i++, list = list->next) {
	MateVFSFileInfo *info = list->data;

	mate_vfs_file_info_ref(info);
	PyList_SetItem(pyresults, i, pymate_vfs_file_info_new(info));
    }

    if (notify->data)
	retobj = PyEval_CallFunction(notify->func, "(OOOO)", 
                                     notify->self, 
                                     pyresults, 
                                     exception, 
                                     notify->data);
    else
	retobj = PyObject_CallFunction(notify->func, "(OOO)", 
                                       notify->self, 
                                       pyresults, 
                                       exception);

    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }

    Py_XDECREF(retobj);
    Py_DECREF(pyresults);
    Py_DECREF(exception);

      /* Supposedly we don't get called after errors? */
    if (error_happened)
	async_notify_free(notify);

    pyg_gil_state_release(state);
}

static PyObject*
pygvfs_async_load_directory(PyObject *self, 
                            PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "callback", 
			      "options", 
			      "items_per_notification", 
			      "priority", 
			      "data", NULL };
    PyObject *uri;
    PyObject *callback;
    MateVFSFileInfoOptions options = MATE_VFS_FILE_INFO_DEFAULT;
    guint items_per_notification = 20; /* Some default to keep the
					  order of the parameters as in
					  the C API */
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    PyObject *data = NULL;
    PyObject *pyself;
    MateVFSURI *c_uri;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OO|iIiO:matevfs.async.load_directory", 
				     kwlist, &uri, &callback, 
				     &options, 
				     &items_per_notification, 
				     &priority, &data))
	return NULL;

      /* XXXX: unblock threads here */

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    c_uri = object_to_uri(uri);
    if (c_uri == NULL)
	return NULL;

    pyself = pymate_vfs_async_handle_new(NULL);
    mate_vfs_async_load_directory_uri(&((PyMateVFSAsyncHandle*)pyself)->fd, 
				       c_uri, 
				       options, 
				       items_per_notification, 
				       priority, 
				       (MateVFSAsyncDirectoryLoadCallback)load_dir_marshal, 
				       async_notify_new(callback, pyself, data, 
                                                        ASYNC_NOTIFY_LOAD_DIRECTORY));

    mate_vfs_uri_unref(c_uri);

    return pyself;
}

static PyObject*
pygvfs_async_create(PyObject *self, 
                    PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "callback", 
			      "open_mode", 
			      "exclusive", 
			      "perm", 
			      "priority", 
			      "data", NULL };
    PyObject *uri;
    PyObject *callback;
    MateVFSOpenMode open_mode = MATE_VFS_OPEN_READ | MATE_VFS_OPEN_WRITE;
    gboolean exclusive = FALSE;
    guint perm = 0644;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    PyObject *data = NULL;
    PyObject *pyself;
    MateVFSURI *c_uri;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OO|iiiiO:matevfs.async.create", 
				     kwlist, &uri, &callback, 
				     &open_mode, &exclusive, 
				     &perm, &priority, &data))
	return NULL;

      /* XXXX: unblock threads here */

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    c_uri = object_to_uri(uri);
    if (c_uri == NULL)
	return NULL;

    pyself = pymate_vfs_async_handle_new(NULL);
    mate_vfs_async_create_uri(&((PyMateVFSAsyncHandle*)pyself)->fd, 
			       c_uri, 
			       open_mode, 
			       exclusive, 
			       perm, 
			       priority, 
			       (MateVFSAsyncOpenCallback)callback_marshal, 
			       async_notify_new(callback, pyself, data, 
                                                ASYNC_NOTIFY_CREATE));

    mate_vfs_uri_unref(c_uri);

    return pyself;
}

static PyObject*
pygvfs_async_create_symbolic_link(PyObject *self, 
                                  PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "reference", "callback", 
			      "priority", 
			      "data", NULL };
    PyObject *uri;
    PyObject *reference;
    PyObject *callback;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    PyObject *data = NULL;
    PyObject *pyself;
    MateVFSURI *c_uri, *c_reference;
    gchar *reference_buffer;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "OOO|iO:matevfs.async.create_symbolic_link", 
				     kwlist, &uri, &reference, &callback, 
				     &priority, &data))
	return NULL;

      /* XXXX: unblock threads here */

    if (!PyCallable_Check(callback)) {
	PyErr_SetString(PyExc_TypeError, "'callback' argument not callable");
	return NULL;
    }

    c_uri = object_to_uri(uri);
    if (c_uri == NULL)
	return NULL;

    c_reference = object_to_uri(reference);
    if (c_reference == NULL) {
	mate_vfs_uri_unref(c_uri);
	return NULL;
    }
      /* hmmm... */
    reference_buffer = mate_vfs_uri_to_string(c_reference, 
                                               MATE_VFS_URI_HIDE_NONE);

    pyself = pymate_vfs_async_handle_new(NULL);
    mate_vfs_async_create_symbolic_link(&((PyMateVFSAsyncHandle*)pyself)->fd, 
					 c_uri, 
					 reference_buffer, 
					 priority, 
					 (MateVFSAsyncOpenCallback)callback_marshal, 
					 async_notify_new(callback, pyself, data, 
                                                          ASYNC_NOTIFY_CREATE_SYMLINK));

    g_free(reference_buffer);
    mate_vfs_uri_unref(c_uri);
    mate_vfs_uri_unref(c_reference);

    return pyself;
}

static PyObject *
pygvfs_async_get_job_limit(PyObject *self)
{
    return PyInt_FromLong(mate_vfs_async_get_job_limit());
}

static PyObject*
pygvfs_async_set_job_limit(PyObject *self, 
                           PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "limit", NULL };
    int limit;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, 
				     "i:matevfs.async.set_job_limit", 
				     kwlist, &limit))
	return NULL;

    mate_vfs_async_set_job_limit(limit);

    Py_INCREF(Py_None);
    return Py_None;
}

/* --- vfs_async_xfer --- */

gboolean _pygvfs_uri_sequence_to_glist(PyObject *seq, GList **list);
gint pygvfs_xfer_progress_callback(MateVFSXferProgressInfo *info, gpointer _data);

typedef struct {
    PyGVFSCustomNotify update_data;
    PyGVFSCustomNotify sync_data;
} PyGVFSAsyncXferData;

static gint
pygvfs_async_xfer_progress_callback(MateVFSAsyncHandle *handle,
                                    MateVFSXferProgressInfo *info,
                                    gpointer _data)
{
    PyGVFSAsyncXferData *full_data = _data;
    PyGVFSCustomNotify *data = &full_data->update_data;
    PyObject *py_handle, *py_info, *callback_return;
    gint retval;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    py_handle = pymate_vfs_async_handle_new(handle);
    py_info = pymate_vfs_xfer_progress_info_new(info);

    if (data->data)
	callback_return = PyObject_CallFunction(data->func, "NOO", py_handle, py_info, data->data);
    else
	callback_return = PyObject_CallFunction(data->func, "NO", py_handle, py_info);

      /* because the programmer may keep a reference to the
       * VFSXferProgressInfo python wrapper but we don't own the
       * VFSXferProgressInfo itself, we remove the link from the
       * python wrapper to the C structure */
    pymate_vfs_xfer_progress_info_set(py_info, NULL);
    Py_DECREF(py_info);

    if (info->phase == MATE_VFS_XFER_PHASE_COMPLETED) {
          /* cleanup everything */
        Py_XDECREF(full_data->sync_data.func);
        Py_XDECREF(full_data->update_data.func);
        Py_XDECREF(full_data->sync_data.data);
        Py_XDECREF(full_data->update_data.data);
        g_free(full_data);
    }

    if (callback_return == NULL) {
        PyErr_Print();
        pyg_gil_state_release(state);
	return MATE_VFS_XFER_ERROR_ACTION_ABORT;
    }
    if (!PyInt_Check(callback_return)) {
	PyErr_SetString(PyExc_TypeError, "progress_update_callback must return an int");
        PyErr_Print();
        pyg_gil_state_release(state);
	return MATE_VFS_XFER_ERROR_ACTION_ABORT;
    }
    retval = PyInt_AsLong(callback_return);
    Py_DECREF(callback_return);
    pyg_gil_state_release(state);
    return retval;
}


static PyObject *
pygvfs_async_xfer(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "source_uri_list", "target_uri_list", "xfer_options",
			      "error_mode", "overwrite_mode",
			      "progress_update_callback", "update_callback_data",
                              "progress_sync_callback", "sync_callback_data",
                              "priority",
                              NULL };
    PyObject *py_source_uri_list, *py_target_uri_list;
    GList *source_uri_list = NULL, *target_uri_list = NULL;
    int xfer_options = -1, error_mode = -1, overwrite_mode = -1;
    PyGVFSCustomNotify *update_callback_data;
    PyGVFSCustomNotify *sync_callback_data;
    PyGVFSAsyncXferData *full_data;
    MateVFSResult result;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    MateVFSAsyncHandle *handle = NULL;

    full_data = g_new0(PyGVFSAsyncXferData, 1);
    update_callback_data = &full_data->update_data;
    sync_callback_data = &full_data->sync_data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOiiiOO|OOi:matevfs.async.xfer",
				     kwlist,
				     &py_source_uri_list, &py_target_uri_list,
				     &xfer_options, &error_mode, &overwrite_mode,
				     &update_callback_data->func, &update_callback_data->data,
				     &sync_callback_data->func, &sync_callback_data->data,
                                     &priority))
	return NULL;

    if (!_pygvfs_uri_sequence_to_glist(py_source_uri_list, &source_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "source_uri_list "
			" must be a sequence of matevfs.URI");
        g_free(full_data);
	return NULL;
    }
    if (!_pygvfs_uri_sequence_to_glist(py_target_uri_list, &target_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "target_uri_list "
			" must be a sequence of matevfs.URI");
	g_list_free(source_uri_list);
        g_free(full_data);
	return NULL;
    }

    if (!PyCallable_Check(update_callback_data->func)) {
	PyErr_SetString(PyExc_TypeError, "progress_update_callback must be callable");
	g_list_free(source_uri_list);
	g_list_free(target_uri_list);
        g_free(full_data);
	return NULL;
    }

    if (sync_callback_data->func == Py_None)
        sync_callback_data->func = NULL;
    if (!sync_callback_data->func) {
        if (error_mode == MATE_VFS_XFER_ERROR_MODE_QUERY) {
            PyErr_SetString(PyExc_ValueError, "callback is required with QUERY error mode");
            g_free(full_data);
            return NULL;
        }
    } else if (!PyCallable_Check(sync_callback_data->func)) {
	PyErr_SetString(PyExc_TypeError, "progress_sync_callback must be callable");
	g_list_free(source_uri_list);
	g_list_free(target_uri_list);
        g_free(full_data);
	return NULL;
    }

    Py_XINCREF(sync_callback_data->func);
    Py_XINCREF(update_callback_data->func);
    Py_XINCREF(sync_callback_data->data);
    Py_XINCREF(update_callback_data->data);

    result = mate_vfs_async_xfer(&handle,
                                  source_uri_list, target_uri_list,
                                  xfer_options,  error_mode, overwrite_mode,
                                  priority,
                                  pygvfs_async_xfer_progress_callback,
                                  full_data,
                                  (sync_callback_data->func?
                                   pygvfs_xfer_progress_callback : NULL),
                                  sync_callback_data);

    g_list_free(source_uri_list);
    g_list_free(target_uri_list);
    if (pymate_vfs_result_check(result))
	return NULL;

    return pymate_vfs_async_handle_new(handle);
}


static void
pygvfs_async_find_directory_callback(MateVFSAsyncHandle *handle,
                                     GList *results /* MateVFSFindDirectoryResult */,
                                     gpointer _data)
{
    PyGVFSCustomNotify *data = _data;
    PyObject *py_results, *py_handle, *callback_return;
    GList *l;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    py_results = PyList_New(0);
    for (l = results; l; l = l->next) {
        MateVFSFindDirectoryResult *res = l->data;
        PyObject *item, *py_uri;

        if (res->result == MATE_VFS_OK) {
            py_uri = pymate_vfs_uri_new(res->uri);
            mate_vfs_uri_ref(res->uri);
        } else {
            py_uri = Py_None;
            Py_INCREF(Py_None);
        }
        item = Py_BuildValue("NN", py_uri,
                             fetch_exception(res->result, NULL));
        PyList_Append(py_results, item);
        Py_DECREF(item);
    }

    py_handle = pymate_vfs_async_handle_new(handle);
    
    if (data->data)
	callback_return = PyObject_CallFunction(data->func, "NNN",
                                                py_handle, py_results,
                                                data->data);
    else
	callback_return = PyObject_CallFunction(data->func, "NN",
                                                py_handle, py_results);

    if (callback_return == NULL)
        PyErr_Print();
    else
        Py_DECREF(callback_return);

    Py_DECREF(data->func);
    g_free(data);
    pyg_gil_state_release(state);
}

static PyObject *
pygvfs_async_find_directory(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "near_uri_list", "kind", "create_if_needed",
			      "find_if_needed", "permissions",
			      "callback", "user_data",
                              "priority",
                              NULL };
    PyObject *py_near_uri_list;
    GList *near_uri_list = NULL;
    int kind, create_if_needed, find_if_needed, permissions;
    PyGVFSCustomNotify *data;
    int priority = MATE_VFS_PRIORITY_DEFAULT;
    MateVFSAsyncHandle *handle = NULL;

    data = g_new0(PyGVFSCustomNotify, 1);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OiiiiO|Oi:matevfs.async.find_directory",
				     kwlist,
				     &py_near_uri_list,
                                     &kind, &create_if_needed, &find_if_needed, &permissions,
				     &data->func, &data->data,
                                     &priority))
	return NULL;

    if (!PyCallable_Check(data->func)) {
	PyErr_SetString(PyExc_TypeError, "callback (6th argument) must be callable");
        g_free(data);
	return NULL;
    }

    if (!_pygvfs_uri_sequence_to_glist(py_near_uri_list, &near_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "near_uri_list "
			" must be a sequence of matevfs.URI");
        g_free(data);
	return NULL;
    }

    Py_INCREF(data->func);
    Py_XINCREF(data->data);

    mate_vfs_async_find_directory(&handle, near_uri_list, kind,
                                   create_if_needed, find_if_needed,
                                   permissions, priority,
                                   pygvfs_async_find_directory_callback, data);

    Py_INCREF(Py_None);
    return Py_None;
}

typedef struct {
    PyObject *func;
    PyObject *data;
    int operation_data_len;
} PyGVFSAsyncFileControlData;

static void
pygvfs_async_file_control_callback(MateVFSAsyncHandle *handle,
                                   MateVFSResult result,
                                   gpointer operation_data_,
                                   gpointer callback_data)
{
    PyGVFSAsyncFileControlData *data = callback_data;
    PyObject *py_operation_data, *py_handle, *callback_return;
    PyObject *py_result;
    PyGILState_STATE state;
    PyGVFSOperationData *operation_data = operation_data_;

    state = pyg_gil_state_ensure();

    if (operation_data->magic == PYGVFS_CONTROL_MAGIC_OUT)
        py_operation_data = operation_data->data;
    else {
        g_warning("file_control() on python-implemented methods"
                  " can only be used from python");
        py_operation_data = Py_None;
    }

    py_handle = pymate_vfs_async_handle_new(handle);
    py_result = fetch_exception(result, NULL);
    
    if (data->data)
	callback_return = PyObject_CallFunction(data->func, "NNON",
                                                py_handle, py_result,
                                                py_operation_data,
                                                data->data);
    else
	callback_return = PyObject_CallFunction(data->func, "NNO",
                                                py_handle, py_result,
                                                py_operation_data);

    if (callback_return == NULL)
        PyErr_Print();
    else
        Py_DECREF(callback_return);

    Py_DECREF(data->func);
    g_free(data);
    pyg_gil_state_release(state);
}

void
pygvfs_operation_data_free(PyGVFSOperationData *data)
{
    if (data->magic == PYGVFS_CONTROL_MAGIC_OUT) {
        PyGILState_STATE state;
        state = pyg_gil_state_ensure();
        Py_XDECREF(data->data);
        pyg_gil_state_release(state);
    }
    data->magic = 0;
    data->data = NULL;
    g_free(data);
}

static PyObject *
pygvfs_async_file_control(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "operation", "operation_data",
			      "callback", "callback_data",
                              NULL };
    char *operation;
    PyGVFSAsyncFileControlData *data;
    PyObject *operation_data_obj;
    PyGVFSOperationData *operation_data;

    data = g_new0(PyGVFSAsyncFileControlData, 1);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sOO|O:matevfs.async.file_control",
				     kwlist,
                                     &operation, &operation_data_obj,
				     &data->func, &data->data)) {
        g_free(data);
	return NULL;
    }
    if (!PyCallable_Check(data->func)) {
	PyErr_SetString(PyExc_TypeError, "callback (3rd argument) must be callable");
        g_free(data);
	return NULL;
    }

    Py_INCREF(data->func);
    Py_XINCREF(data->data);

    operation_data = g_new(PyGVFSOperationData, 1);
    operation_data->magic = PYGVFS_CONTROL_MAGIC_IN;
    Py_INCREF(operation_data_obj);
    operation_data->data = operation_data_obj;
    
    mate_vfs_async_file_control(((PyMateVFSAsyncHandle*) self)->fd,
                                 operation, operation_data,
                                 (GDestroyNotify) pygvfs_operation_data_free,
                                 pygvfs_async_file_control_callback, data);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pymatevfs_async_functions[] = {
    { "open", (PyCFunction)pygvfs_async_open, METH_VARARGS|METH_KEYWORDS }, 
    { "get_file_info", (PyCFunction)pygvfs_async_get_file_info, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "load_directory", (PyCFunction)pygvfs_async_load_directory, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "create", (PyCFunction)pygvfs_async_create, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "create_symbolic_link", 
      (PyCFunction)pygvfs_async_create_symbolic_link, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "get_job_limit", (PyCFunction)pygvfs_async_get_job_limit, 
      METH_NOARGS }, 
    { "set_job_limit", (PyCFunction)pygvfs_async_set_job_limit, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "xfer", (PyCFunction)pygvfs_async_xfer, METH_VARARGS|METH_KEYWORDS }, 
    { "find_directory", (PyCFunction)pygvfs_async_find_directory, METH_VARARGS|METH_KEYWORDS }, 
    { NULL, NULL, 0}
};

PyObject *
pygvfs_async_module_init(void)
{
    PyObject *m;
    PyObject *d;

    PyMateVFSAsyncHandle_Type.ob_type = &PyType_Type;

    if (PyType_Ready(&PyMateVFSAsyncHandle_Type) < 0)
	return NULL;

    m = Py_InitModule("matevfs.async", pymatevfs_async_functions);
    d = PyModule_GetDict(m);

    PyDict_SetItemString(d, "Handle", 
			 (PyObject *)&PyMateVFSAsyncHandle_Type);

    return m;
}

static PyMethodDef pygvhandle_methods[] = {
    { "close", (PyCFunction)pygvhandle_close, 
      METH_VARARGS|METH_KEYWORDS }, 
    { "read", (PyCFunction)pygvhandle_read, METH_VARARGS|METH_KEYWORDS }, 
    { "write", (PyCFunction)pygvhandle_write, METH_VARARGS|METH_KEYWORDS }, 
    { "is_open", (PyCFunction)pygvhandle_is_open, METH_NOARGS }, 
    { "cancel", (PyCFunction)pygvhandle_cancel, METH_NOARGS }, 
    { "control", (PyCFunction)pygvfs_async_file_control, METH_VARARGS|METH_KEYWORDS }, 
    { NULL, NULL, 0 }
};

PyTypeObject PyMateVFSAsyncHandle_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "matevfs.async.Handle",           /* tp_name */
    sizeof(PyMateVFSAsyncHandle),      /* tp_basicsize */
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
    Py_TPFLAGS_DEFAULT,   		/* tp_flags */
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
