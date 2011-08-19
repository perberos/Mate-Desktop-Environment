/* totem-session.h

   Copyright (C) 2004 Bastien Nocera <hadess@hadess.net>

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#ifndef TOTEM_SESSION_H
#define TOTEM_SESSION_H

#include "totem.h"

G_BEGIN_DECLS

void totem_session_add_options (GOptionContext *context);
void totem_session_setup (Totem *totem, char **argv);
void totem_session_restore (Totem *totem, char **filenames);

G_END_DECLS

#endif /* TOTEM_SESSION_H */
