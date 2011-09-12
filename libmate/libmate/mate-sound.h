/*
 * Copyright (C) 1997-1998 Stuart Parmenter and Elliot Lee
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

#ifndef __MATE_SOUND_H__
#define __MATE_SOUND_H__ 1

#ifndef MATE_DISABLE_DEPRECATED

#include <glib.h>

G_BEGIN_DECLS

/* Use this with the Esound functions */
int mate_sound_connection_get (void);

/* Initialize esd connection */
void mate_sound_init(const char* hostname);

/* Closes esd connection */
void mate_sound_shutdown(void);

/* Returns the Esound sample ID for the sample */
int mate_sound_sample_load(const char* sample_name, const char* filename);

/* Loads sample, plays sample, frees sample */
void mate_sound_play(const char* filename);

G_END_DECLS

#endif /* !MATE_DISABLE_DEPRECATED */

#endif /* __MATE_SOUND_H__ */
