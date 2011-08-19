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


#ifndef __GDM_SLAVE_H
#define __GDM_SLAVE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SLAVE         (gdm_slave_get_type ())
#define GDM_SLAVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SLAVE, GdmSlave))
#define GDM_SLAVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SLAVE, GdmSlaveClass))
#define GDM_IS_SLAVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SLAVE))
#define GDM_IS_SLAVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SLAVE))
#define GDM_SLAVE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SLAVE, GdmSlaveClass))

typedef struct GdmSlavePrivate GdmSlavePrivate;

typedef struct
{
        GObject          parent;
        GdmSlavePrivate *priv;
} GdmSlave;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*start) (GdmSlave *slave);
        gboolean (*stop)  (GdmSlave *slave);

        /* signals */
        void (*stopped) (GdmSlave *slave);
} GdmSlaveClass;

GType               gdm_slave_get_type               (void);
gboolean            gdm_slave_start                  (GdmSlave   *slave);
gboolean            gdm_slave_stop                   (GdmSlave   *slave);

char *              gdm_slave_get_primary_session_id_for_user (GdmSlave   *slave,
                                                               const char *username);

gboolean            gdm_slave_get_timed_login_details (GdmSlave  *slave,
                                                       gboolean  *enabled,
                                                       char     **username,
                                                       int       *delay);

gboolean            gdm_slave_add_user_authorization (GdmSlave   *slave,
                                                      const char *username,
                                                      char      **filename);

gboolean            gdm_slave_switch_to_user_session (GdmSlave   *slave,
                                                      const char *username);

gboolean            gdm_slave_connect_to_x11_display (GdmSlave   *slave);
void                gdm_slave_set_busy_cursor        (GdmSlave   *slave);
gboolean            gdm_slave_run_script             (GdmSlave   *slave,
                                                      const char *dir,
                                                      const char *username);
void                gdm_slave_stopped                (GdmSlave   *slave);

G_END_DECLS

#endif /* __GDM_SLAVE_H */
