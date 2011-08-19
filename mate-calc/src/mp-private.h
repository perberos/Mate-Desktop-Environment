/*  Copyright (c) 1987-2008 Sun Microsystems, Inc. All Rights Reserved.
 *  Copyright (c) 2008-2009 Robert Ancell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 */

#ifndef MP_INTERNAL_H
#define MP_INTERNAL_H

#include <glib/gi18n.h>

/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
#  define  __attribute__(x)  /*NOTHING*/
#endif

#define min(a, b)   ((a) <= (b) ? (a) : (b))
#define max(a, b)   ((a) >= (b) ? (a) : (b))

//2E0 BELOW ENSURES AT LEAST ONE GUARD DIGIT
//MP.t = (int) ((float) (accuracy) * log((float)10.) / log((float) MP_BASE) + (float) 2.0);
//if (MP.t > MP_SIZE) {
//    mperr("MP_SIZE TOO SMALL IN CALL TO MPSET, INCREASE MP_SIZE AND DIMENSIONS OF MP ARRAYS TO AT LEAST %d ***", MP.t);
//    MP.t = MP_SIZE;
//}
#define MP_T 100

void mperr(const char *format, ...) __attribute__((format(printf, 1, 2)));
void mp_gcd(int64_t *, int64_t *);
void mp_normalize(MPNumber *);
void convert_to_radians(const MPNumber *x, MPAngleUnit unit, MPNumber *z);
void convert_from_radians(const MPNumber *x, MPAngleUnit unit, MPNumber *z);

#endif /* MP_INTERNAL_H */
