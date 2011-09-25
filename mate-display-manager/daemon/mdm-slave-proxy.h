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


#ifndef __MDM_SLAVE_PROXY_H
#define __MDM_SLAVE_PROXY_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_SLAVE_PROXY         (mdm_slave_proxy_get_type ())
#define MDM_SLAVE_PROXY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SLAVE_PROXY, MdmSlaveProxy))
#define MDM_SLAVE_PROXY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SLAVE_PROXY, MdmSlaveProxyClass))
#define MDM_IS_SLAVE_PROXY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SLAVE_PROXY))
#define MDM_IS_SLAVE_PROXY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SLAVE_PROXY))
#define MDM_SLAVE_PROXY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SLAVE_PROXY, MdmSlaveProxyClass))

typedef struct MdmSlaveProxyPrivate MdmSlaveProxyPrivate;

typedef struct
{
        GObject               parent;
        MdmSlaveProxyPrivate *priv;
} MdmSlaveProxy;

typedef struct
{
        GObjectClass   parent_class;
        void (* exited)            (MdmSlaveProxy  *proxy,
                                    int             exit_code);

        void (* died)              (MdmSlaveProxy  *proxy,
                                    int             signal_number);
} MdmSlaveProxyClass;

GType               mdm_slave_proxy_get_type     (void);
MdmSlaveProxy *     mdm_slave_proxy_new          (void);
void                mdm_slave_proxy_set_command  (MdmSlaveProxy *slave,
                                                  const char    *command);
void                mdm_slave_proxy_set_log_path (MdmSlaveProxy *slave,
                                                  const char    *path);
gboolean            mdm_slave_proxy_start        (MdmSlaveProxy *slave);
gboolean            mdm_slave_proxy_stop         (MdmSlaveProxy *slave);

G_END_DECLS

#endif /* __MDM_SLAVE_PROXY_H */
