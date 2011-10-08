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
#ifndef MATECOMPONENT_ACTIVATION_H
#define MATECOMPONENT_ACTIVATION_H

// hax
#define MATECOMPONENT_TYPE_DOCK_PLACEMENT MATECOMPONENT_TYPE_COMPONENT_DOCK_PLACEMENT
#define MATECOMPONENT_TYPE_DOCK_ITEM_BEHAVIOR MATECOMPONENT_TYPE_COMPONENT_DOCK_ITEM_BEHAVIOR

#define MATECOMPONENT_TYPE_UI_TOOLBAR_STYLE MATECOMPONENT_TYPE_COMPONENT_UI_TOOLBAR_STYLE
#define MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM_STYLE MATECOMPONENT_TYPE_COMPONENT_UI_TOOLBAR_ITEM_STYLE
#define MATECOMPONENT_TYPE_UI_ERROR MATECOMPONENT_TYPE_COMPONENT_UI_ERROR


#include <matecomponent-activation/MateComponent_Unknown.h>
#include <matecomponent-activation/MateComponent_GenericFactory.h>

#include <matecomponent-activation/matecomponent-activation-version.h>

#include <matecomponent-activation/matecomponent-activation-activate.h>
#include <matecomponent-activation/matecomponent-activation-server-info.h>
#include <matecomponent-activation/matecomponent-activation-init.h>
#include <matecomponent-activation/matecomponent-activation-shlib.h>
#include <matecomponent-activation/matecomponent-activation-register.h>

#include <matecomponent-activation/matecomponent-activation-async.h>

extern const guint matecomponent_activation_major_version,
	matecomponent_activation_minor_version, matecomponent_activation_micro_version;
extern const char matecomponent_activation_version[];

#endif /* MATECOMPONENT_ACTIVATION_H */
