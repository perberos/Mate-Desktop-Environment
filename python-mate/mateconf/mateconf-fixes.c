/* MateConf-python
 * Copyright (C) 2002 Johan Dahlin
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
 * 
 * Author: Johan Dahlin <jdahlin@telia.com>
 */

#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf-value.h>

#include "mateconf-fixes.h"

MateConfMetaInfo*
mateconf_meta_info_copy (const MateConfMetaInfo *src)
{
	MateConfMetaInfo *info;
	
	info = mateconf_meta_info_new ();
	 
	info->schema = g_strdup (src->schema);
	info->mod_user = g_strdup (src->mod_user);
	info->mod_time = src->mod_time;
	
	return info;
}

#define BOILERPLATE_TYPE_BOXED(func,name)                                         \
GType                                                                             \
py##func##_get_type (void)                                                        \
{                                                                                 \
	static GType type = 0;                                                    \
	if (type == 0) {                                                          \
		type = g_boxed_type_register_static(name,                         \
                                                    (GBoxedCopyFunc)func##_copy,  \
						    (GBoxedFreeFunc)func##_free); \
	}                                                                         \
	return type;                                                              \
}

BOILERPLATE_TYPE_BOXED(mateconf_value,     "MateConfValue")
BOILERPLATE_TYPE_BOXED(mateconf_entry,     "MateConfEntry")
BOILERPLATE_TYPE_BOXED(mateconf_schema,    "MateConfSchema")
BOILERPLATE_TYPE_BOXED(mateconf_meta_info, "MateConfMetaInfo")
