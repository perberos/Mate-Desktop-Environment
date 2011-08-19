/*
 * matecomponent-moniker-util.h
 *
 * Copyright (C) 2000  Helix Code, Inc.
 *
 * Authors:
 *	Michael Meeks    (michael@helixcode.com)
 *	Ettore Perazzoli (ettore@helixcode.com)
 */

#ifndef _MATECOMPONENT_MONIKER_UTIL_H
#define _MATECOMPONENT_MONIKER_UTIL_H

#include <matecomponent/MateComponent.h>

/* Useful client functions */
MateComponent_Unknown      matecomponent_get_object                            (const CORBA_char        *name,      
								  const char              *interface_name,
								  CORBA_Environment       *opt_ev);


MateComponent_Moniker      matecomponent_moniker_client_new_from_name          (const CORBA_char        *name,
								  CORBA_Environment       *opt_ev);

CORBA_char         *matecomponent_moniker_client_get_name               (MateComponent_Moniker     moniker,
								  CORBA_Environment *opt_ev);

MateComponent_Unknown      matecomponent_moniker_client_resolve_default        (MateComponent_Moniker     moniker,
								  const char        *interface_name,
								  CORBA_Environment *opt_ev);

gboolean            matecomponent_moniker_client_equal                  (MateComponent_Moniker     moniker,
								  const CORBA_char  *name,
								  CORBA_Environment *opt_ev);

typedef void (*MateComponentMonikerAsyncFn) (MateComponent_Unknown     object,
				      CORBA_Environment *ev,
				      gpointer           user_data);
/* Async equivalents */

void                matecomponent_get_object_async                      (const CORBA_char        *name,
								  const char              *interface_name,
								  CORBA_Environment       *ev,
								  MateComponentMonikerAsyncFn     cb,
								  gpointer                 user_data);

void                matecomponent_moniker_client_new_from_name_async    (const CORBA_char        *name,
								  CORBA_Environment       *ev,
								  MateComponentMonikerAsyncFn     cb,
								  gpointer                 user_data);

void                matecomponent_moniker_resolve_async                 (MateComponent_Moniker           moniker,
								  MateComponent_ResolveOptions   *options,
								  const char              *interface_name,
								  CORBA_Environment       *ev,
								  MateComponentMonikerAsyncFn     cb,
								  gpointer                 user_data);

void                matecomponent_moniker_resolve_async_default         (MateComponent_Moniker           moniker,
								  const char              *interface_name,
								  CORBA_Environment       *ev,
								  MateComponentMonikerAsyncFn     cb,
								  gpointer                 user_data);

/* Useful moniker implementation helper functions */
CORBA_char    *matecomponent_moniker_util_get_parent_name      (MateComponent_Moniker     moniker,
							 CORBA_Environment *opt_ev);
MateComponent_Unknown matecomponent_moniker_util_qi_return            (MateComponent_Unknown     object,
							 const CORBA_char  *requested_interface,
							 CORBA_Environment *ev);
const char    *matecomponent_moniker_util_parse_name           (const char        *name, 
							 int               *plen);
int            matecomponent_moniker_util_seek_std_separator   (const CORBA_char  *str,
							 int                min_idx);
char          *matecomponent_moniker_util_escape               (const char        *string,
							 int                offset);
char          *matecomponent_moniker_util_unescape             (const char        *string,
							 int                num_chars);

void           matecomponent_url_register                      (char              *oafiid, 
							 char              *url, 
							 char              *mime_type,
							 MateComponent_Unknown     object,
							 CORBA_Environment *ev);

void           matecomponent_url_unregister                    (char              *oafiid, 
							 char              *url,
							 CORBA_Environment *ev);

MateComponent_Unknown matecomponent_url_lookup                        (char              *oafiid,
							 char              *url,
							 CORBA_Environment *ev);
					   
#endif /* _MATECOMPONENT_MONIKER_UTIL_H */
