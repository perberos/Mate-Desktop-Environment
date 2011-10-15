/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  liboaf: A library for accessing oafd in a nice way.
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

#ifndef MATECOMPONENT_ACTIVATION_SERVER_INFO_H
#define MATECOMPONENT_ACTIVATION_SERVER_INFO_H

#include <matecomponent-activation/MateComponent_Activation_types.h>

#ifdef __cplusplus
extern "C" {
#endif

MateComponent_ActivationProperty *matecomponent_server_info_prop_find        (MateComponent_ServerInfo                      *server,
                                                                const char                             *prop_name);
const char                *matecomponent_server_info_prop_lookup      (MateComponent_ServerInfo                      *server,
                                                                const char                             *prop_name,
                                                                GSList                                 *i18n_languages);
void                       MateComponent_ActivationPropertyValue_copy           (MateComponent_ActivationPropertyValue                   *copy,
                                                                          const MateComponent_ActivationPropertyValue             *original);
void                       MateComponent_ActivationProperty_copy                (MateComponent_ActivationProperty                        *copy,
                                                                          const MateComponent_ActivationProperty                  *original);
void                       CORBA_sequence_MateComponent_ActivationProperty_copy (CORBA_sequence_MateComponent_ActivationProperty         *copy,
                                                                          const CORBA_sequence_MateComponent_ActivationProperty   *original);
void                       MateComponent_ServerInfo_copy              (MateComponent_ServerInfo                      *copy,
                                                                const MateComponent_ServerInfo                *original);
MateComponent_ServerInfo         *MateComponent_ServerInfo_duplicate         (const MateComponent_ServerInfo                *original);
MateComponent_ServerInfoList     *MateComponent_ServerInfoList_duplicate     (const MateComponent_ServerInfoList            *original);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_ACTIVATION_SERVER_INFO_H */


