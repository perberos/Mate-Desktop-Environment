/* This stays outside */
#include <matecorba/matecorba-config.h>

#ifdef MATECORBA_IDL_SERIAL
#if MATECORBA_IDL_SERIAL < MATECORBA_CONFIG_SERIAL
#error "You need to rerun 'matecorba-idl' on the .idl file whose stubs you are using. These stubs were generated with an older version of MateCORBA, and need to be regenerated."
#endif
#endif

#ifndef MATECORBA_H
#define MATECORBA_H 1

#include <matecorba/matecorba-types.h>
#ifdef MATECORBA2_INTERNAL_API
#  include <matecorba/GIOP/giop.h>
#endif
#include <matecorba/orb-core/orb-core.h>
#include <matecorba/poa/poa.h>
#include <matecorba/dynamic/dynamic.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char  *matecorba_version;
extern unsigned int matecorba_major_version;
extern unsigned int matecorba_minor_version;
extern unsigned int matecorba_micro_version;

#ifdef __cplusplus
}
#endif

#endif
