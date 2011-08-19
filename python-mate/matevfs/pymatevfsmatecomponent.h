/* -*- mode: C; c-basic-offset: 4 -*- */
#ifndef __PYMATEVFSMATECOMPONENT_H_
#define __PYMATEVFSMATECOMPONENT_H_

#include <pymatevfs.h>


G_BEGIN_DECLS


struct _PyMateVFSMateComponent_Functions {
    PyObject * (*mime_component_action_new) (MateVFSMimeAction *action);
};

#define pymatevfs_matecomponent_mime_component_action_new (_PyMateVFSMateComponent_API->mime_component_action_new)


#if defined(NO_IMPORT) || defined(NO_IMPORT_PYMATEVFSMATECOMPONENT)
extern struct _PyMateVFSMateComponent_Functions *_PyMateVFSMateComponent_API;
#else
struct _PyMateVFSMateComponent_Functions *_PyMateVFSMateComponent_API;
#endif

static inline PyObject *
pymate_vfs_matecomponent_init(void)
{
    PyObject *module = PyImport_ImportModule("matevfs.matevfsmatecomponent");
    if (module != NULL) {
        PyObject *mdict = PyModule_GetDict(module);
        PyObject *cobject = PyDict_GetItemString(mdict, "_PyMateVFSMateComponent_API");
        if (PyCObject_Check(cobject))
            _PyMateVFSMateComponent_API = (struct _PyMateVFSMateComponent_Functions *)PyCObject_AsVoidPtr(cobject);
        else {
	    Py_FatalError("could not find _PyMateVFSMateComponent_API object");
        }
    } else {
        Py_FatalError("could not import matevfs.matevfsmatecomponent");
    }
    return module;
}

G_END_DECLS

#endif /* __PYMATEVFSMATECOMPONENT_H_ */
