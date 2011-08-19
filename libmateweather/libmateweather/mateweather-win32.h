/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mateweather-win32.h - Win32 portability
 *
 * Copyright 2008, Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __MATEWEATHER_WIN32_H__
#define __MATEWEATHER_WIN32_H__

#ifdef _WIN32

#define localtime_r(t,tmp) (localtime (t) ? ((*tmp) = *localtime (t), tmp) : NULL)

#undef MATELOCALEDIR
#define MATELOCALEDIR _mateweather_win32_get_locale_dir ()

#undef ZONEINFO_DIR
#define ZONEINFO_DIR _mateweather_win32_get_zoneinfo_dir ()

#undef MATEWEATHER_XML_LOCATION_DIR
#define MATEWEATHER_XML_LOCATION_DIR _mateweather_win32_get_xml_location_dir ()

char *_mateweather_win32_get_locale_dir (void);
char *_mateweather_win32_get_zoneinfo_dir (void);
char *_mateweather_win32_get_xml_location_dir (void);

#endif

#endif /* __MATEWEATHER_WIN32_H__ */
