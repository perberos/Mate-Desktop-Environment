/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __POLKIT_MATE_AUTHENTICATOR_H
#define __POLKIT_MATE_AUTHENTICATOR_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POLKIT_MATE_TYPE_AUTHENTICATOR          (polkit_mate_authenticator_get_type())
#define POLKIT_MATE_AUTHENTICATOR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), POLKIT_MATE_TYPE_AUTHENTICATOR, PolkitMateAuthenticator))
#define POLKIT_MATE_AUTHENTICATOR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), POLKIT_MATE_TYPE_AUTHENTICATOR, PolkitMateAuthenticatorClass))
#define POLKIT_MATE_AUTHENTICATOR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), POLKIT_MATE_TYPE_AUTHENTICATOR, PolkitMateAuthenticatorClass))
#define POLKIT_MATE_IS_AUTHENTICATOR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), POLKIT_MATE_TYPE_AUTHENTICATOR))
#define POLKIT_MATE_IS_AUTHENTICATOR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), POLKIT_MATE_TYPE_AUTHENTICATOR))

typedef struct _PolkitMateAuthenticator PolkitMateAuthenticator;
typedef struct _PolkitMateAuthenticatorClass PolkitMateAuthenticatorClass;

GType                      polkit_mate_authenticator_get_type   (void) G_GNUC_CONST;
PolkitMateAuthenticator  *polkit_mate_authenticator_new        (const gchar              *action_id,
                                                                  const gchar              *message,
                                                                  const gchar              *icon_name,
                                                                  PolkitDetails            *details,
                                                                  const gchar              *cookie,
                                                                  GList                    *identities);
void                       polkit_mate_authenticator_initiate   (PolkitMateAuthenticator *authenticator);
void                       polkit_mate_authenticator_cancel     (PolkitMateAuthenticator *authenticator);
const gchar               *polkit_mate_authenticator_get_cookie (PolkitMateAuthenticator *authenticator);

#ifdef __cplusplus
}
#endif

#endif /* __POLKIT_MATE_AUTHENTICATOR_H */
