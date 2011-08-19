#ifndef __MATECORBA_DEBUG_H__
#define __MATECORBA_DEBUG_H__

typedef enum {
	MATECORBA_DEBUG_NONE           = 0,
	MATECORBA_DEBUG_TRACES         = 1 << 0,
	MATECORBA_DEBUG_INPROC_TRACES  = 1 << 1,
	MATECORBA_DEBUG_TIMINGS        = 1 << 2,
	MATECORBA_DEBUG_TYPES          = 1 << 3,
	MATECORBA_DEBUG_MESSAGES       = 1 << 4,
	MATECORBA_DEBUG_ERRORS         = 1 << 5,
	MATECORBA_DEBUG_OBJECTS        = 1 << 6,
	MATECORBA_DEBUG_GIOP           = 1 << 7,
	MATECORBA_DEBUG_REFS           = 1 << 8,
	MATECORBA_DEBUG_FORCE_THREADED = 1 << 9
} OrbitDebugFlags;

#ifndef G_ENABLE_DEBUG

/*static inline void dprintf (OrbitDebugFlags flags, const char *format, ...) { };*/
#ifdef G_HAVE_ISO_VARARGS
#  define dprintf(...)
#  define tprintf(...)
#elif defined(G_HAVE_GNUC_VARARGS)
#  define dprintf(args...)
#  define tprintf(args...)
#else
/* This sucks for compatibility - fix your compiler */
#define MESSAGES (OrbitDebugFlags)0
#define TYPES    (OrbitDebugFlags)0
#define OBJECTS  (OrbitDebugFlags)0
#define GIOP     (OrbitDebugFlags)0
#define ERRORS   (OrbitDebugFlags)0
static inline void dprintf (OrbitDebugFlags flags, const char *format, ...)
{
}
static inline void tprintf (const char *format, ...)
{
}
#endif

#define dump_arg(a,b)

#define tprintf_header(obj,md)
#define tprintf_trace_value(a,b)
#define tprintf_timestamp()
#define tprintf_end_method()
#define tprintf_timestamp()

#else /* G_ENABLE_DEBUG */

#include <stdio.h>

extern OrbitDebugFlags _matecorba_debug_flags;

#ifdef G_HAVE_ISO_VARARGS
#  define dprintf(type, ...) G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_##type)	\
		fprintf (stderr, __VA_ARGS__);		\
	} G_STMT_END
#elif defined(G_HAVE_GNUC_VARARGS)
#  define dprintf(type, args...) G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_##type)	\
		fprintf (stderr, args);			\
	} G_STMT_END
#endif

static inline void
dump_arg (const MateCORBA_IArg *a, CORBA_TypeCode tc)
{
	if (_matecorba_debug_flags & MATECORBA_DEBUG_MESSAGES)
		fprintf (stderr, " '%s' : kind - %d, %c%c",
			 a->name, tc->kind, 
			 a->flags & (MateCORBA_I_ARG_IN | MateCORBA_I_ARG_INOUT) ? 'i' : ' ',
			 a->flags & (MateCORBA_I_ARG_OUT | MateCORBA_I_ARG_INOUT) ? 'o' : ' ');
}

void     MateCORBA_trace_objref     (const CORBA_Object   obj);
void     MateCORBA_trace_any        (const CORBA_any     *any);
void     MateCORBA_trace_typecode   (const CORBA_TypeCode tc);
void     MateCORBA_trace_value      (gconstpointer       *val,
				 CORBA_TypeCode       tc);
void     MateCORBA_trace_header     (CORBA_Object         object,
				 MateCORBA_IMethod       *m_data);
void     MateCORBA_trace_end_method (void);
void     MateCORBA_trace_profiles   (CORBA_Object obj);
void     MateCORBA_trace_timestamp  (void);

#ifdef G_HAVE_ISO_VARARGS
#  define tprintf(...)  dprintf (TRACES, __VA_ARGS__)
#elif defined(G_HAVE_GNUC_VARARGS)
#  define tprintf(args...)  dprintf (TRACES, args)
#endif

#define tprintf_header(obj,md)		G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_TRACES)		\
		MateCORBA_trace_header (obj,md);			\
	} G_STMT_END

#define tprintf_trace_value(a,b)	G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_TRACES)		\
		MateCORBA_trace_value ((gconstpointer *)(a), (b));	\
	} G_STMT_END

#define tprintf_end_method() 		G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_TRACES)		\
		MateCORBA_trace_end_method ();			\
	} G_STMT_END

#define tprintf_timestamp()		G_STMT_START {		\
	if (_matecorba_debug_flags & MATECORBA_DEBUG_TIMINGS)		\
		MateCORBA_trace_timestamp ();			\
	} G_STMT_END

#endif /* G_ENABLE_DEBUG */

#endif /* __MATECORBA_DEBUG_H__ */
