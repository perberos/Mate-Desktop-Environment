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
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "mp.h"
#include "mp-private.h"

// FIXME: Re-add overflow and underflow detection

char *mp_error = NULL;

/*  THIS ROUTINE IS CALLED WHEN AN ERROR CONDITION IS ENCOUNTERED, AND
 *  AFTER A MESSAGE HAS BEEN WRITTEN TO STDERR.
 */
void
mperr(const char *format, ...)
{
    char text[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(text, 1024, format, args);
    va_end(args);

    if (mp_error)
        free(mp_error);
    mp_error = strdup(text);
}


const char *
mp_get_error()
{
    return mp_error;
}


void mp_clear_error()
{
    if (mp_error)
        free(mp_error);
    mp_error = NULL;
}


/*  ROUTINE CALLED BY MP_DIVIDE AND MP_SQRT TO ENSURE THAT
 *  RESULTS ARE REPRESENTED EXACTLY IN T-2 DIGITS IF THEY
 *  CAN BE.  X IS AN MP NUMBER, I AND J ARE INTEGERS.
 */
static void
mp_ext(int i, int j, MPNumber *x)
{
    int q, s;

    if (mp_is_zero(x) || MP_T <= 2 || i == 0)
        return;

    /* COMPUTE MAXIMUM POSSIBLE ERROR IN THE LAST PLACE */
    q = (j + 1) / i + 1;
    s = MP_BASE * x->fraction[MP_T - 2] + x->fraction[MP_T - 1];

    /* SET LAST TWO DIGITS TO ZERO */
    if (s <= q) {
        x->fraction[MP_T - 2] = 0;
        x->fraction[MP_T - 1] = 0;
        return;
    }

    if (s + q < MP_BASE * MP_BASE)
        return;

    /* ROUND UP HERE */
    x->fraction[MP_T - 2] = MP_BASE - 1;
    x->fraction[MP_T - 1] = MP_BASE;

    /* NORMALIZE X (LAST DIGIT B IS OK IN MP_MULTIPLY_INTEGER) */
    mp_multiply_integer(x, 1, x);
}


void
mp_get_eulers(MPNumber *z)
{
    MPNumber t;
    mp_set_from_integer(1, &t);
    mp_epowy(&t, z);
}


void
mp_get_i(MPNumber *z)
{
    mp_set_from_integer(0, z);
    z->im_sign = 1;
    z->im_exponent = 1;
    z->im_fraction[0] = 1;
}


void
mp_abs(const MPNumber *x, MPNumber *z)
{
    if (mp_is_complex(x)){
        MPNumber x_real, x_im;

        mp_real_component(x, &x_real);
        mp_imaginary_component(x, &x_im);

        mp_multiply(&x_real, &x_real, &x_real);
        mp_multiply(&x_im, &x_im, &x_im);
        mp_add(&x_real, &x_im, z);
        mp_root(z, 2, z);
    }
    else {
        mp_set_from_mp(x, z);
        if (z->sign < 0)
            z->sign = -z->sign;
    }
}


void
mp_arg(const MPNumber *x, MPAngleUnit unit, MPNumber *z)
{
    MPNumber x_real, x_im, pi;

    if (mp_is_zero(x)) {
        /* Translators: Error display when attempting to take argument of zero */
        mperr(_("Argument not defined for zero"));
        mp_set_from_integer(0, z);
        return;
    }

    mp_real_component(x, &x_real);
    mp_imaginary_component(x, &x_im);
    mp_get_pi(&pi);

    if (mp_is_zero(&x_im)) {
        if (mp_is_negative(&x_real))
            convert_from_radians(&pi, MP_RADIANS, z);
        else
            mp_set_from_integer(0, z);
    }
    else if (mp_is_zero(&x_real)) {
        mp_set_from_mp(&pi, z);
        if (mp_is_negative(&x_im))
            mp_divide_integer(z, -2, z);
        else
            mp_divide_integer(z, 2, z);
    }
    else if (mp_is_negative(&x_real)) {
        mp_divide(&x_im, &x_real, z);
        mp_atan(z, MP_RADIANS, z);
        if (mp_is_negative(&x_im))
            mp_subtract(z, &pi, z);
        else
            mp_add(z, &pi, z);
    }
    else {
        mp_divide(&x_im, &x_real, z);
        mp_atan(z, MP_RADIANS, z);
    }

    convert_from_radians(z, unit, z);
}


void
mp_conjugate(const MPNumber *x, MPNumber *z)
{
    mp_set_from_mp(x, z);
    z->im_sign = -z->im_sign;
}


void
mp_real_component(const MPNumber *x, MPNumber *z)
{
    mp_set_from_mp(x, z);

    /* Clear imaginary component */
    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);
}


void
mp_imaginary_component(const MPNumber *x, MPNumber *z)
{
    /* Copy imaginary component to real component */
    z->sign = x->im_sign;
    z->exponent = x->im_exponent;
    memcpy(z->fraction, x->im_fraction, sizeof(int) * MP_SIZE);
  
    /* Clear (old) imaginary component */   
    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);
}


static void
mp_add_real(const MPNumber *x, int y_sign, const MPNumber *y, MPNumber *z)
{
    int sign_prod, i, c;
    int exp_diff, med;
    bool x_largest = false;
    const int *big_fraction, *small_fraction;
    MPNumber x_copy, y_copy;

    /* 0 + y = y */
    if (mp_is_zero(x)) {
        mp_set_from_mp(y, z);
        z->sign = y_sign;
        return;
    }
    /* x + 0 = x */ 
    else if (mp_is_zero(y)) {
        mp_set_from_mp(x, z);
        return;
    }

    sign_prod = y_sign * x->sign;
    exp_diff = x->exponent - y->exponent;
    med = abs(exp_diff);
    if (exp_diff < 0) {
        x_largest = false;
    } else if (exp_diff > 0) {
        x_largest = true;
    } else {
        /* EXPONENTS EQUAL SO COMPARE SIGNS, THEN FRACTIONS IF NEC. */
        if (sign_prod < 0) {
            /* Signs are not equal.  find out which mantissa is larger. */
            int j;
            for (j = 0; j < MP_T; j++) {
                int i = x->fraction[j] - y->fraction[j];
                if (i == 0)
                    continue;
                if (i < 0)
                    x_largest = false;
                else if (i > 0)
                    x_largest = true;
                break;
            }

            /* Both mantissas equal, so result is zero. */
            if (j >= MP_T) {
                mp_set_from_integer(0, z);
                return;
            }
        }
    }

    mp_set_from_mp(x, &x_copy);
    mp_set_from_mp(y, &y_copy);
    mp_set_from_integer(0, z);

    if (x_largest) {
        z->sign = x_copy.sign;
        z->exponent = x_copy.exponent;
        big_fraction = x_copy.fraction;
        small_fraction = y_copy.fraction;
    } else {
        z->sign = y_sign;
        z->exponent = y_copy.exponent;
        big_fraction = y_copy.fraction;
        small_fraction = x_copy.fraction;
    }

    /* CLEAR GUARD DIGITS TO RIGHT OF X DIGITS */
    for(i = 3; i >= med; i--)
        z->fraction[MP_T + i] = 0;

    if (sign_prod >= 0) {
        /* HERE DO ADDITION, EXPONENT(Y) >= EXPONENT(X) */
        for (i = MP_T + 3; i >= MP_T; i--)
            z->fraction[i] = small_fraction[i - med];

        c = 0;
        for (; i >= med; i--) {
            c = big_fraction[i] + small_fraction[i - med] + c;

            if (c < MP_BASE) {
                /* NO CARRY GENERATED HERE */
                z->fraction[i] = c;
                c = 0;
            } else {
                /* CARRY GENERATED HERE */
                z->fraction[i] = c - MP_BASE;
                c = 1;
            }
        }

        for (; i >= 0; i--)
        {
            c = big_fraction[i] + c;
            if (c < MP_BASE) {
                z->fraction[i] = c;
                i--;

                /* NO CARRY POSSIBLE HERE */
                for (; i >= 0; i--)
                    z->fraction[i] = big_fraction[i];

                c = 0;
                break;
            }

            z->fraction[i] = 0;
            c = 1;
        }

        /* MUST SHIFT RIGHT HERE AS CARRY OFF END */
        if (c != 0) {
            for (i = MP_T + 3; i > 0; i--)
                z->fraction[i] = z->fraction[i - 1];
            z->fraction[0] = 1;
            z->exponent++;
        }
    }
    else  {
        c = 0;
        for (i = MP_T + med - 1; i >= MP_T; i--) {
            /* HERE DO SUBTRACTION, ABS(Y) > ABS(X) */
            z->fraction[i] = c - small_fraction[i - med];
            c = 0;

            /* BORROW GENERATED HERE */
            if (z->fraction[i] < 0) {
                c = -1;
                z->fraction[i] += MP_BASE;
            }
        }

        for(; i >= med; i--) {
            c = big_fraction[i] + c - small_fraction[i - med];
            if (c >= 0) {
                /* NO BORROW GENERATED HERE */
                z->fraction[i] = c;
                c = 0;
            } else {
                /* BORROW GENERATED HERE */
                z->fraction[i] = c + MP_BASE;
                c = -1;
            }
        }

        for (; i >= 0; i--) {
            c = big_fraction[i] + c;

            if (c >= 0) {
                z->fraction[i] = c;
                i--;

                /* NO CARRY POSSIBLE HERE */
                for (; i >= 0; i--)
                    z->fraction[i] = big_fraction[i];

                break;
            }

            z->fraction[i] = c + MP_BASE;
            c = -1;
        }
    }

    mp_normalize(z);
}


static void
mp_add_with_sign(const MPNumber *x, int y_sign, const MPNumber *y, MPNumber *z)
{
    if (mp_is_complex(x) || mp_is_complex(y)) {
        MPNumber real_x, real_y, im_x, im_y, real_z, im_z;

        mp_real_component(x, &real_x);
        mp_imaginary_component(x, &im_x);
        mp_real_component(y, &real_y);
        mp_imaginary_component(y, &im_y);

        mp_add_real(&real_x, y_sign * y->sign, &real_y, &real_z);
        mp_add_real(&im_x, y_sign * y->im_sign, &im_y, &im_z);

        mp_set_from_complex(&real_z, &im_z, z);
    }
    else
        mp_add_real(x, y_sign * y->sign, y, z);
}


void
mp_add(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    mp_add_with_sign(x, 1, y, z);
}


void
mp_add_integer(const MPNumber *x, int64_t y, MPNumber *z)
{
    MPNumber t;
    mp_set_from_integer(y, &t);
    mp_add(x, &t, z);
}


void
mp_add_fraction(const MPNumber *x, int64_t i, int64_t j, MPNumber *y)
{
    MPNumber t;
    mp_set_from_fraction(i, j, &t);
    mp_add(x, &t, y);
}


void
mp_subtract(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    mp_add_with_sign(x, -1, y, z);
}


void
mp_sgn(const MPNumber *x, MPNumber *z)
{
    if (mp_is_zero(x))
        mp_set_from_integer(0, z);
    else if (mp_is_negative(x))
        mp_set_from_integer(-1, z);
    else
        mp_set_from_integer(1, z);  
}

void
mp_integer_component(const MPNumber *x, MPNumber *z)
{
    int i;

    /* Clear fraction */
    mp_set_from_mp(x, z);
    for (i = z->exponent; i < MP_SIZE; i++)
        z->fraction[i] = 0;
    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);
}

void
mp_fractional_component(const MPNumber *x, MPNumber *z)
{
    int i, shift;

    /* Fractional component of zero is 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* All fractional */
    if (x->exponent <= 0) {
        mp_set_from_mp(x, z);
        return;
    }

    /* Shift fractional component */
    shift = x->exponent;
    for (i = shift; i < MP_SIZE && x->fraction[i] == 0; i++)
        shift++;
    z->sign = x->sign;
    z->exponent = x->exponent - shift;
    for (i = 0; i < MP_SIZE; i++) {
        if (i + shift >= MP_SIZE)
            z->fraction[i] = 0;
        else
            z->fraction[i] = x->fraction[i + shift];
    }
    if (z->fraction[0] == 0)
        z->sign = 0;

    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);
}


void
mp_fractional_part(const MPNumber *x, MPNumber *z)
{
    MPNumber f;
    mp_floor(x, &f);
    mp_subtract(x, &f, z);
}


void
mp_floor(const MPNumber *x, MPNumber *z)
{
    int i;
    bool have_fraction = false, is_negative;

    /* Integer component of zero = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_mp(x, z);
        return;
    }

    /* If all fractional then no integer component */
    if (x->exponent <= 0) {
        mp_set_from_integer(0, z);
        return;
    }

    is_negative = mp_is_negative(x);

    /* Clear fraction */
    mp_set_from_mp(x, z);
    for (i = z->exponent; i < MP_SIZE; i++) {
        if (z->fraction[i])
            have_fraction = true;
        z->fraction[i] = 0;
    }
    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);

    if (have_fraction && is_negative)
       mp_add_integer(z, -1, z);
}


void
mp_ceiling(const MPNumber *x, MPNumber *z)
{
    MPNumber f;

    mp_floor(x, z);
    mp_fractional_component(x, &f);
    if (mp_is_zero(&f))
        return;
    mp_add_integer(z, 1, z);
}


void
mp_round(const MPNumber *x, MPNumber *z)
{
    MPNumber f, one;
    bool do_floor;

    do_floor = !mp_is_negative(x);

    mp_fractional_component(x, &f);
    mp_multiply_integer(&f, 2, &f);
    mp_abs(&f, &f);
    mp_set_from_integer(1, &one);
    if (mp_is_greater_equal(&f, &one))
        do_floor = !do_floor;

    if (do_floor)
        mp_floor(x, z);
    else
        mp_ceiling(x, z);
}

int
mp_compare_mp_to_mp(const MPNumber *x, const MPNumber *y)
{
    int i;

    if (x->sign != y->sign) {
        if (x->sign > y->sign)
            return 1;
        else
            return -1;
    }

    /* x = y = 0 */
    if (mp_is_zero(x))
        return 0;

    /* See if numbers are of different magnitude */
    if (x->exponent != y->exponent) {
        if (x->exponent > y->exponent)
            return x->sign;
        else
            return -x->sign;
    }

    /* Compare fractions */
    for (i = 0; i < MP_SIZE; i++) {
        if (x->fraction[i] == y->fraction[i])
            continue;

        if (x->fraction[i] > y->fraction[i])
            return x->sign;
        else
            return -x->sign;
    }

    /* x = y */
    return 0;
}


void
mp_divide(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    int i, ie;
    MPNumber t;

    /* x/0 */
    if (mp_is_zero(y)) {
        /* Translators: Error displayed attempted to divide by zero */
        mperr(_("Division by zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    /* 0/y = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* z = x × y⁻¹ */
    /* FIXME: Set exponent to zero to avoid overflow in mp_multiply??? */
    mp_reciprocal(y, &t);
    ie = t.exponent;
    t.exponent = 0;
    i = t.fraction[0];
    mp_multiply(x, &t, z);
    mp_ext(i, z->fraction[0], z);
    z->exponent += ie;
}


static void
mp_divide_integer_real(const MPNumber *x, int64_t y, MPNumber *z)
{
    int c, i, k, b2, c2, j1, j2;
    MPNumber x_copy;

    /* x/0 */
    if (y == 0) {
        /* Translators: Error displayed attempted to divide by zero */
        mperr(_("Division by zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    /* 0/y = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* Division by -1 or 1 just changes sign */
    if (y == 1 || y == -1) {
        if (y < 0)
            mp_invert_sign(x, z);
        else
            mp_set_from_mp(x, z);
        return;
    }

    /* Copy x as z may also refer to x */
    mp_set_from_mp(x, &x_copy);
    mp_set_from_integer(0, z);

    if (y < 0) {
        y = -y;
        z->sign = -x_copy.sign;
    }
    else
        z->sign = x_copy.sign;
    z->exponent = x_copy.exponent;

    c = 0;
    i = 0;

    /*  IF y*B NOT REPRESENTABLE AS AN INTEGER HAVE TO SIMULATE
     *  LONG DIVISION.  ASSUME AT LEAST 16-BIT WORD.
     */

    /* Computing MAX */
    b2 = max(MP_BASE << 3, 32767 / MP_BASE);
    if (y < b2) {
        int kh, r1;

        /* LOOK FOR FIRST NONZERO DIGIT IN QUOTIENT */
        do {
            c = MP_BASE * c;
            if (i < MP_T)
                c += x_copy.fraction[i];
            i++;
            r1 = c / y;
            if (r1 < 0)
                goto L210;
        } while (r1 == 0);

        /* ADJUST EXPONENT AND GET T+4 DIGITS IN QUOTIENT */
        z->exponent += 1 - i;
        z->fraction[0] = r1;
        c = MP_BASE * (c - y * r1);
        kh = 1;
        if (i < MP_T) {
            kh = MP_T + 1 - i;
            for (k = 1; k < kh; k++) {
                c += x_copy.fraction[i];
                z->fraction[k] = c / y;
                c = MP_BASE * (c - y * z->fraction[k]);
                i++;
            }
            if (c < 0)
                goto L210;
        }

        for (k = kh; k < MP_T + 4; k++) {
            z->fraction[k] = c / y;
            c = MP_BASE * (c - y * z->fraction[k]);
        }
        if (c < 0)
            goto L210;

        mp_normalize(z);
        return;
    }

    /* HERE NEED SIMULATED DOUBLE-PRECISION DIVISION */
    j1 = y / MP_BASE;
    j2 = y - j1 * MP_BASE;

    /* LOOK FOR FIRST NONZERO DIGIT */
    c2 = 0;
    do {
        c = MP_BASE * c + c2;
        c2 = i < MP_T ? x_copy.fraction[i] : 0;
        i++;
    } while (c < j1 || (c == j1 && c2 < j2));

    /* COMPUTE T+4 QUOTIENT DIGITS */
    z->exponent += 1 - i;
    i--;

    /* MAIN LOOP FOR LARGE ABS(y) CASE */
    for (k = 1; k <= MP_T + 4; k++) {
        int ir, iq, iqj;

        /* GET APPROXIMATE QUOTIENT FIRST */
        ir = c / (j1 + 1);

        /* NOW REDUCE SO OVERFLOW DOES NOT OCCUR */
        iq = c - ir * j1;
        if (iq >= b2) {
            /* HERE IQ*B WOULD POSSIBLY OVERFLOW SO INCREASE IR */
            ++ir;
            iq -= j1;
        }

        iq = iq * MP_BASE - ir * j2;
        if (iq < 0) {
            /* HERE IQ NEGATIVE SO IR WAS TOO LARGE */
            ir--;
            iq += y;
        }

        if (i < MP_T)
            iq += x_copy.fraction[i];
        i++;
        iqj = iq / y;

        /* R(K) = QUOTIENT, C = REMAINDER */
        z->fraction[k - 1] = iqj + ir;
        c = iq - y * iqj;

        if (c < 0)
            goto L210;
    }

    mp_normalize(z);

L210:
    /* CARRY NEGATIVE SO OVERFLOW MUST HAVE OCCURRED */
    mperr("*** INTEGER OVERFLOW IN MP_DIVIDE_INTEGER, B TOO LARGE ***");
    mp_set_from_integer(0, z);
}


void
mp_divide_integer(const MPNumber *x, int64_t y, MPNumber *z)
{
    if (mp_is_complex(x)) {
        MPNumber re_z, im_z;

        mp_real_component(x, &re_z);
        mp_imaginary_component(x, &im_z);
        mp_divide_integer_real(&re_z, y, &re_z);
        mp_divide_integer_real(&im_z, y, &im_z);
        mp_set_from_complex(&re_z, &im_z, z);
    }
    else
        mp_divide_integer_real(x, y, z);
}


bool
mp_is_integer(const MPNumber *x)
{
    MPNumber t1, t2, t3;
    
    if (mp_is_complex(x))
        return false;

    /* This fix is required for 1/3 repiprocal not being detected as an integer */
    /* Multiplication and division by 10000 is used to get around a
     * limitation to the "fix" for Sun bugtraq bug #4006391 in the
     * mp_floor() routine in mp.c, when the exponent is less than 1.
     */
    mp_set_from_integer(10000, &t3);
    mp_multiply(x, &t3, &t1);
    mp_divide(&t1, &t3, &t1);
    mp_floor(&t1, &t2);
    return mp_is_equal(&t1, &t2);

    /* Correct way to check for integer */
    /*int i;

    // Zero is an integer
    if (mp_is_zero(x))
        return true;

    // Fractional
    if (x->exponent <= 0)
        return false;

    // Look for fractional components
    for (i = x->exponent; i < MP_SIZE; i++) {
        if (x->fraction[i] != 0)
            return false;
    }

    return true;*/
}


bool
mp_is_positive_integer(const MPNumber *x)
{
    if (mp_is_complex(x))
        return false;
    else
        return x->sign >= 0 && mp_is_integer(x);
}


bool
mp_is_natural(const MPNumber *x)
{
    if (mp_is_complex(x))
        return false;
    else
        return x->sign > 0 && mp_is_integer(x);
}


bool
mp_is_complex(const MPNumber *x)
{
    return x->im_sign != 0;
}


bool
mp_is_equal(const MPNumber *x, const MPNumber *y)
{
    return mp_compare_mp_to_mp(x, y) == 0;
}


/*  Return e^x for |x| < 1 USING AN O(SQRT(T).M(T)) ALGORITHM
 *  DESCRIBED IN - R. P. BRENT, THE COMPLEXITY OF MULTIPLE-
 *  PRECISION ARITHMETIC (IN COMPLEXITY OF COMPUTATIONAL PROBLEM
 *  SOLVING, UNIV. OF QUEENSLAND PRESS, BRISBANE, 1976, 126-165).
 *  ASYMPTOTICALLY FASTER METHODS EXIST, BUT ARE NOT USEFUL
 *  UNLESS T IS VERY LARGE. SEE COMMENTS TO MP_ATAN AND MPPIGL.
 */
static void
mp_exp(const MPNumber *x, MPNumber *z)
{
    int i, q;
    float rlb;
    MPNumber t1, t2;

    /* e^0 = 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }

    /* Only defined for |x| < 1 */
    if (x->exponent > 0) {
        mperr("*** ABS(X) NOT LESS THAN 1 IN CALL TO MP_EXP ***");
        mp_set_from_integer(0, z);
        return;
    }

    mp_set_from_mp(x, &t1);
    rlb = log((float)MP_BASE);

    /* Compute approximately optimal q (and divide x by 2^q) */
    q = (int)(sqrt((float)MP_T * 0.48f * rlb) + (float) x->exponent * 1.44f * rlb);

    /* HALVE Q TIMES */
    if (q > 0) {
        int ib, ic;

        ib = MP_BASE << 2;
        ic = 1;
        for (i = 1; i <= q; ++i) {
            ic *= 2;
            if (ic < ib && ic != MP_BASE && i < q)
                continue;
            mp_divide_integer(&t1, ic, &t1);
            ic = 1;
        }
    }

    if (mp_is_zero(&t1)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* Sum series, reducing t where possible */
    mp_set_from_mp(&t1, z);
    mp_set_from_mp(&t1, &t2);
    for (i = 2; MP_T + t2.exponent - z->exponent > 0; i++) {
        mp_multiply(&t1, &t2, &t2);
        mp_divide_integer(&t2, i, &t2);
        mp_add(&t2, z, z);
        if (mp_is_zero(&t2))
            break;
    }

    /* Apply (x+1)^2 - 1 = x(2 + x) for q iterations */
    for (i = 1; i <= q; ++i) {
        mp_add_integer(z, 2, &t1);
        mp_multiply(&t1, z, z);
    }

    mp_add_integer(z, 1, z);
}


static void
mp_epowy_real(const MPNumber *x, MPNumber *z)
{
    float r__1;
    int i, ix, xs, tss;
    float rx, rz, rlb;
    MPNumber t1, t2;

    /* e^0 = 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }

    /* If |x| < 1 use mp_exp */
    if (x->exponent <= 0) {
        mp_exp(x, z);
        return;
    }

    /*  SEE IF ABS(X) SO LARGE THAT EXP(X) WILL CERTAINLY OVERFLOW
     *  OR UNDERFLOW.  1.01 IS TO ALLOW FOR ERRORS IN ALOG.
     */
    rlb = log((float)MP_BASE) * 1.01f;

    /* NOW SAFE TO CONVERT X TO REAL */
    rx = mp_cast_to_float(x);

    /* SAVE SIGN AND WORK WITH ABS(X) */
    xs = x->sign;
    mp_abs(x, &t2);

    /* GET FRACTIONAL AND INTEGER PARTS OF ABS(X) */
    ix = mp_cast_to_int(&t2);
    mp_fractional_component(&t2, &t2);

    /* ATTACH SIGN TO FRACTIONAL PART AND COMPUTE EXP OF IT */
    t2.sign *= xs;
    mp_exp(&t2, z);

    /*  COMPUTE E-2 OR 1/E USING TWO EXTRA DIGITS IN CASE ABS(X) LARGE
     *  (BUT ONLY ONE EXTRA DIGIT IF T < 4)
     */
    if (MP_T < 4)
        tss = MP_T + 1;
    else
        tss = MP_T + 2;

    /* LOOP FOR E COMPUTATION. DECREASE T IF POSSIBLE. */
    /* Computing MIN */
    mp_set_from_integer(xs, &t1);

    t2.sign = 0;
    for (i = 2 ; ; i++) {
        if (min(tss, tss + 2 + t1.exponent) <= 2)
            break;

        mp_divide_integer(&t1, i * xs, &t1);
        mp_add(&t2, &t1, &t2);
        if (mp_is_zero(&t1))
            break;
    }

    /* RAISE E OR 1/E TO POWER IX */
    if (xs > 0)
        mp_add_integer(&t2, 2, &t2);
    mp_xpowy_integer(&t2, ix, &t2);

    /* MULTIPLY EXPS OF INTEGER AND FRACTIONAL PARTS */
    mp_multiply(z, &t2, z);

    /*  CHECK THAT RELATIVE ERROR LESS THAN 0.01 UNLESS ABS(X) LARGE
     *  (WHEN EXP MIGHT OVERFLOW OR UNDERFLOW)
     */
    if (fabs(rx) > 10.0f)
        return;

    rz = mp_cast_to_float(z);
    r__1 = rz - exp(rx);
    if (fabs(r__1) < rz * 0.01f)
        return;

    /*  THE FOLLOWING MESSAGE MAY INDICATE THAT
     *  B**(T-1) IS TOO SMALL, OR THAT M IS TOO SMALL SO THE
     *  RESULT UNDERFLOWED.
     */
    mperr("*** ERROR OCCURRED IN MP_EPOWY, RESULT INCORRECT ***");
}


void
mp_epowy(const MPNumber *x, MPNumber *z)
{
    /* e^0 = 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }

    if (mp_is_complex(x)) {
        MPNumber x_real, r, theta;

        mp_real_component(x, &x_real);
        mp_imaginary_component(x, &theta);

        mp_epowy_real(&x_real, &r);
        mp_set_from_polar(&r, MP_RADIANS, &theta, z);
    }
    else
        mp_epowy_real(x, z);
}


/*  RETURNS K = K/GCD AND L = L/GCD, WHERE GCD IS THE
 *  GREATEST COMMON DIVISOR OF K AND L.
 *  SAVE INPUT PARAMETERS IN LOCAL VARIABLES
 */
void
mp_gcd(int64_t *k, int64_t *l)
{
    int64_t i, j;

    i = abs(*k);
    j = abs(*l);
    if (j == 0) {
        /* IF J = 0 RETURN (1, 0) UNLESS I = 0, THEN (0, 0) */
        *k = 1;
        *l = 0;
        if (i == 0)
            *k = 0;
        return;
    }

    /* EUCLIDEAN ALGORITHM LOOP */
    do {
        i %= j;
        if (i == 0) {
            *k = *k / j;
            *l = *l / j;
            return;
        }
        j %= i;
    } while (j != 0);

    /* HERE J IS THE GCD OF K AND L */
    *k = *k / i;
    *l = *l / i;
}


bool
mp_is_zero(const MPNumber *x)
{
    return x->sign == 0 && x->im_sign == 0;
}


bool
mp_is_negative(const MPNumber *x)
{
    return x->sign < 0;
}


bool
mp_is_greater_equal(const MPNumber *x, const MPNumber *y)
{
    return mp_compare_mp_to_mp(x, y) >= 0;
}


bool
mp_is_greater_than(const MPNumber *x, const MPNumber *y)
{
    return mp_compare_mp_to_mp(x, y) > 0;
}


bool
mp_is_less_equal(const MPNumber *x, const MPNumber *y)
{
    return mp_compare_mp_to_mp(x, y) <= 0;
}


/*  RETURNS MP Y = LN(1+X) IF X IS AN MP NUMBER SATISFYING THE
 *  CONDITION ABS(X) < 1/B, ERROR OTHERWISE.
 *  USES NEWTONS METHOD TO SOLVE THE EQUATION
 *  EXP1(-Y) = X, THEN REVERSES SIGN OF Y.
 */
static void
mp_lns(const MPNumber *x, MPNumber *z)
{
    int t, it0;
    MPNumber t1, t2, t3;

    /* ln(1+0) = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* Get starting approximation -ln(1+x) ~= -x + x^2/2 - x^3/3 + x^4/4 */
    mp_set_from_mp(x, &t2);
    mp_divide_integer(x, 4, &t1);
    mp_add_fraction(&t1, -1, 3, &t1);
    mp_multiply(x, &t1, &t1);
    mp_add_fraction(&t1, 1, 2, &t1);
    mp_multiply(x, &t1, &t1);
    mp_add_integer(&t1, -1, &t1);
    mp_multiply(x, &t1, z);

    /* Solve using Newtons method */
    it0 = t = 5;
    while(1)
    {
        int ts2, ts3;

        /* t3 = (e^t3 - 1) */
        /* z = z - (t2 + t3 + (t2 * t3)) */
        mp_epowy(z, &t3);
        mp_add_integer(&t3, -1, &t3);
        mp_multiply(&t2, &t3, &t1);
        mp_add(&t3, &t1, &t3);
        mp_add(&t2, &t3, &t3);
        mp_subtract(z, &t3, z);
        if (t >= MP_T)
            break;

        /*  FOLLOWING LOOP COMPUTES NEXT VALUE OF T TO USE.
         *  BECAUSE NEWTONS METHOD HAS 2ND ORDER CONVERGENCE,
         *  WE CAN ALMOST DOUBLE T EACH TIME.
         */
        ts3 = t;
        t = MP_T;
        do {
            ts2 = t;
            t = (t + it0) / 2;
        } while (t > ts3);
        t = ts2;
    }

    /* CHECK THAT NEWTON ITERATION WAS CONVERGING AS EXPECTED */
    if (t3.sign != 0 && t3.exponent << 1 > it0 - MP_T) {
        mperr("*** ERROR OCCURRED IN MP_LNS, NEWTON ITERATION NOT CONVERGING PROPERLY ***");
    }

    z->sign = -z->sign;
}


static void
mp_ln_real(const MPNumber *x, MPNumber *z)
{
    int e, k;
    float rx, rlx;
    MPNumber t1, t2;

    /* LOOP TO GET APPROXIMATE LN(X) USING SINGLE-PRECISION */
    mp_set_from_mp(x, &t1);
    mp_set_from_integer(0, z);
    for(k = 0; k < 10; k++)
    {
        /* COMPUTE FINAL CORRECTION ACCURATELY USING MP_LNS */
        mp_add_integer(&t1, -1, &t2);
        if (mp_is_zero(&t2) || t2.exponent + 1 <= 0) {
            mp_lns(&t2, &t2);
            mp_add(z, &t2, z);
            return;
        }

        /* REMOVE EXPONENT TO AVOID FLOATING-POINT OVERFLOW */
        e = t1.exponent;
        t1.exponent = 0;
        rx = mp_cast_to_float(&t1);
        t1.exponent = e;
        rlx = log(rx) + (float)e * log((float)MP_BASE);
        mp_set_from_float(-(double)rlx, &t2);

        /* UPDATE Z AND COMPUTE ACCURATE EXP OF APPROXIMATE LOG */
        mp_subtract(z, &t2, z);
        mp_epowy(&t2, &t2);

        /* COMPUTE RESIDUAL WHOSE LOG IS STILL TO BE FOUND */
        mp_multiply(&t1, &t2, &t1);
    }

    mperr("*** ERROR IN MP_LN, ITERATION NOT CONVERGING ***");
}


void
mp_ln(const MPNumber *x, MPNumber *z)
{
    /* ln(0) undefined */
    if (mp_is_zero(x)) {
        /* Translators: Error displayed when attempting to take logarithm of zero */
        mperr(_("Logarithm of zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    /* ln(-x) complex */
    /* FIXME: Make complex numbers optional */
    /*if (mp_is_negative(x)) {
        // Translators: Error displayed attempted to take logarithm of negative value
        mperr(_("Logarithm of negative values is undefined"));
        mp_set_from_integer(0, z);
        return;
    }*/

    if (mp_is_complex(x) || mp_is_negative(x)) {
        MPNumber r, theta, z_real;

        /* ln(re^iθ) = e^(ln(r)+iθ) */
        mp_abs(x, &r);
        mp_arg(x, MP_RADIANS, &theta);

        mp_ln_real(&r, &z_real);
        mp_set_from_complex(&z_real, &theta, z);
    }
    else
        mp_ln_real(x, z);
}


void
mp_logarithm(int64_t n, const MPNumber *x, MPNumber *z)
{
    MPNumber t1, t2;
    
    /* log(0) undefined */
    if (mp_is_zero(x)) {
        /* Translators: Error displayed when attempting to take logarithm of zero */
        mperr(_("Logarithm of zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    /* logn(x) = ln(x) / ln(n) */
    mp_set_from_integer(n, &t1);
    mp_ln(&t1, &t1);
    mp_ln(x, &t2);
    mp_divide(&t2, &t1, z);
}


bool
mp_is_less_than(const MPNumber *x, const MPNumber *y)
{
    return mp_compare_mp_to_mp(x, y) < 0;
}


static void
mp_multiply_real(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    int c, i, xi;
    MPNumber r;

    /* x*0 = 0*y = 0 */    
    if (x->sign == 0 || y->sign == 0) {
        mp_set_from_integer(0, z);
        return;
    }

    z->sign = x->sign * y->sign;
    z->exponent = x->exponent + y->exponent;
    memset(&r, 0, sizeof(MPNumber));

    /* PERFORM MULTIPLICATION */
    c = 8;
    for (i = 0; i < MP_T; i++) {
        int j;

        xi = x->fraction[i];

        /* FOR SPEED, PUT THE NUMBER WITH MANY ZEROS FIRST */
        if (xi == 0)
            continue;

        /* Computing MIN */
        for (j = 0; j < min(MP_T, MP_T + 3 - i); j++)
            r.fraction[i+j+1] += xi * y->fraction[j];
        c--;
        if (c > 0)
            continue;

        /* CHECK FOR LEGAL BASE B DIGIT */
        if (xi < 0 || xi >= MP_BASE) {
            mperr("*** ILLEGAL BASE B DIGIT IN CALL TO MP_MULTIPLY, POSSIBLE OVERWRITING PROBLEM ***");
            mp_set_from_integer(0, z);
            return;
        }

        /*  PROPAGATE CARRIES AT END AND EVERY EIGHTH TIME,
         *  FASTER THAN DOING IT EVERY TIME.
         */
        for (j = MP_T + 3; j >= 0; j--) {
            int ri = r.fraction[j] + c;
            if (ri < 0) {
                mperr("*** INTEGER OVERFLOW IN MP_MULTIPLY, B TOO LARGE ***");
                mp_set_from_integer(0, z);
                return;
            }
            c = ri / MP_BASE;
            r.fraction[j] = ri - MP_BASE * c;
        }
        if (c != 0) {
            mperr("*** ILLEGAL BASE B DIGIT IN CALL TO MP_MULTIPLY, POSSIBLE OVERWRITING PROBLEM ***");
            mp_set_from_integer(0, z);
            return;
        }
        c = 8;
    }

    if (c != 8) {
        if (xi < 0 || xi >= MP_BASE) {
            mperr("*** ILLEGAL BASE B DIGIT IN CALL TO MP_MULTIPLY, POSSIBLE OVERWRITING PROBLEM ***");
            mp_set_from_integer(0, z);
            return;
        }

        c = 0;
        for (i = MP_T + 3; i >= 0; i--) {
            int ri = r.fraction[i] + c;
            if (ri < 0) {
                mperr("*** INTEGER OVERFLOW IN MP_MULTIPLY, B TOO LARGE ***");
                mp_set_from_integer(0, z);
                return;
            }
            c = ri / MP_BASE;
            r.fraction[i] = ri - MP_BASE * c;
        }

        if (c != 0) {
            mperr("*** ILLEGAL BASE B DIGIT IN CALL TO MP_MULTIPLY, POSSIBLE OVERWRITING PROBLEM ***");
            mp_set_from_integer(0, z);
            return;
        }
    }

    /* Clear complex part */
    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);    

    /* NORMALIZE AND ROUND RESULT */
    // FIXME: Use stack variable because of mp_normalize brokeness
    for (i = 0; i < MP_SIZE; i++)
        z->fraction[i] = r.fraction[i];
    mp_normalize(z);
}


void
mp_multiply(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    /* x*0 = 0*y = 0 */
    if (mp_is_zero(x) || mp_is_zero(y)) {
        mp_set_from_integer(0, z);
        return;
    }

    /* (a+bi)(c+di) = (ac-bd)+(ad+bc)i */
    if (mp_is_complex(x) || mp_is_complex(y)) {
        MPNumber real_x, real_y, im_x, im_y, t1, t2, real_z, im_z;

        mp_real_component(x, &real_x);
        mp_imaginary_component(x, &im_x);
        mp_real_component(y, &real_y);
        mp_imaginary_component(y, &im_y);
    
        mp_multiply_real(&real_x, &real_y, &t1);
        mp_multiply_real(&im_x, &im_y, &t2);
        mp_subtract(&t1, &t2, &real_z);
    
        mp_multiply_real(&real_x, &im_y, &t1);
        mp_multiply_real(&im_x, &real_y, &t2);
        mp_add(&t1, &t2, &im_z);

        mp_set_from_complex(&real_z, &im_z, z);
    }
    else {
        mp_multiply_real(x, y, z);
    }
}


static void
mp_multiply_integer_real(const MPNumber *x, int64_t y, MPNumber *z)
{
    int c, i;
    MPNumber x_copy;

    /* x*0 = 0*y = 0 */
    if (mp_is_zero(x) || y == 0) {
        mp_set_from_integer(0, z);
        return;
    }

    /* x*1 = x, x*-1 = -x */
    // FIXME: Why is this not working? mp_ext is using this function to do a normalization
    /*if (y == 1 || y == -1) {
        if (y < 0)
            mp_invert_sign(x, z);
        else
            mp_set_from_mp(x, z);
        return;
    }*/

    /* Copy x as z may also refer to x */
    mp_set_from_mp(x, &x_copy);
    mp_set_from_integer(0, z);

    if (y < 0) {
        y = -y;
        z->sign = -x_copy.sign;
    }
    else
        z->sign = x_copy.sign;
    z->exponent = x_copy.exponent + 4;

    /* FORM PRODUCT IN ACCUMULATOR */
    c = 0;

    /*  IF y*B NOT REPRESENTABLE AS AN INTEGER WE HAVE TO SIMULATE
     *  DOUBLE-PRECISION MULTIPLICATION.
     */

    /* Computing MAX */
    if (y >= max(MP_BASE << 3, 32767 / MP_BASE)) {
        int64_t j1, j2;

        /* HERE J IS TOO LARGE FOR SINGLE-PRECISION MULTIPLICATION */
        j1 = y / MP_BASE;
        j2 = y - j1 * MP_BASE;

        /* FORM PRODUCT */
        for (i = MP_T + 3; i >= 0; i--) {
            int64_t c1, c2, is, ix, t;

            c1 = c / MP_BASE;
            c2 = c - MP_BASE * c1;
            ix = 0;
            if (i > 3)
                ix = x_copy.fraction[i - 4];

            t = j2 * ix + c2;
            is = t / MP_BASE;
            c = j1 * ix + c1 + is;
            z->fraction[i] = t - MP_BASE * is;
        }
    }
    else
    {
        int64_t ri = 0;

        for (i = MP_T + 3; i >= 4; i--) {
            ri = y * x_copy.fraction[i - 4] + c;
            c = ri / MP_BASE;
            z->fraction[i] = ri - MP_BASE * c;
        }

        /* CHECK FOR INTEGER OVERFLOW */
        if (ri < 0) {
            mperr("*** INTEGER OVERFLOW IN mp_multiply_integer, B TOO LARGE ***");
            mp_set_from_integer(0, z);
            return;
        }

        /* HAVE TO TREAT FIRST FOUR WORDS OF R SEPARATELY */
        for (i = 3; i >= 0; i--) {
            int t;

            t = c;
            c = t / MP_BASE;
            z->fraction[i] = t - MP_BASE * c;
        }
    }

    /* HAVE TO SHIFT RIGHT HERE AS CARRY OFF END */
    while (c != 0) {
        int64_t t;

        for (i = MP_T + 3; i >= 1; i--)
            z->fraction[i] = z->fraction[i - 1];
        t = c;
        c = t / MP_BASE;
        z->fraction[0] = t - MP_BASE * c;
        z->exponent++;
    }

    if (c < 0) {
        mperr("*** INTEGER OVERFLOW IN mp_multiply_integer, B TOO LARGE ***");
        mp_set_from_integer(0, z);
        return;
    }

    z->im_sign = 0;
    z->im_exponent = 0;
    memset(z->im_fraction, 0, sizeof(int) * MP_SIZE);    
    mp_normalize(z);
}


void
mp_multiply_integer(const MPNumber *x, int64_t y, MPNumber *z)
{
    if (mp_is_complex(x)) {
        MPNumber re_z, im_z;
        mp_real_component(x, &re_z);
        mp_imaginary_component(x, &im_z);
        mp_multiply_integer_real(&re_z, y, &re_z);
        mp_multiply_integer_real(&im_z, y, &im_z);
        mp_set_from_complex(&re_z, &im_z, z);
    }
    else
        mp_multiply_integer_real(x, y, z);
}


void
mp_multiply_fraction(const MPNumber *x, int64_t numerator, int64_t denominator, MPNumber *z)
{
    if (denominator == 0) {
        mperr(_("Division by zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    if (numerator == 0) {
        mp_set_from_integer(0, z);
        return;
    }

    /* Reduce to lowest terms */
    mp_gcd(&numerator, &denominator);
    mp_divide_integer(x, denominator, z);
    mp_multiply_integer(z, numerator, z);
}


void
mp_invert_sign(const MPNumber *x, MPNumber *z)
{
    mp_set_from_mp(x, z);
    z->sign = -z->sign;
    z->im_sign = -z->im_sign;
}


// FIXME: Is r->fraction large enough?  It seems to be in practise but it may be MP_T+4 instead of MP_T
// FIXME: There is some sort of stack corruption/use of unitialised variables here.  Some functions are
// using stack variables as x otherwise there are corruption errors. e.g. "Cos(45) - 1/Sqrt(2) = -0"
// (try in scientific mode)
void
mp_normalize(MPNumber *x)
{
    int start_index;

    /* Find first non-zero digit */
    for (start_index = 0; start_index < MP_SIZE && x->fraction[start_index] == 0; start_index++);

    /* Mark as zero */
    if (start_index >= MP_SIZE) {
        x->sign = 0;
        x->exponent = 0;
        return;
    }

    /* Shift left so first digit is non-zero */
    if (start_index > 0) {
        int i;

        x->exponent -= start_index;
        for (i = 0; (i + start_index) < MP_SIZE; i++)
            x->fraction[i] = x->fraction[i + start_index];
        for (; i < MP_SIZE; i++)
            x->fraction[i] = 0;
    }
}


static void
mp_pwr(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    MPNumber t;

    /* (-x)^y imaginary */
    /* FIXME: Make complex numbers optional */
    /*if (x->sign < 0) {
        mperr(_("The power of negative numbers is only defined for integer exponents"));
        mp_set_from_integer(0, z);
        return;
    }*/

    /* 0^-y illegal */
    if (mp_is_zero(x) && y->sign < 0) {
        mperr(_("The power of zero is undefined for a negative exponent"));
        mp_set_from_integer(0, z);
        return;
    }

    /* x^0 = 1 */
    if (mp_is_zero(y)) {
        mp_set_from_integer(1, z);
        return;
    }

    mp_ln(x, &t);
    mp_multiply(y, &t, z);
    mp_epowy(z, z);
}


static void
mp_reciprocal_real(const MPNumber *x, MPNumber *z)
{
    MPNumber t1, t2;
    int it0, t;

    /* 1/0 invalid */
    if (mp_is_zero(x)) {
        mperr(_("Reciprocal of zero is undefined"));
        mp_set_from_integer(0, z);
        return;
    }

    /* Start by approximating value using floating point */
    mp_set_from_mp(x, &t1);
    t1.exponent = 0;
    mp_set_from_float(1.0f / mp_cast_to_float(&t1), &t1);
    t1.exponent -= x->exponent;

    it0 = t = 3;
    while(1) {
        int ts2, ts3;

        /* t1 = t1 - (t1 * ((x * t1) - 1))   (2*t1 - t1^2*x) */
        mp_multiply(x, &t1, &t2);
        mp_add_integer(&t2, -1, &t2);
        mp_multiply(&t1, &t2, &t2);
        mp_subtract(&t1, &t2, &t1);
        if (t >= MP_T)
            break;

        /*  FOLLOWING LOOP ALMOST DOUBLES T (POSSIBLE
         *  BECAUSE NEWTONS METHOD HAS 2ND ORDER CONVERGENCE).
         */
        ts3 = t;
        t = MP_T;
        do {
            ts2 = t;
            t = (t + it0) / 2;
        } while (t > ts3);
        t = min(ts2, MP_T);
    }

    /* RETURN IF NEWTON ITERATION WAS CONVERGING */
    if (t2.sign != 0 && (t1.exponent - t2.exponent) << 1 < MP_T - it0) {
        /*  THE FOLLOWING MESSAGE MAY INDICATE THAT B**(T-1) IS TOO SMALL,
         *  OR THAT THE STARTING APPROXIMATION IS NOT ACCURATE ENOUGH.
         */
        mperr("*** ERROR OCCURRED IN MP_RECIPROCAL, NEWTON ITERATION NOT CONVERGING PROPERLY ***");
    }

    mp_set_from_mp(&t1, z);
}


void
mp_reciprocal(const MPNumber *x, MPNumber *z)
{    
    if (mp_is_complex(x)) {
        MPNumber t1, t2;
        MPNumber real_x, im_x;

        mp_real_component(x, &real_x);
        mp_imaginary_component(x, &im_x);

        /* 1/(a+bi) = (a-bi)/(a+bi)(a-bi) = (a-bi)/(a²+b²) */
        mp_multiply(&real_x, &real_x, &t1);
        mp_multiply(&im_x, &im_x, &t2);
        mp_add(&t1, &t2, &t1);
        mp_reciprocal_real(&t1, z);
        mp_conjugate(x, &t1);
        mp_multiply(&t1, z, z);
    }
    else
        mp_reciprocal_real(x, z);
}


static void
mp_root_real(const MPNumber *x, int64_t n, MPNumber *z)
{
    float approximation;
    int ex, np, it0, t;
    MPNumber t1, t2;

    /* x^(1/1) = x */
    if (n == 1) {
        mp_set_from_mp(x, z);
        return;
    }

    /* x^(1/0) invalid */
    if (n == 0) {
        mperr(_("Root must be non-zero"));
        mp_set_from_integer(0, z);
        return;
    }

    np = abs(n);

    /* LOSS OF ACCURACY IF NP LARGE, SO ONLY ALLOW NP <= MAX (B, 64) */
    if (np > max(MP_BASE, 64)) {
        mperr("*** ABS(N) TOO LARGE IN CALL TO MP_ROOT ***");
        mp_set_from_integer(0, z);
        return;
    }

    /* 0^(1/n) = 0 for positive n */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        if (n <= 0)
            mperr(_("Negative root of zero is undefined"));
        return;
    }

    // FIXME: Imaginary root
    if (x->sign < 0 && np % 2 == 0) {
        mperr(_("nth root of negative number is undefined for even n"));
        mp_set_from_integer(0, z);
        return;
    }

    /* DIVIDE EXPONENT BY NP */
    ex = x->exponent / np;

    /* Get initial approximation */
    mp_set_from_mp(x, &t1);
    t1.exponent = 0;
    approximation = exp(((float)(np * ex - x->exponent) * log((float)MP_BASE) -
                         log((fabs(mp_cast_to_float(&t1))))) / (float)np);
    mp_set_from_float(approximation, &t1);
    t1.sign = x->sign;
    t1.exponent -= ex;

    /* MAIN ITERATION LOOP */
    it0 = t = 3;
    while(1) {
        int ts2, ts3;

        /* t1 = t1 - ((t1 * ((x * t1^np) - 1)) / np) */
        mp_xpowy_integer(&t1, np, &t2);
        mp_multiply(x, &t2, &t2);
        mp_add_integer(&t2, -1, &t2);
        mp_multiply(&t1, &t2, &t2);
        mp_divide_integer(&t2, np, &t2);
        mp_subtract(&t1, &t2, &t1);

        /*  FOLLOWING LOOP ALMOST DOUBLES T (POSSIBLE BECAUSE
         *  NEWTONS METHOD HAS 2ND ORDER CONVERGENCE).
         */
        if (t >= MP_T)
            break;

        ts3 = t;
        t = MP_T;
        do {
            ts2 = t;
            t = (t + it0) / 2;
        } while (t > ts3);
        t = min(ts2, MP_T);
    }

    /*  NOW R(I2) IS X**(-1/NP)
     *  CHECK THAT NEWTON ITERATION WAS CONVERGING
     */
    if (t2.sign != 0 && (t1.exponent - t2.exponent) << 1 < MP_T - it0) {
        /*  THE FOLLOWING MESSAGE MAY INDICATE THAT B**(T-1) IS TOO SMALL,
         *  OR THAT THE INITIAL APPROXIMATION OBTAINED USING ALOG AND EXP
         *  IS NOT ACCURATE ENOUGH.
         */
        mperr("*** ERROR OCCURRED IN MP_ROOT, NEWTON ITERATION NOT CONVERGING PROPERLY ***");
    }

    if (n >= 0) {
        mp_xpowy_integer(&t1, n - 1, &t1);
        mp_multiply(x, &t1, z);
        return;
    }

    mp_set_from_mp(&t1, z);
}


void
mp_root(const MPNumber *x, int64_t n, MPNumber *z)
{
    if (!mp_is_complex(x) && mp_is_negative(x) && n % 2 == 1) {
        mp_abs(x, z);
        mp_root_real(z, n, z);
        mp_invert_sign(z, z);
    }
    else if (mp_is_complex(x) || mp_is_negative(x)) {
        MPNumber r, theta;

        mp_abs(x, &r);
        mp_arg(x, MP_RADIANS, &theta);

        mp_root_real(&r, n, &r);
        mp_divide_integer(&theta, n, &theta);
        mp_set_from_polar(&r, MP_RADIANS, &theta, z);
    }
    else
        mp_root_real(x, n, z);
}


void
mp_sqrt(const MPNumber *x, MPNumber *z)
{
    if (mp_is_zero(x))
       mp_set_from_integer(0, z);
    /* FIXME: Make complex numbers optional */
    /*else if (x->sign < 0) {
        mperr(_("Square root is undefined for negative values"));
        mp_set_from_integer(0, z);
    }*/
    else {
        MPNumber t;

        mp_root(x, -2, &t);
        mp_multiply(x, &t, z);
        mp_ext(t.fraction[0], z->fraction[0], z);
    }
}


void
mp_factorial(const MPNumber *x, MPNumber *z)
{
    int i, value;

    /* 0! == 1 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(1, z);
        return;
    }
    if (!mp_is_natural(x)) {
        /* Translators: Error displayed when attempted take the factorial of a fractional number */
        mperr(_("Factorial is only defined for natural numbers"));
        mp_set_from_integer(0, z);
        return;
    }

    /* Convert to integer - if couldn't be converted then the factorial would be too big anyway */
    value = mp_cast_to_int(x);
    mp_set_from_mp(x, z);
    for (i = 2; i < value; i++)
        mp_multiply_integer(z, i, z);
}


void
mp_modulus_divide(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    MPNumber t1, t2;

    if (!mp_is_integer(x) || !mp_is_integer(y)) {
        /* Translators: Error displayed when attemping to do a modulus division on non-integer numbers */
        mperr(_("Modulus division is only defined for integers"));
        mp_set_from_integer(0, z);
    }

    mp_divide(x, y, &t1);
    mp_floor(&t1, &t1);
    mp_multiply(&t1, y, &t2);
    mp_subtract(x, &t2, z);

    mp_set_from_integer(0, &t1);
    if ((mp_is_less_than(y, &t1) && mp_is_greater_than(z, &t1)) || mp_is_less_than(z, &t1))
        mp_add(z, y, z);
}


void
mp_xpowy(const MPNumber *x, const MPNumber *y, MPNumber *z)
{
    if (mp_is_integer(y)) {
        mp_xpowy_integer(x, mp_cast_to_int(y), z);
    } else {
        MPNumber reciprocal;
        mp_reciprocal(y, &reciprocal);
        if (mp_is_integer(&reciprocal))
            mp_root(x, mp_cast_to_int(&reciprocal), z);
        else
            mp_pwr(x, y, z);
    }
}


void
mp_xpowy_integer(const MPNumber *x, int64_t n, MPNumber *z)
{
    int i;
    MPNumber t;

    /* 0^-n invalid */
    if (mp_is_zero(x) && n < 0) {
        /* Translators: Error displayed when attempted to raise 0 to a negative exponent */
        mperr(_("The power of zero is undefined for a negative exponent"));
        mp_set_from_integer(0, z);
        return;
    }

    /* x^0 = 1 */
    if (n == 0) {
        mp_set_from_integer(1, z);
        return;
    }

    /* 0^n = 0 */
    if (mp_is_zero(x)) {
        mp_set_from_integer(0, z);
        return;
    }
  
    /* x^1 = x */
    if (n == 1) {
        mp_set_from_mp(x, z);
        return;
    }

    if (n < 0) {
        mp_reciprocal(x, &t);
        n = -n;
    }
    else
        mp_set_from_mp(x, &t);

    /* Multply x n times */
    // FIXME: Can do mp_multiply(z, z, z) until close to answer (each call doubles number of multiples) */
    mp_set_from_integer(1, z);
    for (i = 0; i < n; i++)
        mp_multiply(z, &t, z);
}


GList*
mp_factorize(const MPNumber *x)
{
    GList *list = NULL;
    MPNumber *factor = g_slice_alloc0(sizeof(MPNumber));
    MPNumber value, tmp, divisor, root;

    mp_abs(x, &value);

    if (mp_is_zero(&value)) {
        mp_set_from_mp(&value, factor);
        list = g_list_append(list, factor);
        return list;
    }

    mp_set_from_integer(1, &tmp);
    if (mp_is_equal(&value, &tmp)) {
        mp_set_from_mp(x, factor);
        list = g_list_append(list, factor);
        return list;
    }

    mp_set_from_integer(2, &divisor);
    while (TRUE) {
        mp_divide(&value, &divisor, &tmp);
        if (mp_is_integer(&tmp)) {
            value = tmp;
            mp_set_from_mp(&divisor, factor);
            list = g_list_append(list, factor);
            factor = g_slice_alloc0(sizeof(MPNumber));
        } else {
            break;
        }
    }

    mp_set_from_integer(3, &divisor);
    mp_sqrt(&value, &root);
    while (mp_is_less_equal(&divisor, &root)) {
        mp_divide(&value, &divisor, &tmp);
        if (mp_is_integer(&tmp)) {
            value = tmp;
            mp_sqrt(&value, &root);
            mp_set_from_mp(&divisor, factor);
            list = g_list_append(list, factor);
            factor = g_slice_alloc0(sizeof(MPNumber));
        } else {
            mp_add_integer(&divisor, 2, &tmp);
            divisor = tmp;
        }
    }

    mp_set_from_integer(1, &tmp);
    if (mp_is_greater_than(&value, &tmp)) {
        mp_set_from_mp(&value, factor);
        list = g_list_append(list, factor);
    } else {
        g_slice_free(MPNumber, factor);
    }

    if (mp_is_negative(x)) {
        mp_invert_sign(list->data, list->data);
    }

    return list;
}
