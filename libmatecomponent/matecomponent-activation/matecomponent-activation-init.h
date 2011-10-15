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
#ifndef MATECOMPONENT_ACTIVATION_INIT_H
#define MATECOMPONENT_ACTIVATION_INIT_H

#include <matecorba/matecorba.h>
#ifndef MATECOMPONENT_DISABLE_DEPRECATED
#include <popt.h>
#endif
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

CORBA_ORB      matecomponent_activation_orb_init   (int   *argc,
                                             char **argv);
CORBA_ORB      matecomponent_activation_orb_get    (void);

gboolean       matecomponent_activation_is_initialized   (void);
CORBA_ORB      matecomponent_activation_init       (int      argc,
                                             char   **argv);
void           matecomponent_activation_preinit    (gpointer app,
                                             gpointer mod_info);
void           matecomponent_activation_postinit   (gpointer app,
                                             gpointer mod_info);

/* deprecated / private to libmatecomponentui */
CORBA_Context  matecomponent_activation_context_get      (void);
#ifndef MATECOMPONENT_DISABLE_DEPRECATED
const char    *matecomponent_activation_hostname_get     (void);
const char    *matecomponent_activation_session_name_get (void);
const char    *matecomponent_activation_domain_get       (void);
#define matecomponent_activation_username_get() g_get_user_name()
extern struct poptOption matecomponent_activation_popt_options[];
char          *matecomponent_activation_get_popt_table_name (void);
#endif /* MATECOMPONENT_DISABLE_DEPRECATED */

gboolean       matecomponent_activation_debug_shutdown      (void);

GOptionGroup  *matecomponent_activation_get_goption_group   (void);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_ACTIVATION_INIT_H */
