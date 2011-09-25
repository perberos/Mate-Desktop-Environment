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

#ifndef __MDM_SESSION_PRIVATE_H
#define __MDM_SESSION_PRIVATE_H

#include <glib-object.h>
#include "mdm-session.h"

G_BEGIN_DECLS

/* state changes */
void             _mdm_session_conversation_started         (MdmSession   *session);
void             _mdm_session_setup_complete               (MdmSession   *session);
void             _mdm_session_setup_failed                 (MdmSession   *session,
                                                            const char   *message);
void             _mdm_session_reset_complete               (MdmSession   *session);
void             _mdm_session_reset_failed                 (MdmSession   *session,
                                                            const char   *message);
void             _mdm_session_authenticated                (MdmSession   *session);
void             _mdm_session_authentication_failed        (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_authorized                   (MdmSession   *session);
void             _mdm_session_authorization_failed         (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_accredited                   (MdmSession   *session);
void             _mdm_session_accreditation_failed         (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_session_opened               (MdmSession   *session);
void             _mdm_session_session_open_failed          (MdmSession   *session,
                                                            const char   *message);
void             _mdm_session_session_started              (MdmSession   *session,
                                                            int           pid);
void             _mdm_session_session_start_failed         (MdmSession   *session,
                                                            const char   *message);
void             _mdm_session_session_exited               (MdmSession   *session,
                                                            int           exit_code);
void             _mdm_session_session_died                 (MdmSession   *session,
                                                            int           signal_number);
void             _mdm_session_closed                       (MdmSession   *session);

/* user settings read from ~/.dmrc / system defaults */
void             _mdm_session_default_language_name_changed     (MdmSession   *session,
                                                                 const char   *language_name);
void             _mdm_session_default_layout_name_changed       (MdmSession   *session,
                                                                 const char   *layout_name);
void             _mdm_session_default_session_name_changed      (MdmSession   *session,
                                                                 const char   *session_name);
/* user is selected/changed internally */
void             _mdm_session_selected_user_changed        (MdmSession   *session,
                                                            const char   *text);

/* call and response stuff */
void             _mdm_session_info_query                   (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_secret_info_query            (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_info                         (MdmSession   *session,
                                                            const char   *text);
void             _mdm_session_problem                      (MdmSession   *session,
                                                            const char   *text);

G_END_DECLS

#endif /* __MDM_SESSION_PRIVATE_H */
