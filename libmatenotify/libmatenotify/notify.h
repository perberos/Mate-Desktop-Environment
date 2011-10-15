/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef _LIBMATENOTIFY_NOTIFY_H_
#define _LIBMATENOTIFY_NOTIFY_H_

#include <glib.h>
#include <time.h>

#include <libmatenotify/notification.h>
#include <libmatenotify/notify-enum-types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes the notifications library.
 *
 * @param app_name The application name.
 *
 * @return TRUE if the library initialized properly.
 */
gboolean        notify_init (const char *app_name);

/**
 * Uninitializes the notifications library.
 *
 * This will be automatically called on exit unless previously called.
 */
void            notify_uninit (void);

/**
 * Returns whether or not the notification library is initialized.
 *
 * @return TRUE if the library is initialized, or FALSE.
 */
gboolean        notify_is_initted (void);

/**
 * Returns the name of the application set when notify_init() was called.
 *
 * @return The name of the application.
 */
const gchar    *notify_get_app_name (void);

/**
 * Returns the capabilities of the notification server.
 *
 * @return A list of capability strings. These strings must be freed.
 */
GList          *notify_get_server_caps (void);

/**
 * Returns the server notification information.
 *
 * The strings returned must be freed.
 *
 * @param ret_name         The returned product name of the server.
 * @param ret_vendor       The returned vendor.
 * @param ret_version      The returned server version.
 * @param ret_spec_version The returned specification version supported.
 *
 * @return TRUE if the call succeeded, or FALSE if there were errors.
 */
gboolean        notify_get_server_info (char **ret_name,
                                        char **ret_vendor,
                                        char **ret_version,
                                        char **ret_spec_version);

#ifdef __cplusplus
}
#endif

#endif /* _LIBMATENOTIFY_NOTIFY_H_ */
