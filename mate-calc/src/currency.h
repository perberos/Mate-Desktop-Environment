#ifndef CURRENCY_H
#define CURRENCY_H

#include <glib/gi18n.h>

#include "mp.h"

struct currency_name {
    char *short_name;
    char *symbol;
    char *long_name;
};

/*
 * List taken from http://www.ecb.int/press/pr/date/2008/html/pr081205.en.html
 * with euro added.
 */
static const struct currency_name currency_names[] = {
    {"AUD", "$",  N_("Australian dollar")},
    {"BGN", "лв", N_("Bulgarian lev")},
    {"BRL", "R$", N_("Brazilian real")},
    {"CAD", "$",  N_("Canadian dollar")},
    {"CHF", "Fr", N_("Swiss franc")},
    {"CNY", "元", N_("Chinese yuan renminbi")},
    {"CZK", "Kč", N_("Czech koruna")},
    {"DKK", "kr", N_("Danish krone")},
    {"EEK", "KR", N_("Estonian kroon")},
    {"EUR", "€",  N_("Euro")},
    {"GBP", "£",  N_("Pound sterling")},
    {"HKD", "$",  N_("Hong Kong dollar")},
    {"HRK", "kn", N_("Croatian kuna")},
    {"HUF", "Ft", N_("Hungarian forint")},
    {"IDR", "Rp", N_("Indonesian rupiah")},
    {"INR", "Rs", N_("Indian rupee")},
    {"ISK", "kr", N_("Icelandic krona")},
    {"JPY", "¥",  N_("Japanese yen")},
    {"KRW", "₩",  N_("South Korean won")},
    {"LTL", "Lt", N_("Lithuanian litas")},
    {"LVL", "Ls", N_("Latvian lats")},
    {"MXN", "$",  N_("Mexican peso")},
    {"MYR", "RM", N_("Malaysian ringgit")},
    {"NOK", "kr", N_("Norwegian krone")},
    {"NZD", "$",  N_("New Zealand dollar")},
    {"PHP", "₱",  N_("Philippine peso")},
    {"PLN", "zł", N_("Polish zloty")},
    {"RON", "L",  N_("New Romanian leu")},
    {"RUB", "руб.", N_("Russian rouble")},
    {"SEK", "kr", N_("Swedish krona")},
    {"SGD", "$",  N_("Singapore dollar")},
    {"THB", "฿",  N_("Thai baht")},
    {"TRY", "TL", N_("New Turkish lira")},
    {"USD", "$",  N_("US dollar")},
    {"ZAR", "R",  N_("South African rand")},
    {NULL, NULL}
};

// FIXME: Should indicate when rates are updated to UI

/* Converts an amount of money from one currency to another */
gboolean currency_convert(const MPNumber *from_amount,
                          const char *source_currency, const char *target_currency,
                          MPNumber *to_amount);

/* Frees up all allocated resources */
void currency_free_resources(void);

#endif /* CURRENCY_H */
