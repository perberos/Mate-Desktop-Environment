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
 *
 */
/* The  folowing API is not intended for application use.
 * It is intended only for people who want to extend OAF bootstraping system.
 * I have no idea why we have all this tralala but Eliot knows and he _tried_
 * to explain it in docs/matecomponent-activation-base-service.txt
 */

/*
 * DO NOT USE this API, it is deprecated and crufty.
 */

#ifndef MATECOMPONENT_ACTIVATION_BASE_SERVICE_H
#define MATECOMPONENT_ACTIVATION_BASE_SERVICE_H

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

#include <glib.h>
#include <matecorba/matecorba.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *name;
	const char *session_name;
	const char *username;
        const char *hostname;
        const char *domain; /* FIXME: unused - remove ? */
} MateComponentActivationBaseService;

typedef struct _MateComponentActivationBaseServiceRegistry MateComponentActivationBaseServiceRegistry;

struct _MateComponentActivationBaseServiceRegistry {
	void   (*lock)         (const MateComponentActivationBaseServiceRegistry *registry,
                                gpointer                                   user_data);
	void   (*unlock)       (const MateComponentActivationBaseServiceRegistry *registry,
                                gpointer                      user_data);
	char * (*check)        (const MateComponentActivationBaseServiceRegistry *registry,
                                const MateComponentActivationBaseService         *base_service,
                                int                          *ret_distance,
                                gpointer                      user_data);
	void   (*register_new) (const MateComponentActivationBaseServiceRegistry *registry,
                                const char                   *ior,
                                const MateComponentActivationBaseService         *base_service,
                                gpointer                      user_data);
	void   (*unregister)   (const MateComponentActivationBaseServiceRegistry *registry,
                                const char                   *ior,
                                const MateComponentActivationBaseService         *base_service,
                                gpointer                      user_data);
};

typedef CORBA_Object (*MateComponentActivationBaseServiceActivator) (
                                const MateComponentActivationBaseService *base_service,
                                const char                       **command,
                                int                                ior_fd,
                                CORBA_Environment                 *ev);

/* unused / deprecated */
void         matecomponent_activation_base_service_registry_add
                               (const MateComponentActivationBaseServiceRegistry *registry,
                                int                                        priority,
                                gpointer                                   user_data);
/* unused / deprecated */
CORBA_Object matecomponent_activation_base_service_check
                               (const MateComponentActivationBaseService *base_service,
                                CORBA_Environment                 *ev);

/* unused / deprecated */
void         matecomponent_activation_base_service_set
                               (const MateComponentActivationBaseService         *base_service,
                                CORBA_Object                               obj,
                                CORBA_Environment                         *ev);
/* unused / deprecated */
void         matecomponent_activation_base_service_unset
                               (const MateComponentActivationBaseService         *base_service,
                                CORBA_Object                               obj,
                                CORBA_Environment                         *ev);
/* unused / deprecated */
void         matecomponent_activation_base_service_activator_add
                               (MateComponentActivationBaseServiceActivator       activator,
                                int                                        priority);

/* Do not release() the returned value */
CORBA_Object matecomponent_activation_service_get
                               (const MateComponentActivationBaseService         *base_service);

void         matecomponent_activation_base_service_debug_shutdown
                               (CORBA_Environment                         *ev);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

#endif /* MATECOMPONENT_ACTIVATION_BASE_SERVICE_H */
