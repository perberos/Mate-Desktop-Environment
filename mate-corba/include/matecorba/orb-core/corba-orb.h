#ifndef CORBA_ORB_H
#define CORBA_ORB_H 1

#include <matecorba/orb-core/orb-types.h>
#include <matecorba/orb-core/corba-orb-type.h>
#include <matecorba/orb-core/corba-typecode-type.h>

#ifdef __cplusplus
extern "C" {
#endif

CORBA_ORB CORBA_ORB_init (int                *argc,
			  char              **argv,
			  CORBA_ORBid         orb_identifier,
			  CORBA_Environment  *ev);

/* Will return TRUE if the named protocol is supported by
 * the ORB. Currently supported values of "name" are:
 *
 *    "IPv4"
 *    "IPv6"
 *    "UNIX"
 *    "IrDA"
 *    "SSL"
 *
 * Unknown or unsupported values of "name" will make this
 * method return FALSE.*/
gboolean  MateCORBA_proto_use (const char *name);

/* Will return the maximum allowed GIOP buffer size. You will
 * need to know this if your are e.g. streaming large data chunks
 * to an MateCORBA2 client. The return type should be gulong but we
 * are bound by the type chosen internally by linc2.
 */
glong MateCORBA_get_giop_recv_limit (void);

#ifdef MATECORBA2_INTERNAL_API

void      MateCORBA_ORB_forw_bind (CORBA_ORB                   orb,
			       CORBA_sequence_CORBA_octet *okey,
			       CORBA_Object                oref,
			       CORBA_Environment          *ev);

guint     MateCORBA_ORB_idle_init     (CORBA_ORB orb);

void      MateCORBA_ORB_start_servers (CORBA_ORB orb);

#endif /* MATECORBA2_INTERNAL_API */

#ifdef __cplusplus
}
#endif

#endif
