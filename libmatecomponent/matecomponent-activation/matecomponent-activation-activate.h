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
#ifndef MATECOMPONENT_ACTIVATION_ACTIVATE_H
#define MATECOMPONENT_ACTIVATION_ACTIVATE_H

#include <matecomponent-activation/MateComponent_Activation_types.h>

#ifdef __cplusplus
extern "C" {
#endif

CORBA_Object matecomponent_activation_name_service_get (CORBA_Environment * ev);


MateComponent_ServerInfoList *matecomponent_activation_query   (const char *requirements,
                                                  char *const *selection_order,
                                                  CORBA_Environment * ev);
CORBA_Object matecomponent_activation_activate          (const char *requirements,
                                                  char *const *selection_order,
                                                  MateComponent_ActivationFlags flags,
                                                  MateComponent_ActivationID * ret_aid,
                                                  CORBA_Environment * ev);
CORBA_Object matecomponent_activation_activate_from_id  (const MateComponent_ActivationID aid,
                                                  MateComponent_ActivationFlags flags,
                                                  MateComponent_ActivationID * ret_aid,
                                                  CORBA_Environment * ev);

void         matecomponent_activation_set_activation_env_value (const char *name,
							 const char *value);

MateComponent_DynamicPathLoadResult
matecomponent_activation_dynamic_add_path (const char *add_path,
				    CORBA_Environment * ev);

MateComponent_DynamicPathLoadResult
matecomponent_activation_dynamic_remove_path (const char *remove_path,
				       CORBA_Environment * ev);
#ifndef MATECOMPONENT_DISABLE_DEPRECATED
/* debugging functions. */
void         matecomponent_activation_set_test_components_enabled (gboolean val);
gboolean     matecomponent_activation_get_test_components_enabled (void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_ACTIVATION_ACTIVATE_H */
