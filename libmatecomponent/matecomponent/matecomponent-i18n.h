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

/*
 * Handles all of the internationalization configuration options.
 * Author: Tom Tromey <tromey@creche.cygnus.com>
 */

#ifndef __MATECOMPONENT_I18N_H__
#define __MATECOMPONENT_I18N_H__ 1

#ifndef MATECOMPONENT_DISABLE_DEPRECATED
 
#ifdef MATECOMPONENT_EXPLICIT_TRANSLATION_DOMAIN
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#endif /* !MATECOMPONENT_DISABLE_DEPRECATED */

#endif /* __MATECOMPONENT_I18N_H__ */
