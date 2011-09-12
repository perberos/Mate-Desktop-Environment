/* libmate-private.h: Uninstalled private header for libmate
 * Copyright (C) 2005 Novell, Inc
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __LIBMATE_PRIVATE_H__
#define __LIBMATE_PRIVATE_H__

#include <glib.h>

#ifdef G_OS_WIN32

	const char* _mate_get_prefix(void) G_GNUC_CONST;
	const char* _mate_get_localedir(void) G_GNUC_CONST;
	const char* _mate_get_libdir(void) G_GNUC_CONST;
	const char* _mate_get_datadir(void) G_GNUC_CONST;
	const char* _mate_get_localstatedir(void) G_GNUC_CONST;
	const char* _mate_get_sysconfdir(void) G_GNUC_CONST;

	#undef LIBMATE_PREFIX
	#define LIBMATE_PREFIX _mate_get_prefix()

	#undef LIBMATE_LOCALEDIR
	#define LIBMATE_LOCALEDIR _mate_get_localedir()

	#undef LIBMATE_LIBDIR
	#define LIBMATE_LIBDIR _mate_get_libdir()

	#undef LIBMATE_DATADIR
	#define LIBMATE_DATADIR _mate_get_datadir()

	#undef LIBMATE_LOCALSTATEDIR
	#define LIBMATE_LOCALSTATEDIR _mate_get_localstatedir()

	#undef LIBMATE_SYSCONFDIR
	#define LIBMATE_SYSCONFDIR _mate_get_sysconfdir()

#endif

#endif
