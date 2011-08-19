/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-context.c - context VFS modules can use to communicate with mate-vfs proper

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Havoc Pennington <hp@redhat.com> */

#include <config.h>
#include "mate-vfs-context.h"

#include "mate-vfs-backend.h"
#include "mate-vfs-cancellation.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-utils.h"
#include <stdio.h>

#if 1
#define DEBUG_MSG (x) printf x
#else
#define DEBUG_MSG (x)
#endif


struct MateVFSContext {
        MateVFSCancellation *cancellation;
};

/* This is a token Context to return in situations
 * where we don't normally have a context: eg, during sync calls
 */
static const MateVFSContext sync_context = {NULL};

/**
 * mate_vfs_context_new:
 * 
 * Creates a new context and cancellation object. Must be called
 * from the main glib event loop.
 *
 * Return value: a newly allocated #MateVFSContext.
 */
MateVFSContext*
mate_vfs_context_new (void)
{
        MateVFSContext *ctx;

        ctx = g_new0(MateVFSContext, 1);

        ctx->cancellation = mate_vfs_cancellation_new();
 
        return ctx;
}

/**
 * mate_vfs_context_free:
 * @ctx: context to be freed.
 *
 * Free @ctx and destroy the associated #MateVFSCancellation.
 */
void
mate_vfs_context_free (MateVFSContext *ctx)
{
        g_return_if_fail(ctx != NULL);
  
	mate_vfs_cancellation_destroy(ctx->cancellation);
	
	g_free(ctx);
}

/**
 * mate_vfs_context_get_cancellation:
 * @ctx: context to get the #MateVFSCancellation from.
 *
 * Retrieve the #MateVFSCancellation associated with @ctx.
 *
 * Return value: @ctx 's #MateVFSCancellation.
 */
MateVFSCancellation*
mate_vfs_context_get_cancellation (const MateVFSContext *ctx)
{
        g_return_val_if_fail(ctx != NULL, NULL);
        return ctx->cancellation;
}

/**
 * mate_vfs_context_peek_current:
 *
 * Get the currently active context. It shouldn't be
 * manipulated but can be compared to contexts module
 * holds to determine whether they are active.
 *
 * Return value: the currently active #MateVFSContext.
 */
const MateVFSContext *
mate_vfs_context_peek_current		  (void)
{
	const MateVFSContext *ret;
	
	_mate_vfs_get_current_context ((MateVFSContext **)&ret);

	/* If the context is NULL, then this must be a synchronous call */
	if (ret == NULL) {
		ret = &sync_context;
	}

	return ret;
}

/**
 * mate_vfs_context_check_cancellation_current:
 * 
 * Check to see if the currently active context has been cancelled.
 *
 * Return value: %TRUE if the currently active context has been cancelled, otherwise %FALSE.
 */
gboolean
mate_vfs_context_check_cancellation_current (void)
{
	const MateVFSContext *current_ctx;

	current_ctx = mate_vfs_context_peek_current ();

	if (current_ctx == &sync_context) {
		return FALSE;
	} else if (current_ctx != NULL) {
		return mate_vfs_cancellation_check (mate_vfs_context_get_cancellation (current_ctx));
	} else {
		return FALSE;
	}	
}
