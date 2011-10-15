/* mate-url.h
 * Copyright (C) 1998 James Henstridge <james@daa.com.au>
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
 */

#ifndef MATE_URL_H
#define MATE_URL_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateURLError:
 * @MATE_URL_ERROR_PARSE: The parsing of the handler failed.
 *
 * The errors that can be returned due to bad parameters being pass to
 * mate_url_show().
 */
typedef enum {
	MATE_URL_ERROR_PARSE,
	MATE_URL_ERROR_LAUNCH,
	MATE_URL_ERROR_URL,
	MATE_URL_ERROR_NO_DEFAULT,
	MATE_URL_ERROR_NOT_SUPPORTED,
	MATE_URL_ERROR_VFS,
	MATE_URL_ERROR_CANCELLED
} MateURLError;

#define MATE_URL_ERROR (mate_url_error_quark())
GQuark mate_url_error_quark(void) G_GNUC_CONST;


gboolean mate_url_show(const char* url, GError** error);

gboolean mate_url_show_with_env(const char* url, char** envp, GError** error);

#ifdef __cplusplus
}
#endif
#endif
