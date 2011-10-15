/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * Main include file for the MateComponent component model
 *
 * Authors:
 *   Miguel de Icaza (miguel@ximian.com)
 *   Michael Meeks   (michael@ximian.com)
 *
 * Copyright 2001 Ximian, Inc.
 */
#ifndef __LIBMATECOMPONENT_H__
#define __LIBMATECOMPONENT_H__

#include <matecomponent/matecomponent-macros.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <matecomponent/matecomponent-types.h>

#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-moniker.h>
#include <matecomponent/matecomponent-moniker-simple.h>
#include <matecomponent/matecomponent-context.h>
#include <matecomponent/matecomponent-exception.h>

#include <matecomponent/matecomponent-item-container.h>
#include <matecomponent/matecomponent-item-handler.h>
#include <matecomponent/matecomponent-moniker-util.h>
#include <matecomponent/matecomponent-moniker-extender.h>

#include <matecomponent/matecomponent-property-bag.h>
#include <matecomponent/matecomponent-property-bag-client.h>

#include <matecomponent/matecomponent-listener.h>
#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent/matecomponent-generic-factory.h>
#include <matecomponent/matecomponent-shlib-factory.h>
#include <matecomponent/matecomponent-main.h>

#include <matecomponent/matecomponent-stream.h>
#include <matecomponent/matecomponent-stream-memory.h>
#include <matecomponent/matecomponent-stream-client.h>

#include <matecomponent/matecomponent-persist.h>
#include <matecomponent/matecomponent-persist-file.h>
#include <matecomponent/matecomponent-persist-stream.h>

#include <matecomponent/matecomponent-storage.h>
#include <matecomponent/matecomponent-storage-memory.h>

#include <matecomponent/matecomponent-application.h>
#include <matecomponent/matecomponent-app-client.h>

#ifdef __cplusplus
}
#endif

#endif /* __LIBMATECOMPONENT_H__ */
