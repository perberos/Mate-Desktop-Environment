/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Fernando Herrera <fherrera@onirica.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __MATECONF_SEARCH_H__
#define __MATECONF_SEARCH_H__

#include "mateconf-editor-window.h"
#include "gedit-output-window.h"
#include "mateconf-tree-model.h"
#include "mateconf-list-model.h"
#include <gtk/gtk.h>

typedef struct _SearchIter SearchIter;

struct _SearchIter {
	const char *pattern;
	gboolean search_values;
	gboolean search_keys;
	GeditOutputWindow *output_window;
	GObject *searching;
	int res;
};

int mateconf_tree_model_build_match_list (MateConfTreeModel *tree_model, GeditOutputWindow *output_window,
				       const char *pattern, gboolean search_keys, gboolean search_values, 
				       GObject *dialog);



#endif /* __MATECONF_SEARCH_H__ */
