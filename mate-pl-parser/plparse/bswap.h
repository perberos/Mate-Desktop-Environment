/* 
   Copyright (C) 2007 Bastien Nocera

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Authors: Bastien Nocera <hadess@hadess.net>

 */

#include <glib.h>

#define uint32_t guint32

#define _X_BE_32(x)	GUINT32_FROM_BE(*x)
#define _X_LE_32(x)	GUINT32_FROM_LE(*x)
#define _X_BE_16(x)	GUINT16_FROM_BE(*x)
#define _X_LE_16(x)	GUINT16_FROM_LE(*x)

