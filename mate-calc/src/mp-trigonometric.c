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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <libintl.h>

#include "mp.h"
#include "mp-private.h"

static int
mp_compare_mp_to_int(const MPNumber *x, int i)
{
    MPNumber t;
    mp_set_from_integer(i, &t);
    return mp_compare_mp_to_mp(x, &t);
}


/* Convert x to radians */
void
convert_to_radians(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber t1, t2;

    switch(unit) {
    default:
    case MP_RADIANS:
        mp_set_from_mp(x, z);
        break;

    case MP_DEGREES:
        mp_get_pi(&t1);
        mp_multiply(x, &t1, &t2);
        mp_divide_integer(&t2, 180, z);
        break;

    case MP_GRADIANS:
        mp_get_pi(&t1);
        mp_multiply(x, &t1, &t2);
        mp_divide_integer(&t2, 200, z);
        break;
    }
}


void
mp_get_pi(MPNumber *z)
{
    mp_set_from_string("3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679", 10, z);
}


void
convert_from_radians(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber t1, t2;

    switch (unit) {
    default:
    case MP_RADIANS:
        mp_set_from_mp(x, z);
        break;

    case MP_DEGREES:
        mp_multiply_integer(x, 180, &t2);
        mp_get_pi(&t1);
        mp_divide(&t2, &t1, z);
        break;

    case MP_GRADIANS:
        mp_multiply_integer(x, 200, &t2);
        mp_get_pi(&t1);
        mp_divide(&t2, &t1, z);
        break;
    }
}


/* z = sin(x) -1 >= x >= 1, do_sin = 1
 * z = cos(x) -1 >= x >= 1, do_sin = 0
 */
static void
mpsin1(const MPNumber *x, MPNumber *z, int do_sin)
{
    int i, b2;
    MPNumber t1, t2;

    /* sin(0) = 0, cos(0) = 1 */
    if (mp_is_zero(x)) {
        if (do_sin == 0)
            mp_set_from_integer(1, z);
        else
            mp_set_from_integer(0, z);
        return;
    }

    mp_multiply(x, x, &t2);
    if (mp_compare_mp_to_int(&t2, 1) > 0) {
        mperr("*** ABS(X) > 1 IN CALL TO MPSIN1 ***");
    }

    if (do_sin == 0) {
        mp_set_from_integer(1, &t1);
        mp_set_from_integer(0, z);
        i = 1;
    } else {
        mp_set_from_mp(x, &t1);
        mp_set_from_mp(&t1, z);
        i = 2;
    }

    /* Taylor series */
    /* POWER SERIES LOOP.  REDUCE T IF POSSIBLE */
    b2 = 2 * max(MP_BASE, 64);
    do {
        if (MP_T + t1.exponent <= 0)
            break;

        /*  IF I*(I+1) IS NOT REPRESENTABLE AS AN INTEGER, THE FOLLOWING
         *  DIVISION BY I*(I+1) HAS TO BE SPLIT UP.
         */
        mp_multiply(&t2, &t1, &t1);
        if (i > b2) {
            mp_divide_integer(&t1, -i, &t1);
            mp_divide_integer(&t1, i + 1, &t1);
        } else {
            mp_divide_integer(&t1, -i * (i + 1), &t1);
        }
        mp_add(&t1, z, z);

        i += 2;
    } while (t1.sign != 0);

    if (do_sin == 0)
        mp_add_integer(z, 1, z);
}


static void
mp_sin_real(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    int xs;
    MPNumber x_radians;

    /* sin(0) = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    convert_to_radians(x, unit, &x_radians);

    xs = x_radians.sign;
    mp_abs(&x_radians, &x_radians);

    /* USE MPSIN1 IF ABS(X) <= 1 */
    if (mp_compare_mp_to_int(&x_radians, 1) <= 0) {
        mpsin1(&x_radians, z, 1);
    }
    /* FIND ABS(X) MODULO 2PI */
    else {
        mp_get_pi(z);
        mp_divide_integer(z, 4, z);
        mp_divide(&x_radians, z, &x_radians);
        mp_divide_integer(&x_radians, 8, &x_radians);
        mp_fractional_component(&x_radians, &x_radians);

        /* SUBTRACT 1/2, SAVE SIGN AND TAKE ABS */
        mp_add_fraction(&x_radians, -1, 2, &x_radians);
        xs = -xs * x_radians.sign;
        if (xs == 0) {
            mp_set_from_integer(0, z);
            return;
        }

        x_radians.sign = 1;
        mp_multiply_integer(&x_radians, 4, &x_radians);

        /* IF NOT LESS THAN 1, SUBTRACT FROM 2 */
        if (x_radians.exponent > 0)
            mp_add_integer(&x_radians, -2, &x_radians);

        if (mp_is_zero(&x_radians)) {
            mp_set_from_integer(0, z);
            return;
        }

        x_radians.sign = 1;
        mp_multiply_integer(&x_radians, 2, &x_radians);

        /*  NOW REDUCED TO FIRST QUADRANT, IF LESS THAN PI/4 USE
         *  POWER SERIES, ELSE COMPUTE COS OF COMPLEMENT
         */
        if (x_radians.exponent > 0) {
            mp_add_integer(&x_radians, -2, &x_radians);
            mp_multiply(&x_radians, z, &x_radians);
            mpsin1(&x_radians, z, 0);
        } else {
            mp_multiply(&x_radians, z, &x_radians);
            mpsin1(&x_radians, z, 1);
        }
    }

    z->sign = xs;
}


static void
mp_cos_real(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    /* cos(0) = 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }

    convert_to_radians(x, unit, z);

    /* Use power series if |x| <= 1 */
    mp_abs(z, z);
    if (mp_compare_mp_to_int(z, 1) <= 0) {
        mpsin1(z, z, 0);
    } else {
        MPNumber t;

        /* cos(x) = sin(π/2 - |x|) */
        mp_get_pi(&t);
        mp_divide_integer(&t, 2, &t);
        mp_subtract(&t, z, z);
        mp_sin(z, MP_RADIANS, z);
    }
}


void
mp_sin(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    if (mp_is_complex(x)) {
        MPNumber x_real, x_im, z_real, z_im, t;

        mp_real_component(x, &x_real);
        mp_imaginary_component(x, &x_im);

        mp_sin_real(&x_real, unit, &z_real);
        mp_cosh(&x_im, &t);
        mp_multiply(&z_real, &t, &z_real);

        mp_cos_real(&x_real, unit, &z_im);
        mp_sinh(&x_im, &t);
        mp_multiply(&z_im, &t, &z_im);

        mp_set_from_complex(&z_real, &z_im, z);
    }
    else
       mp_sin_real(x, unit, z);
}


void
mp_cos(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    if (mp_is_complex(x)) {
        MPNumber x_real, x_im, z_real, z_im, t;

        mp_real_component(x, &x_real);
        mp_imaginary_component(x, &x_im);

        mp_cos_real(&x_real, unit, &z_real);
        mp_cosh(&x_im, &t);
        mp_multiply(&z_real, &t, &z_real);

        mp_sin_real(&x_real, unit, &z_im);
        mp_sinh(&x_im, &t);
        mp_multiply(&z_im, &t, &z_im);
        mp_invert_sign(&z_im, &z_im);

        mp_set_from_complex(&z_real, &z_im, z);
    }
    else
       mp_cos_real(x, unit, z);
}


void
mp_tan(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber cos_x, sin_x;

    /* Check for undefined values */
    mp_cos(x, unit, &cos_x);
    if (mp_is_zero(&cos_x)) {
        /* Translators: Error displayed when tangent value is undefined */
        mperr(_("Tangent is undefined for angles that are multiples of π (180°) from π∕2 (90°)"));
        mp_set_from_integer(0, z);
        return;
    }

    /* tan(x) = sin(x) / cos(x) */
    mp_sin(x, unit, &sin_x);
    mp_divide(&sin_x, &cos_x, z);
}


void
mp_asin(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber t1, t2;

    /* asin⁻¹(0) = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* sin⁻¹(x) = tan⁻¹(x / √(1 - x²)), |x| < 1 */
    if (x->exponent <= 0) {
        mp_set_from_integer(1, &t1);
        mp_set_from_mp(&t1, &t2);
        mp_subtract(&t1, x, &t1);
        mp_add(&t2, x, &t2);
        mp_multiply(&t1, &t2, &t2);
        mp_root(&t2, -2, &t2);
        mp_multiply(x, &t2, z);
        mp_atan(z, unit, z);
        return;
    }

    /* sin⁻¹(1) = π/2, sin⁻¹(-1) = -π/2 */
    mp_set_from_integer(x->sign, &t2);
    if (mp_is_equal(x, &t2)) {
        mp_get_pi(z);
        mp_divide_integer(z, 2 * t2.sign, z);
        convert_from_radians(z, unit, z);
        return;
    }

    /* Translators: Error displayed when inverse sine value is undefined */
    mperr(_("Inverse sine is undefined for values outside [-1, 1]"));
    mp_set_from_integer(0, z);
}


void
mp_acos(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber t1, t2;
    MPNumber MPn1, pi, MPy;

    mp_get_pi(&pi);
    mp_set_from_integer(1, &t1);
    mp_set_from_integer(-1, &MPn1);

    if (mp_is_greater_than(x, &t1) || mp_is_less_than(x, &MPn1)) {
        /* Translators: Error displayed when inverse cosine value is undefined */
        mperr(_("Inverse cosine is undefined for values outside [-1, 1]"));
        mp_set_from_integer(0, z);
    } else if (mp_is_zero(x)) {
        mp_divide_integer(&pi, 2, z);
    } else if (mp_is_equal(x, &t1)) {
        mp_set_from_integer(0, z);
    } else if (mp_is_equal(x, &MPn1)) {
        mp_set_from_mp(&pi, z);
    } else {
        /* cos⁻¹(x) = tan⁻¹(√(1 - x²) / x) */
        mp_multiply(x, x, &t2);
        mp_subtract(&t1, &t2, &t2);
        mp_sqrt(&t2, &t2);
        mp_divide(&t2, x, &t2);
        mp_atan(&t2, MP_RADIANS, &MPy);
        if (x->sign > 0) {
            mp_set_from_mp(&MPy, z);
        } else {
            mp_add(&MPy, &pi, z);
        }
    }

    convert_from_radians(z, unit, z);
}


void
mp_atan(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    int i, q;
    float rx = 0.0;
    MPNumber t1, t2;

    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    mp_set_from_mp(x, &t2);
    if (abs(x->exponent) <= 2)
        rx = mp_cast_to_float(x);

    /* REDUCE ARGUMENT IF NECESSARY BEFORE USING SERIES */
    q = 1;
    while (t2.exponent >= 0)
    {
        if (t2.exponent == 0 && 2 * (t2.fraction[0] + 1) <= MP_BASE)
            break;

        q *= 2;
        
        /* t = t / (√(t² + 1) + 1) */
        mp_multiply(&t2, &t2, z);
        mp_add_integer(z, 1, z);
        mp_sqrt(z, z);
        mp_add_integer(z, 1, z);
        mp_divide(&t2, z, &t2);
    }

    /* USE POWER SERIES NOW ARGUMENT IN (-0.5, 0.5) */
    mp_set_from_mp(&t2, z);
    mp_multiply(&t2, &t2, &t1);

    /* SERIES LOOP.  REDUCE T IF POSSIBLE. */
    for (i = 1; ; i += 2) {
        if (MP_T + 2 + t2.exponent <= 1)
            break;

        mp_multiply(&t2, &t1, &t2);
        mp_multiply_fraction(&t2, -i, i + 2, &t2);

        mp_add(z, &t2, z);
        if (mp_is_zero(&t2))
            break;
    }

    /* CORRECT FOR ARGUMENT REDUCTION */
    mp_multiply_integer(z, q, z);

    /*  CHECK THAT RELATIVE ERROR LESS THAN 0.01 UNLESS EXPONENT
     *  OF X IS LARGE (WHEN ATAN MIGHT NOT WORK)
     */
    if (abs(x->exponent) <= 2) {
        float ry = mp_cast_to_float(z);
        /* THE FOLLOWING MESSAGE MAY INDICATE THAT B**(T-1) IS TOO SMALL. */
        if (fabs(ry - atan(rx)) >= fabs(ry) * 0.01)
            mperr("*** ERROR OCCURRED IN MP_ATAN, RESULT INCORRECT ***");
    }

    convert_from_radians(z, unit, z);
}


void
mp_sinh(const MPNumber *x, MPNumber *z)
{
    MPNumber abs_x;

    /* sinh(0) = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* WORK WITH ABS(X) */
    mp_abs(x, &abs_x);

    /* If |x| < 1 USE MPEXP TO AVOID CANCELLATION, otherwise IF TOO LARGE MP_EPOWY GIVES ERROR MESSAGE */
    if (abs_x.exponent <= 0) {
        MPNumber exp_x, a, b;

        /* ((e^|x| + 1) * (e^|x| - 1)) / e^|x| */
        // FIXME: Solves to e^|x| - e^-|x|, why not lower branch always? */
        mp_epowy(&abs_x, &exp_x);
        mp_add_integer(&exp_x, 1, &a);
        mp_add_integer(&exp_x, -1, &b);
        mp_multiply(&a, &b, z);
        mp_divide(z, &exp_x, z);
    }
    else {
        MPNumber exp_x;

        /* e^|x| - e^-|x| */
        mp_epowy(&abs_x, &exp_x);
        mp_reciprocal(&exp_x, z);
        mp_subtract(&exp_x, z, z);
    }

    /* DIVIDE BY TWO AND RESTORE SIGN */
    mp_divide_integer(z, 2, z);
    mp_multiply_integer(z, x->sign, z);
}


void
mp_cosh(const MPNumber *x, MPNumber *z)
{
    MPNumber t;

    /* cosh(0) = 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }

    /* cosh(x) = (e^x + e^-x) / 2 */
    mp_abs(x, &t);
    mp_epowy(&t, &t);
    mp_reciprocal(&t, z);
    mp_add(&t, z, z);
    mp_divide_integer(z, 2, z);
}


void
mp_tanh(const MPNumber *x, MPNumber *z)
{
    float r__1;
    MPNumber t;

    /* tanh(0) = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    mp_abs(x, &t);

    /* SEE IF ABS(X) SO LARGE THAT RESULT IS +-1 */
    r__1 = (float) MP_T * 0.5 * log((float) MP_BASE);
    mp_set_from_float(r__1, z);
    if (mp_compare_mp_to_mp(&t, z) > 0) {
        mp_set_from_integer(x->sign, z);
        return;
    }

    /* If |x| >= 1/2 use ?, otherwise use ? to avoid cancellation */
    /* |tanh(x)| = (e^|2x| - 1) / (e^|2x| + 1) */
    mp_multiply_integer(&t, 2, &t);
    if (t.exponent > 0) {
        mp_epowy(&t, &t);
        mp_add_integer(&t, -1, z);
        mp_add_integer(&t, 1, &t);
        mp_divide(z, &t, z);
    } else {
        mp_epowy(&t, &t);
        mp_add_integer(&t, 1, z);
        mp_divide(&t, z, z);
    }

    /* Restore sign */
    z->sign = x->sign * z->sign;
}


void
mp_asinh(const MPNumber *x, MPNumber *z)
{
    MPNumber t;

    /* sinh⁻¹(x) = ln(x + √(x² + 1)) */
    mp_multiply(x, x, &t);
    mp_add_integer(&t, 1, &t);
    mp_sqrt(&t, &t);
    mp_add(x, &t, &t);
    mp_ln(&t, z);
}


void
mp_acosh(const MPNumber *x, MPNumber *z)
{
    MPNumber t;

    /* Check x >= 1 */
    mp_set_from_integer(1, &t);
    if (mp_is_less_than(x, &t)) {
        /* Translators: Error displayed when inverse hyperbolic cosine value is undefined */
        mperr(_("Inverse hyperbolic cosine is undefined for values less than or equal to one"));
        mp_set_from_integer(0, z);
        return;
    }

    /* cosh⁻¹(x) = ln(x + √(x² - 1)) */
    mp_multiply(x, x, &t);
    mp_add_integer(&t, -1, &t);
    mp_sqrt(&t, &t);
    mp_add(x, &t, &t);
    mp_ln(&t, z);
}


void
mp_atanh(const MPNumber *x, MPNumber *z)
{
    MPNumber one, minus_one, n, d;

    /* Check -1 <= x <= 1 */
    mp_set_from_integer(1, &one);
    mp_set_from_integer(-1, &minus_one);
    if (mp_is_greater_equal(x, &one) || mp_is_less_equal(x, &minus_one)) {
        /* Translators: Error displayed when inverse hyperbolic tangent value is undefined */
        mperr(_("Inverse hyperbolic tangent is undefined for values outside [-1, 1]"));
        mp_set_from_integer(0, z);
        return;
    }

    /* atanh(x) = 0.5 * ln((1 + x) / (1 - x)) */
    mp_add_integer(x, 1, &n);
    mp_set_from_mp(x, &d);
    mp_invert_sign(&d, &d);
    mp_add_integer(&d, 1, &d);
    mp_divide(&n, &d, z);
    mp_ln(z, z);
    mp_divide_integer(z, 2, z);
}
