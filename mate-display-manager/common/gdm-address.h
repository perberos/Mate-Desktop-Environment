/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __GDM_ADDRESS_H
#define __GDM_ADDRESS_H

#include <glib-object.h>
#ifndef G_OS_WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#undef interface
#endif

G_BEGIN_DECLS

#define GDM_TYPE_ADDRESS (gdm_address_get_type ())
#define	gdm_sockaddr_len(sa) ((sa)->ss_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))

typedef struct _GdmAddress GdmAddress;

GType                    gdm_address_get_type                  (void);

GdmAddress *             gdm_address_new_from_sockaddr         (struct sockaddr *sa,
                                                                size_t           size);

int                      gdm_address_get_family_type           (GdmAddress              *address);
struct sockaddr_storage *gdm_address_get_sockaddr_storage      (GdmAddress              *address);
struct sockaddr_storage *gdm_address_peek_sockaddr_storage     (GdmAddress              *address);

gboolean                 gdm_address_get_hostname              (GdmAddress              *address,
                                                                char                   **hostname);
gboolean                 gdm_address_get_numeric_info          (GdmAddress              *address,
                                                                char                   **numeric_hostname,
                                                                char                   **service);
gboolean                 gdm_address_is_local                  (GdmAddress              *address);
gboolean                 gdm_address_is_loopback               (GdmAddress              *address);

gboolean                 gdm_address_equal                     (GdmAddress              *a,
                                                                GdmAddress              *b);

GdmAddress *             gdm_address_copy                      (GdmAddress              *address);
void                     gdm_address_free                      (GdmAddress              *address);


void                     gdm_address_debug                     (GdmAddress              *address);

const GList *            gdm_address_peek_local_list           (void);


G_END_DECLS

#endif /* __GDM_ADDRESS_H */
