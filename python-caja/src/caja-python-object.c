/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 *  Copyright (C) 2004 Novell, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Dave Camp <dave@ximian.com>
 * 
 */

#include <config.h>

#include "caja-python-object.h"
#include "caja-python.h"

#include <libcaja-extension/caja-extension-types.h>

#include <pygobject.h>

/* Caja extension headers */
#include <libcaja-extension/caja-file-info.h>
#include <libcaja-extension/caja-info-provider.h>
#include <libcaja-extension/caja-column-provider.h>
#include <libcaja-extension/caja-location-widget-provider.h>
#include <libcaja-extension/caja-menu-item.h>
#include <libcaja-extension/caja-menu-provider.h>
#include <libcaja-extension/caja-property-page-provider.h>

#include <string.h>

#define METHOD_PREFIX ""

static GObjectClass *parent_class;

/* These macros assumes the following things:
 *   a METHOD_NAME is defined with is a string
 *   a goto label called beach
 *   the return value is called ret
 */

#define CHECK_METHOD_NAME(self)                                        \
	if (!PyObject_HasAttrString(self, METHOD_NAME))                    \
		goto beach;

#define CHECK_OBJECT(object)										   \
  	if (object->instance == NULL)									   \
  	{																   \
  		g_object_unref (object);									   \
  		goto beach;													   \
  	}																   \

#define CONVERT_LIST(py_files, files)                                  \
	{                                                                  \
		GList *l;                                                      \
        py_files = PyList_New(0);                                      \
		for (l = files; l; l = l->next)                                \
		{                                                              \
			PyList_Append(py_files, pygobject_new((GObject*)l->data)); \
		}                                                              \
	}

#define HANDLE_RETVAL(py_ret)                                          \
    if (!py_ret)                                                       \
    {                                                                  \
        PyErr_Print();                                                 \
		goto beach;                                                    \
 	}                                                                  \
 	else if (py_ret == Py_None)                                        \
 	{                                                                  \
 		goto beach;                                                    \
	}

#define HANDLE_LIST(py_ret, type, type_name)                           \
    {                                                                  \
        Py_ssize_t i = 0;                                              \
    	if (!PySequence_Check(py_ret) || PyString_Check(py_ret))       \
    	{                                                              \
    		PyErr_SetString(PyExc_TypeError,                           \
    						METHOD_NAME " must return a sequence");    \
    		goto beach;                                                \
    	}                                                              \
    	for (i = 0; i < PySequence_Size (py_ret); i++)                 \
    	{                                                              \
    		PyGObject *py_item;                                        \
    		py_item = (PyGObject*)PySequence_GetItem (py_ret, i);      \
    		if (!pygobject_check(py_item, &Py##type##_Type))           \
    		{                                                          \
    			PyErr_SetString(PyExc_TypeError,                       \
    							METHOD_NAME                            \
    							" must return a sequence of "          \
    							type_name);                            \
    			goto beach;                                            \
    		}                                                          \
    		ret = g_list_append (ret, (type*) g_object_ref(py_item->obj));  \
            Py_DECREF(py_item);                                        \
    	}                                                              \
    }


static void
free_pygobject_data(gpointer data, gpointer user_data)
{
	/* Some CajaFile objects are cached and not freed until caja
		itself is closed.  Since PyGObject stores data that must be freed by
		the Python interpreter, we must always free it before the interpreter
		is finalized. */
	g_object_set_data((GObject *)data, "PyGObject::instance-data", NULL);
}

static void
free_pygobject_data_list(GList *list)
{
	if (list == NULL)
		return;
		
	g_list_foreach(list, (GFunc)free_pygobject_data, NULL);
}

#define METHOD_NAME "get_property_pages"
static GList *
caja_python_object_get_property_pages (CajaPropertyPageProvider *provider,
										   GList 						*files)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    PyObject *py_files, *py_ret = NULL;
    GList *ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

	CONVERT_LIST(py_files, files);
	
    py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 "(N)", py_files);
	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, CajaPropertyPage, "caja.PropertyPage");
	
 beach:
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME


static void
caja_python_object_property_page_provider_iface_init (CajaPropertyPageProviderIface *iface)
{
	iface->get_pages = caja_python_object_get_property_pages;
}

#define METHOD_NAME "get_widget"
static GtkWidget *
caja_python_object_get_widget (CajaLocationWidgetProvider *provider,
								   const char 				 	  *uri,
								   GtkWidget 					  *window)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
	GtkWidget *ret = NULL;
	PyObject *py_ret = NULL;
	PyGObject *py_ret_gobj;
	PyObject *py_uri = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();

	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

	py_uri = PyString_FromString(uri);

	py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 "(NN)", py_uri,
								 pygobject_new((GObject *)window));
	HANDLE_RETVAL(py_ret);

	py_ret_gobj = (PyGObject *)py_ret;
	if (!pygobject_check(py_ret_gobj, &PyGtkWidget_Type))
	{
		PyErr_SetString(PyExc_TypeError,
					    METHOD_NAME "should return a gtk.Widget");
		goto beach;
	}
	ret = (GtkWidget *)g_object_ref(py_ret_gobj->obj);

 beach:
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
	return ret;
}
#undef METHOD_NAME

static void
caja_python_object_location_widget_provider_iface_init (CajaLocationWidgetProviderIface *iface)
{
	iface->get_widget = caja_python_object_get_widget;
}

#define METHOD_NAME "get_file_items"
static GList *
caja_python_object_get_file_items (CajaMenuProvider *provider,
									   GtkWidget 			*window,
									   GList 				*files)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL, *py_files;
	PyGILState_STATE state = pyg_gil_state_ensure();
	PyObject *provider_version = NULL;
	
  	debug_enter();

	CHECK_OBJECT(object);	

	if (PyObject_HasAttrString(object->instance, "get_file_items_full"))
	{
		CONVERT_LIST(py_files, files);
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX "get_file_items_full",
									 "(NNN)",
									 pygobject_new((GObject *)provider), 
									 pygobject_new((GObject *)window), 
									 py_files);
	}
	else if (PyObject_HasAttrString(object->instance, "get_file_items"))
	{
		CONVERT_LIST(py_files, files);
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
									 "(NN)", 
									 pygobject_new((GObject *)window), 
									 py_files);
	}
	else
	{
		goto beach;
	}

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, CajaMenuItem, "caja.MenuItem");

 beach:
 	free_pygobject_data_list(files);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

#define METHOD_NAME "get_background_items"
static GList *
caja_python_object_get_background_items (CajaMenuProvider *provider,
											 GtkWidget 			  *window,
											 CajaFileInfo 	  *file)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();

	CHECK_OBJECT(object);

	if (PyObject_HasAttrString(object->instance, "get_background_items_full"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX "get_background_items_full",
									 "(NNN)",
									 pygobject_new((GObject *)provider),
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else if (PyObject_HasAttrString(object->instance, "get_background_items"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
									 "(NN)",
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else
	{
		goto beach;
	}

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, CajaMenuItem, "caja.MenuItem");
	
 beach:
	free_pygobject_data(file, NULL);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

#define METHOD_NAME "get_toolbar_items"
static GList *
caja_python_object_get_toolbar_items (CajaMenuProvider *provider,
										  GtkWidget 		   *window,
										  CajaFileInfo 	   *file)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();
	
  	debug_enter();
  	
	CHECK_OBJECT(object);

	if (PyObject_HasAttrString(object->instance, "get_toolbar_items_full"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX "get_toolbar_items_full",
									 "(NNN)",
									 pygobject_new((GObject *)provider),
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else if (PyObject_HasAttrString(object->instance, "get_toolbar_items"))
	{
		py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
									 "(NN)",
									 pygobject_new((GObject *)window),
									 pygobject_new((GObject *)file));
	}
	else
	{
		goto beach;
	}
	
	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, CajaMenuItem, "caja.MenuItem");
	
 beach:
 	free_pygobject_data(file, NULL);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
caja_python_object_menu_provider_iface_init (CajaMenuProviderIface *iface)
{
	iface->get_background_items = caja_python_object_get_background_items;
	iface->get_toolbar_items = caja_python_object_get_toolbar_items;
	iface->get_file_items = caja_python_object_get_file_items;
}

#define METHOD_NAME "get_columns"
static GList *
caja_python_object_get_columns (CajaColumnProvider *provider)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    GList *ret = NULL;
    PyObject *py_ret;
	PyGILState_STATE state = pyg_gil_state_ensure();                                    \

	debug_enter();
		
	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

    py_ret = PyObject_CallMethod(object->instance, METHOD_PREFIX METHOD_NAME,
								 NULL);

	HANDLE_RETVAL(py_ret);

	HANDLE_LIST(py_ret, CajaColumn, "caja.Column");
	
 beach:
 	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
caja_python_object_column_provider_iface_init (CajaColumnProviderIface *iface)
{
	iface->get_columns = caja_python_object_get_columns;
}


#define METHOD_NAME "cancel_update"
static void
caja_python_object_cancel_update (CajaInfoProvider 		*provider,
									  CajaOperationHandle 	*handle)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
	PyGILState_STATE state = pyg_gil_state_ensure();

  	debug_enter();

	CHECK_OBJECT(object);
	CHECK_METHOD_NAME(object->instance);

    PyObject_CallMethod(object->instance,
								 METHOD_PREFIX METHOD_NAME, "(NN)",
								 pygobject_new((GObject*)provider),
								 pyg_pointer_new(G_TYPE_POINTER, handle));

 beach:
	pyg_gil_state_release(state);
}
#undef METHOD_NAME

#define METHOD_NAME "update_file_info"
static CajaOperationResult
caja_python_object_update_file_info (CajaInfoProvider 		*provider,
										 CajaFile 				*file,
										 GClosure 					*update_complete,
										 CajaOperationHandle   **handle)
{
	CajaPythonObject *object = (CajaPythonObject*)provider;
    CajaOperationResult ret = CAJA_OPERATION_COMPLETE;
    PyObject *py_ret = NULL;
	PyGILState_STATE state = pyg_gil_state_ensure();                                    \

  	debug_enter();

	CHECK_OBJECT(object);

	if (PyObject_HasAttrString(object->instance, "update_file_info_full"))
	{
		py_ret = PyObject_CallMethod(object->instance,
									 METHOD_PREFIX "update_file_info_full", "(NNNN)",
									 pygobject_new((GObject*)provider),
									 pyg_pointer_new(G_TYPE_POINTER, *handle),
									 pyg_boxed_new(G_TYPE_CLOSURE, update_complete, TRUE, TRUE),
									 pygobject_new((GObject*)file));
	}
	else if (PyObject_HasAttrString(object->instance, "update_file_info"))
	{
		py_ret = PyObject_CallMethod(object->instance,
									 METHOD_PREFIX METHOD_NAME, "(N)",
									 pygobject_new((GObject*)file));
	}
	else
	{
		goto beach;
	}
	
	HANDLE_RETVAL(py_ret);

	if (!PyInt_Check(py_ret))
	{
		PyErr_SetString(PyExc_TypeError,
						METHOD_NAME " must return None or a int");
		goto beach;
	}

	ret = PyInt_AsLong(py_ret);
	
 beach:
 	free_pygobject_data(file, NULL);
	Py_XDECREF(py_ret);
	pyg_gil_state_release(state);
    return ret;
}
#undef METHOD_NAME

static void
caja_python_object_info_provider_iface_init (CajaInfoProviderIface *iface)
{
	iface->cancel_update = caja_python_object_cancel_update;
	iface->update_file_info = caja_python_object_update_file_info;
}

static void 
caja_python_object_instance_init (CajaPythonObject *object)
{
	CajaPythonObjectClass *class;
  	debug_enter();

	class = (CajaPythonObjectClass*)(((GTypeInstance*)object)->g_class);

	object->instance = PyObject_CallObject(class->type, NULL);
	if (object->instance == NULL)
		PyErr_Print();
}

static void
caja_python_object_finalize (GObject *object)
{
  	debug_enter();

	if (((CajaPythonObject *)object)->instance != NULL)
		Py_DECREF(((CajaPythonObject *)object)->instance);
}

static void
caja_python_object_class_init (CajaPythonObjectClass *class,
								   gpointer 				  class_data)
{
	debug_enter();

	parent_class = g_type_class_peek_parent (class);
	
	class->type = (PyObject*)class_data;
	
	G_OBJECT_CLASS (class)->finalize = caja_python_object_finalize;
}

GType 
caja_python_object_get_type (GTypeModule *module, 
								 PyObject 	*type)
{
	GTypeInfo *info;
	const char *type_name;
	GType gtype;
	  
	static const GInterfaceInfo property_page_provider_iface_info = {
		(GInterfaceInitFunc) caja_python_object_property_page_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo location_widget_provider_iface_info = {
		(GInterfaceInitFunc) caja_python_object_location_widget_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) caja_python_object_menu_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo column_provider_iface_info = {
		(GInterfaceInitFunc) caja_python_object_column_provider_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo info_provider_iface_info = {
		(GInterfaceInitFunc) caja_python_object_info_provider_iface_init,
		NULL,
		NULL
	};

	debug_enter_args("type=%s", PyString_AsString(PyObject_GetAttrString(type, "__name__")));
	info = g_new0 (GTypeInfo, 1);
	
	info->class_size = sizeof (CajaPythonObjectClass);
	info->class_init = (GClassInitFunc)caja_python_object_class_init;
	info->instance_size = sizeof (CajaPythonObject);
	info->instance_init = (GInstanceInitFunc)caja_python_object_instance_init;

	info->class_data = type;
	Py_INCREF(type);

	type_name = g_strdup_printf("%s+CajaPython",
								PyString_AsString(PyObject_GetAttrString(type, "__name__")));
		
	gtype = g_type_module_register_type (module, 
										 G_TYPE_OBJECT,
										 type_name,
										 info, 0);

	if (PyObject_IsSubclass(type, (PyObject*)&PyCajaPropertyPageProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 CAJA_TYPE_PROPERTY_PAGE_PROVIDER,
									 &property_page_provider_iface_info);
	}

	if (PyObject_IsSubclass(type, (PyObject*)&PyCajaLocationWidgetProvider_Type))
	{
		g_type_module_add_interface (module, gtype,
									 CAJA_TYPE_LOCATION_WIDGET_PROVIDER,
									 &location_widget_provider_iface_info);
	}
	
	if (PyObject_IsSubclass(type, (PyObject*)&PyCajaMenuProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 CAJA_TYPE_MENU_PROVIDER,
									 &menu_provider_iface_info);
	}

	if (PyObject_IsSubclass(type, (PyObject*)&PyCajaColumnProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 CAJA_TYPE_COLUMN_PROVIDER,
									 &column_provider_iface_info);
	}
	
	if (PyObject_IsSubclass(type, (PyObject*)&PyCajaInfoProvider_Type))
	{
		g_type_module_add_interface (module, gtype, 
									 CAJA_TYPE_INFO_PROVIDER,
									 &info_provider_iface_info);
	}
	
	return gtype;
}
