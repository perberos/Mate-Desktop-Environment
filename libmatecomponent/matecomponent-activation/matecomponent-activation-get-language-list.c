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
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <matecomponent-activation/matecomponent-activation-private.h>
#include <glib/gi18n-lib.h>

/* ALEX:
 *        This function is not in use anymore in matecomponent. However, some
 *        versions of libmate and mate-vfs use it, so we can't remove it.
 *        We keep it here for backwards compat. For details, see bug 168948.
 */

/*
 * matecomponent_activation_i18n_get_language_list:
 * 
 * This computes a list of language strings that the user wants.  It searches in
 * the standard environment variables to find the list, which is sorted in order
 * from most desirable to least desirable.  The `C' locale is appended to the
 * list if it does not already appear (other routines depend on this
 * behaviour).
 *
 * The argument is ignored. It used to be the category to use for the locale.
 * 
 * Return value: the list of languages, this list should not be freed as it is
 * owned by mate-i18n.
 */
const GList *
matecomponent_activation_i18n_get_language_list (const gchar *ignored)
{
  static GList *list = NULL;
  const char * const* langs;
  int i;

  MATECOMPONENT_ACTIVATION_LOCK ();

  if (list == NULL) {
    langs = g_get_language_names ();
    for (i = 0; langs[i] != NULL; i++) {
      list = g_list_prepend (list, g_strdup(langs[i]));
    }

    list = g_list_reverse (list);
  }
  
  MATECOMPONENT_ACTIVATION_UNLOCK ();

  return list;
  
}
