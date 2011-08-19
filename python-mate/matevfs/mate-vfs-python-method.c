/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 *  Copyright (C) 2003 Johan Dahlin
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 *  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>
#include <sysmodule.h>

#include <libmatevfs/mate-vfs-module.h>

#include "pymatevfs.h"

//#define MATE_VFS_PYTHON_DIR "/home/jdahlin/projects/method"

#define EXCEPTION_CHECK(retval) \
	if (!retval) {                                                  \
		MateVFSResult result = pymate_vfs_exception_check();  \
		if ((signed)result >= 0) {                              \
                        PyErr_Clear();                                  \
                        GILSTATE_RELEASE(state);                        \
			return result;                                  \
		} else if ((signed)result == -2) {                      \
			PyErr_Print();                                  \
		}                                                       \
                GILSTATE_RELEASE(state);                                \
		return MATE_VFS_ERROR_GENERIC;                         \
	}

#define HANDLE2OBJ(handle) (((MethodContainer)(handle))->object)
#define SET_HANDLE(handle, object2, uri)                              \
    {                                                                 \
	    MethodContainer *container = g_new0(MethodContainer, 1);  \
	    container->object = object2;                              \
	    container->uri = uri;                                     \
            Py_INCREF(container->object);                             \
            *handle = (MateVFSMethodHandle *) container;             \
    }
#define GET_URI(handle) ((MethodContainer*)handle)->uri
#define GET_OBJECT(handle) ((MethodContainer*)handle)->object
#define GILSTATE_ENSURE(state) \
   {                                                                  \
        state = PyGILState_Ensure();                                  \
        /* fprintf(stderr, "x %38s %20s %18u\n", G_STRLOC, G_STRFUNC, state); */\
   }
#define GILSTATE_RELEASE(state) \
   {                                                                  \
        /* fprintf(stderr, ". %38s %20s %18u\n", G_STRLOC, G_STRFUNC, state); */\
        PyGILState_Release(state);                                    \
   }

static MateVFSResult do_open           (MateVFSMethod *method,
				         MateVFSMethodHandle **method_handle,
					 MateVFSURI *uri,
					 MateVFSOpenMode mode,
					 MateVFSContext *context);

static MateVFSResult do_create         (MateVFSMethod *method,
                                         MateVFSMethodHandle **method_handle,
                                         MateVFSURI *uri,
                                         MateVFSOpenMode mode,
                                         gboolean exclusive,
                                         guint perm,
                                         MateVFSContext *context);
 
static MateVFSResult do_close          (MateVFSMethod *method,
                                         MateVFSMethodHandle *method_handle,
                                         MateVFSContext *context);
 
static MateVFSResult do_read           (MateVFSMethod *method,
                                         MateVFSMethodHandle *method_handle,
                                         gpointer buffer,
                                         MateVFSFileSize num_bytes,
                                         MateVFSFileSize *bytes_read,
                                         MateVFSContext *context);
 
static MateVFSResult do_write          (MateVFSMethod *method,
                                         MateVFSMethodHandle *method_handle,
                                         gconstpointer buffer,
                                         MateVFSFileSize num_bytes,
                                         MateVFSFileSize *bytes_written,
                                         MateVFSContext *context);

static MateVFSResult do_seek           (MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSSeekPosition  whence,
					 MateVFSFileOffset    offset,
					 MateVFSContext *context);

static MateVFSResult do_tell		(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileSize *offset_return);

static MateVFSResult do_truncate_handle (MateVFSMethod *method,
					  MateVFSMethodHandle *method_handle,
					  MateVFSFileSize length,
					  MateVFSContext *context);

static MateVFSResult do_open_directory (MateVFSMethod *method,
					 MateVFSMethodHandle **method_handle,
					 MateVFSURI *uri,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

static MateVFSResult do_close_directory (MateVFSMethod *method,
					  MateVFSMethodHandle *method_handle,
					  MateVFSContext *context);

static MateVFSResult do_read_directory (MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileInfo *file_info,
					 MateVFSContext *context);

static MateVFSResult do_get_file_info  (MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSFileInfo *file_info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

static MateVFSResult do_get_file_info_from_handle
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileInfo *file_info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

static gboolean       do_is_local       (MateVFSMethod *method,
					 const MateVFSURI *uri);

static MateVFSResult do_make_directory (MateVFSMethod *method,
					 MateVFSURI *uri,
					 guint perm,
					 MateVFSContext *context);

static MateVFSResult do_find_directory (MateVFSMethod *method,
					 MateVFSURI *find_near_uri,
					 MateVFSFindDirectoryKind kind,
					 MateVFSURI **result_uri,
					 gboolean create_if_needed,
					 gboolean find_if_needed,
					 guint perm,
					 MateVFSContext *context);

static MateVFSResult do_remove_directory (MateVFSMethod *method,
					   MateVFSURI *uri,
					   MateVFSContext *context);

static MateVFSResult do_move           (MateVFSMethod *method,
					 MateVFSURI *old_uri,
					 MateVFSURI *new_uri,
					 gboolean force_replace,
					 MateVFSContext *context);

static MateVFSResult do_unlink         (MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSContext *context);

static MateVFSResult do_check_same_fs  (MateVFSMethod *method,
					 MateVFSURI *a,
					 MateVFSURI *b,
					 gboolean *same_fs_return,
					 MateVFSContext *context);

static MateVFSResult do_set_file_info (MateVFSMethod *method,
					MateVFSURI *a,
					const MateVFSFileInfo *info,
					MateVFSSetFileInfoMask mask,
					MateVFSContext *context);

static MateVFSResult do_truncate       (MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSFileSize length,
					 MateVFSContext *context);

static MateVFSResult do_create_symbolic_link (MateVFSMethod *method,
					       MateVFSURI *uri,
					       const gchar *target_reference,
					       MateVFSContext *context);
#if 0
static MateVFSResult do_monitor_add (MateVFSMethod *method,
				      MateVFSMethodHandle **method_handle,
				      MateVFSURI *uri,
				      MateVFSMonitorType monitor_type);

static MateVFSResult do_monitor_cancel (MateVFSMethod *method,
      					 MateVFSMethodHandle *handle);
#endif

static MateVFSResult do_file_control (MateVFSMethod *method,
				       MateVFSMethodHandle *method_handle,
				       const char *operation,
				       gpointer operation_data,
				       MateVFSContext *context);

struct PyVFSMethod {
	PyObject *instance;
	PyObject *open_func;
	PyObject *create_func;
	PyObject *close_func;
	PyObject *read_func;
	PyObject *write_func;
	PyObject *seek_func;
	PyObject *tell_func;
	PyObject *truncate_handle_func;
	PyObject *open_directory_func;
	PyObject *close_directory_func;
	PyObject *read_directory_func;
	PyObject *get_file_info_func;
	PyObject *get_file_info_from_handle_func;
	PyObject *is_local_func;
	PyObject *make_directory_func;
	PyObject *remove_directory_func;
	PyObject *move_func;
	PyObject *unlink_func;
	PyObject *check_same_fs_func;
	PyObject *set_file_info_func;
	PyObject *truncate_func;
	PyObject *find_directory_func;
	PyObject *create_symbolic_link_func;
	PyObject *monitor_add_func;    /* unused*/
	PyObject *monitor_cancel_func; /* unused*/
	PyObject *file_control_func;   /* unused*/
};

typedef struct {
	PyObject *object;
	const MateVFSURI *uri;
} MethodContainer;


typedef struct PyVFSMethod PyVFSMethod;

static GHashTable *pymethod_hash = NULL;

static MateVFSMethod method = {
        sizeof (MateVFSMethod),
        do_open,
        do_create,
        do_close,
        do_read,
        do_write,
        do_seek,
        do_tell,
        do_truncate_handle,
        do_open_directory,
        do_close_directory,
        do_read_directory,
        do_get_file_info,
        do_get_file_info_from_handle,
        do_is_local,
        do_make_directory,
        do_remove_directory,
        do_move,
        do_unlink,
        do_check_same_fs,
        do_set_file_info,
        do_truncate,
        do_find_directory,
        do_create_symbolic_link,
	NULL, /* monitor_add_func */
	NULL, /* monitor_cancel_func */
	do_file_control
};

/* Helper function for PyMateVFSContext creation */
static PyObject*
context_new (MateVFSContext *context)
{
	if (context == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	} else {
		return pymate_vfs_context_new(context);
	}
}

/* inline maybe */
static struct PyVFSMethod*
get_method_from_uri (const MateVFSURI *uri)
{
	struct PyVFSMethod *pymethod;

	pymethod = g_hash_table_lookup(pymethod_hash,
				       uri->method_string);

	if (!pymethod) {
		g_warning ("There is no method defined for %s",
			   uri->method_string);
		return NULL;
	}
	
	return pymethod;
}

static MateVFSResult
do_open (MateVFSMethod *method,
         MateVFSMethodHandle **method_handle,
         MateVFSURI *uri,
         MateVFSOpenMode open_mode,
         MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->open_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->open_func,
				     Py_BuildValue("(NiN)",
						   pymate_vfs_uri_new(mate_vfs_uri_ref(uri)),
						   open_mode,
						   pycontext));

	EXCEPTION_CHECK(retval);
	SET_HANDLE(method_handle, retval, uri);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_create (MateVFSMethod *method,
           MateVFSMethodHandle **method_handle,
           MateVFSURI *uri,
           MateVFSOpenMode mode,
           gboolean exclusive,
           guint perm,
           MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->create_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->create_func,
				     Py_BuildValue("(NibiN)",
						   pymate_vfs_uri_new(mate_vfs_uri_ref(uri)),
						   mode,
						   exclusive,
						   perm,
						   pycontext));
	EXCEPTION_CHECK(retval);

	SET_HANDLE(method_handle, retval, uri);
	GILSTATE_RELEASE(state);
	
        return MATE_VFS_OK;
}

static MateVFSResult
do_close (MateVFSMethod *method,
          MateVFSMethodHandle *method_handle,
          MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->close_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->close_func,
				     Py_BuildValue("(ON)",
						   GET_OBJECT(method_handle),
						   pycontext));
	EXCEPTION_CHECK(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read (MateVFSMethod *method,
         MateVFSMethodHandle *method_handle,
         gpointer buffer,
         MateVFSFileSize num_bytes,
         MateVFSFileSize *bytes_read,
         MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pybuffer;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->read_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	/* XXX: test buffer reads */
	GILSTATE_ENSURE(state);
	pybuffer = PyBuffer_FromReadWriteMemory((gpointer)buffer, num_bytes);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->read_func,
				     Py_BuildValue("(ONlN)",
						   GET_OBJECT(method_handle),
						   pybuffer,
						   (long)num_bytes,
						   pycontext));
	EXCEPTION_CHECK(retval);
	
	if (PyInt_Check(retval)) {
		*bytes_read = PyInt_AsLong(retval);
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;
	} else if (retval == Py_None) {
		*bytes_read = 0;
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;			
	} else {
		g_warning("vfs_read must return an int or None");
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_ERROR_GENERIC;		
	}
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;	
}

static MateVFSResult
do_write (MateVFSMethod *method,
          MateVFSMethodHandle *method_handle,
          gconstpointer buffer,
          MateVFSFileSize num_bytes,
          MateVFSFileSize *bytes_written,
          MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pybuffer;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->write_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	/* XXX: test buffer writes */
	GILSTATE_ENSURE(state);
	pybuffer = PyBuffer_FromMemory((gpointer)buffer, num_bytes);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->write_func,
				     Py_BuildValue("(ONlN)",
						   GET_OBJECT(method_handle),
						   pybuffer,
						   (long)num_bytes,
						   pycontext));
	EXCEPTION_CHECK(retval);

	if (PyInt_Check(retval)) {
		*bytes_written = PyInt_AsLong(retval);
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;
	} else if (retval == Py_None) {
		*bytes_written = 0;
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;			
	} else {
		g_warning("vfs_write must return an int or None");
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_ERROR_GENERIC;		
	}
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;	
}

static MateVFSResult
do_seek (MateVFSMethod       *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition  whence,
	 MateVFSFileOffset    offset,
	 MateVFSContext      *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->seek_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->seek_func,
				     Py_BuildValue("(OiiN)",
						   GET_OBJECT(method_handle),
						   whence,
						   offset,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	
        return MATE_VFS_OK;
}

static MateVFSResult
do_tell	(MateVFSMethod       *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize     *offset_return)
{
	struct PyVFSMethod *pymethod;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->tell_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	retval = PyObject_CallObject(pymethod->tell_func,
				     Py_BuildValue("(O)",
						   GET_OBJECT(method_handle)));
	EXCEPTION_CHECK(retval);
	
	if (PyInt_Check(retval)) {
		*offset_return = PyInt_AsLong(retval);
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;
	} else if (retval == Py_None) {
		*offset_return = 0;
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;			
	} else {
		g_warning("vfs_tell must return an int or None");
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_ERROR_GENERIC;		
	}	
}

static MateVFSResult
do_truncate_handle (MateVFSMethod       *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize      length,
		    MateVFSContext      *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->truncate_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->truncate_func,
				     Py_BuildValue("(OlN)",
						   GET_OBJECT(method_handle),
						   length,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;	
}

static MateVFSResult
do_open_directory (MateVFSMethod           *method,
		   MateVFSMethodHandle    **method_handle,
		   MateVFSURI              *uri,
		   MateVFSFileInfoOptions   options,
		   MateVFSContext          *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->open_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	
	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->open_directory_func,
				     Py_BuildValue("(NiN)",
						   pymate_vfs_uri_new(mate_vfs_uri_ref(uri)),
						   options,
						   pycontext));
	EXCEPTION_CHECK(retval);
	SET_HANDLE(method_handle, retval, uri);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod       *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext      *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->close_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->close_directory_func,
				     Py_BuildValue("(ON)",
						   GET_OBJECT(method_handle),
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod       *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo     *file_info,
		   MateVFSContext      *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyfile;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->read_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pyfile = pymate_vfs_file_info_new(file_info);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->read_directory_func,
				     Py_BuildValue("(ONN)",
						   GET_OBJECT(method_handle),
						   pyfile,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info  (MateVFSMethod          *method,
		   MateVFSURI             *uri,
		   MateVFSFileInfo        *file_info,
		   MateVFSFileInfoOptions  options,
		   MateVFSContext         *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pyfile;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(uri);
	if (!pymethod->get_file_info_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	mate_vfs_file_info_ref(file_info);
	pyfile = pymate_vfs_file_info_new(file_info);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->get_file_info_func,
				     Py_BuildValue("(NNiN)",
						   pyuri,
						   pyfile,
						   options,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod          *method,
			      MateVFSMethodHandle    *method_handle,
			      MateVFSFileInfo        *file_info,
			      MateVFSFileInfoOptions  options,
			      MateVFSContext         *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyfile;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->get_file_info_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	mate_vfs_file_info_ref(file_info);
	pyfile = pymate_vfs_file_info_new(file_info);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->get_file_info_func,
				     Py_BuildValue("(NNiN)",
						   GET_OBJECT(method_handle),
						   pyfile,
						   options,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}


static gboolean
do_is_local (MateVFSMethod *method,
             const MateVFSURI *uri)
{
	struct PyVFSMethod *pymethod;
	MateVFSURI* uri2 = mate_vfs_uri_dup(uri);
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->is_local_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	GILSTATE_ENSURE(state);
	retval = PyObject_CallObject(pymethod->is_local_func,
				     Py_BuildValue("(N)",
						   pymate_vfs_uri_new(uri2)));
	EXCEPTION_CHECK(retval);

	if (PyObject_IsTrue(retval)) {
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return TRUE;
	} else {
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return FALSE;
	}	
}

static MateVFSResult
do_make_directory (MateVFSMethod  *method,
		   MateVFSURI     *uri,
		   guint            perm,
		   MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(uri);
	if (!pymethod->make_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->make_directory_func,
				     Py_BuildValue("(NiN)",
						   pyuri,
						   perm,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_remove_directory (MateVFSMethod  *method,
		     MateVFSURI     *uri,
		     MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->remove_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->remove_directory_func,
				     Py_BuildValue("(NN)",
						   pyuri,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_move (MateVFSMethod  *method,
	 MateVFSURI     *old_uri,
	 MateVFSURI     *new_uri,
	 gboolean         force_replace,
	 MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri_old;
	PyObject *pyuri_new;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(old_uri);
	if (!pymethod->move_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri_old = pymate_vfs_uri_new(mate_vfs_uri_ref(old_uri));
	pyuri_new = pymate_vfs_uri_new(mate_vfs_uri_ref(new_uri));	
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->move_func,
				     Py_BuildValue("(NNNN)",
						   pyuri_old,
						   pyuri_new,
						   PyBool_FromLong(force_replace),
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_unlink (MateVFSMethod  *method,
	   MateVFSURI     *uri,
	   MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->unlink_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->unlink_func,
				     Py_BuildValue("(NN)",
						   pyuri,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_check_same_fs  (MateVFSMethod  *method,
		   MateVFSURI     *a,
		   MateVFSURI     *b,
		   gboolean        *same_fs_return,
		   MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri_a;
	PyObject *pyuri_b;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(a);
	if (!pymethod->check_same_fs_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri_a = pymate_vfs_uri_new(mate_vfs_uri_ref(a));
	pyuri_b = pymate_vfs_uri_new(mate_vfs_uri_ref(b));	
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->check_same_fs_func,
				     Py_BuildValue("(NNN)",
						   pyuri_a,
						   pyuri_b,
						   pycontext));
	EXCEPTION_CHECK(retval);
	
	if (PyObject_IsTrue(retval))
		*same_fs_return = TRUE;
	else
		*same_fs_return = FALSE;
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;		
}

static MateVFSResult
do_set_file_info (MateVFSMethod          *method,
		  MateVFSURI             *a,
		  const MateVFSFileInfo  *info,
		  MateVFSSetFileInfoMask  mask,
		  MateVFSContext         *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;
	PyObject *pyfile;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(a);
	if (!pymethod->set_file_info_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(a));
	pyfile = pymate_vfs_file_info_new(mate_vfs_file_info_dup(info));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->set_file_info_func,
				     Py_BuildValue("(NNiN)",
						   pyuri,
						   pyfile,
						   mask,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);

	return MATE_VFS_OK;
}

static MateVFSResult
do_truncate (MateVFSMethod   *method,
	     MateVFSURI      *uri,
	     MateVFSFileSize  length,
	     MateVFSContext  *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->truncate_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->truncate_func,
				     Py_BuildValue("(NiN)",
						   pyuri,
						   length,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;
}

static MateVFSResult
do_find_directory (MateVFSMethod             *method,
		   MateVFSURI                *find_near_uri,
		   MateVFSFindDirectoryKind   kind,
		   MateVFSURI               **result_uri,
		   gboolean                    create_if_needed,
		   gboolean                    find_if_needed,
		   guint                       perm,
		   MateVFSContext            *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;
	
	pymethod = get_method_from_uri(find_near_uri);
	if (!pymethod->find_directory_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(find_near_uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->find_directory_func,
				     Py_BuildValue("(NibbiN)",
						   pyuri,
						   kind,
						   create_if_needed,
						   find_if_needed,
						   perm,
						   pycontext));
	EXCEPTION_CHECK(retval);
	
	if (pymate_vfs_uri_check(retval)) {
		*result_uri = pymate_vfs_uri_get(retval);
		mate_vfs_uri_ref(*result_uri);
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_OK;
	} else if (retval == Py_None) {
		*result_uri = NULL;
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_ERROR_NOT_FOUND;			
	} else {
		g_warning("vfs_find_directory must return an matevfs.URI or None");
		Py_DECREF(retval);
		GILSTATE_RELEASE(state);
		return MATE_VFS_ERROR_GENERIC;		
	}	
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod  *method,
			 MateVFSURI     *uri,
			 const gchar     *target_reference,
			 MateVFSContext *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pyuri;	
	PyObject *pycontext;
	PyObject *retval;
	PyGILState_STATE state;

	pymethod = get_method_from_uri(uri);
	if (!pymethod->create_symbolic_link_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pyuri = pymate_vfs_uri_new(mate_vfs_uri_ref(uri));
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->create_symbolic_link_func,
				     Py_BuildValue("(NzN)",
						   pyuri,
						   target_reference,
						   pycontext));
	EXCEPTION_CHECK(retval);
	Py_DECREF(retval);
	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;
}


static MateVFSResult
do_file_control (MateVFSMethod       *method,
		 MateVFSMethodHandle *method_handle,
		 const char           *operation,
		 gpointer              operation_data_,
		 MateVFSContext      *context)
{
	struct PyVFSMethod *pymethod;
	PyObject *pycontext;
	PyObject *retval;
	PyGVFSOperationData *operation_data = operation_data_;

	PyGILState_STATE state;

	pymethod = get_method_from_uri(GET_URI(method_handle));
	if (!pymethod->file_control_func) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	  /* we are forced to do evil tricks due to binding-unfriendly
	   * mate_vfs_file_control API */
	if (operation_data->magic != PYGVFS_CONTROL_MAGIC_IN) {
		g_warning("file_control() on python-implemented methods"
			  " can only be used from python");
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

 	GILSTATE_ENSURE(state);
	pycontext = context_new(context);
	retval = PyObject_CallObject(pymethod->file_control_func,
				     Py_BuildValue("(OsON)",
						   GET_OBJECT(method_handle),
						   operation, operation_data->data,
						   pycontext));
	EXCEPTION_CHECK(retval);

	  /* super evil trick fighting super evil api */
	operation_data->magic = PYGVFS_CONTROL_MAGIC_OUT;
	Py_DECREF(operation_data->data);
	operation_data->data = retval;

	GILSTATE_RELEASE(state);
	return MATE_VFS_OK;

}

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	PyObject *instance, *class_object, *d, *m;
	PyObject *path, *py_vfs_dir, *py_home_dir;
	char *full_name;
	char *home_dir;
	PyVFSMethod *pymethod;
	PyGILState_STATE state = 0;
	
	if (pymethod_hash == NULL) {
		pymethod_hash = g_hash_table_new (g_str_hash,
						  g_str_equal);
	}

	if (g_hash_table_lookup(pymethod_hash, method_name)) {
		g_warning("There is already a python method for: %s",
			  method_name);
		return NULL;
	}

	if (!Py_IsInitialized()) {
		Py_Initialize();
	} else {
		GILSTATE_ENSURE(state);
	}

	PyEval_InitThreads();
	init_pymatevfs();

	py_vfs_dir = PyString_FromString(MATE_VFS_PYTHON_DIR);

	home_dir = g_strdup_printf("%s/.mate2/vfs/pythonmethod",
				   g_get_home_dir());
	py_home_dir = PyString_FromString(home_dir);
	g_free(home_dir);

	path = PySys_GetObject("path");
	PyList_Insert(path, 0, py_vfs_dir);
	PyList_Insert(path, 0, py_home_dir);

	Py_DECREF(py_vfs_dir);
	Py_DECREF(py_home_dir);

	m = PyImport_ImportModule(g_strdup(method_name));
	if (!m) {
		PyErr_Print();
		return NULL;
	}
	
	d = PyModule_GetDict(m);

	full_name = g_strdup_printf("%s_method", method_name);
	
	class_object = PyDict_GetItemString(d, full_name);
	if (!class_object) {
		g_warning("module does not have %s defined", full_name);
		return NULL;
	}

	if (!PyClass_Check(class_object)) {
		g_warning("%s must be a class", full_name);
		return NULL;
	}
	g_free(full_name);
	       
	instance = PyInstance_New(class_object,
				  Py_BuildValue("(ss)", method_name, args),
				  NULL);

	pymethod = g_new0(PyVFSMethod, 1);
	pymethod->instance = instance;
	
	pymethod->open_func =                      PyObject_GetAttrString(instance, "vfs_open");
	pymethod->close_func =                     PyObject_GetAttrString(instance, "vfs_close");
	pymethod->create_func =                    PyObject_GetAttrString(instance, "vfs_create");
	pymethod->read_func =                      PyObject_GetAttrString(instance, "vfs_read");	
	pymethod->write_func =                     PyObject_GetAttrString(instance, "vfs_write");
	pymethod->seek_func =                      PyObject_GetAttrString(instance, "vfs_seek");	
	pymethod->tell_func =                      PyObject_GetAttrString(instance, "vfs_tell");
	pymethod->truncate_handle_func =           PyObject_GetAttrString(instance, "vfs_truncate_handle");
	pymethod->open_directory_func =            PyObject_GetAttrString(instance, "vfs_open_directory");
	pymethod->close_directory_func =           PyObject_GetAttrString(instance, "vfs_close_directory");
	pymethod->read_directory_func =            PyObject_GetAttrString(instance, "vfs_read_directory");
	pymethod->get_file_info_func =             PyObject_GetAttrString(instance, "vfs_get_file_info");
	pymethod->get_file_info_from_handle_func = PyObject_GetAttrString(instance, "vfs_get_file_info_from_handle");
	pymethod->is_local_func =                  PyObject_GetAttrString(instance, "vfs_is_local");
	pymethod->make_directory_func =            PyObject_GetAttrString(instance, "vfs_make_directory");
	pymethod->find_directory_func =            PyObject_GetAttrString(instance, "vfs_find_directory");
	pymethod->remove_directory_func =          PyObject_GetAttrString(instance, "vfs_remove_directory");
	pymethod->move_func =                      PyObject_GetAttrString(instance, "vfs_move");
	pymethod->unlink_func =                    PyObject_GetAttrString(instance, "vfs_unlink");
	pymethod->check_same_fs_func =             PyObject_GetAttrString(instance, "vfs_check_same_fs");
	pymethod->set_file_info_func =             PyObject_GetAttrString(instance, "vfs_set_file_info");
	pymethod->truncate_func =                  PyObject_GetAttrString(instance, "vfs_truncate");
	pymethod->create_symbolic_link_func =      PyObject_GetAttrString(instance, "vfs_create_symbolic_link");
#if 0
	add_method(monitor_add,               "vfs_monitor_add");
	add_method(monitor_cancel,            "vfs_monitor_cancel");
#endif
	pymethod->file_control_func =              PyObject_GetAttrString(instance, "vfs_file_control");

	g_hash_table_insert(pymethod_hash,
			    g_strdup(method_name),
			    pymethod);

#undef add_method

	/* XXX: Make sure open/close/create/read are there */

	if (state) {
		GILSTATE_RELEASE(state);
	} else {
		PyEval_ReleaseLock();
	}

	return &method;

}
 
void
vfs_module_shutdown (MateVFSMethod *method)
{
	Py_Finalize();
}
