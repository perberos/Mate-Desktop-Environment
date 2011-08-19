/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  caja-gdu.h
 *
 *  Copyright (C) 2008-2009 Red Hat, Inc.
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
 *  Author: Tomas Bzatek <tbzatek@redhat.com>
 *
 */

#ifndef CAJA_GDU_H
#define CAJA_GDU_H

#include <glib-object.h>
#include <libcaja-extension/caja-menu-provider.h>
#include <gdu/gdu.h>

G_BEGIN_DECLS

#define CAJA_TYPE_GDU	  (caja_gdu_get_type ())
#define CAJA_GDU(o)		  (G_TYPE_CHECK_INSTANCE_CAST ((o), CAJA_TYPE_GDU, CajaGdu))
#define CAJA_IS_GDU(o)	  (G_TYPE_CHECK_INSTANCE_TYPE ((o), CAJA_TYPE_GDU))

typedef struct _CajaGdu      CajaGdu;
typedef struct _CajaGduClass CajaGduClass;

struct _CajaGdu
{
	GObject parent;
};

struct _CajaGduClass
{
	GObjectClass parent_class;
};

GType caja_gdu_get_type      (void) G_GNUC_CONST;
void  caja_gdu_register_type (GTypeModule *module);

G_END_DECLS

#endif  /* CAJA_GDU_H */

