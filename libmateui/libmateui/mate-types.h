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
/*
  @NOTATION@
 */

#ifndef MATE_TYPES_H
#define MATE_TYPES_H
/****
  Mate-wide useful types.
  ****/



#ifdef __cplusplus
extern "C" {
#endif

/* string is a g_malloc'd string which should be freed, or NULL if the
   user cancelled. */
typedef void (* MateStringCallback)(gchar * string, gpointer data);

/* See mate-uidefs for the Yes/No Ok/Cancel defines which can be
   "reply" */
typedef void (* MateReplyCallback)(gint reply, gpointer data);

/* Do something never, only when the user wants, or always. */
typedef enum {
  MATE_PREFERENCES_NEVER,
  MATE_PREFERENCES_USER,
  MATE_PREFERENCES_ALWAYS
} MatePreferencesType;

#ifdef __cplusplus
}
#endif

#endif
