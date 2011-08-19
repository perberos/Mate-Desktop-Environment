/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * (C) Copyright 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BLUETOOTH_CHOOSER_COMBO_H__
#define __BLUETOOTH_CHOOSER_COMBO_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BLUETOOTH_TYPE_CHOOSER_COMBO     (bluetooth_chooser_combo_get_type ())
#define BLUETOOTH_CHOOSER_COMBO(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BLUETOOTH_TYPE_CHOOSER_COMBO, BluetoothChooserCombo))
#define BLUETOOTH_IS_CHOOSER_COMBO(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BLUETOOTH_TYPE_CHOOSER_COMBO))

#define BLUETOOTH_CHOOSER_COMBO_FIRST_DEVICE "00:00:00:00:00:00"

typedef struct _BluetoothChooserCombo BluetoothChooserCombo;

typedef struct _BluetoothChooserComboClass {
  GtkVBoxClass parent_class;

  void (*chooser_created) (BluetoothChooserCombo *self, GtkWidget *chooser);
} BluetoothChooserComboClass;

GType		bluetooth_chooser_combo_get_type	(void);

GtkWidget *	bluetooth_chooser_combo_new		(void);

G_END_DECLS

#endif /* __BLUETOOTH_CHOOSER_COMBO_H__ */
