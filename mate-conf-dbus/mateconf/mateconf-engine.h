/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATECONF_MATECONF_ENGINE_H
#define MATECONF_MATECONF_ENGINE_H

#include <glib.h>
#include <mateconf/mateconf-error.h>

G_BEGIN_DECLS

/* A configuration engine (stack of config sources); normally there's
 * just one of these on the system.  
 */
typedef struct _MateConfEngine MateConfEngine;
  
MateConfEngine* mateconf_engine_get_default     (void);
/* returns NULL on error; requests single specified source */
MateConfEngine* mateconf_engine_get_for_address (const gchar* address,
                                           GError** err);
MateConfEngine* mateconf_engine_get_for_addresses (GSList *addresses,
                                             GError** err);
void         mateconf_engine_unref           (MateConfEngine* conf);
void         mateconf_engine_ref             (MateConfEngine* conf);

#ifdef MATECONF_ENABLE_INTERNALS
MateConfEngine *mateconf_engine_get_local               (const char  *address,
						   GError     **err);
MateConfEngine *mateconf_engine_get_local_for_addresses (GSList      *addresses,
						   GError     **err);
#endif

/* For use by language bindings only, will be deprecated in MATE 2.0
 * when we can make MateConfEngine a GObject
 */
void         mateconf_engine_set_user_data  (MateConfEngine   *engine,
                                          gpointer       data,
                                          GDestroyNotify dnotify);
gpointer     mateconf_engine_get_user_data  (MateConfEngine   *engine);


G_END_DECLS

#endif



