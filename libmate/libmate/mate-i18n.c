/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "mate-i18n.h"

/**
 * mate_i18n_get_language_list:
 * @ignored: Ignored, pass NULL.
 *
 * This function is deprecated. You should be using g_get_language_names() instead.
 *
 * This computes a list of language strings that the user wants.  It searches in
 * the standard environment variables to find the list, which is sorted in order
 * from most desirable to least desirable.  The `C' locale is appended to the
 * list if it does not already appear (other routines depend on this
 * behaviour).
 *
 * The @ignored argument used to be the category name to use, but this was
 * removed since there is only one useful thing to pass here. For further
 * details, see http://bugzilla.gnome.org/show_bug.cgi?id=168948
 *
 * Return value: the list of languages, this list should not be freed as it is
 * owned by mate-i18n.
 **/
const GList *
mate_i18n_get_language_list (const gchar *ignored)
{
  static GStaticRecMutex lang_list_lock = G_STATIC_REC_MUTEX_INIT;
  static GList *list = NULL;
  const char * const* langs;
  int i;

  g_static_rec_mutex_lock (&lang_list_lock);

  if (list == NULL) {
    langs = g_get_language_names ();
    for (i = 0; langs[i] != NULL; i++) {
      list = g_list_prepend (list, g_strdup(langs[i]));
    }

    list = g_list_reverse (list);
  }

  g_static_rec_mutex_unlock (&lang_list_lock);

  return list;
}

static int numeric_c_locale_depth = 0;
static char *numeric_locale = NULL;

/**
 * mate_i18n_push_c_numeric_locale:
 *
 * Description:  Saves the current LC_NUMERIC locale and sets it to "C"
 * This way you can safely read write floating point numbers all in the
 * same format.  You should make sure that code between
 * mate_i18n_push_c_numeric_locale() and mate_i18n_pop_c_numeric_locale()
 * doesn't do any setlocale calls or locale may end up in a strange setting.
 * Also make sure to always pop the c numeric locale after you've pushed it.
 * The calls can be nested.
 **/
void
mate_i18n_push_c_numeric_locale (void)
{
	if (numeric_c_locale_depth == 0) {
		g_free (numeric_locale);
		numeric_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
		setlocale (LC_NUMERIC, "C");
	}
	numeric_c_locale_depth ++;
}

/**
 * mate_i18n_pop_c_numeric_locale:
 *
 * Description:  Restores the LC_NUMERIC locale to what it was
 * before the matching mate_i18n_push_c_numeric_locale(). If these calls
 * were nested, then this is a no-op until we get to the most outermost
 * layer. Code in between these should not do any setlocale calls
 * to change the LC_NUMERIC locale or things may come out very strange.
 **/
void
mate_i18n_pop_c_numeric_locale (void)
{
	if (numeric_c_locale_depth == 0) {
		return;
	}

	numeric_c_locale_depth --;

	if (numeric_c_locale_depth == 0) {
		setlocale (LC_NUMERIC, numeric_locale);
		g_free (numeric_locale);
		numeric_locale = NULL;
	}
}
