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

#ifndef __MATE_SCORE_H__
#define __MATE_SCORE_H__ 1

#ifndef MATE_DISABLE_DEPRECATED

#include <time.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
 * mate_score_init()
 * creates a child process with which we communicate through a pair of pipes,
 * then drops privileges.
 * this should be called as the first statement in main().
 * returns 0 on success, drops privs and returns -1 on failure
 */

gint mate_score_init(const gchar* gamename);

/* Returns the position in the top-ten starting from 1, or 0 if it isn't in the table
 *
 * level
 * Pass in NULL unless you want to keep per-level scores for the game
 *
 * higher_to_lower_score_order
 * Pass in TRUE if higher scores are "better" in the game
 */
gint mate_score_log(gfloat score, const gchar *level, gboolean higher_to_lower_score_order);

/* Returns number of items in the arrays
 *
 * gamename
 * Will be auto-determined if NULL
 */
gint mate_score_get_notable(const gchar *gamename, const gchar* level, gchar*** names, gfloat** scores, time_t** scoretimes);
#ifdef __cplusplus
}
#endif

#endif /* __MATE_SCORE_H__ */

#endif /* MATE_DISABLE_DEPRECATED */
