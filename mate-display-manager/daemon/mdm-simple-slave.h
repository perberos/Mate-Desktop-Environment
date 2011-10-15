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


#ifndef __MDM_SIMPLE_SLAVE_H
#define __MDM_SIMPLE_SLAVE_H

#include <glib-object.h>
#include "mdm-slave.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_SIMPLE_SLAVE         (mdm_simple_slave_get_type ())
#define MDM_SIMPLE_SLAVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SIMPLE_SLAVE, MdmSimpleSlave))
#define MDM_SIMPLE_SLAVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SIMPLE_SLAVE, MdmSimpleSlaveClass))
#define MDM_IS_SIMPLE_SLAVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SIMPLE_SLAVE))
#define MDM_IS_SIMPLE_SLAVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SIMPLE_SLAVE))
#define MDM_SIMPLE_SLAVE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SIMPLE_SLAVE, MdmSimpleSlaveClass))

typedef struct MdmSimpleSlavePrivate MdmSimpleSlavePrivate;

typedef struct
{
        MdmSlave               parent;
        MdmSimpleSlavePrivate *priv;
} MdmSimpleSlave;

typedef struct
{
        MdmSlaveClass   parent_class;
} MdmSimpleSlaveClass;

GType               mdm_simple_slave_get_type   (void);
MdmSlave *          mdm_simple_slave_new        (const char       *id);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_SIMPLE_SLAVE_H */
