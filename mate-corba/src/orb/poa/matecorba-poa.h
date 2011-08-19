#ifndef MATECORBA_POA_H
#define MATECORBA_POA_H 1

#include "config.h"

/*
 * Our object key is 28 bytes long and looks like this:
 *
 * .----- adaptor prefix -----.---------- ObjectId -----------,
 * |      4            16     |      4              4         |
 * | adaptor idx | orb cookie | object idx | object id random |
 */

#define MATECORBA_ADAPTOR_KEY_LEN     (128/8)
#define MATECORBA_ADAPTOR_PREFIX_LEN  (sizeof (CORBA_long) + MATECORBA_ADAPTOR_KEY_LEN)

/* We have an an auto-inc + 32 bits of poor randomness */
#define MATECORBA_OBJECT_ID_LEN 4

/*
 * Exported by poa.c.
 */
void               MateCORBA_POA_handle_held_requests  (PortableServer_POA poa);

PortableServer_POA MateCORBA_POA_setup_root            (CORBA_ORB          orb,
						    CORBA_Environment *ev);

CORBA_boolean      MateCORBA_POA_deactivate            (PortableServer_POA poa,
						    CORBA_boolean      etherealize_objects,
						    CORBA_Environment  *ev);

gboolean           MateCORBA_POA_is_inuse              (PortableServer_POA  poa,
					            CORBA_boolean       consider_children,
                                                    CORBA_Environment  *ev);

PortableServer_Current 
		   MateCORBA_POACurrent_new            (CORBA_ORB orb);

/*
 * Exported by poa-manager.c
 */
void               MateCORBA_POAManager_register_poa   (PortableServer_POAManager  poa_mgr, 
						    PortableServer_POA         poa);

void               MateCORBA_POAManager_unregister_poa (PortableServer_POAManager  poa_mgr, 
						    PortableServer_POA         poa);
PortableServer_POAManager
                   MateCORBA_POAManager_new            (CORBA_ORB                  orb);

#endif /* MATECORBA_POA_H */
