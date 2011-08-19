/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Heavily based on code from Rhythmbox and Gedit.
 *
 * Copyright (C) 2005 Raphael Slinckx
 * Copyright (C) 2007 Philip Withnall
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Saturday 19th May 2007: Philip Withnall: Add exception clause.
 * See license_change file for details.
 */

#include <config.h>

#include "totem-python-plugin.h"
#include "totem-plugin.h"

#include <pygobject.h>
#include <string.h>

static GObjectClass *parent_class;

static PyObject *
call_python_method (TotemPythonObject *object,
		    TotemObject       *totem,
		    gchar             *method)
{
	PyGILState_STATE state;
	PyObject *py_ret = NULL;

	state = pyg_gil_state_ensure();

	g_return_val_if_fail (PyObject_HasAttrString (object->instance, method), NULL);

	if (totem == NULL) {
		py_ret = PyObject_CallMethod (object->instance,
					      method,
					      NULL);
	} else {
		py_ret = PyObject_CallMethod (object->instance,
					      method,
					      "(N)",
					      pygobject_new (G_OBJECT (totem)));
	}

	if (!py_ret)
		PyErr_Print ();

	pyg_gil_state_release (state);

	return py_ret;
}

static gboolean
check_py_object_is_gtk_widget (PyObject *py_obj)
{
	static PyTypeObject *_PyGtkWidget_Type = NULL;
	PyGILState_STATE state;
	gboolean res = FALSE;

	state = pyg_gil_state_ensure();

	if (_PyGtkWidget_Type == NULL) {
		PyObject *module;

		if ((module = PyImport_ImportModule ("gtk"))) {
			PyObject *moddict = PyModule_GetDict (module);
			_PyGtkWidget_Type = (PyTypeObject *) PyDict_GetItemString (moddict, "Widget");
		}

		if (_PyGtkWidget_Type == NULL) {
			PyErr_SetString(PyExc_TypeError, "could not find python gtk widget type");
			PyErr_Print();
			res = FALSE;
			goto done;
		}
	}

	res = PyObject_TypeCheck (py_obj, _PyGtkWidget_Type) ? TRUE : FALSE;

done:
	pyg_gil_state_release (state);
	return res;
}

static void
impl_deactivate (TotemPlugin *plugin,
		 TotemObject *totem)
{
	PyGILState_STATE state = pyg_gil_state_ensure ();
	TotemPythonObject *object = (TotemPythonObject *)plugin;

	if (PyObject_HasAttrString (object->instance, "deactivate")) {
		PyObject *py_ret = call_python_method (object, totem, "deactivate");

		if (py_ret) {
			Py_XDECREF (py_ret);
		}
	} else {
		TOTEM_PLUGIN_CLASS (parent_class)->deactivate (plugin, totem);
	}

	pyg_gil_state_release (state);
}

static gboolean
impl_activate (TotemPlugin *plugin,
	       TotemObject *totem,
	       GError **error)
{
	PyGILState_STATE state = pyg_gil_state_ensure ();
	TotemPythonObject *object = (TotemPythonObject *)plugin;

	if (PyObject_HasAttrString (object->instance, "activate")) {
		PyObject *py_ret = call_python_method (object, totem, "activate");

		if (py_ret) {
			Py_XDECREF (py_ret);
		}
	} else {
		TOTEM_PLUGIN_CLASS (parent_class)->activate (plugin, totem, error);
	}

	pyg_gil_state_release (state);

	return TRUE;
}

static GtkWidget *
impl_create_configure_dialog (TotemPlugin *plugin)
{
	PyGILState_STATE state = pyg_gil_state_ensure ();
	TotemPythonObject *object = (TotemPythonObject *)plugin;
	GtkWidget *ret = NULL;

	if (PyObject_HasAttrString (object->instance, "create_configure_dialog")) {
		PyObject *py_ret = call_python_method (object, NULL, "create_configure_dialog");

		if (py_ret) {
			if (check_py_object_is_gtk_widget (py_ret)) {
				ret = GTK_WIDGET (pygobject_get (py_ret));
				g_object_ref (ret);
			} else {
				PyErr_SetString(PyExc_TypeError, "return value for create_configure_dialog is not a GtkWidget");
				PyErr_Print();
			}

			Py_DECREF (py_ret);
		}
	} else {
		ret = TOTEM_PLUGIN_CLASS (parent_class)->create_configure_dialog (plugin);
	}

	pyg_gil_state_release (state);
	return ret;
}

static gboolean
impl_is_configurable (TotemPlugin *plugin)
{
	PyGILState_STATE state = pyg_gil_state_ensure ();
	TotemPythonObject *object = (TotemPythonObject *) plugin;
	PyObject *dict = object->instance->ob_type->tp_dict;
	gboolean result;

	if (dict == NULL)
		result = FALSE;
	else if (!PyDict_Check(dict))
		result = FALSE;
	else
		result = PyDict_GetItemString(dict, "create_configure_dialog") != NULL;

	pyg_gil_state_release (state);

	return result;
}

static void
totem_python_object_init (TotemPythonObject *object)
{
	TotemPythonObjectClass *class;
	PyGILState_STATE state;

	state = pyg_gil_state_ensure();

	g_debug ("Creating Python plugin instance");

	class = (TotemPythonObjectClass*) (((GTypeInstance*) object)->g_class);

	object->instance = PyObject_CallObject (class->type, NULL);
	if (object->instance == NULL)
		PyErr_Print();

	pyg_gil_state_release (state);
}

static void
totem_python_object_finalize (GObject *object)
{
	g_debug ("Finalizing Python plugin instance");

	if (((TotemPythonObject *) object)->instance) {
		PyGILState_STATE state;

		state = pyg_gil_state_ensure();
		Py_DECREF (((TotemPythonObject *) object)->instance);
		pyg_gil_state_release (state);
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
totem_python_object_class_init (TotemPythonObjectClass *klass,
			     gpointer                class_data)
{
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	klass->type = (PyObject*) class_data;

	object_class->finalize = totem_python_object_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->create_configure_dialog = impl_create_configure_dialog;
	plugin_class->is_configurable = impl_is_configurable;
}

GType
totem_python_object_get_type (GTypeModule *module,
			      PyObject    *type)
{
	GType gtype;
	gchar *type_name;

	GTypeInfo info = {
		sizeof (TotemPythonObjectClass),
		NULL,		/* base_init */
		NULL,		/* base_finalize */
		(GClassInitFunc) totem_python_object_class_init,
		NULL,		/* class_finalize */
		type,		/* class_data */
		sizeof (TotemPythonObject),
		0,		/* n_preallocs */
		(GInstanceInitFunc) totem_python_object_init
	};

	/* no need for pyg_gil_state_ensure() here since this function is only
	 * used from within totem_python_module_load() where this is already
	 * done */

	Py_INCREF (type);

	type_name = g_strdup_printf ("%s+TotemPythonPlugin",
				     PyString_AsString (PyObject_GetAttrString (type, "__name__")));

	g_debug ("Registering Python plugin instance: %s", type_name);
	gtype = g_type_module_register_type (module,
					     TOTEM_TYPE_PLUGIN,
					     type_name,
					     &info, 0);
	g_free (type_name);

	return gtype;
}
