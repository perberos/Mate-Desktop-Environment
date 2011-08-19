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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <signal.h>

#include <gmodule.h>

#include "totem-plugin.h"
#include "totem-python-module.h"
#include "totem-python-plugin.h"

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

#define TOTEM_PYTHON_MODULE_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						 TOTEM_TYPE_PYTHON_MODULE, \
						 TotemPythonModulePrivate))

typedef struct
{
	gchar *module;
	gchar *path;
	GType type;
} TotemPythonModulePrivate;

enum
{
	PROP_0,
	PROP_PATH,
	PROP_MODULE
};

/* indicates whether all initializations in
 * totem_python_module_init_python were successful */
gboolean python_initialized = FALSE;

#ifndef PYGOBJECT_CAN_MARSHAL_GVALUE
static PyObject *
pyg_value_g_value_as_pyobject (const GValue *value)
{
	return pyg_value_as_pyobject((GValue *)g_value_get_boxed(value), FALSE);
}

static int
pyg_value_g_value_from_pyobject (GValue *value, PyObject *obj)
{
	GType type;
	GValue obj_value = { 0, };
	
	type = pyg_type_from_object((PyObject *) obj->ob_type);
	if (! type) {
		PyErr_Clear();
		return -1;
	}
	g_value_init(&obj_value, type);
	if (pyg_value_from_pyobject(&obj_value, obj) == -1) {
		g_value_unset(&obj_value);
		return -1;
	}
	g_value_set_boxed(value, &obj_value);
	g_value_unset(&obj_value);
	return 0;
}
#endif /* PYGOBJECT_CAN_MARSHAL_GVALUE */

/* Exported by Totem Python module */
void pytotem_register_classes (PyObject *d);
void pytotem_add_constants (PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pytotem_functions[];

/* We retreive this to check for correct class hierarchy */
static PyTypeObject *PyTotemPlugin_Type;

G_DEFINE_TYPE (TotemPythonModule, totem_python_module, G_TYPE_TYPE_MODULE);

static void
totem_python_module_init_python (void)
{
	PyObject *pygtk, *mdict, *require;
	PyObject *totem, *gtk, *pygtk_version, *pygtk_required_version;
	PyObject *gettext, *install, *gettext_args;
	PyObject *sys_path;
	struct sigaction old_sigint;
	gint res;
	char *argv[] = { "totem", NULL };
	GList *paths;

	if (Py_IsInitialized ()) {
		g_warning ("Python should only be initialized once, since it's in class_init");
		g_return_if_reached ();
	}

	/* Hack to make python not overwrite SIGINT: this is needed to avoid
	 * the crash reported on gedit bug #326191 */

	/* Save old handler */
	res = sigaction (SIGINT, NULL, &old_sigint);
	if (res != 0) {
		g_warning ("Error initializing Python interpreter: cannot get "
		           "handler to SIGINT signal (%s)",
		           strerror (errno));

		return;
	}

	/* Python initialization */
	Py_Initialize ();

	/* Restore old handler */
	res = sigaction (SIGINT, &old_sigint, NULL);
	if (res != 0) {
		g_warning ("Error initializing Python interpreter: cannot restore "
		           "handler to SIGINT signal (%s)",
		           strerror (errno));
		return;
	}

	PySys_SetArgv (1, argv);

	/* pygtk.require("2.0") */
	pygtk = PyImport_ImportModule ("pygtk");
	if (pygtk == NULL) {
		g_warning ("Could not import pygtk, check your installation");
		PyErr_Print();
		return;
	}

	mdict = PyModule_GetDict (pygtk);
	require = PyDict_GetItemString (mdict, "require");
	PyObject_CallObject (require, Py_BuildValue ("(S)", PyString_FromString ("2.0")));
	if (PyErr_Occurred ()) {
		g_warning ("Could not get required pygtk version, check your installation");
		PyErr_Print();
		return;
	}

	/* import gobject */
	init_pygobject ();

	/* disable pyg* log hooks, since ours is more interesting */
#ifdef pyg_disable_warning_redirections
	pyg_disable_warning_redirections ();
#endif
#ifndef PYGOBJECT_CAN_MARSHAL_GVALUE
	pyg_register_gtype_custom (G_TYPE_VALUE, pyg_value_g_value_as_pyobject, pyg_value_g_value_from_pyobject);
#endif

	/* import gtk */
	init_pygtk ();

	pyg_enable_threads ();

	gtk = PyImport_ImportModule ("gtk");
	if (gtk == NULL) {
		g_warning ("Could not import gtk");
		PyErr_Print();
		return;
	}

	mdict = PyModule_GetDict (gtk);
	pygtk_version = PyDict_GetItemString (mdict, "pygtk_version");
	pygtk_required_version = Py_BuildValue ("(iii)", 2, 12, 0);
	if (PyObject_Compare (pygtk_version, pygtk_required_version) == -1) {
		g_warning("PyGTK %s required, but %s found, check your installation.",
				  PyString_AsString (PyObject_Repr (pygtk_required_version)),
				  PyString_AsString (PyObject_Repr (pygtk_version)));
		Py_DECREF (pygtk_required_version);
		return;
	}
	Py_DECREF (pygtk_required_version);

	/* import totem */
	paths = totem_get_plugin_paths ();
	sys_path = PySys_GetObject ("path");
	while (paths != NULL) {
		PyObject *path;

		path = PyString_FromString (paths->data);
		if (PySequence_Contains (sys_path, path) == 0) {
			PyList_Insert (sys_path, 0, path);
		}
		Py_DECREF (path);
		g_free (paths->data);
		paths = g_list_delete_link (paths, paths);
	}

	totem = PyImport_ImportModule ("totem");

	if (totem == NULL) {
		g_warning ("Could not import Python module 'totem', check your installation");
		PyErr_Print ();
		return;
	}

	/* add pytotem_functions */
	for (res = 0; pytotem_functions[res].ml_name != NULL; res++) {
		PyObject *func;

		func = PyCFunction_New (&pytotem_functions[res], totem);
		if (func == NULL) {
			g_warning ("unable object for function '%s' create", pytotem_functions[res].ml_name);
			PyErr_Print ();
			return;
		}
		if (PyModule_AddObject (totem, pytotem_functions[res].ml_name, func) < 0) {
			g_warning ("unable to insert function '%s' in 'totem' module", pytotem_functions[res].ml_name);
			PyErr_Print ();
			return;
		}
	}
	mdict = PyModule_GetDict (totem);

	pytotem_register_classes (mdict);
	pytotem_add_constants (totem, "TOTEM_");

	/* Retreive the Python type for totem.Plugin */
	PyTotemPlugin_Type = (PyTypeObject *) PyDict_GetItemString (mdict, "Plugin");
	if (PyTotemPlugin_Type == NULL) {
		PyErr_Print ();
		return;
	}

	/* i18n support */
	gettext = PyImport_ImportModule ("gettext");
	if (gettext == NULL) {
		g_warning ("Could not import gettext");
		PyErr_Print();
		return;
	}

	mdict = PyModule_GetDict (gettext);
	install = PyDict_GetItemString (mdict, "install");
	gettext_args = Py_BuildValue ("ss", GETTEXT_PACKAGE, MATELOCALEDIR);
	PyObject_CallObject (install, gettext_args);
	Py_DECREF (gettext_args);

	/* ideally totem should clean up the python stuff again at some point,
	 * for which we'd need to save the result of SaveThread so we can then
	 * restore the state in a class_finalize or so, but since totem doesn't
	 * do any clean-up at this point, we'll just skip this as well */
	PyEval_SaveThread();

	/* Python and the supporting libraries were initialised
	 * successfully */
	python_initialized = TRUE;
}

static gboolean
totem_python_module_load (GTypeModule *gmodule)
{
	TotemPythonModulePrivate *priv = TOTEM_PYTHON_MODULE_GET_PRIVATE (gmodule);
	PyGILState_STATE state;
	PyObject *main_module, *main_locals, *locals, *key, *value;
	PyObject *module, *fromlist;
	Py_ssize_t pos = 0;
	gboolean res = FALSE;

	g_return_val_if_fail (Py_IsInitialized (), FALSE);
	if (python_initialized == FALSE)
		return FALSE;

	state = pyg_gil_state_ensure();

	main_module = PyImport_AddModule ("__main__");
	if (main_module == NULL)
	{
		g_warning ("Could not get __main__.");
		goto done;
	}

	/* If we have a special path, we register it */
	if (priv->path != NULL)
	{
		PyObject *sys_path = PySys_GetObject ("path");
		PyObject *path = PyString_FromString (priv->path);

		if (PySequence_Contains(sys_path, path) == 0)
			PyList_Insert (sys_path, 0, path);

		Py_DECREF(path);
	}

	main_locals = PyModule_GetDict (main_module);
	/* we need a fromlist to be able to import modules with a '.' in the
	   name. */
	fromlist = PyTuple_New(0);
	module = PyImport_ImportModuleEx (priv->module, main_locals,
					  main_locals, fromlist);
	Py_DECREF (fromlist);
	if (!module) {
		PyErr_Print ();
		goto done;
	}

	locals = PyModule_GetDict (module);
	while (PyDict_Next (locals, &pos, &key, &value))
	{
		if (!PyType_Check(value))
			continue;

		if (PyObject_IsSubclass (value, (PyObject*) PyTotemPlugin_Type))
		{
			priv->type = totem_python_object_get_type (gmodule, value);
			res = TRUE;
			goto done;
		}
	}

	g_warning ("Failed to find any totem.Plugin-derived classes in Python plugin");

done:

	pyg_gil_state_release (state);
	return res;
}

static void
totem_python_module_unload (GTypeModule *module)
{
	TotemPythonModulePrivate *priv = TOTEM_PYTHON_MODULE_GET_PRIVATE (module);
	g_debug ("Unloading Python module");

	priv->type = 0;
}

GObject *
totem_python_module_new_object (TotemPythonModule *module)
{
	TotemPythonModulePrivate *priv = TOTEM_PYTHON_MODULE_GET_PRIVATE (module);
	TotemPythonObject *object;

	if (priv->type == 0)
		return NULL;

	g_debug ("Creating object of type %s", g_type_name (priv->type));
	object = (TotemPythonObject*) (g_object_new (priv->type,
						  "name", priv->module,
						  NULL));
	if (object->instance == NULL) {
		g_warning ("Could not instantiate Python object");
		return NULL;
	}

	/* FIXME, HACK: this is a hack because the gobject object->instance references
	 * isn't the same gobject as we think it is. Which Causes Issues.
	 *
	 * This still has issues, notably that it isn't safe to call any totem.Plugin methods
	 * from python before we get here.
	 *
	 * The solution is to not have weird proxy objects.
	 */
	g_object_set (((PyGObject*)(object->instance))->obj, "name", priv->module, NULL);

	return G_OBJECT (object);
}

static void
totem_python_module_init (TotemPythonModule *module)
{
	g_debug ("Init of Python module");
}

static void
totem_python_module_finalize (GObject *object)
{
	TotemPythonModulePrivate *priv = TOTEM_PYTHON_MODULE_GET_PRIVATE (object);

	if (priv && priv->type) {
		g_debug ("Finalizing Python module %s", g_type_name (priv->type));

		g_free (priv->module);
		g_free (priv->path);
	}

	G_OBJECT_CLASS (totem_python_module_parent_class)->finalize (object);
}

static void
totem_python_module_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	/* no readable properties */
	g_return_if_reached ();
}

static void
totem_python_module_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	TotemPythonModule *mod = TOTEM_PYTHON_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE:
			TOTEM_PYTHON_MODULE_GET_PRIVATE (mod)->module = g_value_dup_string (value);
			break;
		case PROP_PATH:
			TOTEM_PYTHON_MODULE_GET_PRIVATE (mod)->path = g_value_dup_string (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
totem_python_module_class_init (TotemPythonModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	object_class->finalize = totem_python_module_finalize;
	object_class->get_property = totem_python_module_get_property;
	object_class->set_property = totem_python_module_set_property;

	g_object_class_install_property
			(object_class,
			 PROP_MODULE,
			 g_param_spec_string ("module",
					      "Module Name",
					      "The Python module to load for this plugin",
					      NULL,
					      G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property
			(object_class,
			 PROP_PATH,
			 g_param_spec_string ("path",
					      "Path",
					      "The Python path to use when loading this module",
					      NULL,
					      G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (TotemPythonModulePrivate));

	module_class->load = totem_python_module_load;
	module_class->unload = totem_python_module_unload;

	/* Init Python subsystem, this should happen only once
	 * in the process lifetime, and doing it here is OK since
	 * class_init is called once */
	totem_python_module_init_python ();
}

TotemPythonModule *
totem_python_module_new (const gchar *path,
			 const gchar *module)
{
	TotemPythonModule *result;
	gchar *dir;

	if (module == NULL || module[0] == '\0')
		return NULL;

	dir = g_path_get_dirname (path);
	result = g_object_new (TOTEM_TYPE_PYTHON_MODULE,
			       "module", module,
			       "path", dir,
			       NULL);
	g_free (dir);

	g_type_module_set_name (G_TYPE_MODULE (result), module);

	return result;
}

/* --- these are not module methods, they are here out of convenience --- */

#if 0
static gint idle_garbage_collect_id = 0;

static gboolean
run_gc (gpointer data)
{
	gboolean ret = (PyGC_Collect () != 0);

	if (!ret)
		idle_garbage_collect_id = 0;

	return ret;
}
#endif

void
totem_python_garbage_collect (void)
{
#if 0
	if (Py_IsInitialized() && idle_garbage_collect_id == 0) {
		idle_garbage_collect_id = g_idle_add (run_gc, NULL);
	}
#endif
}

#if 0
static gboolean
finalise_collect_cb (gpointer data)
{
	while (PyGC_Collect ())
		;

	/* Useful if Python is refusing to give up its shell reference for some reason.
	PyRun_SimpleString ("import gc, gobject\nfor o in gc.get_objects():\n\tif isinstance(o, gobject.GObject):\n\t\tprint o, gc.get_referrers(o)");
	*/

	return TRUE;
}
#endif

void
totem_python_shutdown (void)
{
#if 0
	if (Py_IsInitialized ()) {
		if (idle_garbage_collect_id != 0) {
			g_source_remove (idle_garbage_collect_id);
			idle_garbage_collect_id = 0;
		}

		while (run_gc (NULL))
			/* loop */;

		/* This helps to force Python to give up its shell reference */
		g_idle_add (finalise_collect_cb, NULL);

		/* Disable for now, due to bug 334188
		Py_Finalize ();*/
	}
#endif
}
