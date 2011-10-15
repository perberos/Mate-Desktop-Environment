/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-persist-client.h: Client-side utility functions dealing with persistancy
 *
 * Author:
 *   ÉRDI Gergõ <cactus@cactus.rulez.org>
 *
 * Copyright 2001 Gergõ Érdi
 */
#ifndef _MATECOMPONENT_PERSIST_CLIENT_H_
#define _MATECOMPONENT_PERSIST_CLIENT_H_

#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

void matecomponent_object_save_to_stream (MateComponent_Unknown     object,
				   MateComponent_Stream      stream,
				   CORBA_Environment *opt_ev);
MateComponent_Unknown matecomponent_object_from_stream (MateComponent_Stream      stream,
					  CORBA_Environment *opt_ev);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_PERSIST_CLIENT_H_ */
