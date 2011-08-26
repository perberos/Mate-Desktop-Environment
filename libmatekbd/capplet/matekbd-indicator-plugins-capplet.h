/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MATEKBD_INDICATOR_PLUGINS_CAPPLET_H__
#define __MATEKBD_INDICATOR_PLUGINS_CAPPLET_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "libmatekbd/matekbd-desktop-config.h"
#include "libmatekbd/matekbd-indicator-config.h"
#include "libmatekbd/matekbd-keyboard-config.h"

#include "libmatekbd/matekbd-indicator-plugin-manager.h"
#include "libmatekbd/matekbd-util.h"

typedef struct _MatekbdIndicatorPluginsCapplet {
	MatekbdIndicatorPluginContainer plugin_container;
	MatekbdDesktopConfig cfg;
	MatekbdIndicatorConfig applet_cfg;
	MatekbdKeyboardConfig kbd_cfg;
	MatekbdIndicatorPluginManager plugin_manager;
	XklEngine *engine;
	XklConfigRegistry *config_registry;

	GtkWidget *capplet;
} MatekbdIndicatorPluginsCapplet;

#define NAME_COLUMN 0
#define FULLPATH_COLUMN 1

#define CappletGetUiWidget( gipc, name ) \
  GTK_WIDGET ( gtk_builder_get_object ( \
    GTK_BUILDER( g_object_get_data( G_OBJECT( (gipc)->capplet ), "uiData" ) ), \
    name ) )

extern void CappletFillActivePluginList (MatekbdIndicatorPluginsCapplet *
					 gipc);

extern char *CappletGetSelectedPluginPath (GtkTreeView * plugins_list,
					   MatekbdIndicatorPluginsCapplet *
					   gipc);

extern void CappletEnablePlugin (GtkWidget * btnAdd,
				 MatekbdIndicatorPluginsCapplet * gipc);

#endif
