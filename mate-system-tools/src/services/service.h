/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of services-admin, a mate-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho <carlosg@mate.org>.
 */

#ifndef __SERVICE_H
#define __SERVICE_H

#include <glib.h>
#include <oobs/oobs.h>

typedef struct _ServiceDescription ServiceDescription;

struct _ServiceDescription {
	gboolean dangerous;
	gchar *icon;
	gchar *description;
	gchar *long_description;
};

const ServiceDescription *service_search (OobsService *service);

#endif /* __SERVICE_H */
