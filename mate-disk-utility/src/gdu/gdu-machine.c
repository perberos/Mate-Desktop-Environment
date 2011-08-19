/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-machine.c
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>
#include <dbus/dbus-glib.h>
#include <stdlib.h>

#include "gdu-private.h"
#include "gdu-util.h"
#include "gdu-pool.h"
#include "gdu-adapter.h"
#include "gdu-expander.h"
#include "gdu-machine.h"
#include "gdu-presentable.h"
#include "gdu-linux-md-drive.h"

/**
 * SECTION:gdu-machine
 * @title: GduMachine
 * @short_description: Machines
 *
 * #GduMachine objects are used to represent connections to a udisk
 * instance on a local or remote machine.
 *
 * See the documentation for #GduPresentable for the big picture.
 */

struct _GduMachinePrivate
{
        GduPool *pool;
        gchar *id;
};

static GObjectClass *parent_class = NULL;

static void gdu_machine_presentable_iface_init (GduPresentableIface *iface);
G_DEFINE_TYPE_WITH_CODE (GduMachine, gdu_machine, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_machine_presentable_iface_init))


static void
gdu_machine_finalize (GObject *object)
{
        GduMachine *machine = GDU_MACHINE (object);

        g_free (machine->priv->id);

        if (machine->priv->pool != NULL)
                g_object_unref (machine->priv->pool);

        if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gdu_machine_class_init (GduMachineClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = gdu_machine_finalize;

        g_type_class_add_private (klass, sizeof (GduMachinePrivate));
}

static void
gdu_machine_init (GduMachine *machine)
{
        machine->priv = G_TYPE_INSTANCE_GET_PRIVATE (machine, GDU_TYPE_MACHINE, GduMachinePrivate);
}

GduMachine *
_gdu_machine_new (GduPool *pool)
{
        GduMachine *machine;
        const gchar *ssh_user_name;
        const gchar *ssh_address;
        static guint count = 0;

        machine = GDU_MACHINE (g_object_new (GDU_TYPE_MACHINE, NULL));
        machine->priv->pool = g_object_ref (pool);

        ssh_user_name = gdu_pool_get_ssh_user_name (machine->priv->pool);
        if (ssh_user_name == NULL)
                ssh_user_name = g_get_user_name ();
        ssh_address = gdu_pool_get_ssh_address (machine->priv->pool);

        if (ssh_address != NULL) {
                machine->priv->id = g_strdup_printf ("__machine_root_%d_for_%s@%s__",
                                                     count++,
                                                     ssh_user_name,
                                                     ssh_address);
        } else {
                machine->priv->id = g_strdup_printf ("__machine_root_%d__", count++);
        }
        return machine;
}

static const gchar *
gdu_machine_get_id (GduPresentable *presentable)
{
        GduMachine *machine = GDU_MACHINE (presentable);
        return machine->priv->id;
}

static GduDevice *
gdu_machine_get_device (GduPresentable *presentable)
{
        return NULL;
}

static GduPresentable *
gdu_machine_get_enclosing_presentable (GduPresentable *presentable)
{
        return NULL;
}

static char *
gdu_machine_get_name (GduPresentable *presentable)
{
        GduMachine *machine = GDU_MACHINE (presentable);
        const gchar *ssh_address;
        gchar *ret;

        ssh_address = gdu_pool_get_ssh_address (machine->priv->pool);

        if (ssh_address == NULL) {
                ret = g_strdup (_("Local Storage"));
        } else {
                /* TODO: use display-hostname */
                ret = g_strdup_printf (_("Storage on %s"), ssh_address);
        }

        return ret;
}

static gchar *
gdu_machine_get_vpd_name (GduPresentable *presentable)
{
        GduMachine *machine = GDU_MACHINE (presentable);
        const gchar *ssh_user_name;
        const gchar *ssh_address;
        gchar *ret;

        ssh_user_name = gdu_pool_get_ssh_user_name (machine->priv->pool);
        if (ssh_user_name == NULL)
                ssh_user_name = g_get_user_name ();
        ssh_address = gdu_pool_get_ssh_address (machine->priv->pool);

        if (ssh_address == NULL) {
                ret = g_strdup_printf ("%s@localhost", ssh_user_name);
        } else {
                /* TODO: include IP address */
                ret = g_strdup_printf ("%s@%s", ssh_user_name, ssh_address);
        }

        /* TODO: include udisks and OS version numbers? */

        return ret;

}

static gchar *
gdu_machine_get_description (GduPresentable *presentable)
{
        return gdu_machine_get_vpd_name (presentable);
}

static GIcon *
gdu_machine_get_icon (GduPresentable *presentable)
{
        GIcon *icon;
        icon = g_themed_icon_new_with_default_fallbacks ("computer"); /* TODO? */
        return icon;
}

static guint64
gdu_machine_get_offset (GduPresentable *presentable)
{
        return 0;
}

static guint64
gdu_machine_get_size (GduPresentable *presentable)
{
        return 0;
}

static GduPool *
gdu_machine_get_pool (GduPresentable *presentable)
{
        GduMachine *machine = GDU_MACHINE (presentable);
        return g_object_ref (machine->priv->pool);
}

static gboolean
gdu_machine_is_allocated (GduPresentable *presentable)
{
        return FALSE;
}

static gboolean
gdu_machine_is_recognized (GduPresentable *presentable)
{
        return FALSE;
}

static void
gdu_machine_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_machine_get_id;
        iface->get_device                = gdu_machine_get_device;
        iface->get_enclosing_presentable = gdu_machine_get_enclosing_presentable;
        iface->get_name                  = gdu_machine_get_name;
        iface->get_description           = gdu_machine_get_description;
        iface->get_vpd_name              = gdu_machine_get_vpd_name;
        iface->get_icon                  = gdu_machine_get_icon;
        iface->get_offset                = gdu_machine_get_offset;
        iface->get_size                  = gdu_machine_get_size;
        iface->get_pool                  = gdu_machine_get_pool;
        iface->is_allocated              = gdu_machine_is_allocated;
        iface->is_recognized             = gdu_machine_is_recognized;
}
