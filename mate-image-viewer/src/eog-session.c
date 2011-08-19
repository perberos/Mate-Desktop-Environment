/* Eye Of Mate - Session Handler
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-session.h) by:
 * 	- Gedit Team
 * 	- Federico Mena-Quintero <federico@ximian.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-session.h"
#include "eog-window.h"
#include "eog-application.h"

void
eog_session_init (EogApplication *application)
{
	g_return_if_fail (EOG_IS_APPLICATION (application));

	/* FIXME: Session management is currently a no-op in eog. */
}

gboolean
eog_session_is_restored	(void)
{
	return FALSE;
}

gboolean
eog_session_load (void)
{
	return TRUE;
}
