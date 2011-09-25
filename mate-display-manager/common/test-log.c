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
#include <pwd.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>

#include "mdm-common.h"
#include "mdm-log.h"

static void
test_log (void)
{
        mdm_log_init ();

        g_debug ("Test debug 1");
        mdm_log_set_debug (TRUE);
        g_debug ("Test debug 2");

        g_message ("Test message");
        g_warning ("Test warning");
        g_critical ("Test critical");
        g_error ("Test error");
}

int
main (int argc, char **argv)
{

        test_log ();

        return 0;
}
