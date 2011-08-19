/* -*- Mode: C; c-basic-offset: 4 -*- */
#ifndef _PYMATECOMPONENT_H_
#define _PYMATECOMPONENT_H_

#include <Python.h>

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

GClosure * pymatecomponent_closure_new (PyObject *callback,
                                 PyObject *extra_args,
                                 PyObject *swap_data);
G_END_DECLS

#endif
