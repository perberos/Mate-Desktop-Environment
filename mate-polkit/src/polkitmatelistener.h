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

#ifndef __POLKIT_MATE_LISTENER_H
#define __POLKIT_MATE_LISTENER_H

#include <polkitagent/polkitagent.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POLKIT_MATE_TYPE_LISTENER          (polkit_mate_listener_get_type())
#define POLKIT_MATE_LISTENER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), POLKIT_MATE_TYPE_LISTENER, PolkitMateListener))
#define POLKIT_MATE_LISTENER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), POLKIT_MATE_TYPE_LISTENER, PolkitMateListenerClass))
#define POLKIT_MATE_LISTENER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), POLKIT_MATE_TYPE_LISTENER, PolkitMateListenerClass))
#define POLKIT_MATE_IS_LISTENER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), POLKIT_MATE_TYPE_LISTENER))
#define POLKIT_MATE_IS_LISTENER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), POLKIT_MATE_TYPE_LISTENER))

typedef struct _PolkitMateListener PolkitMateListener;
typedef struct _PolkitMateListenerClass PolkitMateListenerClass;

GType                 polkit_mate_listener_get_type   (void) G_GNUC_CONST;
PolkitAgentListener  *polkit_mate_listener_new        (void);

#ifdef __cplusplus
}
#endif

#endif /* __POLKIT_MATE_LISTENER_H */
