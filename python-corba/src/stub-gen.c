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
#include <string.h>

#define PY_SIZE_T_CLEAN

#include "pymatecorba-private.h"

/* repo_id -> CORBA_TypeCode hash */
static GHashTable *type_codes = NULL;
/* repo_id -> PyObject stub hash */
static GHashTable *stubs = NULL;

static void
init_hash_tables(void)
{
    static gboolean called = FALSE;

    if (called) return;
    called = TRUE;

    type_codes = g_hash_table_new_full(g_str_hash, g_str_equal,
    				       NULL,
    				       (GDestroyNotify)CORBA_Object_release);
    stubs = g_hash_table_new(g_str_hash, g_str_equal);
}

void
pymatecorba_register_stub(CORBA_TypeCode tc, PyObject *stub)
{
    init_hash_tables();

    if (tc->repo_id) {
	CORBA_Object_duplicate((CORBA_Object)tc, NULL);
	g_hash_table_replace(type_codes, tc->repo_id, tc);
    }

    if (stub) {
	PyObject *stub_dict = NULL;
	Py_INCREF(stub);
	g_hash_table_insert(stubs, tc->repo_id, stub);

	if (!strncmp(tc->repo_id, "IDL:omg.org/CORBA", 17)) {
	    gchar *other_repo_id = g_strconcat("IDL:", &tc->repo_id[12], NULL);

	    g_hash_table_insert(stubs, other_repo_id, stub);
	}

	if (PyType_Check(stub))
	    stub_dict = ((PyTypeObject *)stub)->tp_dict;
	else if (PyClass_Check(stub))
	    stub_dict = ((PyClassObject *)stub)->cl_dict;

	if (stub_dict && !PyDict_GetItemString(stub_dict, "__typecode__")) {
	    PyObject *py_tc = pycorba_typecode_new(tc);

	    PyDict_SetItemString(stub_dict, "__typecode__", py_tc);
	    Py_DECREF(py_tc);
	}
    }
}

CORBA_TypeCode
pymatecorba_lookup_typecode(const gchar *repo_id)
{
    if (repo_id == NULL) return NULL;
    return  g_hash_table_lookup(type_codes, repo_id);
}

PyObject *
pymatecorba_get_stub(CORBA_TypeCode tc)
{
    PyObject *stub;

    init_hash_tables();

    if (!tc->repo_id) return NULL;

    stub = g_hash_table_lookup(stubs, tc->repo_id);
    /* if we didn't get a typecode, and the repoid is in the type_codes
     * hash, try generating a stub. */
    if (!stub && tc->repo_id && g_hash_table_lookup(type_codes, tc->repo_id) == NULL) {
	pymatecorba_generate_typecode_stubs(tc);
        stub = g_hash_table_lookup(stubs, tc->repo_id);
    }
    return stub;
}
PyObject *
pymatecorba_get_stub_from_repo_id(const gchar *repo_id)
{
    init_hash_tables();

    if (repo_id == NULL) return NULL;
    return g_hash_table_lookup(stubs, repo_id);
}

static void
add_stub_to_container(CORBA_TypeCode tc, const gchar *name, PyObject *stub)
{
    PyObject *container;
    gchar *pyname;

    container = _pymatecorba_get_container(tc->repo_id, FALSE);
    if (!container)
	return;

    pyname = _pymatecorba_escape_name(name);
    if (PyType_Check(container)) {
	PyObject *container_dict = ((PyTypeObject *)container)->tp_dict;

	PyDict_SetItemString(container_dict, pyname, stub);
    } else {
	PyObject_SetAttrString(container, pyname, stub);
    }
    g_free(pyname);
    if (PyErr_Occurred())
	PyErr_Clear();

    /* set __module__ if it is not an alias ... */
    if (tc->kind != CORBA_tk_alias &&
	(PyType_Check(stub) || PyClass_Check(stub))) {
	PyObject *module = NULL;

	if (PyModule_Check(container)) {
	    const gchar *name;
	    name = PyModule_GetName(container);
	    if (name) module = PyString_FromString(name);
	} else {
	    module = PyObject_GetAttrString(container, "__module__");
	}
	if (module) {
	    PyObject_SetAttrString(stub, "__module__", module);
	    Py_DECREF(module);
	}
    }
	
    Py_DECREF(container);
}

static PyObject *
generate_struct_stub(CORBA_TypeCode tc)
{
    PyObject *stub, *class_dict;

    class_dict = PyDict_New();
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
				 tc->name, (PyObject *)&PyCORBA_Struct_Type,
				 class_dict);
    Py_DECREF(class_dict);
    return stub;
}

static PyObject *
generate_union_stub(CORBA_TypeCode tc)
{
    PyObject *stub, *class_dict;

    class_dict = PyDict_New();
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
				 tc->name, (PyObject *)&PyCORBA_Union_Type,
				 class_dict);
    pymatecorba_add_union_members_to_stub(stub, tc);
    Py_DECREF(class_dict);
    return stub;
}

static PyObject *
generate_exception_stub(CORBA_TypeCode tc)
{
    PyObject *exception;
    gchar *name;
    gint i;

    if (!strncmp(tc->repo_id, "IDL:omg.org/", 12))
	name = g_strdup(&tc->repo_id[12]);
    else if (!strncmp(tc->repo_id, "IDL:", 4))
	name = g_strdup(&tc->repo_id[4]);
    else
	name = g_strdup(tc->repo_id);

    for (i = 0; name[i] != '\0'; i++) {
	if (name[i] == '/') {
	    name[i] = '.';
	} else if (name[i] == ':') {
	    name[i] = '\0';
	    break;
	}
    }
    exception = PyErr_NewException(name, pymatecorba_user_exception, PyDict_New());
    g_free(name);
    return exception;
}

static PyObject *
generate_enum_stub(CORBA_TypeCode tc)
{
    PyObject *container;
    PyObject *stub, *values;
    gchar *pyname;
    Py_ssize_t i;

    container = _pymatecorba_get_container(tc->repo_id, FALSE);
    if (!container)
	return NULL;

    stub = _pymatecorba_generate_enum(tc, &values);
    for (i = 0; i < tc->sub_parts; i++) {
	PyObject *item = PyTuple_GetItem(values, i);
	pyname = _pymatecorba_escape_name(tc->subnames[i]);
	PyObject_SetAttrString(container, pyname, item);
	g_free(pyname);
    }

    Py_DECREF(container);

    return stub;
}

void
pymatecorba_generate_typecode_stubs(CORBA_TypeCode tc)
{
    PyObject *stub = NULL;

    init_hash_tables();
    
    switch (tc->kind) {
    case CORBA_tk_null:
    case CORBA_tk_void:
    case CORBA_tk_short:
    case CORBA_tk_long:
    case CORBA_tk_ushort:
    case CORBA_tk_ulong:
    case CORBA_tk_float:
    case CORBA_tk_double:
    case CORBA_tk_boolean:
    case CORBA_tk_char:
    case CORBA_tk_octet:
    case CORBA_tk_any:
    case CORBA_tk_TypeCode:
    case CORBA_tk_Principal:
	break;
    case CORBA_tk_objref:
	break;
    case CORBA_tk_struct:
	stub = generate_struct_stub(tc);
	break;
    case CORBA_tk_union:
	stub = generate_union_stub(tc);
	break;
    case CORBA_tk_enum:
	stub = generate_enum_stub(tc);
	break;
    case CORBA_tk_string:
    case CORBA_tk_sequence:
    case CORBA_tk_array:
	break;
    case CORBA_tk_alias:
	stub = pymatecorba_get_stub(tc->subtypes[0]);
	break;
    case CORBA_tk_except:
	stub = generate_exception_stub(tc);
	break;
    case CORBA_tk_longlong:
    case CORBA_tk_ulonglong:
    case CORBA_tk_longdouble:
    case CORBA_tk_wchar:
    case CORBA_tk_wstring:
    case CORBA_tk_fixed:
    case CORBA_tk_value:
    case CORBA_tk_value_box:
    case CORBA_tk_native:
    case CORBA_tk_abstract_interface:
	break;
    }

    if (stub)
	add_stub_to_container(tc, tc->name, stub);

    pymatecorba_register_stub(tc, stub);
}

void
pymatecorba_generate_iinterface_stubs(MateCORBA_IInterface *iface)
{
    CORBA_TypeCode tc;
    PyObject *stub, *bases, *class_dict, *slots;
    PyObject **base_list;
    gint i, j, n_bases;

    init_hash_tables();

    tc = iface->tc;
    /* has wrapper already been generated? */
    if (g_hash_table_lookup(stubs, tc->repo_id))
	return;

    /* create bases tuple */
    base_list = g_new(PyObject *, iface->base_interfaces._length);
    for (i = 0; i < iface->base_interfaces._length; i++) {
	const gchar *base_repo_id = iface->base_interfaces._buffer[i];
	PyObject *base = pymatecorba_get_stub_from_repo_id(base_repo_id);
	
	/* if we haven't wrapped the base, try and look it up */
	if (!base) {
	    MateCORBA_IInterface *base_iface;
	    CORBA_Environment ev;

	    CORBA_exception_init(&ev);
	    base_iface = MateCORBA_small_get_iinterface(CORBA_OBJECT_NIL,
						    base_repo_id, &ev);
	    if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning("repo id for base %s has not been registered",
			  base_repo_id);
		CORBA_exception_free(&ev);
		for (j = 0; j < i; j++) Py_DECREF(base_list[j]);
		g_free(base_list);
		return;
	    }
	    CORBA_exception_free(&ev);

	    /* generate interface, then get it */
	    pymatecorba_generate_iinterface_stubs(base_iface);
	    base = pymatecorba_get_stub_from_repo_id(base_repo_id);
	    if (!base) {
		g_warning("could not generate stub for base %s", base_repo_id);
		for (j = 0; j < i; j++) Py_DECREF(base_list[j]);
		g_free(base_list);
		return;
	    }
	}
	Py_INCREF(base);
	base_list[i] = base;
    }

    /* get rid of unneeded bases  */
    n_bases = iface->base_interfaces._length;
    for (i = 0; i < iface->base_interfaces._length; i++) {
	for (j = 0; j < iface->base_interfaces._length; j++) {
	    if (i == j) continue;
	    /* if base[j] is a subclass of base[i], we can ignore [i] */
	    if (base_list[j] &&
		PyType_IsSubtype((PyTypeObject *)base_list[j],
				 (PyTypeObject *)base_list[i])) {
		Py_DECREF(base_list[i]);
		base_list[i] = NULL;
		n_bases--;
		break;
	    }
	}
    }
    bases = PyTuple_New(n_bases);
    n_bases = 0;
    for (i = 0; i < iface->base_interfaces._length; i++) {
	if (base_list[i])
	    PyTuple_SetItem(bases, n_bases++, base_list[i]);
    }
    g_free(base_list);

    class_dict = PyDict_New();
    slots = PyTuple_New(0);
    PyDict_SetItemString(class_dict, "__slots__", slots);
    Py_DECREF(slots);
    stub = PyObject_CallFunction((PyObject *)&PyType_Type, "sOO",
				 tc->name, bases, class_dict);
    Py_DECREF(bases);
    Py_DECREF(class_dict);
    if (!stub) {
	g_message("couldn't build stub %s:", tc->name);
	PyErr_Print();
	PyErr_Clear();
	return;
    }

    pymatecorba_add_imethods_to_stub(stub, &iface->methods);

    add_stub_to_container(tc, tc->name, stub);

    pymatecorba_register_stub(tc, stub);
}

static PyObject *
get_iinterface_stub_from_objref(CORBA_Object objref, const gchar *repo_id,
				CORBA_Environment *ev)
{
    PyObject *stub;
    MateCORBA_IInterface *iface;
    gint i;

    stub = pymatecorba_get_stub_from_repo_id(repo_id);
    if (stub) return stub;

    iface = MateCORBA_small_get_iinterface(objref, repo_id, ev);
    if (ev->_major != CORBA_NO_EXCEPTION)
	return NULL;

    /* make sure all base classes have stubs ... */
    for (i = 0; i < iface->base_interfaces._length; i++) {
	const gchar *base_repo_id = iface->base_interfaces._buffer[i];
	PyObject *base_stub;

	if (!base_repo_id) continue;
	base_stub = get_iinterface_stub_from_objref(objref, base_repo_id, ev);
	if (ev->_major != CORBA_NO_EXCEPTION) {
	    CORBA_free(iface);
	    return NULL;
	}
    }
    /* finally, generate stub */
    pymatecorba_generate_iinterface_stubs(iface);
    stub = pymatecorba_get_stub_from_repo_id(repo_id);
    return stub;
}

PyObject *
pymatecorba_get_stub_from_objref(CORBA_Object objref)
{
    CORBA_Environment ev;
    CORBA_string repo_id = NULL;
    PyObject *stub = NULL;

    CORBA_exception_init(&ev);
    repo_id = MateCORBA_small_get_type_id(objref, &ev);
    if (ev._major != CORBA_NO_EXCEPTION) goto cleanup;

    stub = get_iinterface_stub_from_objref(objref, repo_id, &ev);

 cleanup:
    if (repo_id) CORBA_free(repo_id);
    CORBA_exception_free(&ev);
    return stub;
}
