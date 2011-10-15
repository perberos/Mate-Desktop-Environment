/* mdm.h
 * Copyright (C) 2005 Raffaele Sandrini
 * Copyright (C) 2005 Red Hat, Inc.
 * Copyright (C) 2002, 2003 George Lebl
 * Copyright (C) 2001 Queen of England,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *      Raffaele Sandrini <rasa@gmx.ch>
 *      George Lebl <jirka@5z.com>
 *      Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __MDM_LOGOUT_ACTION_H__
#define __MDM_LOGOUT_ACTION_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MDM_LOGOUT_ACTION_NONE = 0,
	MDM_LOGOUT_ACTION_SHUTDOWN = 1 << 0,
	MDM_LOGOUT_ACTION_REBOOT = 1 << 1,
	MDM_LOGOUT_ACTION_SUSPEND = 1 << 2
} MdmLogoutAction;

gboolean mdm_is_available(void);

void mdm_new_login(void);

void mdm_set_logout_action(MdmLogoutAction action);

MdmLogoutAction mdm_get_logout_action(void);

gboolean mdm_supports_logout_action(MdmLogoutAction action);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_LOGOUT_ACTION_H__ */
