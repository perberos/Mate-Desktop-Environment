/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-running-context.c: An interface to track running objects
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright (C) 2000, Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_RUNNING_CONTEXT_H_
#define _MATECOMPONENT_RUNNING_CONTEXT_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateComponentRunningContextPrivate MateComponentRunningContextPrivate;
typedef struct _MateComponentRunningContext        MateComponentRunningContext;

struct _MateComponentRunningContext {
	MateComponentObject parent;

	MateComponentRunningContextPrivate *priv;
};

typedef struct {
	MateComponentObjectClass parent;

	POA_MateComponent_RunningContext__epv epv;

	void (*last_unref) (void);
} MateComponentRunningContextClass;

GType         matecomponent_running_context_get_type        (void) G_GNUC_CONST;

MateComponentObject *matecomponent_running_context_new             (void);

/*
 *   This interface is private, and purely for speed
 * of impl. of the context.
 */
void        matecomponent_running_context_add_object_T      (CORBA_Object object);
void        matecomponent_running_context_remove_object_T   (CORBA_Object object);
void        matecomponent_running_context_ignore_object     (CORBA_Object object);
void        matecomponent_running_context_trace_objects_T   (CORBA_Object object,
						      const char  *fn,
						      int          line,
						      int          mode);
void        matecomponent_running_context_at_exit_unref     (CORBA_Object object);

#ifdef MATECOMPONENT_OBJECT_DEBUG
#	define           matecomponent_running_context_add_object_T(o)      G_STMT_START{matecomponent_running_context_trace_objects((o),G_STRFUNC,__LINE__,0);}G_STMT_END
#	define           matecomponent_running_context_remove_object_T(o)   G_STMT_START{matecomponent_running_context_trace_objects((o),G_STRFUNC,__LINE__,1);}G_STMT_END
#	define           matecomponent_running_context_ignore_object(o)     G_STMT_START{matecomponent_running_context_trace_objects((o),G_STRFUNC,__LINE__,2);}G_STMT_END
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_RUNNING_CONTEXT_H_ */

