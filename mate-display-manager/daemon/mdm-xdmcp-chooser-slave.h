/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __MDM_XDMCP_CHOOSER_SLAVE_H
#define __MDM_XDMCP_CHOOSER_SLAVE_H

#include <glib-object.h>
#include "mdm-slave.h"

G_BEGIN_DECLS

#define MDM_TYPE_XDMCP_CHOOSER_SLAVE         (mdm_xdmcp_chooser_slave_get_type ())
#define MDM_XDMCP_CHOOSER_SLAVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_XDMCP_CHOOSER_SLAVE, MdmXdmcpChooserSlave))
#define MDM_XDMCP_CHOOSER_SLAVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_XDMCP_CHOOSER_SLAVE, MdmXdmcpChooserSlaveClass))
#define MDM_IS_XDMCP_CHOOSER_SLAVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_XDMCP_CHOOSER_SLAVE))
#define MDM_IS_XDMCP_CHOOSER_SLAVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_XDMCP_CHOOSER_SLAVE))
#define MDM_XDMCP_CHOOSER_SLAVE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_XDMCP_CHOOSER_SLAVE, MdmXdmcpChooserSlaveClass))

typedef struct MdmXdmcpChooserSlavePrivate MdmXdmcpChooserSlavePrivate;

typedef struct
{
        MdmSlave                     parent;
        MdmXdmcpChooserSlavePrivate *priv;
} MdmXdmcpChooserSlave;

typedef struct
{
        MdmSlaveClass   parent_class;

        void (* hostname_selected)          (MdmXdmcpChooserSlave  *slave,
                                             const char            *hostname);
} MdmXdmcpChooserSlaveClass;

GType               mdm_xdmcp_chooser_slave_get_type   (void);
MdmSlave *          mdm_xdmcp_chooser_slave_new        (const char       *id);

G_END_DECLS

#endif /* __MDM_XDMCP_CHOOSER_SLAVE_H */
