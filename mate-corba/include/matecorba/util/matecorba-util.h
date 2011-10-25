#ifndef MATECORBA_UTIL_H
#define MATECORBA_UTIL_H 1

#include <glib.h>
#include <matecorba/matecorba-config.h>
#include <matecorba/util/basic_types.h>
#include <matecorba/util/matecorba-genrand.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	/* Align a value upward to a boundary, expressed as a number of bytes.
	   E.g. align to an 8-byte boundary with argument of 8.  */

	/*
	 *   (this + boundary - 1)
	 *          &
	 *    ~(boundary - 1)
	 */

	#define ALIGN_VALUE(this, boundary) \
		((((gulong) (this)) + (((gulong) (boundary)) - 1)) & (~(((gulong) (boundary)) - 1)))

	#define ALIGN_ADDRESS(this, boundary) \
		((gpointer) ALIGN_VALUE(this, boundary))

	#ifdef MATECORBA2_INTERNAL_API

		gulong MateCORBA_wchar_strlen(CORBA_wchar* wstr);

		#define num2hexdigit(n) (((n) > 9) ? ((n) + 'a' - 10) : ((n) + '0'))

	#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
