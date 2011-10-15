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
#ifndef MATECOMPONENT_ACTIVATION_REGISTER_H
#define MATECOMPONENT_ACTIVATION_REGISTER_H

#include <matecorba/matecorba.h>
#include <matecomponent-activation/MateComponent_Activation_types.h>

#ifdef __cplusplus
extern "C" {
#endif

MateComponent_RegistrationResult
	matecomponent_activation_register_active_server      (const char   *iid,
						       CORBA_Object  obj,
						       GSList       *reg_env);
MateComponent_RegistrationResult
        matecomponent_activation_register_active_server_ext  (const char               *iid,
                                                       CORBA_Object              obj,
                                                       GSList                   *reg_env,
                                                       MateComponent_RegistrationFlags  flags,
                                                       CORBA_Object             *existing,
                                                       const char               *description);
void    matecomponent_activation_unregister_active_server    (const char   *iid,
						       CORBA_Object  obj);

GSList *matecomponent_activation_registration_env_set        (GSList       *reg_env,
						       const char   *name,
						       const char   *value);
void    matecomponent_activation_registration_env_free       (GSList       *reg_env);

void    matecomponent_activation_registration_env_set_global (GSList       *reg_env,
						       gboolean      append_if_existing);


#ifndef MATECOMPONENT_DISABLE_DEPRECATED

MateComponent_RegistrationResult
	matecomponent_activation_active_server_register   (const char   *registration_id,
						    CORBA_Object  obj);

void    matecomponent_activation_active_server_unregister (const char   *iid,
						    CORBA_Object  obj);

char       *matecomponent_activation_make_registration_id (const char *iid,
						    const char *display);
#endif /* MATECOMPONENT_DISABLE_DEPRECATED */


const char *matecomponent_activation_iid_get       (void);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_ACTIVATION_REGISTER_H */
