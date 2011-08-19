/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-exception.c: a generic exception -> user string converter.
 *
 * Authors:
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */
#ifndef _MATECOMPONENT_EXCEPTION_H_
#define _MATECOMPONENT_EXCEPTION_H_

#include <glib.h>
#include <matecomponent/MateComponent.h>

#define matecomponent_exception_set(opt_ev,repo_id) G_STMT_START{                  \
     if (opt_ev) {                                                          \
         CORBA_exception_set (opt_ev, CORBA_USER_EXCEPTION, repo_id, NULL); \
     } else {                                                               \
	 char *text = matecomponent_exception_repoid_to_text (repo_id);	    \
	 g_log (G_LOG_DOMAIN,						    \
		G_LOG_LEVEL_CRITICAL,					    \
		"file %s: line %d: matecomponent exception: `%s'",                 \
		__FILE__,						    \
		__LINE__,						    \
		text);							    \
	 g_free (text);							    \
     } }G_STMT_END

#ifdef G_DISABLE_CHECKS

#define matecomponent_return_if_fail(expr,opt_ev) G_STMT_START{		\
     if (!(expr)) {							\
         if (opt_ev)                                                    \
	     CORBA_exception_set (opt_ev, CORBA_USER_EXCEPTION,         \
				  ex_MateComponent_BadArg, NULL);              \
         return;                                                        \
     };	}G_STMT_END

#define matecomponent_return_val_if_fail(expr,val,opt_ev) G_STMT_START{	\
     if (!(expr)) {							\
         if (opt_ev)                                                    \
	     CORBA_exception_set (opt_ev, CORBA_USER_EXCEPTION,         \
				  ex_MateComponent_BadArg, NULL);              \
         return val;                                                    \
     };	}G_STMT_END

#else /* !G_DISABLE_CHECKS */
#define matecomponent_return_if_fail(expr,opt_ev) G_STMT_START{		\
     if (!(expr)) {							\
         if (opt_ev)                                                    \
	     CORBA_exception_set (opt_ev, CORBA_USER_EXCEPTION,         \
				  ex_MateComponent_BadArg, NULL);              \
	 g_log (G_LOG_DOMAIN,						\
		G_LOG_LEVEL_CRITICAL,					\
		"file %s: line %d (%s): assertion `%s' failed.",	\
		__FILE__,						\
		__LINE__,						\
		G_STRFUNC,					\
		#expr);							\
         return;                                                        \
     };	}G_STMT_END
         
#define matecomponent_return_val_if_fail(expr,val,opt_ev) G_STMT_START{	\
     if (!(expr)) {							\
         if (opt_ev)                                                    \
	     CORBA_exception_set (opt_ev, CORBA_USER_EXCEPTION,         \
				  ex_MateComponent_BadArg, NULL);              \
	 g_log (G_LOG_DOMAIN,						\
		G_LOG_LEVEL_CRITICAL,					\
		"file %s: line %d (%s): assertion `%s' failed.",	\
		__FILE__,						\
		__LINE__,						\
		G_STRFUNC,					\
		#expr);							\
         return val;                                                    \
     };	}G_STMT_END
#endif

#define MATECOMPONENT_EX(ev)         ((ev) != NULL && (ev)->_major != CORBA_NO_EXCEPTION)

#define MATECOMPONENT_USER_EX(ev,id) ((ev != NULL) && (ev)->_major == CORBA_USER_EXCEPTION &&	\
			       (ev)->_id != NULL && !strcmp ((ev)->_id, id))

#define MATECOMPONENT_EX_REPOID(ev)  (ev)->_id

#define MATECOMPONENT_RET_EX(ev)		\
	G_STMT_START{			\
		if (MATECOMPONENT_EX (ev))	\
			return;		\
	}G_STMT_END

#define MATECOMPONENT_RET_VAL_EX(ev,v)		\
	G_STMT_START{			\
		if (MATECOMPONENT_EX (ev))	\
			return (v);	\
	}G_STMT_END

typedef char *(*MateComponentExceptionFn)     (CORBA_Environment *ev, gpointer user_data);

char *matecomponent_exception_get_text        (CORBA_Environment *ev);
char *matecomponent_exception_repoid_to_text  (const char *repo_id);


void  matecomponent_exception_add_handler_str (const char *repo_id,
					const char *str);

void  matecomponent_exception_add_handler_fn  (const char *repo_id,
					MateComponentExceptionFn fn,
					gpointer          user_data,
					GDestroyNotify    destroy_fn);

void  matecomponent_exception_general_error_set (CORBA_Environment *ev,
					  CORBA_TypeCode     opt_deriv,
					  const char        *format,
					  ...) G_GNUC_PRINTF(3,4);

const char *matecomponent_exception_general_error_get (CORBA_Environment *ev);

#endif /* _MATECOMPONENT_EXCEPTION_H_ */
