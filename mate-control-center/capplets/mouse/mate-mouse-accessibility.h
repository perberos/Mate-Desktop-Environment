/*
 * Copyright (C) 2007 Gerd Kohlberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MATE_MOUSE_A11Y_H
#define __MATE_MOUSE_A11Y_H

#include <mateconf/mateconf-client.h>

#ifdef __cplusplus
extern "C" {
#endif

void setup_accessibility (GtkBuilder *dialog, MateConfClient *client);

#ifdef __cplusplus
}
#endif

#endif /* __MATE_MOUSE_A11Y_H */
