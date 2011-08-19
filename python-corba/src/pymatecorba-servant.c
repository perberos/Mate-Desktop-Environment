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

/* macro to simplify conditionals based on MateCORBA version */
#include <matecorba/matecorba-config.h>
#define MATECORBA_VERSION_CHECK(x,y,z) \
  (MATECORBA_MAJOR_VERSION > x || \
   (MATECORBA_MAJOR_VERSION == x && (MATECORBA_MINOR_VERSION > y || \
     (MATECORBA_MINOR_VERSION == y && MATECORBA_MICRO_VERSION >= z))))

#if !MATECORBA_VERSION_CHECK(2,7,0)
#  define MATECORBA2_INTERNAL_API
#endif
#include "pymatecorba-private.h"
#include <structmember.h>
#include <matecorba/poa/poa-types.h>
#include <matecorba/matecorba-config.h>


struct _PyMateCORBAInterfaceInfo {
    MateCORBA_IInterface *iinterface;
    PortableServer_ClassInfo class_info;
    CORBA_unsigned_long classid;

    PyObject *poa_class, *stub_class;
    
    GHashTable *meth_hash;
    PortableServer_ServantBase__vepv *vepv;
};

PyTypeObject PyPortableServer_Servant_Type;

static MateCORBASmallSkeleton impl_finder_func(PortableServer_ServantBase *servant,
					   const gchar *opname,
					   gpointer *m_data,
					   gpointer *impl);

#define MAX_CLASSID 512
static MateCORBA_VepvIdx *
get_fake_vepvmap(void)
{
    static MateCORBA_VepvIdx *fake_vepvmap = NULL;
    int i;

    if (!fake_vepvmap) {
	fake_vepvmap = g_new0 (MateCORBA_VepvIdx, MAX_CLASSID);
	for (i = 1; i < MAX_CLASSID; i++)
	    fake_vepvmap[i] = 1;
    }

    return fake_vepvmap;
}

void
_pymatecorba_register_skel(MateCORBA_IInterface *iinterface)
{
    static GHashTable *interface_info_hash = NULL; 
    PyMateCORBAInterfaceInfo *info;
    PyObject *instance_dict, *pyinfo, *container;
    gulong length, i, j, maxepvlen;

    if (!interface_info_hash)
	interface_info_hash = g_hash_table_new(g_str_hash, g_str_equal);

    if (g_hash_table_lookup(interface_info_hash, iinterface->tc->repo_id))
	return;

    info = g_new0(PyMateCORBAInterfaceInfo, 1);
    info->iinterface = iinterface;
#if MATECORBA_VERSION_CHECK(2,7,0)
    info->class_info.impl_finder = &impl_finder_func;
#else
    info->class_info.small_relay_call = &impl_finder_func;
#endif
    info->class_info.class_name = g_strdup(iinterface->tc->repo_id);
    info->class_info.class_id = &info->classid;
    info->class_info.idata = iinterface;
    info->class_info.vepvmap = get_fake_vepvmap();

    info->meth_hash = g_hash_table_new(g_str_hash, g_str_equal);

    g_assert(iinterface->base_interfaces._length >= 1);
    length = iinterface->base_interfaces._length - 1;

    info->vepv = (PortableServer_ServantBase__vepv *) g_new0(gpointer, 2);
    info->vepv[0] = g_new0(PortableServer_ServantBase__epv, 1);

    maxepvlen = iinterface->methods._length;
    for (i = 0; i < length; i++) {
	PyMateCORBAInterfaceInfo *base_info;
	const CORBA_char *repo_id;

	repo_id = iinterface->base_interfaces._buffer[i];
	base_info = g_hash_table_lookup(interface_info_hash, repo_id);
	if (!base_info) {
	    g_warning("have not registered base interface '%s' needed by '%s'",
		      repo_id, iinterface->tc->repo_id);
	    continue;
	}

	maxepvlen = MAX(maxepvlen, base_info->iinterface->methods._length);
	for (j = 0; j < base_info->iinterface->methods._length; j++) {
	    MateCORBA_IMethod *imethod = &base_info->iinterface->methods._buffer[j];
	    g_hash_table_insert (info->meth_hash, imethod->name, imethod);
	}
    }
    /* empty epv large enough to cover all base epvs */
    info->vepv[1] = (PortableServer_ServantBase__vepv)g_new0(gpointer,
							     maxepvlen+1);


    instance_dict = PyDict_New();
    pyinfo = PyCObject_FromVoidPtr(info, NULL);
    PyDict_SetItemString(instance_dict, "__interface_info__", pyinfo);
    Py_DECREF(pyinfo);

    info->poa_class = PyObject_CallFunction((PyObject *)&PyType_Type, "s(O)O",
					    iinterface->tc->name,
					    (PyObject *)&PyPortableServer_Servant_Type,
					    instance_dict);
    Py_DECREF(instance_dict);

    for (i = 0; i < iinterface->methods._length; i++) {
	MateCORBA_IMethod *imethod = &iinterface->methods._buffer[i];

	g_hash_table_insert(info->meth_hash, imethod->name, imethod);
    }
    g_hash_table_insert(interface_info_hash, iinterface->tc->repo_id, info);

    /* add servant to module */
    container = _pymatecorba_get_container(iinterface->tc->repo_id, TRUE);
    if (container) {
	gchar *pyname;

	pyname = _pymatecorba_escape_name(iinterface->tc->name);
	if (PyType_Check(container)) {
	    PyObject *container_dict = ((PyTypeObject *)container)->tp_dict;

	    PyDict_SetItemString(container_dict, pyname, info->poa_class);
	} else {
	    PyObject_SetAttrString(container, pyname, info->poa_class);
	}
	g_free(pyname);
	Py_DECREF(container);
    }
}

static void
pymatecorba_servant_generic_skel_func(PortableServer_ServantBase *servant,
				  gpointer retval, gpointer *argv,
				  gpointer ctx, CORBA_Environment *ev,
				  gpointer impl)
{
    PyPortableServer_Servant *pyservant;
    MateCORBA_IMethod *imethod;
    gchar *pyname;
    PyObject *method = NULL;
    CORBA_TypeCode ret_tc = NULL, *arg_tc = NULL;
    PyObject *args = NULL, *ret = NULL;
    gint n_args, n_rets, i, argpos, retpos;
    gboolean has_ret;
    PyGILState_STATE state;

    state = pymatecorba_gil_state_ensure();

    pyservant = SERVANT_TO_PYSERVANT(servant);
    imethod = (MateCORBA_IMethod *)impl;

    /* look up the method on the servant */
    pyname = _pymatecorba_escape_name(imethod->name);
    if (pyservant->delegate != Py_None)
	method = PyObject_GetAttrString(pyservant->delegate, pyname);
    else
	method = PyObject_GetAttrString((PyObject *)pyservant, pyname);
    g_free(pyname);
    if (!method) {
	PyErr_Clear();
	CORBA_exception_set_system(ev, ex_CORBA_NO_IMPLEMENT,
				   CORBA_COMPLETED_NO);
	goto cleanup;
    }

    /* count argument types */
    ret_tc = imethod->ret;
    while (ret_tc && ret_tc->kind == CORBA_tk_alias)
	ret_tc = ret_tc->subtypes[0];
    has_ret = ret_tc != CORBA_OBJECT_NIL && ret_tc->kind != CORBA_tk_void;
    arg_tc = g_new(CORBA_TypeCode, imethod->arguments._length);
    n_args = n_rets = 0;
    for (i = 0; i < imethod->arguments._length; i++) {
	if ((imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0)
	    n_args++;
	if ((imethod->arguments._buffer[i].flags &
	     (MateCORBA_I_ARG_OUT | MateCORBA_I_ARG_INOUT)) != 0)
	    n_rets++;
	arg_tc[i] = imethod->arguments._buffer[i].tc;
	while (arg_tc[i]->kind == CORBA_tk_alias)
	    arg_tc[i] = arg_tc[i]->subtypes[0];
    }

    /* demarshal arguments */
    args = PyTuple_New(n_args);
    argpos = 0;
    for (i = 0; i < imethod->arguments._length; i++) {
	gint flags = imethod->arguments._buffer[i].flags;

	if ((flags & (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT)) != 0) {
	    CORBA_any any = { NULL, NULL, FALSE };
	    PyObject *item;

	    any._type = imethod->arguments._buffer[i].tc;
	    any._value = argv[i];
	    item = pymatecorba_demarshal_any(&any);
	    if (!item) {
		CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
					   CORBA_COMPLETED_NO);
		goto cleanup;
	    }
	    PyTuple_SetItem(args, argpos++, item);
	}
    }

    /* invoke */
    ret = PyObject_CallObject(method, args);

    /* check exception */
    if (pymatecorba_check_python_ex(ev))
	goto cleanup;

    /* if the result isn't a sequence, wrap it in a length 1 tuple */
    if (n_rets + (has_ret ? 1 : 0) == 0) {
	Py_DECREF(ret);
	ret = PyTuple_New(0);
    } else if (n_rets + (has_ret ? 1 : 0) == 1) {
	/* wrap a tuple round the return value ... */
	ret = Py_BuildValue("(N)", ret);
    } else if (n_rets + (has_ret ? 1 : 0) != PySequence_Length(ret)) {
	g_warning("%s: return sequence length is wrong (expected %d, got %d)",
		  imethod->name, n_rets + (has_ret ? 1 : 0),
		  PySequence_Length(ret));
	CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
				   CORBA_COMPLETED_MAYBE);
	goto cleanup;
    }
    retpos = 0;
    if (has_ret) {
	CORBA_any any = { NULL, NULL, FALSE };
	PyObject *item;

	item = PySequence_GetItem(ret, retpos++);
	if (!item) {
	    PyErr_Clear();
	    g_warning("%s: couldn't get return val", imethod->name);
	    CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
				       CORBA_COMPLETED_MAYBE);
	    goto cleanup;
	}
	any._type = imethod->ret;
	switch (ret_tc->kind) {
	case CORBA_tk_any:
	case CORBA_tk_sequence:
	case CORBA_tk_array:
	    *(gpointer *)retval = MateCORBA_small_alloc(ret_tc);
	    any._value = *(gpointer *)retval;
	    break;
	case CORBA_tk_struct:
	case CORBA_tk_union:
	    if ((imethod->flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0) {
		*(gpointer *)retval = MateCORBA_small_alloc(ret_tc);
		any._value = *(gpointer *)retval;
		break;
	    }
	    /* else fall through */
	default:
	    any._value = retval;
	}
	if (!pymatecorba_marshal_any(&any, item)) {
	    Py_DECREF(item);
	    g_warning("%s: could not marshal return", imethod->name);
	    CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
				       CORBA_COMPLETED_MAYBE);
	    goto cleanup;
	}
    }

    /* handle inout and out args */
    for (i = 0; i < imethod->arguments._length; i++) {
	gint flags = imethod->arguments._buffer[i].flags;
	CORBA_TypeCode tc = arg_tc[i];
	PyObject *item;
	CORBA_any any = { NULL, NULL, FALSE };


	if ((flags & (MateCORBA_I_ARG_INOUT | MateCORBA_I_ARG_OUT)) == 0)
	    continue;

	/* read value from result sequence */
	item = PySequence_GetItem(ret, retpos++);
	if (!item) {
	    PyErr_Clear();
	    g_warning("%s: could not get arg from tuple", imethod->name);
	    CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
				       CORBA_COMPLETED_MAYBE);
	    goto cleanup;
	}

	/* set up any */
	any._type = imethod->arguments._buffer[i].tc;
	if ((flags & MateCORBA_I_ARG_INOUT) != 0) {
	    /* clean up any internal allocations in the inout ... */
	    MateCORBA_small_freekids(tc, argv[i], NULL);
	    any._value = argv[i];
	} else if ((flags & MateCORBA_I_ARG_OUT) != 0) {
	    /* is it a variable length type?  If so, allocate storage. */
	    if (tc->kind == CORBA_tk_any || tc->kind == CORBA_tk_sequence ||
                ((tc->kind == CORBA_tk_struct || tc->kind == CORBA_tk_union ||
                  tc->kind == CORBA_tk_array) &&
                 (flags & MateCORBA_I_COMMON_FIXED_SIZE) == 0))
		*(gpointer *)argv[i] = MateCORBA_small_alloc(tc);

	    any._value = *(gpointer *)argv[i];
	}

	if (!pymatecorba_marshal_any(&any, item)) {
	    Py_DECREF(item);
	    g_warning("%s: could not marshal arg", imethod->name);
	    CORBA_exception_set_system(ev, ex_CORBA_DATA_CONVERSION,
				       CORBA_COMPLETED_MAYBE);
	    goto cleanup;
	}
    }

 cleanup:
    g_free(arg_tc);
    Py_XDECREF(method);
    Py_XDECREF(args);
    Py_XDECREF(ret);
    pymatecorba_gil_state_release(state);
}

static MateCORBASmallSkeleton
impl_finder_func(PortableServer_ServantBase *servant,
		 const gchar *opname, gpointer *m_data, gpointer *impl)
{
    PyPortableServer_Servant *pyservant;
    gpointer value;
    MateCORBA_IMethod *imethod;

    pyservant = SERVANT_TO_PYSERVANT(servant);
    if (!g_hash_table_lookup_extended(pyservant->info->meth_hash, opname,
				      NULL, &value)) {
	return NULL;
    }
    imethod = (MateCORBA_IMethod *)value;

    *m_data = imethod; /* imethod */
    *impl = imethod;

    return pymatecorba_servant_generic_skel_func;
}

  /* caller must CORBA_Object_release the POA */
static PortableServer_POA
_pymatecorba_servant_get_poa(PyPortableServer_Servant *self)
{
    PyObject *pypoa;
    PortableServer_POA poa;

    pypoa = PyObject_CallMethod((PyObject *)self, "_default_POA", NULL);
    if (!pypoa)
	return CORBA_OBJECT_NIL;
    if (!PyObject_TypeCheck(pypoa, &PyPortableServer_POA_Type)) {
	Py_DECREF(pypoa);
	PyErr_SetString(PyExc_TypeError, "could not lookup default POA");
	return CORBA_OBJECT_NIL;
    }
    poa = (PortableServer_POA) CORBA_Object_duplicate(((PyCORBA_Object *)pypoa)->objref, NULL);
    Py_DECREF(pypoa);
    return poa;
}

static void
pymatecorba_servant_dealloc(PyPortableServer_Servant *self)
{
    PortableServer_ServantBase *servant;

    servant = PYSERVANT_TO_SERVANT(self);

    if (self->activator_poa) {
        PortableServer_ObjectId *id;
          /* deactivate the object */
        id = PortableServer_POA_servant_to_id(self->activator_poa, servant, NULL);
        PortableServer_POA_deactivate_object(self->activator_poa, id, NULL);
        CORBA_free(id);
        CORBA_Object_release((CORBA_Object) self->activator_poa, NULL);
    }
    PortableServer_ServantBase__fini(servant, NULL);
    Py_CLEAR(self->this);
    Py_CLEAR(self->delegate);

    if (self->ob_type->tp_free)
	self->ob_type->tp_free((PyObject *)self);
    else
	PyObject_DEL(self);
}

#define pymatecorba_servant_repr 0


static PyObject *
pymatecorba_servant_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *pyinfo;
    PyMateCORBAInterfaceInfo *info;
    PyPortableServer_Servant *self;
    PortableServer_ServantBase *servant;
    CORBA_Environment ev;

    /* get the InterfaceInfo struct ... */
    pyinfo = PyObject_GetAttrString((PyObject *)type, "__interface_info__");
    if (!pyinfo)
	return NULL;
    if (!PyCObject_Check(pyinfo)) {
	Py_DECREF(pyinfo);
	PyErr_SetString(PyExc_TypeError,
			"__interface_info__ attribute not a cobject");
	return NULL;
    }
    info = PyCObject_AsVoidPtr(pyinfo);
    Py_DECREF(pyinfo);

    self = (PyPortableServer_Servant *)type->tp_alloc(type, 0);
    self->info = info;
    self->delegate = Py_None;
    Py_INCREF(self->delegate);

    servant = PYSERVANT_TO_SERVANT(self);
    servant->vepv = info->vepv;
    MateCORBA_classinfo_register(&info->class_info);
    MATECORBA_SERVANT_SET_CLASSINFO(servant, &info->class_info);

    CORBA_exception_init(&ev);
    PortableServer_ServantBase__init(servant, &ev);
    if (pymatecorba_check_ex(&ev)) {
	Py_DECREF(self);
	return NULL;
    }

    return (PyObject *)self;
}

static int
pymatecorba_servant_init(PyPortableServer_Servant *self,
		     PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "delegate", NULL };
    PyObject *delegate = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:Servant.__init__",
				     kwlist, &delegate))
	return -1;

    Py_XDECREF(self->delegate);
    self->delegate = delegate;
    Py_INCREF(self->delegate);

    return 0;
}

static PyObject *
pymatecorba_servant__default_POA(PyPortableServer_Servant *self)
{
    if (!_pymatecorba_poa) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    CORBA_Object_duplicate((CORBA_Object)_pymatecorba_poa, NULL);
    return pymatecorba_poa_new(_pymatecorba_poa);
}

static PyObject *
pymatecorba_servant__this(PyPortableServer_Servant *self)
{
    CORBA_Environment ev;
    PortableServer_ServantBase *servant;
    PortableServer_ObjectId *objid;
    CORBA_Object objref;

    if (self->this) {
	Py_INCREF(self->this);
	return self->this;
    }
    
    g_assert(!self->activator_poa);
    self->activator_poa = _pymatecorba_servant_get_poa(self);
    if (G_UNLIKELY(self->activator_poa == CORBA_OBJECT_NIL))
        return NULL;
    servant = PYSERVANT_TO_SERVANT(self);

    CORBA_exception_init(&ev);
    objid = PortableServer_POA_activate_object(self->activator_poa, servant, &ev);
    CORBA_free(objid);
    if (pymatecorba_check_ex(&ev)) {
    	return NULL;
    }
    
    CORBA_exception_init(&ev);
    objref = PortableServer_POA_servant_to_reference(self->activator_poa, servant, &ev);
    if (pymatecorba_check_ex(&ev))
    	return NULL;

    self->this = pycorba_object_new(objref);
    CORBA_Object_release(objref, NULL);
    Py_INCREF(self->this);
    return self->this;
}

static PyMethodDef pymatecorba_servant_methods[] = {
    { "_default_POA", (PyCFunction)pymatecorba_servant__default_POA, METH_NOARGS },
    { "_this", (PyCFunction)pymatecorba_servant__this, METH_NOARGS },
    { NULL, NULL, 0 }
};

static PyMemberDef pymatecorba_servant_members[] = {
    { "_delegate", T_OBJECT, offsetof(PyPortableServer_Servant, delegate),
      0, "delegate object" },
    { NULL, 0, 0, 0 }
};

PyTypeObject PyPortableServer_Servant_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "PortableServer.Servant",           /* tp_name */
    sizeof(PyPortableServer_Servant),   /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)pymatecorba_servant_dealloc, /* tp_dealloc */
    (printfunc)0,                       /* tp_print */
    (getattrfunc)0,                     /* tp_getattr */
    (setattrfunc)0,                     /* tp_setattr */
    (cmpfunc)0,                         /* tp_compare */
    (reprfunc)pymatecorba_servant_repr,     /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    (hashfunc)0,                        /* tp_hash */
    (ternaryfunc)0,                     /* tp_call */
    (reprfunc)0,                        /* tp_str */
    (getattrofunc)0,                    /* tp_getattro */
    (setattrofunc)0,                    /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    NULL, /* Documentation string */
    (traverseproc)0,                    /* tp_traverse */
    (inquiry)0,                         /* tp_clear */
    (richcmpfunc)0,                     /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    (getiterfunc)0,                     /* tp_iter */
    (iternextfunc)0,                    /* tp_iternext */
    pymatecorba_servant_methods,            /* tp_methods */
    pymatecorba_servant_members,            /* tp_members */
    0,                                  /* tp_getset */
    (PyTypeObject *)0,                  /* tp_base */
    (PyObject *)0,                      /* tp_dict */
    (descrgetfunc)0,                    /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)pymatecorba_servant_init,     /* tp_init */
    0,                                  /* tp_alloc */
    (newfunc)pymatecorba_servant_new,       /* tp_new */
    0,                                  /* tp_free */
    (inquiry)0,                         /* tp_is_gc */
    (PyObject *)0,                      /* tp_bases */
};
