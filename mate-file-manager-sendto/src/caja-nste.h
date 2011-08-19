/*
 *  Caja SendTo extension 
 * 
 *  Copyright (C) 2005 Roberto Majadas 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Roberto Majadas <roberto.majadas@openshine.com> 
 * 
 */

#ifndef CAJA_NSTE_H
#define CAJA_NSTE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define CAJA_TYPE_NSTE  (caja_nste_get_type ())
#define CAJA_NSTE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), CAJA_TYPE_NSTE, CajaNste))
#define CAJA_IS_NSTE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CAJA_TYPE_NSTE))

typedef struct _CajaNste      CajaNste;
typedef struct _CajaNsteClass CajaNsteClass;

struct _CajaNste {
	GObject __parent;
};

struct _CajaNsteClass {
	GObjectClass __parent;
};

GType caja_nste_get_type      (void);
void  caja_nste_register_type (GTypeModule *module);

G_END_DECLS

#endif /* CAJA_NSTE_H */
