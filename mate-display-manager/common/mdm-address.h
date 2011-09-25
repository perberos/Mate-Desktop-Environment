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


#ifndef __MDM_ADDRESS_H
#define __MDM_ADDRESS_H

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

#define MDM_TYPE_ADDRESS (mdm_address_get_type ())
#define	mdm_sockaddr_len(sa) ((sa)->ss_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))

typedef struct _MdmAddress MdmAddress;

GType                    mdm_address_get_type                  (void);

MdmAddress *             mdm_address_new_from_sockaddr         (struct sockaddr *sa,
                                                                size_t           size);

int                      mdm_address_get_family_type           (MdmAddress              *address);
struct sockaddr_storage *mdm_address_get_sockaddr_storage      (MdmAddress              *address);
struct sockaddr_storage *mdm_address_peek_sockaddr_storage     (MdmAddress              *address);

gboolean                 mdm_address_get_hostname              (MdmAddress              *address,
                                                                char                   **hostname);
gboolean                 mdm_address_get_numeric_info          (MdmAddress              *address,
                                                                char                   **numeric_hostname,
                                                                char                   **service);
gboolean                 mdm_address_is_local                  (MdmAddress              *address);
gboolean                 mdm_address_is_loopback               (MdmAddress              *address);

gboolean                 mdm_address_equal                     (MdmAddress              *a,
                                                                MdmAddress              *b);

MdmAddress *             mdm_address_copy                      (MdmAddress              *address);
void                     mdm_address_free                      (MdmAddress              *address);


void                     mdm_address_debug                     (MdmAddress              *address);

const GList *            mdm_address_peek_local_list           (void);


G_END_DECLS

#endif /* __MDM_ADDRESS_H */
