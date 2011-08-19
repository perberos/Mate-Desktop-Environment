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

#include <glib/gi18n.h>

#include "financial.h"
#include "mp.h"

static void
calc_ctrm(MathEquation *equation, MPNumber *t, MPNumber *pint, MPNumber *fv, MPNumber *pv)
{

/*  Cterm - pint (periodic interest rate).
 *          fv  (future value).
 *          pv  (present value).
 *
 *          RESULT = log(fv / pv) / log(1 + pint)
 */
    MPNumber MP1, MP2, MP3, MP4;

    mp_divide(fv, pv, &MP1);
    mp_ln(&MP1, &MP2);
    mp_add_integer(pint, 1, &MP3);
    mp_ln(&MP3, &MP4);
    mp_divide(&MP2, &MP4, t);
}


static void
calc_ddb(MathEquation *equation, MPNumber *t, MPNumber *cost, MPNumber *life, MPNumber *period)
{

/*  Ddb   - cost    (amount paid for asset).
 *          life    (useful life of the asset).
 *          period  (time period for depreciation allowance).
 *
 *          bv = 0.0;
 *          for (i = 0; i < life; i++)
 *            {
 *              VAL = ((cost - bv) * 2) / life
 *              bv += VAL
 *            }
 *          RESULT = VAL
 *
 */

    int i;
    int len;
    MPNumber MPbv, MP1, MP2;

    mp_set_from_integer(0, &MPbv);
    len = mp_cast_to_int(period);
    for (i = 0; i < len; i++) {
        mp_subtract(cost, &MPbv, &MP1);
        mp_multiply_integer(&MP1, 2, &MP2);
        mp_divide(&MP2, life, t);
        mp_set_from_mp(&MPbv, &MP1);
        mp_add(&MP1, t, &MPbv); /* TODO: why result is MPbv, for next loop? */
    }

    if (len >= 0) {
        math_equation_set_status (equation, ("Error: the number of periods must be positive"));
        mp_set_from_integer(0, t);
    }
}


static void
calc_fv(MathEquation *equation, MPNumber *t, MPNumber *pmt, MPNumber *pint, MPNumber *n)
{

/*  Fv    - pmt (periodic payment).
 *          pint (periodic interest rate).
 *          n   (number of periods).
 *
 *          RESULT = pmt * (pow(1 + pint, n) - 1) / pint
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_add_integer(pint, 1, &MP1);
    mp_xpowy(&MP1, n, &MP2);
    mp_add_integer(&MP2, -1, &MP3);
    mp_multiply(pmt, &MP3, &MP4);
    mp_divide(&MP4, pint, t);
}


static void
calc_gpm(MathEquation *equation, MPNumber *t, MPNumber *cost, MPNumber *margin)
{

/*  Gpm   - cost (cost of sale).
 *          margin (gross profit margin.
 *
 *          RESULT = cost / (1 - margin)
 */

    MPNumber MP1, MP2;

    mp_set_from_integer(1, &MP1);
    mp_subtract(&MP1, margin, &MP2);
    mp_divide(cost, &MP2, t);
}


static void
calc_pmt(MathEquation *equation, MPNumber *t, MPNumber *prin, MPNumber *pint, MPNumber *n)
{

/*  Pmt   - prin (principal).
 *          pint  (periodic interest rate).
 *          n    (term).
 *
 *          RESULT = prin * (pint / (1 - pow(pint + 1, -1 * n)))
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_add_integer(pint, 1, &MP1);
    mp_multiply_integer(n, -1, &MP2);
    mp_xpowy(&MP1, &MP2, &MP3);
    mp_multiply_integer(&MP3, -1, &MP4);
    mp_add_integer(&MP4, 1, &MP1);
    mp_divide(pint, &MP1, &MP2);
    mp_multiply(prin, &MP2, t);
}


static void
calc_pv(MathEquation *equation, MPNumber *t, MPNumber *pmt, MPNumber *pint, MPNumber *n)
{

/*  Pv    - pmt (periodic payment).
 *          pint (periodic interest rate).
 *          n   (term).
 *
 *          RESULT = pmt * (1 - pow(1 + pint, -1 * n)) / pint
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_add_integer(pint, 1, &MP1);
    mp_multiply_integer(n, -1, &MP2);
    mp_xpowy(&MP1, &MP2, &MP3);
    mp_multiply_integer(&MP3, -1, &MP4);
    mp_add_integer(&MP4, 1, &MP1);
    mp_divide(&MP1, pint, &MP2);
    mp_multiply(pmt, &MP2, t);
}


static void
calc_rate(MathEquation *equation, MPNumber *t, MPNumber *fv, MPNumber *pv, MPNumber *n)
{

/*  Rate  - fv (future value).
 *          pv (present value).
 *          n  (term).
 *
 *          RESULT = pow(fv / pv, 1 / n) - 1
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_divide(fv, pv, &MP1);
    mp_set_from_integer(1, &MP2);
    mp_divide(&MP2, n, &MP3);
    mp_xpowy(&MP1, &MP3, &MP4);
    mp_add_integer(&MP4, -1, t);
}


static void
calc_sln(MathEquation *equation, MPNumber *t, MPNumber *cost, MPNumber *salvage, MPNumber *life)
{

/*  Sln   - cost    (cost of the asset).
 *          salvage (salvage value of the asset).
 *          life    (useful life of the asset).
 *
 *          RESULT = (cost - salvage) / life
 */

    MPNumber MP1;
    mp_subtract(cost, salvage, &MP1);
    mp_divide(&MP1, life, t);
}


static void
calc_syd(MathEquation *equation, MPNumber *t, MPNumber *cost, MPNumber *salvage, MPNumber *life, MPNumber *period)
{

/*  Syd   - cost    (cost of the asset).
 *          salvage (salvage value of the asset).
 *          life    (useful life of the asset).
 *          period  (period for which depreciation is computed).
 *
 *          RESULT = (cost - salvage) * (life - period + 1) /
 *                   (life * (life + 1)) / 2
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_subtract(life, period, &MP2);
    mp_add_integer(&MP2, 1, &MP3);
    mp_add_integer(life, 1, &MP2);
    mp_multiply(life, &MP2, &MP4);
    mp_set_from_integer(2, &MP2);
    mp_divide(&MP4, &MP2, &MP1);
    mp_divide(&MP3, &MP1, &MP2);
    mp_subtract(cost, salvage, &MP1);
    mp_multiply(&MP1, &MP2, t);
}


static void
calc_term(MathEquation *equation, MPNumber *t, MPNumber *pmt, MPNumber *fv, MPNumber *pint)
{

/*  Term  - pmt (periodic payment).
 *          fv  (future value).
 *          pint (periodic interest rate).
 *
 *          RESULT = log(1 + (fv * pint / pmt)) / log(1 + pint)
 */

    MPNumber MP1, MP2, MP3, MP4;

    mp_add_integer(pint, 1, &MP1);
    mp_ln(&MP1, &MP2);
    mp_multiply(fv, pint, &MP1);
    mp_divide(&MP1, pmt, &MP3);
    mp_add_integer(&MP3, 1, &MP4);
    mp_ln(&MP4, &MP1);
    mp_divide(&MP1, &MP2, t);
}


void
do_finc_expression(MathEquation *equation, int function, MPNumber *arg1, MPNumber *arg2, MPNumber *arg3, MPNumber *arg4)
{
    MPNumber result;
    switch (function) {
     case FINC_CTRM_DIALOG:
       calc_ctrm(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_DDB_DIALOG:
       calc_ddb(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_FV_DIALOG:
       calc_fv(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_GPM_DIALOG:
       calc_gpm(equation, &result, arg1, arg2);
       break;
     case FINC_PMT_DIALOG:
       calc_pmt(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_PV_DIALOG:
       calc_pv(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_RATE_DIALOG:
       calc_rate(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_SLN_DIALOG:
       calc_sln(equation, &result, arg1, arg2, arg3);
       break;
     case FINC_SYD_DIALOG:
       calc_syd(equation, &result, arg1, arg2, arg3, arg4);
       break;
     case FINC_TERM_DIALOG:
       calc_term(equation, &result, arg1, arg2, arg3);
       break;
    }
    math_equation_set_number(equation, &result);
}
