/* -*- mode: C; c-basic-offset: 4 -*- */
#include <pygobject.h>
#include "pymatevfs-private.h"
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-address.h>
#include <libmatevfs/mate-vfs-resolve.h>
#include <libmatevfs/mate-vfs-dns-sd.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-mime.h>
#include "pymatevfsmatecomponent.h"

static GHashTable *monitor_hash;
static gint monitor_id_counter = 0;

static PyObject *pymatevfs_exc;
static PyObject *pymatevfs_not_found_exc;
static PyObject *pymatevfs_generic_exc;
static PyObject *pymatevfs_internal_exc;
static PyObject *pymatevfs_bad_parameters_exc;
static PyObject *pymatevfs_not_supported_exc;
static PyObject *pymatevfs_io_exc;
static PyObject *pymatevfs_corrupted_data_exc;
static PyObject *pymatevfs_wrong_format_exc;
static PyObject *pymatevfs_bad_file_exc;
static PyObject *pymatevfs_too_big_exc;
static PyObject *pymatevfs_no_space_exc;
static PyObject *pymatevfs_read_only_exc;
static PyObject *pymatevfs_invalid_uri_exc;
static PyObject *pymatevfs_not_open_exc;
static PyObject *pymatevfs_invalid_open_mode_exc;
static PyObject *pymatevfs_access_denied_exc;
static PyObject *pymatevfs_too_many_open_files_exc;
static PyObject *pymatevfs_eof_exc;
static PyObject *pymatevfs_not_a_directory_exc;
static PyObject *pymatevfs_in_progress_exc;
static PyObject *pymatevfs_interrupted_exc;
static PyObject *pymatevfs_file_exists_exc;
static PyObject *pymatevfs_loop_exc;
static PyObject *pymatevfs_not_permitted_exc;
static PyObject *pymatevfs_is_directory_exc;
static PyObject *pymatevfs_no_memory_exc;
static PyObject *pymatevfs_host_not_found_exc;
static PyObject *pymatevfs_invalid_host_name_exc;
static PyObject *pymatevfs_host_has_no_address_exc;
static PyObject *pymatevfs_login_failed_exc;
static PyObject *pymatevfs_cancelled_exc;
static PyObject *pymatevfs_directory_busy_exc;
static PyObject *pymatevfs_directory_not_empty_exc;
static PyObject *pymatevfs_too_many_links_exc;
static PyObject *pymatevfs_read_only_file_system_exc;
static PyObject *pymatevfs_not_same_file_system_exc;
static PyObject *pymatevfs_name_too_long_exc;
static PyObject *pymatevfs_service_not_available_exc;
static PyObject *pymatevfs_service_obsolete_exc;
static PyObject *pymatevfs_protocol_error_exc;
static PyObject *pymatevfs_no_master_browser_exc;
#if 0
static PyObject *pymatevfs_no_default_exc;
static PyObject *pymatevfs_no_handler_exc;
static PyObject *pymatevfs_parse_exc;
static PyObject *pymatevfs_launch_exc;
#endif

static PyTypeObject *_PyGObject_Type;
#define PyGObject_Type (*_PyGObject_Type)


extern void pygvvolume_add_constants(PyObject *m);

static void
pygvfs_lazy_load_pymatevfsmatecomponent(void)
{
    static PyObject *pymatevfs_module = NULL;
    if (!pymatevfs_module)
        pymatevfs_module = pymate_vfs_matecomponent_init();
}


gboolean
pymate_vfs_result_check(MateVFSResult result)
{
    PyObject *exc;

    /* be optimistic */
    if(result == MATE_VFS_OK)
	return FALSE;

    switch(result)
    {
    case MATE_VFS_ERROR_NOT_FOUND:
	exc = pymatevfs_not_found_exc;
	break;
    case MATE_VFS_ERROR_GENERIC:
	exc = pymatevfs_generic_exc;
	break;
    case MATE_VFS_ERROR_INTERNAL:
	exc = pymatevfs_internal_exc;
	break;
    case MATE_VFS_ERROR_BAD_PARAMETERS:
	exc = pymatevfs_bad_parameters_exc;
	break;
    case MATE_VFS_ERROR_NOT_SUPPORTED:
	exc = pymatevfs_not_supported_exc;
	break;
    case MATE_VFS_ERROR_IO:
	exc = pymatevfs_io_exc;
	break;
    case MATE_VFS_ERROR_CORRUPTED_DATA:
	exc = pymatevfs_corrupted_data_exc;
	break;
    case MATE_VFS_ERROR_WRONG_FORMAT:
	exc = pymatevfs_wrong_format_exc;
	break;
    case MATE_VFS_ERROR_BAD_FILE:
	exc = pymatevfs_bad_file_exc;
	break;
    case MATE_VFS_ERROR_TOO_BIG:
	exc = pymatevfs_too_big_exc;
	break;
    case MATE_VFS_ERROR_NO_SPACE:
	exc = pymatevfs_no_space_exc;
	break;
    case MATE_VFS_ERROR_READ_ONLY:
	exc = pymatevfs_read_only_exc;
	break;
    case MATE_VFS_ERROR_INVALID_URI:
	exc = pymatevfs_invalid_uri_exc;
	break;
    case MATE_VFS_ERROR_NOT_OPEN:
	exc = pymatevfs_not_open_exc;
	break;
    case MATE_VFS_ERROR_INVALID_OPEN_MODE:
	exc = pymatevfs_invalid_open_mode_exc;
	break;
    case MATE_VFS_ERROR_ACCESS_DENIED:
	exc = pymatevfs_access_denied_exc;
	break;
    case MATE_VFS_ERROR_TOO_MANY_OPEN_FILES:
	exc = pymatevfs_too_many_open_files_exc;
	break;
    case MATE_VFS_ERROR_EOF:
	exc = pymatevfs_eof_exc;
	break;
    case MATE_VFS_ERROR_NOT_A_DIRECTORY:
	exc = pymatevfs_not_a_directory_exc;
	break;
    case MATE_VFS_ERROR_IN_PROGRESS:
	exc = pymatevfs_in_progress_exc;
	break;
    case MATE_VFS_ERROR_INTERRUPTED:
	exc = pymatevfs_interrupted_exc;
	break;
    case MATE_VFS_ERROR_FILE_EXISTS:
	exc = pymatevfs_file_exists_exc;
	break;
    case MATE_VFS_ERROR_LOOP:
	exc = pymatevfs_loop_exc;
	break;
    case MATE_VFS_ERROR_NOT_PERMITTED:
	exc = pymatevfs_not_permitted_exc;
	break;
    case MATE_VFS_ERROR_IS_DIRECTORY:
	exc = pymatevfs_is_directory_exc;
	break;
    case MATE_VFS_ERROR_NO_MEMORY:
	exc = pymatevfs_no_memory_exc;
	break;
    case MATE_VFS_ERROR_HOST_NOT_FOUND:
	exc = pymatevfs_host_not_found_exc;
	break;
    case MATE_VFS_ERROR_INVALID_HOST_NAME:
	exc = pymatevfs_invalid_host_name_exc;
	break;
    case MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS:
	exc = pymatevfs_host_has_no_address_exc;
	break;
    case MATE_VFS_ERROR_LOGIN_FAILED:
	exc = pymatevfs_login_failed_exc;
	break;
    case MATE_VFS_ERROR_CANCELLED:
	exc = pymatevfs_cancelled_exc;
	break;
    case MATE_VFS_ERROR_DIRECTORY_BUSY:
	exc = pymatevfs_directory_busy_exc;
	break;
    case MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY:
	exc = pymatevfs_directory_not_empty_exc;
	break;
    case MATE_VFS_ERROR_TOO_MANY_LINKS:
	exc = pymatevfs_too_many_links_exc;
	break;
    case MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
	exc = pymatevfs_read_only_file_system_exc;
	break;
    case MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM:
	exc = pymatevfs_not_same_file_system_exc;
	break;
    case MATE_VFS_ERROR_NAME_TOO_LONG:
	exc = pymatevfs_name_too_long_exc;
	break;
    case MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE:
	exc = pymatevfs_service_not_available_exc;
	break;
    case MATE_VFS_ERROR_SERVICE_OBSOLETE:
	exc = pymatevfs_service_obsolete_exc;
	break;
    case MATE_VFS_ERROR_PROTOCOL_ERROR:
	exc = pymatevfs_protocol_error_exc;
	break;
    case MATE_VFS_ERROR_NO_MASTER_BROWSER:
	exc = pymatevfs_no_master_browser_exc;
	break;
#if 0	
    case MATE_VFS_ERROR_NO_DEFAULT:
	exc = pymatevfs_no_default_exc;
	break;
    case MATE_VFS_ERROR_NO_HANDLER:
	exc = pymatevfs_no_handler_exc;
	break;
    case MATE_VFS_ERROR_PARSE:
	exc = pymatevfs_parse_exc;
	break;
    case MATE_VFS_ERROR_LAUNCH:
	exc = pymatevfs_launch_exc;
	break;
#endif	
    default:
	exc = NULL;
	break;
    }


    if(exc)
    {
	const char *msg;
	msg = mate_vfs_result_to_string(result);
	PyErr_SetString(exc, (char*)msg);
	return TRUE;
    }

    return FALSE;
}


MateVFSResult pymate_vfs_exception_check(void)
{
    if (!PyErr_Occurred()) {
	return -1;
    }
    
    if (PyErr_ExceptionMatches(pymatevfs_not_found_exc)) {
	return MATE_VFS_ERROR_NOT_FOUND;
    } else if (PyErr_ExceptionMatches(pymatevfs_generic_exc)) {
	return MATE_VFS_ERROR_GENERIC;
    } else if (PyErr_ExceptionMatches(pymatevfs_internal_exc)) {
	return MATE_VFS_ERROR_INTERNAL;
    } else if (PyErr_ExceptionMatches(pymatevfs_bad_parameters_exc)) {
	return MATE_VFS_ERROR_BAD_FILE;
    } else if (PyErr_ExceptionMatches(pymatevfs_not_supported_exc)) {
	return MATE_VFS_ERROR_NOT_SUPPORTED;
    } else if (PyErr_ExceptionMatches(pymatevfs_io_exc)) {
	return MATE_VFS_ERROR_IO;
    } else if (PyErr_ExceptionMatches(pymatevfs_corrupted_data_exc)) {
	return MATE_VFS_ERROR_CORRUPTED_DATA;
    } else if (PyErr_ExceptionMatches(pymatevfs_wrong_format_exc)) {
	return MATE_VFS_ERROR_WRONG_FORMAT;
    } else if (PyErr_ExceptionMatches(pymatevfs_bad_file_exc)) {
	return MATE_VFS_ERROR_BAD_FILE;
    } else if (PyErr_ExceptionMatches(pymatevfs_too_big_exc)) {
	return MATE_VFS_ERROR_TOO_BIG;
    } else if (PyErr_ExceptionMatches(pymatevfs_no_space_exc)) {
	return MATE_VFS_ERROR_NO_SPACE;
    } else if (PyErr_ExceptionMatches(pymatevfs_read_only_exc)) {
	return MATE_VFS_ERROR_READ_ONLY;
    } else if (PyErr_ExceptionMatches(pymatevfs_invalid_uri_exc)) {
	return MATE_VFS_ERROR_INVALID_URI;
    } else if (PyErr_ExceptionMatches(pymatevfs_not_open_exc)) {
	return MATE_VFS_ERROR_NOT_OPEN;
    } else if (PyErr_ExceptionMatches(pymatevfs_invalid_open_mode_exc)) {
	return MATE_VFS_ERROR_INVALID_OPEN_MODE;
    } else if (PyErr_ExceptionMatches(pymatevfs_access_denied_exc)) {
	return MATE_VFS_ERROR_ACCESS_DENIED;
    } else if (PyErr_ExceptionMatches(pymatevfs_too_many_open_files_exc)) {
	return MATE_VFS_ERROR_TOO_MANY_OPEN_FILES;
    } else if (PyErr_ExceptionMatches(pymatevfs_eof_exc)) {
	return MATE_VFS_ERROR_EOF;
    } else if (PyErr_ExceptionMatches(pymatevfs_not_a_directory_exc)) {
	return MATE_VFS_ERROR_NOT_A_DIRECTORY;
    } else if (PyErr_ExceptionMatches(pymatevfs_in_progress_exc)) {
	return MATE_VFS_ERROR_IN_PROGRESS;
    } else if (PyErr_ExceptionMatches(pymatevfs_interrupted_exc)) {
	return MATE_VFS_ERROR_INTERRUPTED;
    } else if (PyErr_ExceptionMatches(pymatevfs_file_exists_exc)) {
	return MATE_VFS_ERROR_FILE_EXISTS;
    } else if (PyErr_ExceptionMatches(pymatevfs_loop_exc)) {
	return MATE_VFS_ERROR_LOOP;
    } else if (PyErr_ExceptionMatches(pymatevfs_not_permitted_exc)) {
	return MATE_VFS_ERROR_NOT_PERMITTED;
    } else if (PyErr_ExceptionMatches(pymatevfs_is_directory_exc)) {
	return MATE_VFS_ERROR_IS_DIRECTORY;
    } else if (PyErr_ExceptionMatches(pymatevfs_no_memory_exc)) {
	return MATE_VFS_ERROR_NO_MEMORY;
    } else if (PyErr_ExceptionMatches(pymatevfs_host_not_found_exc)) {
	return MATE_VFS_ERROR_HOST_NOT_FOUND;
    } else if (PyErr_ExceptionMatches(pymatevfs_invalid_host_name_exc)) {
	return MATE_VFS_ERROR_INVALID_HOST_NAME;
    } else if (PyErr_ExceptionMatches(pymatevfs_host_has_no_address_exc)) {
	return MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS;
    } else if (PyErr_ExceptionMatches(pymatevfs_login_failed_exc)) {
	return MATE_VFS_ERROR_LOGIN_FAILED;
    } else if (PyErr_ExceptionMatches(pymatevfs_cancelled_exc)) {
	return MATE_VFS_ERROR_CANCELLED;
    } else if (PyErr_ExceptionMatches(pymatevfs_directory_busy_exc)) {
	return MATE_VFS_ERROR_DIRECTORY_BUSY;
    } else if (PyErr_ExceptionMatches(pymatevfs_directory_not_empty_exc)) {
	return MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY;
    } else if (PyErr_ExceptionMatches(pymatevfs_too_many_links_exc)) {
	return MATE_VFS_ERROR_TOO_MANY_LINKS;
    } else if (PyErr_ExceptionMatches(pymatevfs_read_only_file_system_exc)) {
	return MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM;
    } else if (PyErr_ExceptionMatches(pymatevfs_not_same_file_system_exc)) {
	return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
    } else if (PyErr_ExceptionMatches(pymatevfs_name_too_long_exc)) {
	return MATE_VFS_ERROR_NAME_TOO_LONG;
    } else if (PyErr_ExceptionMatches(pymatevfs_service_not_available_exc)) {
	return MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;
    } else if (PyErr_ExceptionMatches(pymatevfs_service_obsolete_exc)) {
	return MATE_VFS_ERROR_SERVICE_OBSOLETE;
    } else if (PyErr_ExceptionMatches(pymatevfs_protocol_error_exc)) {
	return MATE_VFS_ERROR_PROTOCOL_ERROR;
    } else if (PyErr_ExceptionMatches(pymatevfs_no_master_browser_exc)) {
	return MATE_VFS_ERROR_NO_MASTER_BROWSER;
    }
    
    return -2;
}

static PyObject *
pygvfs_create(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "open_mode", "exclusive", "perm", NULL };
    PyObject *uri;
    MateVFSOpenMode open_mode = MATE_VFS_OPEN_NONE;
    gboolean exclusive = FALSE;
    guint perm = 0666;
    MateVFSHandle *handle;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii:matevfs.create",
				     kwlist, &uri, &open_mode, &exclusive,
				     &perm))
	return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_create_uri(&handle, pymate_vfs_uri_get(uri),
				      open_mode, exclusive, perm);
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_create(&handle, PyString_AsString(uri),
				  open_mode, exclusive, perm);
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }

    if (pymate_vfs_result_check(result))
	return NULL;

    return pymate_vfs_handle_new(handle);
}

static PyObject *
pygvfs_get_file_info(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "options", NULL };
    PyObject *uri;
    MateVFSFileInfo *finfo;
    MateVFSFileInfoOptions options = MATE_VFS_FILE_INFO_DEFAULT;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O|i:matevfs.get_file_info",
				     kwlist, &uri, &options))
	return NULL;

    finfo = mate_vfs_file_info_new();
    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_get_file_info_uri(pymate_vfs_uri_get(uri), finfo,
					     options);
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_get_file_info(PyString_AsString(uri), finfo,
					 options);
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	mate_vfs_file_info_unref(finfo);
	return NULL;
    }

    if (pymate_vfs_result_check(result)) {
	mate_vfs_file_info_unref(finfo);
	return NULL;
    }
    return pymate_vfs_file_info_new(finfo);
}

static PyObject *
pygvfs_set_file_info(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "info", "mask", NULL };
    PyObject *uri;
    PyMateVFSFileInfo *finfo;
    MateVFSSetFileInfoMask mask = MATE_VFS_SET_FILE_INFO_NONE;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "OO!i:matevfs.set_file_info",
				     kwlist, &uri,
                                     &PyMateVFSFileInfo_Type, &finfo,
                                     &mask))
	return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_set_file_info_uri(pymate_vfs_uri_get(uri),
                                             finfo->finfo, mask);
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_set_file_info(PyString_AsString(uri),
                                         finfo->finfo, mask);
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }
    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_truncate(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "length", NULL };
    PyObject *uri, *py_length;
    MateVFSFileSize length;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:matevfs.truncate",
				     kwlist, &uri, &py_length))
	return NULL;

    length = PyLong_Check(py_length) ? PyLong_AsUnsignedLongLong(py_length)
	: PyInt_AsLong(py_length);
    if (PyErr_Occurred()) return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type))
	result = mate_vfs_truncate_uri(pymate_vfs_uri_get(uri), length);
    else if (PyString_Check(uri))
	result = mate_vfs_truncate(PyString_AsString(uri), length);
    else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }

    if (pymate_vfs_result_check(result))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_create_symbolic_link(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "target", NULL };
    PyObject *uri;
    char *target;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "Os:matevfs.create_symbolic_link", kwlist,
                                     &uri, &target))
        return NULL;
    
    if (!PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        PyErr_SetString(PyExc_TypeError, "uri must be a matevfs.URI");
        return NULL;
    }

    pyg_begin_allow_threads;
    result = mate_vfs_create_symbolic_link(pymate_vfs_uri_get(uri), target);
    pyg_end_allow_threads;
        
    if (pymate_vfs_result_check(result))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_make_directory(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "perm", NULL };
    PyObject *uri;
    gint perm;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "Oi:matevfs.make_directory", kwlist,
				     &uri, &perm))
	return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_make_directory_for_uri(pymate_vfs_uri_get(uri),
						  perm);
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_make_directory(PyString_AsString(uri), perm);
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }

    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_remove_directory(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", NULL };
    PyObject *uri;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O:matevfs.remove_directory", kwlist,
				     &uri))
	return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_remove_directory_from_uri(pymate_vfs_uri_get(uri));
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_remove_directory(PyString_AsString(uri));
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }

    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_unlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", NULL };
    PyObject *uri;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O:matevfs.unlink", kwlist, &uri))
	return NULL;

    if (PyObject_TypeCheck(uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	result = mate_vfs_unlink_from_uri(pymate_vfs_uri_get(uri));
        pyg_end_allow_threads;
    } else if (PyString_Check(uri)) {
        pyg_begin_allow_threads;
	result = mate_vfs_unlink(PyString_AsString(uri));
        pyg_end_allow_threads;
    } else {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }

    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_exists(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", NULL };
    PyObject *py_uri;
    MateVFSURI *uri = NULL;
    gboolean exists;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:matevfs.exists",
				     kwlist, &py_uri))
	return NULL;

    if (PyObject_TypeCheck(py_uri, &PyMateVFSURI_Type)) {
        pyg_begin_allow_threads;
	uri = mate_vfs_uri_ref(pymate_vfs_uri_get(py_uri));
        pyg_end_allow_threads;
    } else if (PyString_Check(py_uri)) {
        pyg_begin_allow_threads;
	uri = mate_vfs_uri_new(PyString_AsString(py_uri));
        pyg_end_allow_threads;
    }
    if (!uri) {
	PyErr_SetString(PyExc_TypeError,
			"uri must be a matevfs.URI or a string");
	return NULL;
    }
    exists = mate_vfs_uri_exists(uri);
    mate_vfs_uri_unref(uri);

    return PyInt_FromLong(exists);
}

static PyObject *
pygvfs_get_file_mime_type(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"path", "fast", "suffix_only", NULL };
    const char *path;
    const char *mime;
    gboolean fast = TRUE;
    gboolean suffix = FALSE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                    "s|ii:matevfs.get_file_mime_type",
                                    kwlist,
                                    &path, &fast, &suffix))
    return NULL;

    if (fast)
        mime = mate_vfs_get_file_mime_type_fast(path, NULL);
    else
        mime = mate_vfs_get_file_mime_type(path, NULL, suffix);

    if (mime == NULL) {
        Py_XINCREF(Py_None);
        return Py_None;
    }

    return PyString_FromString(mime);
}

static PyObject *
pygvfs_get_mime_type(PyObject *self, PyObject *args)
{
    char *text_uri, *mime;
    
    if(!PyArg_ParseTuple(args, "s:matevfs.get_mime_type",
			 &text_uri))
	return NULL;

    pyg_begin_allow_threads;
    mime = mate_vfs_get_mime_type(text_uri);
    pyg_end_allow_threads;
    if (mime)
	return PyString_FromString(mime);
    else {
	PyErr_SetString(PyExc_RuntimeError,
			"there was an error reading the file");
	return NULL;
    }
}

static PyObject *
pygvfs_get_mime_type_for_data(PyObject *self, PyObject *args)
{
    char *data;
    char const *mime;
    Py_ssize_t data_size, data_size1 = PY_SSIZE_T_MIN;
    
    if(!PyArg_ParseTuple(args, "s#|i:matevfs.get_mime_type_for_data",
			 &data, &data_size, &data_size1))
	return NULL;
    if (data_size1 != PY_SSIZE_T_MIN)
	PyErr_Warn(PyExc_DeprecationWarning, "ignoring deprecated argument data_size");
    pyg_begin_allow_threads;
    mime = mate_vfs_get_mime_type_for_data(data, data_size);
    pyg_end_allow_threads;
    if (mime)
	return PyString_FromString(mime);
    else {
	PyErr_SetString(PyExc_RuntimeError,
			"there was an error reading the file");
	return NULL;
    }
}

static PyObject *
pygvfs_mime_get_icon(PyObject *self, PyObject *args)
{
    char *mime_type;
    const char *retval;
        
    if(!PyArg_ParseTuple(args, "s:matevfs.mime_get_icon",
			 &mime_type))
	return NULL;
        
    retval = mate_vfs_mime_get_icon(mime_type);
    if (retval == NULL) {
            Py_INCREF(Py_None);
            return Py_None;
    }
    return PyString_FromString(retval);
}

static PyObject *
pygvfs_mime_set_icon(PyObject *self, PyObject *args)
{
    char *mime_type, *filename;
    MateVFSResult result;
        
    if(!PyArg_ParseTuple(args, "ss:matevfs.mime_set_icon",
			 &mime_type, &filename))
	return NULL;

    result = mate_vfs_mime_set_icon(mime_type, filename);
    if (pymate_vfs_result_check(result)) {
	return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_get_description(PyObject *self, PyObject *args)
{
    char *mime_type;
    const char *description;
    
    if(!PyArg_ParseTuple(args, "s:matevfs.mime_get_description",
			 &mime_type))
	return NULL;
    description = mate_vfs_mime_get_description(mime_type);
    if (description)
	return PyString_FromString(description);
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}

static PyObject *
pygvfs_mime_set_description(PyObject *self, PyObject *args)
{
    char *mime_type, *description;
    MateVFSResult result;
        
    if(!PyArg_ParseTuple(args, "ss:matevfs.mime_set_description",
			 &mime_type, &description))
	return NULL;

    result = mate_vfs_mime_set_description(mime_type, description);
    if (pymate_vfs_result_check(result)) {
	return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_can_be_executable(PyObject *self, PyObject *args)
{
    char *mime_type;
    
    if(!PyArg_ParseTuple(args, "s:matevfs.mime_can_be_executable",
			 &mime_type))
	return NULL;

    return PyInt_FromLong(mate_vfs_mime_can_be_executable(mime_type));
}

static PyObject *
pygvfs_mime_set_can_be_executable(PyObject *self, PyObject *args)
{
    char *mime_type;
    gboolean new_value;
    MateVFSResult result;
        
    if(!PyArg_ParseTuple(args, "si:matevfs.mime_set_description",
			 &mime_type, &new_value))
	return NULL;

    result = mate_vfs_mime_set_can_be_executable(mime_type, new_value);
    if (pymate_vfs_result_check(result)) {
	return NULL;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static void
pygvfs_monitor_marshal(MateVFSMonitorHandle *handle,
		       const gchar *monitor_uri,
		       const gchar *info_uri,
		       MateVFSMonitorEventType event_type,
		       PyGVFSCustomNotify *cunote)
{
    PyObject *retobj;
    PyGILState_STATE state;
    
    state = pyg_gil_state_ensure();

    if (cunote->data)
	retobj = PyEval_CallFunction(cunote->func, "(ssiO)", monitor_uri,
				     info_uri, event_type, cunote->data);
    else
	retobj = PyObject_CallFunction(cunote->func, "(ssi)", monitor_uri,
				       info_uri, event_type);

    if (retobj == NULL) {
	PyErr_Print();
	PyErr_Clear();
    }
    
    Py_XDECREF(retobj);
    
    pyg_gil_state_release(state);
}

static PyObject*
pygvfs_monitor_add(PyObject *self, PyObject *args)
{
    char *text_uri;
    int monitor_type;
    PyObject *callback;
    PyObject *extra = NULL;
    PyGVFSCustomNotify *cunote;
    MateVFSMonitorHandle *handle;
    MateVFSResult result;
    gint monitor_id;
    
    if (!PyArg_ParseTuple(args, "siO|O:matevfs.monitor_add",
			  &text_uri, &monitor_type,
			  &callback, &extra)) {
	return NULL;
    }
    
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "third argument not callable");
        return NULL;
    }
    
    cunote = g_new0(PyGVFSCustomNotify, 1);
    cunote->func = callback;
    cunote->data = extra;
    Py_INCREF(cunote->func);
    Py_XINCREF(cunote->data);
    pyg_begin_allow_threads;
    result = mate_vfs_monitor_add(&handle, text_uri, monitor_type,
		   (MateVFSMonitorCallback)pygvfs_monitor_marshal,
		   cunote);
    pyg_end_allow_threads;

    if (pymate_vfs_result_check(result)) {
	return NULL;
    }

    do
        monitor_id = ++monitor_id_counter;
    while (g_hash_table_lookup(monitor_hash, GINT_TO_POINTER(monitor_id)));

    g_hash_table_insert(monitor_hash,
			GINT_TO_POINTER(monitor_id),
			handle);
    
    return PyInt_FromLong(monitor_id);
}

static PyObject*
pygvfs_monitor_cancel(PyObject *self, PyObject *args)
{
    gint monitor_id;
    MateVFSMonitorHandle *handle;
    
    if (!PyArg_ParseTuple(args, "i:matevfs.monitor_cancel",
			  &monitor_id)) {
	return NULL;
    }
    
    handle = g_hash_table_lookup(monitor_hash, GINT_TO_POINTER(monitor_id));
    if (handle == NULL) {
	PyErr_SetString(PyExc_ValueError, "Invalid monitor id");
	return NULL;
    }

    mate_vfs_monitor_cancel(handle);
    g_hash_table_remove(monitor_hash, GINT_TO_POINTER(monitor_id));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_read_entire_file(PyObject *self, PyObject *args)
{
    MateVFSResult  result;
    char           *uri;
    char           *file_contents;
    int             file_size;
    PyObject       *rv;

    if (!PyArg_ParseTuple(args, "s:matevfs.read_entire_file", &uri))
	return NULL;

    pyg_begin_allow_threads;
    result = mate_vfs_read_entire_file (uri, &file_size, &file_contents);
    pyg_end_allow_threads;

    if (pymate_vfs_result_check(result))
	return NULL;
    rv = PyString_FromStringAndSize(file_contents, file_size);
    g_free(file_contents);
    return rv;
}

static PyObject *
pygvfs_mime_application_new(MateVFSMimeApplication *mimeapp)
{
    PyObject *uri_schemes;
    GList *l;
    int i;

    uri_schemes = PyList_New(g_list_length(mimeapp->supported_uri_schemes));
    for (i = 0, l = mimeapp->supported_uri_schemes; l; ++i, l = l->next)
	PyList_SET_ITEM(uri_schemes, i, PyString_FromString((const char *) l->data));

    return Py_BuildValue("sssOiNO", mimeapp->id, mimeapp->name,
			 mimeapp->command,
			 mimeapp->can_open_multiple_files? Py_True : Py_False, 
			 mimeapp->expects_uris,
			 uri_schemes,
			 mimeapp->requires_terminal? Py_True : Py_False);
}

static PyObject *
pygvfs_mime_get_default_application(PyObject *self, PyObject *args)
{
    const char *mime_type;
    MateVFSMimeApplication *mimeapp;
    PyObject *retval;

    if(!PyArg_ParseTuple(args, "s:matevfs.mime_get_default_application",
			 &mime_type))
	return NULL;
    if (!(mimeapp = mate_vfs_mime_get_default_application(mime_type))) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    retval = pygvfs_mime_application_new(mimeapp);
    mate_vfs_mime_application_free(mimeapp);
    return retval;
}


gint
pygvfs_xfer_progress_callback(MateVFSXferProgressInfo *info, gpointer _data)
{
    PyGVFSCustomNotify *data = _data;
    PyObject *py_info, *callback_return;
    gint retval;
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();
    py_info = pymate_vfs_xfer_progress_info_new(info);
    if (data->data)
	callback_return = PyObject_CallFunction(data->func, "OO", py_info, data->data);
    else
	callback_return = PyObject_CallFunction(data->func, "O", py_info);

      /* because the programmer may keep a reference to the
       * VFSXferProgressInfo python wrapper but we don't own the
       * VFSXferProgressInfo itself, we remove the link from the
       * python wrapper to the C structure */
    pymate_vfs_xfer_progress_info_set(py_info, NULL);

    Py_DECREF(py_info);

    if (callback_return == NULL) {
        PyErr_Print();
        pyg_gil_state_release(state);
	return MATE_VFS_XFER_ERROR_ACTION_ABORT;
    }
    if (!PyInt_Check(callback_return)) {
	PyErr_SetString(PyExc_TypeError, "progress callback must return an int");
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
pygvfs_xfer_uri(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "source_uri", "target_uri", "xfer_options",
			      "error_mode", "overwrite_mode",
			      "progress_callback", "data", NULL };
    PyObject *source_uri, *target_uri;
    int xfer_options = -1, error_mode = -1, overwrite_mode = -1;
    PyGVFSCustomNotify custom_data = {NULL, NULL};
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!O!iii|OO:matevfs.xfer_uri",
				     kwlist,
				     &PyMateVFSURI_Type, &source_uri,
				     &PyMateVFSURI_Type, &target_uri,
				     &xfer_options, &error_mode, &overwrite_mode,
				     &custom_data.func, &custom_data.data))
	return NULL;

    if (custom_data.func == Py_None)
        custom_data.func = NULL;
    if (!custom_data.func) {
        if (error_mode == MATE_VFS_XFER_ERROR_MODE_QUERY) {
            PyErr_SetString(PyExc_ValueError, "callback is required with QUERY error mode");
            return NULL;
        }
    } else if (!PyCallable_Check(custom_data.func)) {
	PyErr_SetString(PyExc_TypeError, "progress_callback must be callable");
	return NULL;
    }

    pyg_begin_allow_threads;
    result = mate_vfs_xfer_uri(pymate_vfs_uri_get(source_uri),
				pymate_vfs_uri_get(target_uri),
				xfer_options,  error_mode, overwrite_mode,
				(custom_data.func?
                                 pygvfs_xfer_progress_callback : NULL),
                                &custom_data);
    pyg_end_allow_threads;
    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

gboolean
_pygvfs_uri_sequence_to_glist(PyObject *seq, GList **list)
{
    int len, i;
    PyObject *item;

    if (!PySequence_Check(seq))
	return FALSE;
    *list = NULL;
    len = PySequence_Length(seq);
    for (i = 0; i < len; ++i) {
	item = PySequence_GetItem(seq, i);
	if (!PyObject_TypeCheck(item, &PyMateVFSURI_Type)) {
	    Py_DECREF(item);
	    if (*list)
		g_list_free(*list);
	    return FALSE;
	}
	*list = g_list_append(*list, pymate_vfs_uri_get(item));
	Py_DECREF(item);
    }
    return TRUE;
}

static PyObject *
pygvfs_xfer_uri_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "source_uri_list", "target_uri_list", "xfer_options",
			      "error_mode", "overwrite_mode",
			      "progress_callback", "data", NULL };
    PyObject *py_source_uri_list, *py_target_uri_list;
    GList *source_uri_list = NULL, *target_uri_list = NULL;
    int xfer_options = -1, error_mode = -1, overwrite_mode = -1;
    PyGVFSCustomNotify custom_data = {NULL, NULL};
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OOiii|OO:matevfs.xfer_uri_list",
				     kwlist,
				     &py_source_uri_list, &py_target_uri_list,
				     &xfer_options, &error_mode, &overwrite_mode,
				     &custom_data.func, &custom_data.data))
	return NULL;

    if (!_pygvfs_uri_sequence_to_glist(py_source_uri_list, &source_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "source_uri_list "
			" must be a sequence of matevfs.URI");
	return NULL;
    }
    if (!_pygvfs_uri_sequence_to_glist(py_target_uri_list, &target_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "target_uri_list "
			" must be a sequence of matevfs.URI");
	g_list_free(source_uri_list);
	return NULL;
    }

    if (custom_data.func == Py_None)
        custom_data.func = NULL;
    if (!custom_data.func) {
        if (error_mode == MATE_VFS_XFER_ERROR_MODE_QUERY) {
            PyErr_SetString(PyExc_ValueError, "callback is required with QUERY error mode");
            return NULL;
        }
    } else if (!PyCallable_Check(custom_data.func)) {
	PyErr_SetString(PyExc_TypeError, "progress_callback must be callable");
	g_list_free(source_uri_list);
	g_list_free(target_uri_list);
	return NULL;
    }

    pyg_begin_allow_threads;
    result = mate_vfs_xfer_uri_list(source_uri_list, target_uri_list,
				     xfer_options,  error_mode, overwrite_mode,
				     (custom_data.func?
                                      pygvfs_xfer_progress_callback : NULL),
                                     &custom_data);
    pyg_end_allow_threads;

    g_list_free(source_uri_list);
    g_list_free(target_uri_list);
    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_xfer_delete_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "delete_uri_list",
			      "xfer_options", "error_mode",
			      "progress_callback", "data", NULL };
    PyObject *py_delete_uri_list;
    GList *delete_uri_list = NULL;
    int xfer_options = -1, error_mode = -1;
    PyGVFSCustomNotify custom_data = {NULL, NULL};
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oii|OO:matevfs.xfer_delete_list",
				     kwlist,
				     &py_delete_uri_list,
				     &error_mode, &xfer_options,
				     &custom_data.func, &custom_data.data))
	return NULL;

    if (!_pygvfs_uri_sequence_to_glist(py_delete_uri_list, &delete_uri_list)) {
	PyErr_SetString(PyExc_TypeError, "delete_uri_list "
			" must be a sequence of matevfs.URI");
	return NULL;
    }

    if (custom_data.func == Py_None)
        custom_data.func = NULL;
    if (!custom_data.func) {
        if (error_mode == MATE_VFS_XFER_ERROR_MODE_QUERY) {
            PyErr_SetString(PyExc_ValueError, "callback is required with QUERY error mode");
            return NULL;
        }
    } else if (!PyCallable_Check(custom_data.func)) {
	PyErr_SetString(PyExc_TypeError, "progress_callback must be callable");
	g_list_free(delete_uri_list);
	return NULL;
    }

    pyg_begin_allow_threads;
    result = mate_vfs_xfer_delete_list(delete_uri_list,
					error_mode, xfer_options,
					(custom_data.func?
                                         pygvfs_xfer_progress_callback : NULL),
                                        &custom_data);
    pyg_end_allow_threads;

    g_list_free(delete_uri_list);
    if (pymate_vfs_result_check(result))
	return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_get_default_action_type(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:matevfs.mime_get_default_action_type",
				     kwlist, &mime_type))
	return NULL;
    return PyInt_FromLong(mate_vfs_mime_get_default_action_type(mime_type));
}

static PyObject *
pygvfs_mime_action_new(MateVFSMimeAction *action)
{
    switch (action->action_type)
    {
    case MATE_VFS_MIME_ACTION_TYPE_NONE:
	return Py_BuildValue("(iO)", action->action_type, Py_None);

    case MATE_VFS_MIME_ACTION_TYPE_APPLICATION:
	return Py_BuildValue("(iN)", action->action_type,
			     pygvfs_mime_application_new(action->action.application));

    case MATE_VFS_MIME_ACTION_TYPE_COMPONENT: {
        pygvfs_lazy_load_pymatevfsmatecomponent();
        return pymatevfs_matecomponent_mime_component_action_new(action);
    }
    default:
	PyErr_SetString(PyExc_ValueError, "unknown action type returned");
	return NULL;
    }
}

static PyObject *
pygvfs_mime_get_default_action(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    MateVFSMimeAction *action;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:matevfs.mime_get_default_action",
				     kwlist, &mime_type))
	return NULL;
    if (!(action = mate_vfs_mime_get_default_action(mime_type))) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    retval = pygvfs_mime_action_new(action);
    mate_vfs_mime_action_free(action);
    return retval;
}


static PyObject *
pygvfs_mime_applications_list_new(GList *list)
{
    PyObject *retval;
    PyObject *py_app;
    guint i, len = g_list_length(list);
    
    retval = PyList_New(len);
    for (i = 0; list; ++i, list = list->next) {
	g_assert(i < len);
	py_app = pygvfs_mime_application_new((MateVFSMimeApplication *) list->data);
	PyList_SET_ITEM(retval, i, py_app);
    }
    return retval;
}

static PyObject *
pygvfs_mime_get_short_list_applications(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    GList *list;
    PyObject *py_list;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:matevfs.mime_get_short_list_applications",
				     kwlist, &mime_type))
	return NULL;
    list = mate_vfs_mime_get_short_list_applications(mime_type);
    py_list = pygvfs_mime_applications_list_new(list);
    mate_vfs_mime_application_list_free(list);
    return py_list;
}

static PyObject *
pygvfs_mime_get_all_applications(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", NULL };
    char *mime_type;
    GList *list;
    PyObject *py_list;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:matevfs.mime_get_all_applications",
				     kwlist, &mime_type))
	return NULL;
    list = mate_vfs_mime_get_all_applications(mime_type);
    py_list = pygvfs_mime_applications_list_new(list);
    mate_vfs_mime_application_list_free(list);
    return py_list;
}

static PyObject *
pygvfs_mime_set_default_action_type(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "action_type", NULL };
    char *mime_type;
    MateVFSMimeActionType action_type;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "si:matevfs.mime_set_default_action_type",
				     kwlist, &mime_type, &action_type))
	return NULL;
    result = mate_vfs_mime_set_default_action_type(mime_type, action_type);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_set_default_application(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_id", NULL };
    char *mime_type, *application_id;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_set_default_application",
				     kwlist, &mime_type, &application_id))
	return NULL;
    result = mate_vfs_mime_set_default_application(mime_type, application_id);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_set_default_component(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "component_iid", NULL };
    char *mime_type, *component_iid;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_set_default_component",
				     kwlist, &mime_type, &component_iid))
	return NULL;
    result = mate_vfs_mime_set_default_component(mime_type, component_iid);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static int
string_list_converter(PyObject *in, void *out)
{
    guint len, i;
    GList *list = NULL;
    PyObject *item;

    if (!PySequence_Check(in)) {
	PyErr_SetString(PyExc_TypeError, "argument must be a sequence");
	return 0;
    }
    len = PySequence_Length(in);
    for (i = 0; i < len; ++i) {
	item = PySequence_GetItem(in, i);
	if (!PyString_Check(item)) {
	    Py_DECREF(item);
	    g_list_free(list);
	    return 0;
	}
	list = g_list_append(list, PyString_AsString(item));
	Py_DECREF(item);
    }
    *((GList **) out) = list;
    return 1;
}

static PyObject *
pygvfs_mime_set_short_list_applications(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_ids", NULL };
    char *mime_type;
    GList *application_ids;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sO&:matevfs.mime_set_short_list_applications",
				     kwlist, &mime_type,
				     string_list_converter, &application_ids))
	return NULL;
    result = mate_vfs_mime_set_short_list_applications(mime_type, application_ids);
    g_list_free(application_ids);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_set_short_list_components(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "component_iids", NULL };
    char *mime_type;
    GList *component_iids;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sO&:matevfs.mime_set_short_list_components",
				     kwlist, &mime_type,
				     string_list_converter, &component_iids))
	return NULL;
    result = mate_vfs_mime_set_short_list_components(mime_type, component_iids);
    g_list_free(component_iids);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
pygvfs_mime_add_application_to_short_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_id", NULL };
    char *mime_type, *application_id;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_add_application_to_short_list",
				     kwlist, &mime_type, &application_id))
	return NULL;
    result = mate_vfs_mime_add_application_to_short_list(mime_type, application_id);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_remove_application_from_short_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_id", NULL };
    char *mime_type, *application_id;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_remove_application_from_short_list",
				     kwlist, &mime_type, &application_id))
	return NULL;
    result = mate_vfs_mime_remove_application_from_short_list(mime_type, application_id);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_add_component_to_short_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "component_iid", NULL };
    char *mime_type, *component_iid;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_add_component_to_short_list",
				     kwlist, &mime_type, &component_iid))
	return NULL;
    result = mate_vfs_mime_add_component_to_short_list(mime_type, component_iid);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_remove_component_from_short_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "component_iid", NULL };
    char *mime_type, *component_iid;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_remove_component_from_short_list",
				     kwlist, &mime_type, &component_iid))
	return NULL;
    result = mate_vfs_mime_remove_component_from_short_list(mime_type, component_iid);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_add_extension(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "extension", NULL };
    char *mime_type, *extension;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_add_extension",
				     kwlist, &mime_type, &extension))
	return NULL;
    result = mate_vfs_mime_add_extension(mime_type, extension);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_remove_extension(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "extension", NULL };
    char *mime_type, *extension;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_remove_extension",
				     kwlist, &mime_type, &extension))
	return NULL;
    result = mate_vfs_mime_remove_extension(mime_type, extension);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_extend_all_applications(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_ids", NULL };
    char *mime_type;
    GList *application_ids;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sO&:matevfs.mime_extend_all_applications",
				     kwlist, &mime_type,
				     string_list_converter, &application_ids))
	return NULL;
    result = mate_vfs_mime_extend_all_applications(mime_type, application_ids);
    g_list_free(application_ids);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_remove_from_all_applications(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "application_ids", NULL };
    char *mime_type;
    GList *application_ids;
    MateVFSResult result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sO&:matevfs.mime_remove_from_all_applications",
				     kwlist, &mime_type,
				     string_list_converter, &application_ids))
	return NULL;
    result = mate_vfs_mime_remove_from_all_applications(mime_type, application_ids);
    g_list_free(application_ids);
    if (pymate_vfs_result_check(result))
	return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pygvfs_mime_application_new_from_id(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "id", NULL };
    char *id;
    MateVFSMimeApplication *app;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:matevfs.mime_application_new_from_id",
				     kwlist, &id))
	return NULL;
    app = mate_vfs_mime_application_new_from_id(id);

    if (!app) {
	  /* not sure if NULL is a valid return value, but we handle
	   * it just in case... */
	PyErr_SetString(PyExc_ValueError, "unknown application id");
	return NULL;
    }
    return pygvfs_mime_application_new(app);
}

static PyObject*
pyvfs_format_file_size_for_display(PyObject *self, PyObject *args)
{
    guint64 size;
    char *cstring;
    PyObject *string;

    if(!PyArg_ParseTuple(args, "K", &size))
	return NULL;

    cstring = mate_vfs_format_file_size_for_display(size);
    string = PyString_FromString(cstring);
    g_free(cstring);

    return string;
} 



static PyObject *
pygvfs_resolve(PyObject *self, PyObject *args)
{
    char *hostname;
    PyObject *retval;
    PyObject *list;
    MateVFSResult res;
    MateVFSResolveHandle *handle;
    MateVFSAddress *address;

    if (!PyArg_ParseTuple(args, "s", &hostname))
	return NULL;

    pyg_begin_allow_threads;

    res = mate_vfs_resolve(hostname, &handle);

    if (pymate_vfs_result_check(res)) {
	retval = NULL;
	goto out;
    }

    list = PyList_New(0);

    while (mate_vfs_resolve_next_address(handle, &address))
    {
	int type;
	char *str;
	PyObject *pair;

	type = mate_vfs_address_get_family_type(address);
	str = mate_vfs_address_to_string(address);

	pair = Py_BuildValue("(is)", type, str);
	g_free(str);

	PyList_Append(list, pair);
	Py_DECREF(pair);
    }

    mate_vfs_resolve_free(handle);

    retval = list;

 out:
    pyg_end_allow_threads;
    return retval;
}


static PyObject *
pygvfs_connect_to_server(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri", "display_name", "icon", NULL };
    char *uri, *display_name, *icon;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:matevfs.connect_to_server",
				     kwlist, &uri, &display_name, &icon))
	return NULL;
    mate_vfs_connect_to_server(uri, display_name, icon);
    Py_INCREF(Py_None);
    return Py_None;
}

#define pygvfs_generic_str_map_func(name, argname)              \
static PyObject *                                               \
pygvfs_##name(PyObject *self, PyObject *args, PyObject *kwargs) \
{                                                               \
    static char *kwlist[] = { #argname, NULL };                 \
    char *str;                                                  \
    PyObject *retval;                                           \
                                                                \
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,              \
				     "s:matevfs."#name,        \
				     kwlist, &str))             \
	return NULL;                                            \
    str = mate_vfs_##name(str);                                \
    if (!str) {                                                 \
        PyErr_SetString(PyExc_RuntimeError, "unknown error");   \
        return NULL;                                            \
    }                                                           \
    retval = PyString_FromString(str);                          \
    g_free(str);                                                \
    return retval;                                              \
}

pygvfs_generic_str_map_func(escape_string, string);
pygvfs_generic_str_map_func(escape_path_string, path);
pygvfs_generic_str_map_func(escape_host_and_path_string, string);
pygvfs_generic_str_map_func(escape_slashes, string);

static PyObject *
pygvfs_escape_set(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "string", "match_set", NULL };
    char *str, *str1;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.escape_set",
				     kwlist, &str, &str1))
	return NULL;
    str = mate_vfs_escape_set(str, str1);
    if (!str) {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
        return NULL;
    }
    retval = PyString_FromString(str);
    g_free(str);
    return retval;
}

static PyObject *
pygvfs_unescape_string(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "escaped_string", "illegal_characters", NULL };
    char *str, *str1;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.unescape_string",
				     kwlist, &str, &str1))
	return NULL;
    str = mate_vfs_escape_set(str, str1);
    if (!str) {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
        return NULL;
    }
    retval = PyString_FromString(str);
    g_free(str);
    return retval;
}

pygvfs_generic_str_map_func(make_uri_canonical, uri);
pygvfs_generic_str_map_func(make_path_name_canonical, path);
pygvfs_generic_str_map_func(unescape_string_for_display, escaped);
pygvfs_generic_str_map_func(get_local_path_from_uri, uri);
pygvfs_generic_str_map_func(get_uri_from_local_path, local_full_path);

static PyObject *
pygvfs_is_executable_command_string(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "command_string", NULL };
    char *str;
    gboolean retval;
    PyObject *py_retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s:matevfs.is_executable_command_string",
				     kwlist, &str))
	return NULL;
    retval = mate_vfs_is_executable_command_string(str);
    py_retval = retval? Py_True : Py_False;
    Py_INCREF(py_retval);
    return py_retval;
}

static PyObject *
pygvfs_get_volume_free_space(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "vfs_uri", NULL };
    PyObject *py_uri;
    MateVFSFileSize size = 0;
    MateVFSResult result;
    PyObject *py_retval;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "O!:matevfs.get_volume_free_space",
				     kwlist,
                                     &PyMateVFSURI_Type, &py_uri))
	return NULL;
    result = mate_vfs_get_volume_free_space(pymate_vfs_uri_get(py_uri), &size);
    if (pymate_vfs_result_check(result))
        return NULL;
    py_retval = PyLong_FromUnsignedLongLong(size);
    return py_retval;
}

pygvfs_generic_str_map_func(icon_path_from_filename, filename);

static PyObject *
pygvfs_open_fd(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "filedes", NULL };
    int filedes;
    MateVFSResult result;
    PyObject *py_retval;
    MateVFSHandle *handle = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "i:matevfs.open_fd",
				     kwlist, &filedes))
	return NULL;
    result = mate_vfs_open_fd(&handle, filedes);
    if (pymate_vfs_result_check(result))
        return NULL;
    py_retval = pymate_vfs_handle_new(handle);
    return py_retval;
}

static PyObject *
pygvfs_is_primary_thread(void)
{
    gboolean retval;
    PyObject *py_retval;

    retval = mate_vfs_is_primary_thread();
    py_retval = retval? Py_True : Py_False;
    Py_INCREF(py_retval);
    return py_retval;
}

pygvfs_generic_str_map_func(format_uri_for_display, uri);
pygvfs_generic_str_map_func(make_uri_from_input, uri);

static PyObject *
pygvfs_make_uri_from_input_with_dirs(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "escaped_string", "illegal_characters", NULL };
    char *str;
    int dirs;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "si:matevfs.make_uri_from_input_with_dirs",
				     kwlist, &str, &dirs))
	return NULL;
    str = mate_vfs_make_uri_from_input_with_dirs(str, dirs);
    if (!str) {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
        return NULL;
    }
    retval = PyString_FromString(str);
    g_free(str);
    return retval;
}

pygvfs_generic_str_map_func(make_uri_canonical_strip_fragment, uri);

static PyObject *
pygvfs_uris_match(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "uri_1", "uri_2", NULL };
    char *uri1, *uri2;
    gboolean retval;
    PyObject *py_retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.uris_match",
				     kwlist, &uri1, &uri2))
	return NULL;
    retval = mate_vfs_uris_match(uri1, uri2);
    py_retval = retval? Py_True : Py_False;
    Py_INCREF(py_retval);
    return py_retval;
}

pygvfs_generic_str_map_func(get_uri_scheme, uri);
pygvfs_generic_str_map_func(make_uri_from_shell_arg, uri);

static PyObject *
pygvfs_url_show(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "url", "env", NULL };
    char *url, **env;
    MateVFSResult result;
    PyObject *py_env = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "s|O!:matevfs.url_show",
				     kwlist, &url, &PyList_Type, &py_env))
	return NULL;

    if (!py_env)
        env = NULL;
    else {
        int len = PyList_Size(py_env);
        int i;
        env = g_new(char *, len + 1);
        for (i = 0; i < len; ++i) {
            PyObject *item = PyList_GET_ITEM(py_env, i);
            if (!PyString_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "second argument (env) "
                                "must be a list of strings");
                g_free(env);
                return NULL;
            }
            env[i] = PyString_AsString(item);
        }
        env[len] = NULL;
    }

    result = mate_vfs_url_show_with_env(url, env);
    if (env) g_free(env);
    if (pymate_vfs_result_check(result))
        return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static inline PyObject *
pygvfs_dns_service_new(MateVFSDNSSDService *service)
{
    return Py_BuildValue("sss", service->name, service->type, service->domain);
}

static PyObject *
_wrap_mate_vfs_dns_sd_browse_sync(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "domain", "type", "timeout_msec", NULL };
    char *domain, *type;
    int timeout_msec;
    int n_services;
    MateVFSDNSSDService *services = NULL;
    MateVFSResult result;
    PyObject *py_services;
    int i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ssi:matevfs.dns_sd_browse_sync", kwlist,
                                     &domain, &type, &timeout_msec))
	return NULL;

    pyg_unblock_threads();
    result = mate_vfs_dns_sd_browse_sync(domain, type, timeout_msec,
                                          &n_services, &services);
    pyg_block_threads();
    if (pymate_vfs_result_check(result))
        return NULL;
    py_services = PyList_New(n_services);
    for (i = 0; i < n_services; ++i)
        PyList_SET_ITEM(py_services, i, pygvfs_dns_service_new(services + i));
    mate_vfs_dns_sd_service_list_free(services, n_services);
    return py_services;
}

static void
__text_hash_to_dict(char *key, char *value, PyObject *dict)
{
    PyObject *pyval = PyString_FromString(value);
    PyDict_SetItemString(dict, key, pyval);
    Py_DECREF(pyval);
}

static PyObject *
_wrap_mate_vfs_dns_sd_resolve_sync(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "name", "type", "domain", "timeout_msec", NULL };
    char *name, *domain, *type;
    int timeout_msec;
    MateVFSResult result;
    char *host, *text_raw;
    int port, text_raw_len;
    GHashTable *hash;
    PyObject *py_hash;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "sssi:matevfs.dns_sd_resolve_sync", kwlist,
                                     &name, &type, &domain, &timeout_msec))
	return NULL;

    pyg_unblock_threads();
    result = mate_vfs_dns_sd_resolve_sync(name, type, domain, timeout_msec,
                                           &host, &port, &hash,
                                           &text_raw_len, &text_raw);
    pyg_block_threads();

    if (pymate_vfs_result_check(result))
        return NULL;

    py_hash = PyDict_New();
    g_hash_table_foreach(hash, (GHFunc) __text_hash_to_dict, py_hash);
    g_hash_table_destroy(hash);
    retval = Py_BuildValue("Ns#", py_hash, text_raw, (Py_ssize_t) text_raw_len);
    g_free(text_raw);
    return retval;
}

static PyObject *
_wrap_mate_vfs_dns_sd_list_browse_domains_sync(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "domain", "timeout_msec", NULL };
    char *domain;
    int timeout_msec;
    MateVFSResult result;
    GList *domains, *l;
    PyObject *retval;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "si:matevfs.dns_sd_list_browse_domains_sync", kwlist,
                                      &domain, &timeout_msec))
	return NULL;

    pyg_unblock_threads();
    result = mate_vfs_dns_sd_list_browse_domains_sync(domain,
                                                       timeout_msec,
                                                       &domains);
    pyg_block_threads();
    if (pymate_vfs_result_check(result))
        return NULL;
    retval = PyList_New(0);
    for (l = domains; l; l = l->next) {
        PyObject *item = PyString_FromString((char *) l->data);
        PyList_Append(retval, item);
        Py_DECREF(item);
        g_free(l->data);
    }
    g_list_free(domains);
    return retval;
}

static PyObject *
_wrap_mate_vfs_get_default_browse_domains(PyObject *self, PyObject *args, PyObject *kwargs)
{
    GList *domains, *l;
    PyObject *retval;

    pyg_unblock_threads();
    domains = mate_vfs_get_default_browse_domains();
    pyg_block_threads();

    retval = PyList_New(0);
    for (l = domains; l; l = l->next) {
        PyObject *item = PyString_FromString((char *) l->data);
        PyList_Append(retval, item);
        Py_DECREF(item);
        g_free(l->data);
    }
    g_list_free(domains);
    return retval;
}


static PyObject *
_wrap_mate_vfs_mime_type_get_equivalence(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "mime_type", "base_mime_type", NULL };
    char *mime_type, *base_mime_type;
    MateVFSMimeEquivalence result;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				     "ss:matevfs.mime_type_get_equivalence", kwlist,
                                      &mime_type, &base_mime_type))
	return NULL;

    result = mate_vfs_mime_type_get_equivalence(mime_type, base_mime_type);
    return PyInt_FromLong(result);
}


static PyMethodDef pymatevfs_functions[] = {
    { "create", (PyCFunction)pygvfs_create, METH_VARARGS|METH_KEYWORDS },
    { "get_file_info", (PyCFunction)pygvfs_get_file_info,
      METH_VARARGS|METH_KEYWORDS },
    { "set_file_info", (PyCFunction)pygvfs_set_file_info,
      METH_VARARGS|METH_KEYWORDS },
    { "truncate", (PyCFunction)pygvfs_truncate, METH_VARARGS|METH_KEYWORDS },
    { "make_directory", (PyCFunction)pygvfs_make_directory,
      METH_VARARGS|METH_KEYWORDS },
    { "remove_directory", (PyCFunction)pygvfs_remove_directory,
      METH_VARARGS|METH_KEYWORDS },
    { "create_symbolic_link", (PyCFunction)pygvfs_create_symbolic_link,
      METH_VARARGS|METH_KEYWORDS },
    { "unlink", (PyCFunction)pygvfs_unlink, METH_VARARGS|METH_KEYWORDS },
    { "exists", (PyCFunction)pygvfs_exists, METH_VARARGS|METH_KEYWORDS },
    { "format_file_size_for_display", pyvfs_format_file_size_for_display, METH_VARARGS},
    { "get_file_mime_type", (PyCFunction) pygvfs_get_file_mime_type, METH_VARARGS|METH_KEYWORDS},
    { "get_mime_type", (PyCFunction)pygvfs_get_mime_type, METH_VARARGS },
    { "get_mime_type_for_data", (PyCFunction)pygvfs_get_mime_type_for_data,
      METH_VARARGS },
    { "mime_get_icon", (PyCFunction)pygvfs_mime_get_icon, METH_VARARGS },
    { "mime_set_icon", (PyCFunction)pygvfs_mime_set_icon, METH_VARARGS },
    { "mime_get_description", (PyCFunction)pygvfs_mime_get_description, 
      METH_VARARGS },
    { "mime_set_description", (PyCFunction)pygvfs_mime_set_description, 
      METH_VARARGS },
    { "mime_can_be_executable", (PyCFunction)pygvfs_mime_can_be_executable, 
      METH_VARARGS },
    { "mime_set_can_be_executable", (PyCFunction)pygvfs_mime_set_can_be_executable,
      METH_VARARGS },
    { "monitor_add", pygvfs_monitor_add, METH_VARARGS},
    { "monitor_cancel", pygvfs_monitor_cancel, METH_VARARGS},
    { "read_entire_file", pygvfs_read_entire_file, METH_VARARGS},
    { "mime_get_default_application", (PyCFunction)pygvfs_mime_get_default_application, 
      METH_VARARGS },
    { "xfer_uri", (PyCFunction)pygvfs_xfer_uri,  METH_VARARGS|METH_KEYWORDS },
    { "xfer_uri_list", (PyCFunction)pygvfs_xfer_uri_list,  METH_VARARGS|METH_KEYWORDS },
    { "xfer_delete_list", (PyCFunction)pygvfs_xfer_delete_list,  METH_VARARGS|METH_KEYWORDS },
    { "mime_get_default_action_type", (PyCFunction)pygvfs_mime_get_default_action_type,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_get_default_action", (PyCFunction)pygvfs_mime_get_default_action,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_get_short_list_applications", (PyCFunction)pygvfs_mime_get_short_list_applications,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_get_all_applications", (PyCFunction)pygvfs_mime_get_all_applications,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_set_default_action_type", (PyCFunction)pygvfs_mime_set_default_action_type,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_set_default_application", (PyCFunction)pygvfs_mime_set_default_application,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_set_default_component", (PyCFunction)pygvfs_mime_set_default_component,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_set_short_list_applications", (PyCFunction)pygvfs_mime_set_short_list_applications,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_set_short_list_components", (PyCFunction)pygvfs_mime_set_short_list_components,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_add_application_to_short_list", (PyCFunction)pygvfs_mime_add_application_to_short_list,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_remove_application_from_short_list",
      (PyCFunction)pygvfs_mime_remove_application_from_short_list, METH_VARARGS|METH_KEYWORDS },
    { "mime_add_component_to_short_list", (PyCFunction)pygvfs_mime_add_component_to_short_list,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_remove_component_from_short_list", (PyCFunction)pygvfs_mime_remove_component_from_short_list,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_add_extension", (PyCFunction)pygvfs_mime_add_extension,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_remove_extension", (PyCFunction)pygvfs_mime_remove_extension,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_extend_all_applications", (PyCFunction)pygvfs_mime_extend_all_applications,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_remove_from_all_applications", (PyCFunction)pygvfs_mime_remove_from_all_applications,
      METH_VARARGS|METH_KEYWORDS },
    { "mime_application_new_from_id", (PyCFunction)pygvfs_mime_application_new_from_id,
      METH_VARARGS|METH_KEYWORDS },
    { "connect_to_server", (PyCFunction)pygvfs_connect_to_server,
      METH_VARARGS|METH_KEYWORDS },
    { "escape_string", (PyCFunction)pygvfs_escape_string,
      METH_VARARGS|METH_KEYWORDS },
    { "escape_path_string", (PyCFunction)pygvfs_escape_path_string,
      METH_VARARGS|METH_KEYWORDS },
    { "escape_host_and_path_string", (PyCFunction)pygvfs_escape_host_and_path_string,
      METH_VARARGS|METH_KEYWORDS },
    { "escape_slashes", (PyCFunction)pygvfs_escape_slashes,
      METH_VARARGS|METH_KEYWORDS },
    { "escape_set", (PyCFunction)pygvfs_escape_set,
      METH_VARARGS|METH_KEYWORDS },
    { "unescape_string", (PyCFunction)pygvfs_unescape_string,
      METH_VARARGS|METH_KEYWORDS },
    { "make_uri_canonical", (PyCFunction)pygvfs_make_uri_canonical,
      METH_VARARGS|METH_KEYWORDS },
    { "make_path_name_canonical", (PyCFunction)pygvfs_make_path_name_canonical,
      METH_VARARGS|METH_KEYWORDS },
    { "unescape_string_for_display", (PyCFunction)pygvfs_unescape_string_for_display,
      METH_VARARGS|METH_KEYWORDS },
    { "get_local_path_from_uri", (PyCFunction)pygvfs_get_local_path_from_uri,
      METH_VARARGS|METH_KEYWORDS },
    { "get_uri_from_local_path", (PyCFunction)pygvfs_get_uri_from_local_path,
      METH_VARARGS|METH_KEYWORDS },
    { "is_executable_command_string", (PyCFunction)pygvfs_is_executable_command_string,
      METH_VARARGS|METH_KEYWORDS },
    { "get_volume_free_space", (PyCFunction)pygvfs_get_volume_free_space,
      METH_VARARGS|METH_KEYWORDS },
    { "icon_path_from_filename", (PyCFunction)pygvfs_icon_path_from_filename,
      METH_VARARGS|METH_KEYWORDS },
    { "open_fd", (PyCFunction)pygvfs_open_fd,
      METH_VARARGS|METH_KEYWORDS },
    { "is_primary_thread", (PyCFunction)pygvfs_is_primary_thread,
      METH_NOARGS },
    { "format_uri_for_display", (PyCFunction)pygvfs_format_uri_for_display,
      METH_VARARGS|METH_KEYWORDS },
    { "make_uri_from_input", (PyCFunction)pygvfs_make_uri_from_input,
      METH_VARARGS|METH_KEYWORDS },
    { "make_uri_from_input_with_dirs", (PyCFunction)pygvfs_make_uri_from_input_with_dirs,
      METH_VARARGS|METH_KEYWORDS },
    { "make_uri_canonical_strip_fragment", (PyCFunction)pygvfs_make_uri_canonical_strip_fragment,
      METH_VARARGS|METH_KEYWORDS },
    { "uris_match", (PyCFunction)pygvfs_uris_match,
      METH_VARARGS|METH_KEYWORDS },
    { "get_uri_scheme", (PyCFunction)pygvfs_get_uri_scheme,
      METH_VARARGS|METH_KEYWORDS },
    { "make_uri_from_shell_arg", (PyCFunction)pygvfs_make_uri_from_shell_arg,
      METH_VARARGS|METH_KEYWORDS },
    { "url_show", (PyCFunction)pygvfs_url_show,
      METH_VARARGS|METH_KEYWORDS },
    { "resolve", pygvfs_resolve, METH_VARARGS},
    { "dns_sd_browse_sync", (PyCFunction)_wrap_mate_vfs_dns_sd_browse_sync,
      METH_VARARGS|METH_KEYWORDS},
    { "dns_sd_resolve_sync", (PyCFunction)_wrap_mate_vfs_dns_sd_resolve_sync,
      METH_VARARGS|METH_KEYWORDS},
    { "dns_sd_list_browse_domains_sync",
      (PyCFunction)_wrap_mate_vfs_dns_sd_list_browse_domains_sync,
      METH_VARARGS|METH_KEYWORDS},
    { "get_default_browse_domains",
      (PyCFunction)_wrap_mate_vfs_get_default_browse_domains, METH_NOARGS},
    { "mime_type_get_equivalence",
      (PyCFunction)_wrap_mate_vfs_mime_type_get_equivalence,
      METH_VARARGS|METH_KEYWORDS},

    { NULL, NULL, 0 }
    
};

static void
register_constants(PyObject *module)
{
#define regconst(x) PyModule_AddIntConstant(module, #x, MATE_VFS_ ## x)
    regconst(FILE_FLAGS_NONE);
    regconst(FILE_FLAGS_SYMLINK);
    regconst(FILE_FLAGS_LOCAL);
    regconst(FILE_TYPE_UNKNOWN);
    regconst(FILE_TYPE_REGULAR);
    regconst(FILE_TYPE_DIRECTORY);
    regconst(FILE_TYPE_FIFO);
    regconst(FILE_TYPE_SOCKET);
    regconst(FILE_TYPE_CHARACTER_DEVICE);
    regconst(FILE_TYPE_BLOCK_DEVICE);
    regconst(FILE_TYPE_SYMBOLIC_LINK);
    regconst(FILE_INFO_FIELDS_NONE);
    regconst(FILE_INFO_FIELDS_TYPE);
    regconst(FILE_INFO_FIELDS_PERMISSIONS);
    regconst(FILE_INFO_FIELDS_FLAGS);
    regconst(FILE_INFO_FIELDS_DEVICE);
    regconst(FILE_INFO_FIELDS_INODE);
    regconst(FILE_INFO_FIELDS_LINK_COUNT);
    regconst(FILE_INFO_FIELDS_SIZE);
    regconst(FILE_INFO_FIELDS_BLOCK_COUNT);
    regconst(FILE_INFO_FIELDS_IO_BLOCK_SIZE);
    regconst(FILE_INFO_FIELDS_ATIME);
    regconst(FILE_INFO_FIELDS_MTIME);
    regconst(FILE_INFO_FIELDS_CTIME);
    regconst(FILE_INFO_FIELDS_SYMLINK_NAME);
    regconst(FILE_INFO_FIELDS_MIME_TYPE);
    regconst(FILE_INFO_FIELDS_ACCESS);
    regconst(FILE_INFO_FIELDS_IDS);
    regconst(PERM_SUID);
    regconst(PERM_SGID);
    regconst(PERM_STICKY);
    regconst(PERM_USER_READ);
    regconst(PERM_USER_WRITE);
    regconst(PERM_USER_EXEC);
    regconst(PERM_USER_ALL);
    regconst(PERM_GROUP_READ);
    regconst(PERM_GROUP_WRITE);
    regconst(PERM_GROUP_EXEC);
    regconst(PERM_GROUP_ALL);
    regconst(PERM_OTHER_READ);
    regconst(PERM_OTHER_WRITE);
    regconst(PERM_OTHER_EXEC);
    regconst(PERM_OTHER_ALL);
    regconst(FILE_INFO_DEFAULT);
    regconst(FILE_INFO_GET_MIME_TYPE);
    regconst(FILE_INFO_FORCE_FAST_MIME_TYPE);
    regconst(FILE_INFO_FORCE_SLOW_MIME_TYPE);
    regconst(FILE_INFO_FOLLOW_LINKS);
    regconst(FILE_INFO_GET_ACCESS_RIGHTS);
    regconst(FILE_INFO_NAME_ONLY);
    regconst(SET_FILE_INFO_NONE);
    regconst(SET_FILE_INFO_NAME);
    regconst(SET_FILE_INFO_PERMISSIONS);
    regconst(SET_FILE_INFO_OWNER);
    regconst(SET_FILE_INFO_TIME);
    regconst(DIRECTORY_VISIT_DEFAULT);
    regconst(DIRECTORY_VISIT_SAMEFS);
    regconst(DIRECTORY_VISIT_LOOPCHECK);
    regconst(OPEN_NONE);
    regconst(OPEN_READ);
    regconst(OPEN_WRITE);
    regconst(OPEN_RANDOM);
    regconst(OPEN_TRUNCATE);
    regconst(SEEK_START);
    regconst(SEEK_CURRENT);
    regconst(SEEK_END);
    regconst(MONITOR_FILE);
    regconst(MONITOR_DIRECTORY);
    regconst(MONITOR_EVENT_CHANGED);
    regconst(MONITOR_EVENT_DELETED);
    regconst(MONITOR_EVENT_STARTEXECUTING);
    regconst(MONITOR_EVENT_STOPEXECUTING);
    regconst(MONITOR_EVENT_CREATED);
    regconst(MONITOR_EVENT_METADATA_CHANGED);
    regconst(MIME_APPLICATION_ARGUMENT_TYPE_URIS);
    regconst(MIME_APPLICATION_ARGUMENT_TYPE_PATHS);
    regconst(MIME_APPLICATION_ARGUMENT_TYPE_URIS_FOR_NON_FILES);
    regconst(XFER_DEFAULT);
    regconst(XFER_FOLLOW_LINKS);
    regconst(XFER_RECURSIVE);
    regconst(XFER_SAMEFS);
    regconst(XFER_DELETE_ITEMS);
    regconst(XFER_EMPTY_DIRECTORIES);
    regconst(XFER_NEW_UNIQUE_DIRECTORY);
    regconst(XFER_REMOVESOURCE);
    regconst(XFER_USE_UNIQUE_NAMES);
    regconst(XFER_LINK_ITEMS);
    regconst(XFER_FOLLOW_LINKS_RECURSIVE);
    regconst(XFER_PROGRESS_STATUS_OK);
    regconst(XFER_PROGRESS_STATUS_VFSERROR);
    regconst(XFER_PROGRESS_STATUS_OVERWRITE);
    regconst(XFER_PROGRESS_STATUS_DUPLICATE);
    regconst(XFER_OVERWRITE_MODE_ABORT);
    regconst(XFER_OVERWRITE_MODE_QUERY);
    regconst(XFER_OVERWRITE_MODE_REPLACE);
    regconst(XFER_OVERWRITE_MODE_SKIP);
    regconst(XFER_OVERWRITE_ACTION_ABORT);
    regconst(XFER_OVERWRITE_ACTION_REPLACE);
    regconst(XFER_OVERWRITE_ACTION_REPLACE_ALL);
    regconst(XFER_OVERWRITE_ACTION_SKIP);
    regconst(XFER_OVERWRITE_ACTION_SKIP_ALL);
    regconst(XFER_ERROR_MODE_ABORT);
    regconst(XFER_ERROR_MODE_QUERY);
    regconst(XFER_ERROR_ACTION_ABORT);
    regconst(XFER_ERROR_ACTION_RETRY);
    regconst(XFER_ERROR_ACTION_SKIP);
    regconst(XFER_PHASE_INITIAL);
    regconst(XFER_CHECKING_DESTINATION);
    regconst(XFER_PHASE_COLLECTING);
    regconst(XFER_PHASE_READYTOGO);
    regconst(XFER_PHASE_OPENSOURCE);
    regconst(XFER_PHASE_OPENTARGET);
    regconst(XFER_PHASE_COPYING);
    regconst(XFER_PHASE_MOVING);
    regconst(XFER_PHASE_READSOURCE);
    regconst(XFER_PHASE_WRITETARGET);
    regconst(XFER_PHASE_CLOSESOURCE);
    regconst(XFER_PHASE_CLOSETARGET);
    regconst(XFER_PHASE_DELETESOURCE);
    regconst(XFER_PHASE_SETATTRIBUTES);
    regconst(XFER_PHASE_FILECOMPLETED);
    regconst(XFER_PHASE_CLEANUP);
    regconst(XFER_PHASE_COMPLETED);
    regconst(DIRECTORY_KIND_DESKTOP);
    regconst(DIRECTORY_KIND_TRASH);
    regconst(PERM_ACCESS_READABLE);
    regconst(PERM_ACCESS_WRITABLE);
    regconst(PERM_ACCESS_EXECUTABLE);
    regconst(PRIORITY_MIN);
    regconst(PRIORITY_MAX);
    regconst(PRIORITY_DEFAULT);
    regconst(MIME_UNRELATED);
    regconst(MIME_IDENTICAL);
    regconst(MIME_PARENT);

#undef regconst
}

static void initialize_exceptions (PyObject *d)
{
    pymatevfs_exc = PyErr_NewException ("matevfs.Error",
                                                 PyExc_RuntimeError, NULL);
    PyDict_SetItemString(d, "Error", pymatevfs_exc);
 
#define register_exception(c_name, py_name)                             \
    pymatevfs_##c_name##_exc =                                   \
        PyErr_NewException ("matevfs." py_name "Error",               \
                            pymatevfs_exc, NULL);                \
    PyDict_SetItemString(d, py_name "Error", pymatevfs_##c_name##_exc);
 
    register_exception(not_found,             "NotFound");
    register_exception(generic,               "Generic");
    register_exception(internal,              "Internal");
    register_exception(bad_parameters,        "BadParameters");
    register_exception(not_supported,         "NotSupported");
    register_exception(io,                    "IO");
    register_exception(corrupted_data,        "CorruptedData");
    register_exception(wrong_format,          "WrongFormat");
    register_exception(bad_file,              "BadFile");
    register_exception(too_big,               "TooBig");
    register_exception(no_space,              "NoSpace");
    register_exception(read_only,             "ReadOnly");    
    register_exception(invalid_uri,           "InvalidURI");
    register_exception(not_open,              "NotOpen");    
    register_exception(invalid_open_mode,     "InvalidOpenMode");
    register_exception(access_denied,         "AccessDenied");    
    register_exception(too_many_open_files,   "TooManyOpenFiles");
    register_exception(eof,                   "EOF");    
    register_exception(not_a_directory,       "NotADirectory");
    register_exception(in_progress,           "InProgress");
    register_exception(interrupted,           "Interrupted");
    register_exception(file_exists,           "FileExists");
    register_exception(loop,                  "Loop");
    register_exception(not_permitted,         "NotPermitted");
    register_exception(is_directory,          "IsDirectory");
    register_exception(no_memory,             "NoMemory");
    register_exception(host_not_found,        "HostNotFound");
    register_exception(invalid_host_name,     "InvalidHostName");
    register_exception(host_has_no_address,   "HostHasNoAddress");
    register_exception(login_failed,          "LoginFailed");
    register_exception(cancelled,             "Cancelled");
    register_exception(directory_busy,        "DirectoryBusy");
    register_exception(directory_not_empty,   "DirectoryNotEmpty");
    register_exception(too_many_links,        "TooManyLinks");
    register_exception(read_only_file_system, "ReadOnlyFileSystem");
    register_exception(not_same_file_system,  "NotSameFileSystem");
    register_exception(name_too_long,         "NameTooLong");
    register_exception(service_not_available, "ServiceNotAvailable");
    register_exception(service_obsolete,      "ServiceObsolete");
    register_exception(protocol_error,        "ProtocolError");
    register_exception(no_master_browser,     "NoMasterBrowser");
#if 0    
    register_exception(no_default,            "NoDefault");
    register_exception(no_handler,            "NoHandler");
    register_exception(parse,                 "Parse");
    register_exception(launch,                "Launch");
#endif    
#undef register_exception
}

struct _PyMateVFS_Functions pymatevfs_api_functions = {
    pymate_vfs_exception_check,
    pymate_vfs_uri_new,
    &PyMateVFSURI_Type,
    pymate_vfs_file_info_new,
    &PyMateVFSFileInfo_Type,
    pymate_vfs_context_new,
    &PyMateVFSContext_Type,

};

/* initialise stuff extension classes */
static void
pymatefs_register_gobject_based_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gobject")) != NULL) {
        PyObject *moddict = PyModule_GetDict(module);

        _PyGObject_Type = (PyTypeObject *)PyDict_GetItemString(moddict, "GObject");
        if (_PyGObject_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name GObject from gobject");
            return;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gobject");
        return;
    }

    pygobject_register_class(d, "Volume", MATE_VFS_TYPE_VOLUME, &PyMateVFSVolume_Type,
                             Py_BuildValue("(O)", &PyGObject_Type));
    pygobject_register_class(d, "Drive", MATE_VFS_TYPE_DRIVE, &PyMateVFSDrive_Type,
                             Py_BuildValue("(O)", &PyGObject_Type));
    pygobject_register_class(d, "VolumeMonitor", MATE_VFS_TYPE_VOLUME_MONITOR,
                             &PyMateVFSVolumeMonitor_Type,
                             Py_BuildValue("(O)", &PyGObject_Type));
}

DL_EXPORT(void)
init_matevfs(void)
{
    PyObject *m, *d, *o;

    PyMateVFSURI_Type.ob_type = &PyType_Type;
    PyMateVFSContext_Type.ob_type = &PyType_Type;
    PyMateVFSFileInfo_Type.ob_type = &PyType_Type;
    PyMateVFSDirectoryHandle_Type.ob_type = &PyType_Type;
    PyMateVFSHandle_Type.ob_type = &PyType_Type;

    init_pygobject();
    if (!mate_vfs_init()) {
	PyErr_SetString(PyExc_RuntimeError, "could not initialise matevfs");
	return;
    }

    if (PyType_Ready(&PyMateVFSURI_Type) < 0)
	return;
    if (PyType_Ready(&PyMateVFSContext_Type) < 0)
	return;
    if (PyType_Ready(&PyMateVFSFileInfo_Type) < 0)
	return;
    if (PyType_Ready(&PyMateVFSDirectoryHandle_Type) < 0)
	return;
    if (PyType_Ready(&PyMateVFSHandle_Type) < 0)
	return;
    if (PyType_Ready(&PyMateVFSXferProgressInfo_Type) < 0)
	return;

    m = Py_InitModule("matevfs._matevfs", pymatevfs_functions);
    d = PyModule_GetDict(m);

    register_constants(m);
    initialize_exceptions(d);
    PyDict_SetItemString(d, "Error", pymatevfs_exc);

    PyDict_SetItemString(d, "URI", (PyObject *)&PyMateVFSURI_Type);
    PyDict_SetItemString(d, "Context", (PyObject *)&PyMateVFSContext_Type);
    PyDict_SetItemString(d, "FileInfo", (PyObject *)&PyMateVFSFileInfo_Type);
    PyDict_SetItemString(d, "DirectoryHandle",
			 (PyObject *)&PyMateVFSDirectoryHandle_Type);
    PyDict_SetItemString(d, "Handle", (PyObject *)&PyMateVFSHandle_Type);


    pymatefs_register_gobject_based_classes(d);
    pygvvolume_add_constants(m);

    PyDict_SetItemString(d, "async", pygvfs_async_module_init ());

    PyDict_SetItemString(d, "open_directory",
			 (PyObject *)&PyMateVFSDirectoryHandle_Type);
    PyDict_SetItemString(d, "open", (PyObject *)&PyMateVFSHandle_Type);

    PyDict_SetItemString(d, "_PyMateVFS_API",
			 o=PyCObject_FromVoidPtr(&pymatevfs_api_functions,NULL));
    Py_DECREF(o);

    monitor_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
}
