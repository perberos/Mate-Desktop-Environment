/* -*- mode: C; c-basic-offset: 4 -*-
 * pymatecorba - a Python language mapping for the MateCORBA2 CORBA ORB
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "pymatecorba-private.h"

/* escape names that happen to be python keywords.  Uses the standard
 * 'keyword' module, so that we don't need to maintain our own list. */
gchar *
_pymatecorba_escape_name(const gchar *name)
{
    static PyObject *iskeyword = NULL;
    PyObject *py_ret;
    gchar *ret;

    if (!iskeyword) {
	PyObject *keyword_mod;

	keyword_mod = PyImport_ImportModule("keyword");
	g_assert(keyword_mod != NULL);
	iskeyword = PyObject_GetAttrString(keyword_mod, "iskeyword");
	g_assert(iskeyword != NULL);
	Py_DECREF(keyword_mod);
    }

    py_ret = PyObject_CallFunction(iskeyword, "s", name);
    if (py_ret && PyObject_IsTrue(py_ret))
	ret = g_strconcat("_", name, NULL);
    else
	ret = g_strdup(name);
    Py_XDECREF(py_ret);
    PyErr_Clear();

    return ret;
}

static PyMethodDef fake_module_methods[] = { { NULL, NULL } };

PyObject *
_pymatecorba_get_container(const gchar *repo_id, gboolean is_poa)
{
    const gchar *slash;
    PyObject *parent = NULL;

    if (strncmp(repo_id, "IDL:", 4) != 0) {
	g_warning("bad repo_id %s", repo_id);
	return NULL;
    }
    repo_id += 4;

    /* get rid of omg.org prefix ... */
    if (strncmp(repo_id, "omg.org/", 8) == 0)
	repo_id += 8;

    while ((slash = strchr(repo_id, '/')) != NULL) {
	gchar *component = g_strndup(repo_id, slash - repo_id);

	/* check if we can already find the component */
	if (parent) {
	    PyObject *attr = PyObject_GetAttrString(parent, component);
	    gchar *escaped_name, *importname;

	    if (attr) {
		Py_DECREF(parent);
		parent = attr;
		goto cont;
	    }
	    PyErr_Clear();
	    if (!PyModule_Check(parent)) {
		g_warning("parent not a module, and component not found");
		g_free(component);
		Py_DECREF(parent);
		parent = NULL;
		break;
	    }
	    escaped_name = _pymatecorba_escape_name(component);
	    importname = g_strconcat(PyModule_GetName(parent),
				     ".", escaped_name, NULL);
	    g_free(escaped_name);
	    attr = PyImport_ImportModule(importname);
	    if (attr) {
		Py_DECREF(parent);
		parent = attr;
		g_free(importname);
		goto cont;
	    }
	    PyErr_Clear();
	    attr = Py_InitModule(importname, fake_module_methods);
	    g_free(importname);
	    if (!attr) {
		g_warning("could not construct module");
		g_free(component);
		Py_DECREF(parent);
		parent = NULL;
		break;
	    }
	    Py_INCREF(attr); /* you don't own the return of Py_InitModule */
	    PyObject_SetAttrString(parent, component, attr);
	    Py_DECREF(parent);
	    parent = attr;
	} else {
	    PyObject *mod;
	    gchar *modname;

	    if (is_poa)
		modname = g_strconcat(component, "__POA", NULL);
	    else
		modname = _pymatecorba_escape_name(component);
	    mod = PyImport_ImportModule(modname);
	    if (mod) {
		g_free(modname);
		parent = mod;
		goto cont;
	    }
	    PyErr_Clear();
	    mod = Py_InitModule(modname, fake_module_methods);
	    g_free(modname);
	    if (!mod) {
		g_warning("could not construct module");
		g_free(component);
		break;
	    }
	    parent = mod;
	    Py_INCREF(parent);
	}
    cont:
	g_free(component);
	repo_id = &slash[1];
    }
    if (!parent) {
	if (is_poa)
	    parent = PyImport_ImportModule("_GlobalIDL__POA");
	else
	    parent = PyImport_ImportModule("_GlobalIDL");
	if (!parent) {
	    PyErr_Clear();
	    if (is_poa)
		parent = Py_InitModule("_GlobalIDL__POA", fake_module_methods);
	    else
		parent = Py_InitModule("_GlobalIDL", fake_module_methods);
	    if (!parent) {
		g_warning("could not create _GlobalIDL module");
	    } else
                Py_INCREF(parent); /* you don't own the return of Py_InitModule */
	}
    }
    return parent;
}
