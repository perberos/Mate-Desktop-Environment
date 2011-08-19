/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Jon McCann <jmccann@redhat.com>
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
 *	Jon McCann <jmccann@redhat.com>
 */

#ifndef __GSM_CONSOLEKIT_H__
#define __GSM_CONSOLEKIT_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSM_TYPE_CONSOLEKIT             (gsm_consolekit_get_type ())
#define GSM_CONSOLEKIT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_CONSOLEKIT, GsmConsolekit))
#define GSM_CONSOLEKIT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_CONSOLEKIT, GsmConsolekitClass))
#define GSM_IS_CONSOLEKIT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_CONSOLEKIT))
#define GSM_IS_CONSOLEKIT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_CONSOLEKIT))
#define GSM_CONSOLEKIT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), GSM_TYPE_CONSOLEKIT, GsmConsolekitClass))
#define GSM_CONSOLEKIT_ERROR            (gsm_consolekit_error_quark ())

typedef struct _GsmConsolekit        GsmConsolekit;
typedef struct _GsmConsolekitClass   GsmConsolekitClass;
typedef struct _GsmConsolekitPrivate GsmConsolekitPrivate;
typedef enum   _GsmConsolekitError   GsmConsolekitError;

struct _GsmConsolekit
{
        GObject               parent;

        GsmConsolekitPrivate *priv;
};

struct _GsmConsolekitClass
{
        GObjectClass parent_class;

        void (* request_completed) (GsmConsolekit *manager,
                                    GError        *error);

        void (* privileges_completed) (GsmConsolekit *manager,
                                       gboolean       success,
                                       gboolean       ask_later,
                                       GError        *error);
};

enum _GsmConsolekitError {
        GSM_CONSOLEKIT_ERROR_RESTARTING = 0,
        GSM_CONSOLEKIT_ERROR_STOPPING
};

#define GSM_CONSOLEKIT_SESSION_TYPE_LOGIN_WINDOW "LoginWindow"

GType            gsm_consolekit_get_type        (void);

GQuark           gsm_consolekit_error_quark     (void);

GsmConsolekit   *gsm_consolekit_new             (void) G_GNUC_MALLOC;

gboolean         gsm_consolekit_can_switch_user (GsmConsolekit *manager);

gboolean         gsm_consolekit_get_restart_privileges (GsmConsolekit *manager);

gboolean         gsm_consolekit_get_stop_privileges    (GsmConsolekit *manager);

gboolean         gsm_consolekit_can_stop        (GsmConsolekit *manager);

gboolean         gsm_consolekit_can_restart     (GsmConsolekit *manager);

void             gsm_consolekit_attempt_stop    (GsmConsolekit *manager);

void             gsm_consolekit_attempt_restart (GsmConsolekit *manager);

void             gsm_consolekit_set_session_idle (GsmConsolekit *manager,
                                                  gboolean       is_idle);

gchar           *gsm_consolekit_get_current_session_type (GsmConsolekit *manager);

GsmConsolekit   *gsm_get_consolekit             (void);

G_END_DECLS

#endif /* __GSM_CONSOLEKIT_H__ */
