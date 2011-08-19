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
 * Authors: Carlos Garnacho <garparr@teleline.es>.
 */

#ifndef _TABLE_H
#define _TABLE_H

#include <gtk/gtk.h>

enum {
	COL_ACTIVE,
	COL_DESC,
	COL_TOOLTIP,
	COL_IMAGE,
	COL_DANGEROUS,
	COL_OBJECT,
	COL_ITER,
	COL_LAST
};

enum {
	POPUP_SETTINGS
};

void			table_create				(void);
void                    table_empty                             (void);

#endif /* _TABLE_H */
