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

#ifndef PYMATECORBA_H
#define PYMATECORBA_H

#include <Python.h>
#include <matecorba/matecorba.h>

typedef struct {
    PyObject_HEAD
    CORBA_Object objref;
    PyObject    *in_weakreflist;
} PyCORBA_Object;

typedef struct {
    PyObject_HEAD
    CORBA_ORB orb;
} PyCORBA_ORB;

typedef struct {
    PyObject_HEAD
    CORBA_TypeCode tc;
} PyCORBA_TypeCode;

typedef struct {
    PyObject_HEAD
    CORBA_any any;
} PyCORBA_Any;

struct _PyMateCORBA_APIStruct {
    PyTypeObject *corba_object_type;
    PyTypeObject *corba_orb_type;
    PyTypeObject *corba_typecode_type;
    PyTypeObject *corba_any_type;
    PyTypeObject *portable_server_poa_type;
    PyTypeObject *portable_server_poamanager_type;

    PyObject *(* corba_object_new)(CORBA_Object objref);
    PyObject *(* corba_orb_new)(CORBA_ORB orb);
    PyObject *(* corba_typecode_new)(CORBA_TypeCode tc);
    PyObject *(* corba_any_new)(CORBA_any *any);
    PyObject *(* poa_new)(PortableServer_POA poa);
    PyObject *(* poamanager_new)(PortableServer_POAManager poa);

    gboolean  (* check_ex)(CORBA_Environment *ev);
    gboolean  (* marshal_any)(CORBA_any *any, PyObject *value);
    PyObject *(* demarshal_any)(CORBA_any *any);
    gboolean  (* check_python_ex)(CORBA_Environment *ev);

};

#ifndef _INSIDE_PYMATECORBA_

#if defined(NO_IMPORT) || defined(NO_IMPORT_PYMATECORBA)
extern struct _PyMateCORBA_APIStruct *_PyMateCORBA_API;
#else
struct _PyMateCORBA_APIStruct *_PyMateCORBA_API;

/* macro used to initialise the module */
#define init_pymatecorba() { \
    PyObject *pymatecorba = PyImport_ImportModule("MateCORBA"); \
    if (pymatecorba != NULL) { \
        PyObject *module_dict = PyModule_GetDict(pymatecorba); \
        PyObject *cobject = PyDict_GetItemString(module_dict, "_PyMateCORBA_API");\
        if (PyCObject_Check(cobject)) \
            _PyMateCORBA_API = (struct _PyMateCORBA_APIStruct *) \
                PyCObject_AsVoidPtr(cobject); \
        else { \
            Py_FatalError("could not find _PyMateCORBA_API object"); \
            return; \
        } \
    } else { \
        Py_FatalError("could not import MateCORBA module"); \
        return; \
    } \
}

#endif

/* types */
#define PyCORBA_Object_Type       *(_PyMateCORBA_API->corba_object_type)
#define PyCORBA_ORB_Type          *(_PyMateCORBA_API->corba_orb_type)
#define PyCORBA_TypeCode_Type     *(_PyMateCORBA_API->corba_typecode_type)
#define PyCORBA_Any_Type          *(_PyMateCORBA_API->corba_any_type)
#define PyPortableServer_POA_Type *(_PyMateCORBA_API->portable_server_poa_type)
#define PyPortableServer_POAManager_Type *(_PyMateCORBA_API->portable_server_poamanager_type)

/* constructors for above types ... */
#define pycorba_object_new        (* _PyMateCORBA_API->corba_object_new)
#define pycorba_orb_new           (* _PyMateCORBA_API->corba_orb_new)
#define pycorba_typecode_new      (* _PyMateCORBA_API->corba_typecode_new)
#define pycorba_any_new           (* _PyMateCORBA_API->corba_any_new)
#define pymatecorba_poa_new           (* _PyMateCORBA_API->poa_new)
#define pymatecorba_poamanager_new    (* _PyMateCORBA_API->poamanager_new)

/* utility functions */
#define pymatecorba_check_ex          (* _PyMateCORBA_API->check_ex)
#define pymatecorba_marshal_any       (* _PyMateCORBA_API->marshal_any)
#define pymatecorba_demarshal_any     (* _PyMateCORBA_API->demarshal_any)
#define pymatecorba_check_python_ex   (* _PyMateCORBA_API->check_python_ex)

#endif

#endif
