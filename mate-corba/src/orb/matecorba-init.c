#include "config.h"
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef _WIN32
#  include <process.h>
#endif
#include <matecorba/matecorba.h>

#include "matecorba-init.h"
#include "poa/matecorba-poa.h"
#include "orb-core/orb-core-private.h"

#ifdef G_OS_WIN32
#  define getuid() 0
#endif

void
MateCORBA_init_internals (CORBA_ORB          orb,
		      CORBA_Environment *ev)
{
	PortableServer_POA       root_poa;
	PortableServer_Current   poa_current;
	DynamicAny_DynAnyFactory dynany_factory;
	GTimeVal                 t;

	root_poa = MateCORBA_POA_setup_root (orb, ev);
	MateCORBA_set_initial_reference (orb, "RootPOA", root_poa);
	MateCORBA_RootObject_release (root_poa);

	poa_current = MateCORBA_POACurrent_new (orb);
	MateCORBA_set_initial_reference (orb, "POACurrent", poa_current);
	MateCORBA_RootObject_release (poa_current);

	dynany_factory = MateCORBA_DynAnyFactory_new (orb, ev);
	MateCORBA_set_initial_reference (orb, "DynAnyFactory", dynany_factory);
	MateCORBA_RootObject_release (dynany_factory);

	/* need to srand for linc's node creation */
	g_get_current_time (&t);
	srand (t.tv_sec ^ t.tv_usec ^ getpid () ^ getuid ());
}

const char  *matecorba_version       = MATECORBA_VERSION;
unsigned int matecorba_major_version = MATECORBA_MAJOR_VERSION;
unsigned int matecorba_minor_version = MATECORBA_MINOR_VERSION; 
unsigned int matecorba_micro_version = MATECORBA_MICRO_VERSION;
