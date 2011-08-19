/*
 * Copyright (C) 2005 Red Hat, Inc.
 *
 *   mateconf-types.c: wrappers for some specialised MateConf types.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include "mateconf-types.h"
#define NO_IMPORT_PYGOBJECT
#include <pygobject.h>

typedef struct {
  PyObject_HEAD
  MateConfEngine *engine;
} PyMateConfEngine;
extern PyTypeObject PyMateConfEngine_Type;

static void
pymateconf_engine_dealloc (PyMateConfEngine *self)
{
  pyg_begin_allow_threads;
  mateconf_engine_unref (self->engine);
  pyg_end_allow_threads;
  PyObject_DEL (self);
}


static PyObject *
pymateconf_engine_associate_schema(PyMateConfEngine *self, PyObject *args, PyObject *kwargs)
{
	gchar *key, *schema_key;
	gboolean result;
        GError *err = NULL;
	char *kwlist[] = {"key", "schema_key", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                         "ss:mateconf.Engine.associate_schema",
                                         kwlist, &key, &schema_key))
            return NULL;

        result = mateconf_engine_associate_schema(self->engine, key, schema_key, &err);

        if (pyg_error_check(&err))
            return NULL;
       
        if (result) {
            Py_INCREF(Py_True);
            return Py_True;
        } else {
            Py_INCREF(Py_False);
            return Py_False;
        }
}

static PyMethodDef pymateconf_engine_methods[] = {
        {"associate_schema", (PyCFunction)pymateconf_engine_associate_schema,
         METH_KEYWORDS, NULL},

        {NULL, NULL, 0, NULL}
};


PyTypeObject PyMateConfEngine_Type = {
  PyObject_HEAD_INIT (NULL)
  0,
  "mateconf.MateConfEngine",
  sizeof (PyMateConfEngine),
  0,
  (destructor) pymateconf_engine_dealloc,
  (printfunc) 0,
  (getattrfunc) 0,
  (setattrfunc) 0,
  (cmpfunc) 0,
  (reprfunc) 0,
  0,
  0,
  0,
  (hashfunc) 0,
  (ternaryfunc) 0,
  (reprfunc) 0,
  (getattrofunc) 0,
  (setattrofunc) 0,
  0,
  Py_TPFLAGS_DEFAULT,
  0,                           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  pymateconf_engine_methods,      /* tp_methods */
};

PyObject *
pymateconf_engine_new (MateConfEngine *engine)
{
  PyMateConfEngine *self;

  if (engine == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  self = (PyMateConfEngine *) PyObject_NEW (PyMateConfEngine,	&PyMateConfEngine_Type);
  if (self == NULL)
    return NULL;

  pyg_begin_allow_threads;
  self->engine = engine;
  mateconf_engine_ref (engine);
  pyg_end_allow_threads;

  return (PyObject *) self;
}

MateConfEngine *
pymateconf_engine_from_pyobject (PyObject *object)
{
  PyMateConfEngine *self;

  if (object == NULL)
    return NULL;

  if (!PyObject_TypeCheck (object, &PyMateConfEngine_Type))
    {
      PyErr_SetString (PyExc_TypeError, "unable to convert argument to MateConfEngine*");
      return NULL;
    }

  self = (PyMateConfEngine *) object;

  return self->engine;
}

void
pymateconf_register_engine_type (PyObject *moddict)
{
  PyMateConfEngine_Type.ob_type = &PyType_Type;

  PyType_Ready(&PyMateConfEngine_Type);
}
