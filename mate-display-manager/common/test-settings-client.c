/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>
#include <glib-object.h>

#include "mdm-settings-client.h"
#include "mdm-settings-keys.h"

static void
notify_cb (guint             id,
           MdmSettingsEntry *entry,
           gpointer          user_data)
{
        g_debug ("Value changed: %s=%s",
                 mdm_settings_entry_get_key (entry),
                 mdm_settings_entry_get_value (entry));
}

static gboolean
test_settings_client (gpointer data)
{
        char    *strval;
        gboolean boolval;
        gboolean res;

        strval = NULL;
        res = mdm_settings_client_get_string (MDM_KEY_WILLING, &strval);
        g_debug ("Got res=%d %s=%s", res, MDM_KEY_WILLING, strval);
        g_free (strval);

        res = mdm_settings_client_get_boolean (MDM_KEY_XDMCP_ENABLE, &boolval);
        g_debug ("Got res=%d %s=%s", res, MDM_KEY_XDMCP_ENABLE, boolval ? "true" : "false");

        g_debug ("Adding notify for all keys");
        mdm_settings_client_notify_add ("/", notify_cb, NULL, NULL);

        g_debug ("Setting boolean key %s to %s", MDM_KEY_XDMCP_ENABLE, !boolval ? "true" : "false");
        mdm_settings_client_set_boolean (MDM_KEY_XDMCP_ENABLE, !boolval);
        g_debug ("Setting boolean key %s to %s", MDM_KEY_XDMCP_ENABLE, boolval ? "true" : "false");
        mdm_settings_client_set_boolean (MDM_KEY_XDMCP_ENABLE, boolval);

        return FALSE;
}

int
main (int argc, char **argv)
{
        GMainLoop *loop;

        g_type_init ();

        if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                exit (1);
        }

        g_idle_add (test_settings_client, NULL);

        loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (loop);

        return 0;
}
