/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-main.h: MateComponent Main
 *
 * Author:
 *    Miguel de Icaza  (miguel@gnu.org)
 *    Nat Friedman     (nat@nat.org)
 *    Peter Wainwright (prw@wainpr.demo.co.uk)
 *
 * Copyright 1999 Helix Code, Inc.
 */

#ifndef __LIBMATECOMPONENT_MAIN_H__
#define __LIBMATECOMPONENT_MAIN_H__

#include <glib-object.h>
#include <matecomponent/MateComponent.h>

#ifdef __cplusplus
extern "C" {
#endif

gboolean                    matecomponent_is_initialized        (void);
gboolean		    matecomponent_init			 (int *argc,
							  char **argv);
gboolean                    matecomponent_init_full             (int *argc,
							  char **argv,
							  CORBA_ORB opt_orb,
							  PortableServer_POA opt_poa,
							  PortableServer_POAManager opt_manager);
int                         matecomponent_debug_shutdown        (void);
void			    matecomponent_main			 (void);
void                        matecomponent_main_quit             (void);
guint			    matecomponent_main_level            (void);
gboolean		    matecomponent_activate		 (void);
void			    matecomponent_setup_x_error_handler (void);

CORBA_ORB		    matecomponent_orb			 (void);
PortableServer_POA	    matecomponent_poa			 (void);
PortableServer_POAManager   matecomponent_poa_manager		 (void);
PortableServer_POA          matecomponent_poa_get_threaded      (MateCORBAThreadHint         hint,
							  ...);
PortableServer_POA          matecomponent_poa_get_threadedv     (MateCORBAThreadHint         hint,
							  va_list                 args);
PortableServer_POA          matecomponent_poa_new_from          (PortableServer_POA      tmpl,
							  const char             *name,
							  const CORBA_PolicyList *opt_policies,
							  CORBA_Environment      *opt_ev);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMATECOMPONENT_MAIN_H__ */
