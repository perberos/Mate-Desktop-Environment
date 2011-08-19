/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2002 Fernando Herrera <fherrera@onirica.com>
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

#ifndef __MATECONF_SEARCH_DIALOG_H__
#define __MATECONF_SEARCH_DIALOG_H__

#include <gtk/gtk.h>

#define MATECONF_TYPE_SEARCH_DIALOG		  (mateconf_search_dialog_get_type ())
#define MATECONF_SEARCH_DIALOG(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_SEARCH_DIALOG, MateConfSearchDialog))
#define MATECONF_SEARCH_DIALOG_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_SEARCH_DIALOG, MateConfSearchDialogClass))
#define MATECONF_IS_SEARCH_DIALOG(obj)	          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_SEARCH_DIALOG))
#define MATECONF_IS_SEARCH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_SEARCH_DIALOG))
#define MATECONF_SEARCH_DIALOG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_SEARCH_DIALOG, MateConfSearchDialogClass))

typedef struct _MateConfSearchDialog MateConfSearchDialog;
typedef struct _MateConfSearchDialogClass MateConfSearchDialogClass;

struct _MateConfSearchDialog {
	GtkDialog parent_instance;

	GtkWidget *entry;
	GtkWidget *search_in_keys;
	GtkWidget *search_in_values;
	GtkWidget *search_button;
};

struct _MateConfSearchDialogClass {
	GtkDialogClass parent_class;
};

GType mateconf_search_dialog_get_type (void);
GtkWidget *mateconf_search_dialog_new (GtkWindow *parent);

#endif /* __MATECONF_SEARCH_DIALOG_H__ */

