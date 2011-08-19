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

#ifndef PYMATECORBA_PRIVATE_H
#define PYMATECORBA_PRIVATE_H

#ifdef PYMATECORBA_H
#  error "don't include pymatecorba.h and pymatecorba-private.h together"
#endif

#define _INSIDE_PYMATECORBA_
#include "pymatecorba.h"
#undef _INSIDE_PYMATECORBA_


#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif


/* public API */
extern PyTypeObject PyCORBA_Object_Type;
extern PyTypeObject PyCORBA_ORB_Type;
extern PyTypeObject PyCORBA_TypeCode_Type;
extern PyTypeObject PyCORBA_Any_Type;

PyObject *pycorba_object_new(CORBA_Object objref);
PyObject *pycorba_object_new_with_type(CORBA_Object objref, CORBA_TypeCode tc);
PyObject *pycorba_orb_new(CORBA_ORB orb);
PyObject *pycorba_typecode_new(CORBA_TypeCode tc);
PyObject *pycorba_any_new(CORBA_any *any);

/* private API */

typedef struct {
    PyObject_VAR_HEAD

    CORBA_fixed_d_s fixed;
} PyCORBA_fixed;
extern PyTypeObject PyCORBA_fixed_Type;

void      pymatecorba_register_stub(CORBA_TypeCode tc, PyObject *stub);
CORBA_TypeCode pymatecorba_lookup_typecode(const gchar *repo_id);
PyObject *pymatecorba_get_stub(CORBA_TypeCode tc);
PyObject *pymatecorba_get_stub_from_repo_id(const gchar *repo_id);
PyObject *pymatecorba_get_stub_from_objref(CORBA_Object objref);

void      pymatecorba_generate_typecode_stubs(CORBA_TypeCode tc);
void      pymatecorba_generate_iinterface_stubs(MateCORBA_IInterface *iface);

extern PyTypeObject PyCORBA_Method_Type;
extern PyTypeObject PyCORBA_BoundMethod_Type;
void      pymatecorba_add_imethods_to_stub(PyObject *stub,
				       MateCORBA_IMethods *imethods);

gboolean  pymatecorba_marshal_any(CORBA_any *any, PyObject *value);
PyObject *pymatecorba_demarshal_any(CORBA_any *any);

extern PyObject *pymatecorba_exception;
extern PyObject *pymatecorba_system_exception;
extern PyObject *pymatecorba_user_exception;

gboolean  pymatecorba_check_ex(CORBA_Environment *ev);
gboolean  pymatecorba_check_python_ex(CORBA_Environment *ev);
void      pymatecorba_register_exceptions(PyObject *corbamod);

extern PyTypeObject PyCORBA_Struct_Type;
extern PyTypeObject PyCORBA_Union_Type;
extern PyTypeObject PyCORBA_UnionMember_Type;
void      pymatecorba_add_union_members_to_stub(PyObject *stub, CORBA_TypeCode tc);

extern PyTypeObject PyCORBA_Enum_Type;
PyObject *_pymatecorba_generate_enum(CORBA_TypeCode tc, PyObject **values_p);
PyObject *pycorba_enum_from_long(CORBA_TypeCode tc, long value);


/* utils */
gchar    *_pymatecorba_escape_name(const gchar *name);
PyObject *_pymatecorba_get_container(const gchar *repo_id, gboolean is_poa);

/* skels */

typedef struct _PyMateCORBAInterfaceInfo PyMateCORBAInterfaceInfo;
typedef struct {
    PyObject_HEAD

    PortableServer_ServantBase servant;

    PyMateCORBAInterfaceInfo *info;

    PyObject *delegate;
    PyObject *this;
    PortableServer_POA activator_poa; /* the POA that activated this
                                       * servant, or CORBA_OBJECT_NIL */
} PyPortableServer_Servant;

/* simple macros to go back and forth between Python object and servant */
#define PYSERVANT_TO_SERVANT(pyservant) (&(pyservant)->servant)
#define SERVANT_TO_PYSERVANT(servant)   ((PyPortableServer_Servant *)((guchar *)(servant) - offsetof(PyPortableServer_Servant, servant)))

extern PyTypeObject PyPortableServer_Servant_Type;
void _pymatecorba_register_skel(MateCORBA_IInterface *iinterface);

extern PyTypeObject PyMateCORBA_ObjectAdaptor_Type;
extern PyTypeObject PyPortableServer_POA_Type;
extern PyTypeObject PyPortableServer_POAManager_Type;
PyObject *pymatecorba_poa_new(PortableServer_POA poa);
PyObject *pymatecorba_poamanager_new(PortableServer_POAManager poamanager);


extern PortableServer_POA _pymatecorba_poa;


#define pymatecorba_gil_state_ensure() (PyEval_ThreadsInitialized()? (PyGILState_Ensure()) : 0)

#define pymatecorba_gil_state_release(state) G_STMT_START { \
    if (PyEval_ThreadsInitialized())                    \
        PyGILState_Release(state);                      \
    } G_STMT_END

#define pymatecorba_begin_allow_threads             \
    G_STMT_START {                              \
        PyThreadState *_save = NULL;            \
        if (PyEval_ThreadsInitialized())        \
            _save = PyEval_SaveThread();

#define pymatecorba_end_allow_threads               \
        if (PyEval_ThreadsInitialized())        \
            PyEval_RestoreThread(_save);        \
    } G_STMT_END


  /* pycorba-policy.c */

typedef struct {
    PyObject_VAR_HEAD
    CORBA_Object objref;
} PyCORBA_Policy;

extern PyTypeObject PyCORBA_Policy_Type;

PyObject * pycorba_policy_new(CORBA_Object policy);


#endif
