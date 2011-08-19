/* -*- Mode: C; c-basic-offset: 4 -*- */

#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>

#define NO_IMPORT_PYMATECORBA
#include <pymatecorba.h>

#include <matecomponent/matecomponent-types.h>

static PyObject *
pymatecomponent_corbaany_from_value(const GValue *value)
{
    return pycorba_any_new(g_value_get_boxed(value));

}
static int
pymatecomponent_corbaany_to_value(GValue *value, PyObject *object)
{
    CORBA_any *any;

    if (!PyObject_TypeCheck(object, &PyCORBA_Any_Type))
	return -1;

    any = &((PyCORBA_Any*)object)->any;
    g_value_set_boxed(value, any);
    return 0;
}

static PyObject *
pymatecomponent_corbatypecode_from_value(const GValue *value)
{
    return pycorba_typecode_new(g_value_get_boxed(value));

}
static int
pymatecomponent_corbatypecode_to_value(GValue *value, PyObject *object)
{
    CORBA_TypeCode tc;

    if (!PyObject_TypeCheck(object, &PyCORBA_TypeCode_Type))
	return -1;

    tc = ((PyCORBA_TypeCode *)object)->tc;
    g_value_set_boxed(value, tc);
    return 0;
}

static PyObject *
pymatecomponent_corbaobject_from_value(const GValue *value)
{
    return pycorba_object_new((CORBA_Object)g_value_get_boxed(value));
}
static int
pymatecomponent_corbaobject_to_value(GValue *value, PyObject *object)
{
    CORBA_Object objref;

    if (!PyObject_TypeCheck(object, &PyCORBA_Object_Type))
	return -1;

    objref = ((PyCORBA_Object *)object)->objref;
    g_value_set_boxed(value, objref);
    return 0;
}

static int
pymatecomponent_unknown_to_value(GValue *value, PyObject *object)
{
    CORBA_Object objref;
    CORBA_Environment ev;
    gboolean type_matches;

    if (!PyObject_TypeCheck(object, &PyCORBA_Object_Type))
	return -1;

    objref = ((PyCORBA_Object *)object)->objref;

    /* check if it is a MateComponent::Unknown */
    CORBA_exception_init(&ev);
    type_matches = CORBA_Object_is_a(objref, "IDL:MateComponent/Unknown:1.0", &ev);
    if (pymatecorba_check_ex(&ev))
	return -1;
    if (!type_matches)
	return -1;

    g_value_set_boxed(value, objref);
    return 0;
}

void
_pymatecomponent_register_boxed_types(PyObject *moddict)
{
	pyg_register_boxed_custom(MATECOMPONENT_TYPE_CORBA_ANY,
				  pymatecomponent_corbaany_from_value,
				  pymatecomponent_corbaany_to_value);
	pyg_register_boxed_custom(MATECOMPONENT_TYPE_CORBA_TYPECODE,
				  pymatecomponent_corbatypecode_from_value,
				  pymatecomponent_corbatypecode_to_value);
	pyg_register_boxed_custom(MATECOMPONENT_TYPE_CORBA_OBJECT,
				  pymatecomponent_corbaobject_from_value,
				  pymatecomponent_corbaobject_to_value);
	pyg_register_boxed_custom(MATECOMPONENT_TYPE_UNKNOWN,
				  pymatecomponent_corbaobject_from_value,
				  pymatecomponent_unknown_to_value);
}

