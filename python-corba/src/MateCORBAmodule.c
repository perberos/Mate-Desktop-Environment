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

#define PY_SIZE_T_CLEAN

#include "pymatecorba-private.h"
#include <string.h>
#include <sysmodule.h>
#include <matecorba/poa/poa-types.h>
#ifdef HAVE_MATECORBA2_IMODULE
#  include <MateCORBAservices/matecorba-imodule.h>
#endif

PortableServer_POA _pymatecorba_poa;

static void
pymatecorba_handle_types_and_interfaces(CORBA_sequence_MateCORBA_IInterface *ifaces,
				    CORBA_sequence_CORBA_TypeCode *types,
				    const gchar *file)
{
    gint i;

    for (i = 0; i < ifaces->_length; i++) {
	if (ifaces->_buffer[i].tc->kind == CORBA_tk_null)
	    g_warning("%s is possibly broken: tc->kind == tk_null",
		      file);

	pymatecorba_generate_iinterface_stubs(&ifaces->_buffer[i]);
	_pymatecorba_register_skel(&ifaces->_buffer[i]);
    }

    for (i = 0; i < types->_length; i++) {
	CORBA_TypeCode tc = types->_buffer[i];

	if (tc->kind == CORBA_tk_null ||
	    (tc->kind==CORBA_tk_alias && tc->subtypes[0]->kind==CORBA_tk_null))
	    g_warning("%s is possibly broken: tc->kind == tk_null",
		      file);
	pymatecorba_generate_typecode_stubs(tc);
    }
}

static PyObject *
pymatecorba_load_typelib(PyObject *self, PyObject *args)
{
    gchar *typelib;
    CORBA_sequence_MateCORBA_IInterface *ifaces;
    CORBA_sequence_CORBA_TypeCode *types;

    if (!PyArg_ParseTuple(args, "s", &typelib))
	return NULL;
    if (!MateCORBA_small_load_typelib(typelib)) {
	PyErr_SetString(PyExc_RuntimeError, "could not load typelib");
	return NULL;
    }
    ifaces = MateCORBA_small_get_iinterfaces(typelib);
    types = MateCORBA_small_get_types(typelib);

    pymatecorba_handle_types_and_interfaces(ifaces, types, typelib);

    CORBA_free(ifaces);
    CORBA_free(types);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
pymatecorba_load_file(PyObject *self, PyObject *args)
{
#ifdef HAVE_MATECORBA2_IMODULE
    gchar *path, *cpp_args = "";
    CORBA_sequence_MateCORBA_IInterface *ifaces;
    CORBA_sequence_CORBA_TypeCode *types;

    if (!PyArg_ParseTuple(args, "s|s", &path, &cpp_args))
	return NULL;
    ifaces = MateCORBA_iinterfaces_from_file(path, cpp_args, &types);
    if (!ifaces) {
	PyErr_Format(PyExc_RuntimeError, "could not load '%s'", path);
	return NULL;
    }

    pymatecorba_handle_types_and_interfaces(ifaces, types, path);

#if 0
    CORBA_free(ifaces);
    CORBA_free(types);
#endif

    Py_INCREF(Py_None);
    return Py_None;
#else
    PyErr_SetString(PyExc_RuntimeError,
		    "pymatecorba was not configured with support for the "
		    "imodule service");
    return NULL;
#endif
}

static PyMethodDef matecorba_functions[] = {
    { "load_typelib", pymatecorba_load_typelib, METH_VARARGS },
    { "load_file",    pymatecorba_load_file,    METH_VARARGS },
    { NULL, NULL, 0 }
};

static PyObject *
pymatecorba_corba_orb_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "argv", "orb_id", NULL};
    PyObject *py_argv = NULL;
    gchar *orb_id = "matecorba-local-orb";
    int argc, i;
    gchar **argv;
    CORBA_Environment ev;
    CORBA_ORB orb;
    PyObject *pyorb;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O!s:CORBA.ORB_init", kwlist,
                                     &PyList_Type, &py_argv, &orb_id))
	return NULL;

    if (py_argv && PyList_Size(py_argv) > 0) {
	argc = PyList_Size(py_argv);
	argv = g_new(gchar *, argc);
	for (i = 0; i < argc; i++) {
	    PyObject *item = PyList_GetItem(py_argv, i);
	    
	    if (!PyString_Check(item)) {
		PyErr_SetString(PyExc_TypeError,
				"argv must be a list of strings");
		g_free(argv);
		return NULL;
	    }
            argv[i] = PyString_AsString(item);
	}
    } else {
	argc = 1;
	argv = g_new(gchar *, argc);
	argv[0] = "python";
    }
    CORBA_exception_init(&ev);
    orb = CORBA_ORB_init(&argc, argv, orb_id, &ev);
    g_free(argv);

      /* Check if we have a threaded ORB */
    if (strstr(orb_id, "matecorba-io-thread"))
        PyEval_InitThreads();

    _pymatecorba_poa = (PortableServer_POA) CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;
    PortableServer_POAManager_activate (PortableServer_POA__get_the_POAManager (_pymatecorba_poa, &ev), &ev);
    if (pymatecorba_check_ex(&ev))
	return NULL;

    pyorb = pycorba_orb_new(orb);
    CORBA_Object_duplicate((CORBA_Object)orb, NULL);
    return pyorb;
}

static PyMethodDef corba_functions[] = {
    { "ORB_init", (PyCFunction) pymatecorba_corba_orb_init, METH_KEYWORDS },
    { NULL, NULL, 0 }
};

static void
add_tc_constants(PyObject *corbamod)
{
    PyModule_AddObject(corbamod, "TC_null",
		       pycorba_typecode_new(TC_null));
    PyModule_AddObject(corbamod, "TC_void",
		       pycorba_typecode_new(TC_void));
    PyModule_AddObject(corbamod, "TC_short",
		       pycorba_typecode_new(TC_CORBA_short));
    PyModule_AddObject(corbamod, "TC_long",
		       pycorba_typecode_new(TC_CORBA_long));
    PyModule_AddObject(corbamod, "TC_longlong",
		       pycorba_typecode_new(TC_CORBA_long_long));
    PyModule_AddObject(corbamod, "TC_ushort",
		       pycorba_typecode_new(TC_CORBA_unsigned_short));
    PyModule_AddObject(corbamod, "TC_ulong",
		       pycorba_typecode_new(TC_CORBA_unsigned_long));
    PyModule_AddObject(corbamod, "TC_ulonglong",
		       pycorba_typecode_new(TC_CORBA_unsigned_long_long));
    PyModule_AddObject(corbamod, "TC_float",
		       pycorba_typecode_new(TC_CORBA_float));
    PyModule_AddObject(corbamod, "TC_double",
		       pycorba_typecode_new(TC_CORBA_double));
    PyModule_AddObject(corbamod, "TC_longdouble",
		       pycorba_typecode_new(TC_CORBA_long_double));
    PyModule_AddObject(corbamod, "TC_boolean",
		       pycorba_typecode_new(TC_CORBA_boolean));
    PyModule_AddObject(corbamod, "TC_char",
		       pycorba_typecode_new(TC_CORBA_char));
    PyModule_AddObject(corbamod, "TC_wchar",
		       pycorba_typecode_new(TC_CORBA_wchar));
    PyModule_AddObject(corbamod, "TC_octet",
		       pycorba_typecode_new(TC_CORBA_octet));
    PyModule_AddObject(corbamod, "TC_any",
		       pycorba_typecode_new(TC_CORBA_any));
    PyModule_AddObject(corbamod, "TC_TypeCode",
		       pycorba_typecode_new(TC_CORBA_TypeCode));
    PyModule_AddObject(corbamod, "TC_Object",
		       pycorba_typecode_new(TC_CORBA_Object));
    PyModule_AddObject(corbamod, "TC_string",
		       pycorba_typecode_new(TC_CORBA_string));
    PyModule_AddObject(corbamod, "TC_wstring",
		       pycorba_typecode_new(TC_CORBA_wstring));
}

static void
register_corba_types(void)
{
    pymatecorba_register_stub(TC_null, NULL);
    pymatecorba_register_stub(TC_void, NULL);
    pymatecorba_register_stub(TC_CORBA_short, NULL);
    pymatecorba_register_stub(TC_CORBA_long, NULL);
    pymatecorba_register_stub(TC_CORBA_long_long, NULL);
    pymatecorba_register_stub(TC_CORBA_unsigned_short, NULL);
    pymatecorba_register_stub(TC_CORBA_unsigned_long, NULL);
    pymatecorba_register_stub(TC_CORBA_unsigned_long_long, NULL);
    pymatecorba_register_stub(TC_CORBA_float, NULL);
    pymatecorba_register_stub(TC_CORBA_double, NULL);
    pymatecorba_register_stub(TC_CORBA_long_double, NULL);
    pymatecorba_register_stub(TC_CORBA_boolean, NULL);
    pymatecorba_register_stub(TC_CORBA_char, NULL);
    pymatecorba_register_stub(TC_CORBA_wchar, NULL);
    pymatecorba_register_stub(TC_CORBA_octet, NULL);
    pymatecorba_register_stub(TC_CORBA_any, NULL);
    pymatecorba_register_stub(TC_CORBA_TypeCode,(PyObject*)&PyCORBA_TypeCode_Type);
    pymatecorba_register_stub(TC_CORBA_Object, (PyObject *)&PyCORBA_Object_Type);
    pymatecorba_register_stub(TC_CORBA_string, NULL);
    pymatecorba_register_stub(TC_CORBA_wstring, NULL);

    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ConstructionPolicy); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Current); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_DomainManager); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Policy); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_AliasDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ArrayDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_AttributeDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ConstantDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Contained); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Container); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_EnumDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ExceptionDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_FixedDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_IDLType); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_InterfaceDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_IRObject); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ModuleDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_NativeDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_OperationDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_PrimitiveDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Repository); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_SequenceDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_StringDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_StructDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_TypedefDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_UnionDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueBoxDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueMemberDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_WstringDef); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Identifier); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_completion_status);
    pymatecorba_generate_typecode_stubs(TC_CORBA_exception_type);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_PolicyType); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_PolicyList); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_PolicyErrorCode); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_PolicyError);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_DomainManagersList); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ScopedName); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_RepositoryId); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_DefinitionKind);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_VersionSpec); */
    /* ** pymatecorba_generate_typecode_stubs(TC_CORBA_Contained_Description); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_InterfaceDefSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueDefSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ContainedSeq); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_StructMember);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_StructMemberSeq); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_Initializer);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_InitializerSeq); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_UnionMember);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_UnionMemberSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_EnumMemberSeq); */
    /* ** pymatecorba_generate_typecode_stubs(TC_CORBA_Container_Description); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Container_DescriptionSeq); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_PrimitiveKind);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ModuleDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ConstantDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_TypeDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ExceptionDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_AttributeMode);
    pymatecorba_generate_typecode_stubs(TC_CORBA_AttributeDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_OperationMode);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ParameterMode);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ParameterDescription);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ParDescriptionSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ContextIdentifier); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ContextIdSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ExceptionDefSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ExcDescriptionSeq); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_OperationDescription);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_RepositoryIdSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_OpDescriptionSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_AttrDescriptionSeq); */
    /* ** pymatecorba_generate_typecode_stubs(TC_CORBA_InterfaceDef_FullInterfaceDescription); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_InterfaceDescription);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Visibility); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_ValueMember);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueMemberSeq); */
    /* ** pymatecorba_generate_typecode_stubs(TC_CORBA_ValueDef_FullValueDescription); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_ValueDescription);
    pymatecorba_generate_typecode_stubs(TC_CORBA_TCKind);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ValueModifier); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_TypeCode_Bounds);
    pymatecorba_generate_typecode_stubs(TC_CORBA_TypeCode_BadKind);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_AnySeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_BooleanSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_CharSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_WCharSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_OctetSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ShortSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_UShortSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_LongSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ULongSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_LongLongSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ULongLongSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_FloatSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_DoubleSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_Flags); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_NamedValue);
    pymatecorba_generate_typecode_stubs(TC_CORBA_SetOverrideType);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_RequestSeq); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ServiceType); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ServiceOption); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ServiceDetailType); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_ServiceDetail);
    pymatecorba_generate_typecode_stubs(TC_CORBA_ServiceInformation);
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ORB_ObjectId); */
    /* pymatecorba_generate_typecode_stubs(TC_CORBA_ORB_ObjectIdList); */
    pymatecorba_generate_typecode_stubs(TC_CORBA_ORB_InvalidName);
}

static PyMethodDef portableserver_functions[] = {
    { NULL, NULL, 0 }
};

static struct _PyMateCORBA_APIStruct api = {
    &PyCORBA_Object_Type,
    &PyCORBA_ORB_Type,
    &PyCORBA_TypeCode_Type,
    &PyCORBA_Any_Type,
    &PyPortableServer_POA_Type,
    &PyPortableServer_POAManager_Type,

    pycorba_object_new,
    pycorba_orb_new,
    pycorba_typecode_new,
    pycorba_any_new,
    pymatecorba_poa_new,
    pymatecorba_poamanager_new,

    pymatecorba_check_ex,
    pymatecorba_marshal_any,
    pymatecorba_demarshal_any,
    pymatecorba_check_python_ex
};

void initMateCORBA(void);


static void
add_matecorba_constants(PyObject *matecorbamod)
{
#define add_constant(arg) \
    PyModule_AddIntConstant(matecorbamod, #arg, MATECORBA_##arg);
    
    add_constant(THREAD_HINT_PER_OBJECT);
    add_constant(THREAD_HINT_PER_POA);
    add_constant(THREAD_HINT_PER_CONNECTION);
    add_constant(THREAD_HINT_PER_REQUEST);
    add_constant(THREAD_HINT_ONEWAY_AT_IDLE);
    add_constant(THREAD_HINT_ALL_AT_IDLE);
    add_constant(THREAD_HINT_ON_CONTEXT);
    add_constant(THREAD_HINT_NONE);

#undef add_constant
}


DL_EXPORT(void)
initMateCORBA(void)
{
    PyObject *mod, *modules_dict, *corbamod, *psmod;

#define INIT_TYPE(tp) G_STMT_START{ \
    if (!tp.ob_type)  tp.ob_type  = &PyType_Type; \
    if (!tp.tp_alloc) tp.tp_alloc = PyType_GenericAlloc; \
    if (!tp.tp_new)   tp.tp_new   = PyType_GenericNew; \
    if (PyType_Ready(&tp) < 0) \
        return; }G_STMT_END

    INIT_TYPE(PyCORBA_TypeCode_Type);
    INIT_TYPE(PyCORBA_Object_Type);
    INIT_TYPE(PyCORBA_Method_Type);
    INIT_TYPE(PyCORBA_BoundMethod_Type);
    INIT_TYPE(PyCORBA_ORB_Type);
    INIT_TYPE(PyCORBA_Any_Type);
    INIT_TYPE(PyCORBA_Struct_Type);
    INIT_TYPE(PyCORBA_Union_Type);
    INIT_TYPE(PyCORBA_UnionMember_Type);
    PyCORBA_Enum_Type.tp_base = &PyInt_Type;
    INIT_TYPE(PyCORBA_Enum_Type);
    INIT_TYPE(PyCORBA_fixed_Type);
    INIT_TYPE(PyPortableServer_Servant_Type);
    INIT_TYPE(PyMateCORBA_ObjectAdaptor_Type);
    PyPortableServer_POA_Type.tp_base = &PyMateCORBA_ObjectAdaptor_Type;
    INIT_TYPE(PyPortableServer_POA_Type);
    INIT_TYPE(PyPortableServer_POAManager_Type);
    INIT_TYPE(PyCORBA_Policy_Type);

#undef INIT_TYPE

    modules_dict = PySys_GetObject("modules");

    mod = Py_InitModule("MateCORBA", matecorba_functions);

    PyModule_AddObject(mod, "matecorba_version",
		       Py_BuildValue("(iii)",
				     matecorba_major_version,
				     matecorba_minor_version,
				     matecorba_micro_version));

    PyModule_AddObject(mod, "__version__",
		       Py_BuildValue("(iii)",
				     PYMATECORBA_MAJOR_VERSION,
				     PYMATECORBA_MINOR_VERSION,
				     PYMATECORBA_MICRO_VERSION));
    PyModule_AddObject(mod, "_PyMateCORBA_API",
		       PyCObject_FromVoidPtr(&api, NULL));
    add_matecorba_constants(mod);

    corbamod = Py_InitModule("MateCORBA.CORBA", corba_functions);
    Py_INCREF(corbamod);
    PyModule_AddObject(mod, "CORBA", corbamod);
    PyDict_SetItemString(modules_dict, "CORBA", corbamod);

    PyModule_AddObject(corbamod, "TypeCode",
		       (PyObject *)&PyCORBA_TypeCode_Type);
    PyModule_AddObject(corbamod, "Object",
		       (PyObject *)&PyCORBA_Object_Type);
    PyModule_AddObject(corbamod, "ORB",
		       (PyObject *)&PyCORBA_ORB_Type);
    PyModule_AddObject(corbamod, "Any",
		       (PyObject *)&PyCORBA_Any_Type);
    PyModule_AddObject(corbamod, "fixed",
		       (PyObject *)&PyCORBA_fixed_Type);

    PyModule_AddObject(corbamod, "TRUE",  Py_True);
    PyModule_AddObject(corbamod, "FALSE", Py_False);
    PyModule_AddObject(corbamod, "Policy",
		       (PyObject *)&PyCORBA_Policy_Type);

    pymatecorba_register_exceptions(corbamod);

    register_corba_types();
    add_tc_constants(corbamod);

    psmod = Py_InitModule("MateCORBA.PortableServer", portableserver_functions);
    Py_INCREF(psmod);
    PyModule_AddObject(mod, "PortableServer", psmod);
    PyDict_SetItemString(modules_dict, "PortableServer", psmod);
    PyModule_AddObject(psmod, "ObjectAdaptor",
		       (PyObject *)&PyMateCORBA_ObjectAdaptor_Type);
    PyModule_AddObject(psmod, "POA",
		       (PyObject *)&PyPortableServer_POA_Type);
    PyModule_AddObject(psmod, "POAManager",
		       (PyObject *)&PyPortableServer_POAManager_Type);
    PyModule_AddObject(psmod, "Servant",
		       (PyObject *)&PyPortableServer_Servant_Type);

    PyModule_AddIntConstant(psmod, "ORB_CTRL_MODEL", PortableServer_ORB_CTRL_MODEL);
    PyModule_AddIntConstant(psmod, "SINGLE_THREAD_MODEL", PortableServer_SINGLE_THREAD_MODEL);
    PyModule_AddIntConstant(psmod, "MAIN_THREAD_MODEL", PortableServer_MAIN_THREAD_MODEL);

}
