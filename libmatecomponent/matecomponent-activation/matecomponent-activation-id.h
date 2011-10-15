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
#ifndef MATECOMPONENT_ACTIVATION_ID_H
#define MATECOMPONENT_ACTIVATION_ID_H

#ifndef MATECOMPONENT_DISABLE_DEPRECATED

#include <matecomponent-activation/MateComponent_Activation_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* If you wish to manipulate the internals of this structure, please
   use g_malloc/g_free to allocate memory. */
typedef struct
{
	char *iid;		/* Implementation ID */
	char *user;		/* user name */
	char *host;		/* DNS name or IP address */
	char *domain;		/* FIXME: unused - remove ? */
}
MateComponentActivationInfo;


MateComponent_ActivationID    matecomponent_activation_info_stringify      (const MateComponentActivationInfo *actinfo);
MateComponentActivationInfo  *matecomponent_activation_servinfo_to_actinfo (const MateComponent_ServerInfo    *servinfo);
MateComponentActivationInfo  *matecomponent_activation_id_parse            (const CORBA_char           *actid);
MateComponentActivationInfo  *matecomponent_activation_info_new            (void);
void                   matecomponent_activation_info_free           (MateComponentActivationInfo       *actinfo);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

#endif /* MATECOMPONENT_ACTIVATION_ID_H */
