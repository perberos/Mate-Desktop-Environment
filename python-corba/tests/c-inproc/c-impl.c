#define MATECORBA2_STUBS_API

#include <Python.h>
#include <pymatecorba.h>
#include "testcall.h"

typedef struct {
    POA_PyMateCORBA_TestCall baseServant;
    PyMateCORBA_TestCall this;
} PyMateCORBA_TestCall_Servant;


static void
TestCall_op1(PortableServer_Servant servant,
	     PyMateCORBA_TestCall other,
	     CORBA_Environment *ev)
{
    PyMateCORBA_TestCall_Servant *self = (PyMateCORBA_TestCall_Servant *)servant;
    printf("  C impl of TestCall.op1 invoked\n");
    PyMateCORBA_TestCall_op2(other, self->this, ev);
}

static void
TestCall_op2(PortableServer_Servant servant,
	     PyMateCORBA_TestCall other,
	     CORBA_Environment *ev)
{
    printf("  C impl of TestCall.op2 invoked\n");
    PyMateCORBA_TestCall_op3(other, ev);
}

static void
TestCall_op3(PortableServer_Servant servant,
	     CORBA_Environment *ev)
{
    printf("  C impl of TestCall.op3 invoked\n");
}

static PortableServer_ServantBase__epv TestCall_base_epv = {
    NULL,
    NULL,
    NULL
};

static POA_PyMateCORBA_TestCall__epv TestCall_epv = {
    NULL,
    TestCall_op1,
    TestCall_op2,
    TestCall_op3
};

static POA_PyMateCORBA_TestCall__vepv TestCall_vepv = {
    &TestCall_base_epv,
    &TestCall_epv
};


static CORBA_Object
create_TestCall(CORBA_ORB orb, CORBA_Environment *ev)
{
    /* hold the actual servant/objref ... */
    static PyMateCORBA_TestCall_Servant servant;

    PortableServer_ObjectId *objid;
    PortableServer_POA poa;

    if (servant.this) {
	return CORBA_Object_duplicate(servant.this, ev);
    }
    servant.baseServant._private = NULL;
    servant.baseServant.vepv = &TestCall_vepv;

    POA_PyMateCORBA_TestCall__init((PortableServer_ServantBase *)&servant, ev);
    g_assert(ev->_major == CORBA_NO_EXCEPTION);

    poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", ev);
    g_assert(ev->_major == CORBA_NO_EXCEPTION);

    objid = PortableServer_POA_activate_object(poa, (PortableServer_ServantBase *)&servant, ev);
    g_assert(ev->_major == CORBA_NO_EXCEPTION);

    servant.this = PortableServer_POA_servant_to_reference(poa, (PortableServer_ServantBase *)&servant, ev);
    g_assert(ev->_major == CORBA_NO_EXCEPTION);

    CORBA_free(objid);

    return CORBA_Object_duplicate(servant.this, ev);
}

static PyObject *
_wrap_create_TestCall(PyObject *self, PyObject *args)
{
    PyCORBA_ORB *orb;
    CORBA_Environment ev;
    CORBA_Object objref;
    PyObject *py_objref;

    if (!PyArg_ParseTuple(args, "O!", &PyCORBA_ORB_Type, &orb))
	return NULL;

    CORBA_exception_init(&ev);
    objref = create_TestCall(orb->orb, &ev);
    g_assert(ev._major == CORBA_NO_EXCEPTION);

    py_objref = pycorba_object_new(objref);
    CORBA_Object_release(objref, NULL);
    return py_objref;
}

static PyMethodDef cTestCall_functions[] = {
    { "create_TestCall", (PyCFunction)_wrap_create_TestCall, METH_VARARGS },
    { NULL, NULL, 0 }
};

void initcTestCall(void);

DL_EXPORT(void)
initcTestCall(void)
{
    PyObject *mod;

    init_pymatecorba();

    //MateCORBA_small_flags &= ~ MATECORBA_SMALL_FAST_LOCALS;

    mod = Py_InitModule("cTestCall", cTestCall_functions);
}
