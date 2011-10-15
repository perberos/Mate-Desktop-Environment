/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation: A library for accessing matecomponent-activation-server.
 *
 *  Copyright (C) 1999, 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Elliot Lee <sopwith@redhat.com>
 */
#ifndef MATECOMPONENT_ACTIVATION_SHLIB_H
#define MATECOMPONENT_ACTIVATION_SHLIB_H

#include <matecomponent-activation/MateComponent_Activation_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *iid;

	/* This routine should call matecomponent_activation_plugin_use(servant, impl_ptr),
         * as should all routines which activate CORBA objects
	 * implemented by this shared library. This needs to be done
         * before making any CORBA calls on the object, or
	 * passing that object around. First thing after servant creation
         * always works. :)
         */

        CORBA_Object (*activate) (PortableServer_POA poa,
                                  const char *iid,
                                  gpointer impl_ptr,	/* This pointer should be stored by the implementation
                                                         * to be passed to matecomponent_activation_plugin_unuse() in the
                                                         * implementation's destruction routine. */
				  CORBA_Environment *ev);
        gpointer dummy[4];
} MateComponentActivationPluginObject;

typedef struct {
	const MateComponentActivationPluginObject *plugin_object_list;
	const char *description;
        gpointer dummy[8];
} MateComponentActivationPlugin;

void  matecomponent_activation_plugin_use    (PortableServer_Servant servant,
                                       gpointer impl_ptr);

void  matecomponent_activation_plugin_unuse  (gpointer impl_ptr);

#ifndef MATECOMPONENT_DISABLE_DEPRECATED
CORBA_Object matecomponent_activation_activate_shlib_server (MateComponent_ActivationResult *sh,
                                                      CORBA_Environment    *ev);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_ACTIVATION_SHLIB_H */
