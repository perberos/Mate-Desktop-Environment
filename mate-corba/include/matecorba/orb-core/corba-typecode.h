#ifndef CORBA_TYPECODE_H
#define CORBA_TYPECODE_H 1

#include <matecorba/orb-core/corba-typecode-type.h>
#include <matecorba/orb-core/corba-any-type.h>
#include <matecorba/orb-core/matecorba-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CORBA_tk_recursive 0xffffffff
#define CORBA_tk_last (CORBA_tk_abstract_interface + 1)

struct CORBA_TypeCode_struct {
	struct MateCORBA_RootObject_struct parent;
	CORBA_unsigned_long  kind;          /* The type */
	CORBA_unsigned_long  flags;	    /* native - impl. flags */
	CORBA_short          c_length;      /* native - C size */
	CORBA_short          c_align;       /* native - C align */
	CORBA_unsigned_long  length;        /* length of sub types / parts */
	CORBA_unsigned_long  sub_parts;     /* length of sub parts */
	CORBA_TypeCode      *subtypes;	    /* for struct, exception, union, alias, array, sequence */
	CORBA_TypeCode       discriminator; /* for union */
	char                *name;
	char                *repo_id;
	char               **subnames;      /* for struct, exception, union, enum */
	CORBA_long          *sublabels;     /* for union */
	CORBA_long           default_index; /* for union */
	CORBA_unsigned_long  recurse_depth; /* for recursive sequence */
	CORBA_unsigned_short digits;        /* for fixed */
	CORBA_short scale;	            /* for fixed */
};

extern MATECORBA2_MAYBE_CONST MateCORBA_RootObject_Interface MateCORBA_TypeCode_epv;

#define TC_null ((CORBA_TypeCode)&TC_null_struct)
#define TC_void ((CORBA_TypeCode)&TC_void_struct)
#define TC_CORBA_short ((CORBA_TypeCode)&TC_CORBA_short_struct)
#define TC_CORBA_long ((CORBA_TypeCode)&TC_CORBA_long_struct)
#define TC_CORBA_long_long ((CORBA_TypeCode)&TC_CORBA_long_long_struct)
#define TC_CORBA_unsigned_short ((CORBA_TypeCode)&TC_CORBA_unsigned_short_struct)
#define TC_CORBA_unsigned_long ((CORBA_TypeCode)&TC_CORBA_unsigned_long_struct)
#define TC_CORBA_unsigned_long_long ((CORBA_TypeCode)&TC_CORBA_unsigned_long_long_struct)
#define TC_CORBA_float ((CORBA_TypeCode)&TC_CORBA_float_struct)
#define TC_CORBA_double ((CORBA_TypeCode)&TC_CORBA_double_struct)
#define TC_CORBA_long_double ((CORBA_TypeCode)&TC_CORBA_long_double_struct)
#define TC_CORBA_boolean ((CORBA_TypeCode)&TC_CORBA_boolean_struct)
#define TC_CORBA_char ((CORBA_TypeCode)&TC_CORBA_char_struct)
#define TC_CORBA_wchar ((CORBA_TypeCode)&TC_CORBA_wchar_struct)
#define TC_CORBA_octet ((CORBA_TypeCode)&TC_CORBA_octet_struct)
#define TC_CORBA_any ((CORBA_TypeCode)&TC_CORBA_any_struct)
#define TC_CORBA_TypeCode ((CORBA_TypeCode)&TC_CORBA_TypeCode_struct)
#define TC_CORBA_Principal ((CORBA_TypeCode)&TC_CORBA_Principal_struct)
#define TC_CORBA_Object ((CORBA_TypeCode)&TC_CORBA_Object_struct)
#define TC_CORBA_string ((CORBA_TypeCode)&TC_CORBA_string_struct)
#define TC_CORBA_wstring ((CORBA_TypeCode)&TC_CORBA_wstring_struct)

extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_null_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_void_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_char_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_wchar_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_string_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_wstring_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_long_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_unsigned_long_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_short_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_unsigned_short_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_octet_struct;
#define TC_Object_struct TC_CORBA_Object_struct
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_Object_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_any_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_TypeCode_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_boolean_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_float_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_double_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_long_double_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_long_long_struct;
extern MATECORBA2_MAYBE_CONST struct CORBA_TypeCode_struct TC_CORBA_unsigned_long_long_struct;

#ifdef MATECORBA2_INTERNAL_API

#include <matecorba/GIOP/giop-basics.h>

void        MateCORBA_encode_CORBA_TypeCode (CORBA_TypeCode  tc,
					 GIOPSendBuffer *buf);
gboolean    MateCORBA_decode_CORBA_TypeCode (CORBA_TypeCode *tc,
					 GIOPRecvBuffer *buf);

const char *MateCORBA_tk_to_name            (CORBA_unsigned_long tk);

#endif /*  MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
