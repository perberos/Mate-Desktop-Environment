/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#ifndef __MODEM_APPLET_H
#define __MODEM_APPLET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mate-panel-applet.h>

#define TYPE_MODEM_APPLET           (modem_applet_get_type ())
#define MODEM_APPLET(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MODEM_APPLET, ModemApplet))
#define MODEM_APPLET_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST    ((obj), TYPE_MODEM_APPLET, ModemAppletClass))
#define IS_MODEM_APPLET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MODEM_APPLET))
#define IS_MODEM_APPLET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE    ((obj), TYPE_MODEM_APPLET))
#define MODEM_APPLET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS  ((obj), TYPE_MODEM_APPLET, ModemAppletClass))

typedef struct _ModemApplet      ModemApplet;
typedef struct _ModemAppletClass ModemAppletClass;

struct _ModemApplet {
  MatePanelApplet parent;
};

struct _ModemAppletClass {
  MatePanelAppletClass parent_class;
};

#ifdef __cplusplus
}
#endif

#endif /* __MODEM_APPLET_H */
