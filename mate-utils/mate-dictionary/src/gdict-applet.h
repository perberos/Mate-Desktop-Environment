/* gdict-applet.h - MATE Dictionary Applet
 *
 * Copyright (c) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GDICT_APPLET_H__
#define __GDICT_APPLET_H__

#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <libgdict/gdict.h>

G_BEGIN_DECLS

#define GDICT_TYPE_APPLET		(gdict_applet_get_type ())
#define GDICT_APPLET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_APPLET, GdictApplet))
#define GDICT_IS_APPLET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_APPLET))

typedef struct _GdictApplet        GdictApplet;
typedef struct _GdictAppletClass   GdictAppletClass;
typedef struct _GdictAppletPrivate GdictAppletPrivate;

struct _GdictApplet
{
  MatePanelApplet parent_instance;
  
  GdictAppletPrivate *priv;  
};

GType gdict_applet_get_type (void);

G_END_DECLS

#endif /* __GDICT_APPLET_H__ */
