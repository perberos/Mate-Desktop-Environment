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


#ifndef __MDM_SLAVE_H
#define __MDM_SLAVE_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_SLAVE         (mdm_slave_get_type ())
#define MDM_SLAVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SLAVE, MdmSlave))
#define MDM_SLAVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SLAVE, MdmSlaveClass))
#define MDM_IS_SLAVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SLAVE))
#define MDM_IS_SLAVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SLAVE))
#define MDM_SLAVE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SLAVE, MdmSlaveClass))

typedef struct MdmSlavePrivate MdmSlavePrivate;

typedef struct
{
        GObject          parent;
        MdmSlavePrivate *priv;
} MdmSlave;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*start) (MdmSlave *slave);
        gboolean (*stop)  (MdmSlave *slave);

        /* signals */
        void (*stopped) (MdmSlave *slave);
} MdmSlaveClass;

GType               mdm_slave_get_type               (void);
gboolean            mdm_slave_start                  (MdmSlave   *slave);
gboolean            mdm_slave_stop                   (MdmSlave   *slave);

char *              mdm_slave_get_primary_session_id_for_user (MdmSlave   *slave,
                                                               const char *username);

gboolean            mdm_slave_get_timed_login_details (MdmSlave  *slave,
                                                       gboolean  *enabled,
                                                       char     **username,
                                                       int       *delay);

gboolean            mdm_slave_add_user_authorization (MdmSlave   *slave,
                                                      const char *username,
                                                      char      **filename);

gboolean            mdm_slave_switch_to_user_session (MdmSlave   *slave,
                                                      const char *username);

gboolean            mdm_slave_connect_to_x11_display (MdmSlave   *slave);
void                mdm_slave_set_busy_cursor        (MdmSlave   *slave);
gboolean            mdm_slave_run_script             (MdmSlave   *slave,
                                                      const char *dir,
                                                      const char *username);
void                mdm_slave_stopped                (MdmSlave   *slave);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_SLAVE_H */
