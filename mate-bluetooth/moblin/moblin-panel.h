/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _MOBLIN_PANEL_H
#define _MOBLIN_PANEL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MOBLIN_TYPE_PANEL moblin_panel_get_type()

#define MOBLIN_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MOBLIN_TYPE_PANEL, MoblinPanel))

#define MOBLIN_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MOBLIN_TYPE_PANEL, MoblinPanelClass))

#define MOBLIN_IS_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MOBLIN_TYPE_PANEL))

#define MOBLIN_IS_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MOBLIN_TYPE_PANEL))

#define MOBLIN_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MOBLIN_TYPE_PANEL, MoblinPanelClass))

typedef struct _MoblinPanel MoblinPanel;
typedef struct _MoblinPanelClass MoblinPanelClass;
typedef struct _MoblinPanelPrivate MoblinPanelPrivate;

struct _MoblinPanel
{
  GtkHBox parent;
};

struct _MoblinPanelClass
{
  GtkHBoxClass parent_class;
  MoblinPanelPrivate *priv;

  void (*status_connecting) (MoblinPanel *self, gboolean connecting);
};

GType moblin_panel_get_type (void);

GtkWidget *moblin_panel_new (void);

void moblin_panel_reset_view (MoblinPanel *self);

G_END_DECLS

#endif /* _MOBLIN_PANEL_H */
