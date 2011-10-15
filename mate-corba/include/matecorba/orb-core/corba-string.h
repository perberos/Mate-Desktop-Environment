#ifndef CORBA_STRING_H
#define CORBA_STRING_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

CORBA_char  *CORBA_string_alloc  (CORBA_unsigned_long len);
CORBA_wchar *CORBA_wstring_alloc (CORBA_unsigned_long len);

/*
 * MateCORBA extension.
 */
CORBA_char          *CORBA_string_dup  (const CORBA_char *str);

CORBA_wchar         *CORBA_wstring_dup (const CORBA_wchar *str);

CORBA_unsigned_long  CORBA_wstring_len (CORBA_wchar *ws);

#ifdef MATECORBA2_INTERNAL_API

gpointer CORBA_string__freekids         (gpointer mem,
					 gpointer data);

CORBA_sequence_CORBA_octet *
         MateCORBA_sequence_CORBA_octet_dup (const CORBA_sequence_CORBA_octet *seq);

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
