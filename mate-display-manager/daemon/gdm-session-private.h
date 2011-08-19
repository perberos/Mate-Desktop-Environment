/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 *
 */

#ifndef __GDM_SESSION_PRIVATE_H
#define __GDM_SESSION_PRIVATE_H

#include <glib-object.h>
#include "gdm-session.h"

G_BEGIN_DECLS

/* state changes */
void             _gdm_session_conversation_started         (GdmSession   *session);
void             _gdm_session_setup_complete               (GdmSession   *session);
void             _gdm_session_setup_failed                 (GdmSession   *session,
                                                            const char   *message);
void             _gdm_session_reset_complete               (GdmSession   *session);
void             _gdm_session_reset_failed                 (GdmSession   *session,
                                                            const char   *message);
void             _gdm_session_authenticated                (GdmSession   *session);
void             _gdm_session_authentication_failed        (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_authorized                   (GdmSession   *session);
void             _gdm_session_authorization_failed         (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_accredited                   (GdmSession   *session);
void             _gdm_session_accreditation_failed         (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_session_opened               (GdmSession   *session);
void             _gdm_session_session_open_failed          (GdmSession   *session,
                                                            const char   *message);
void             _gdm_session_session_started              (GdmSession   *session,
                                                            int           pid);
void             _gdm_session_session_start_failed         (GdmSession   *session,
                                                            const char   *message);
void             _gdm_session_session_exited               (GdmSession   *session,
                                                            int           exit_code);
void             _gdm_session_session_died                 (GdmSession   *session,
                                                            int           signal_number);
void             _gdm_session_closed                       (GdmSession   *session);

/* user settings read from ~/.dmrc / system defaults */
void             _gdm_session_default_language_name_changed     (GdmSession   *session,
                                                                 const char   *language_name);
void             _gdm_session_default_layout_name_changed       (GdmSession   *session,
                                                                 const char   *layout_name);
void             _gdm_session_default_session_name_changed      (GdmSession   *session,
                                                                 const char   *session_name);
/* user is selected/changed internally */
void             _gdm_session_selected_user_changed        (GdmSession   *session,
                                                            const char   *text);

/* call and response stuff */
void             _gdm_session_info_query                   (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_secret_info_query            (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_info                         (GdmSession   *session,
                                                            const char   *text);
void             _gdm_session_problem                      (GdmSession   *session,
                                                            const char   *text);

G_END_DECLS

#endif /* __GDM_SESSION_PRIVATE_H */
