/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2005 James M. Cape <jcape@ignore-your.tv>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef __MDM_ENTRY_MENU_ITEM__
#define __MDM_ENTRY_MENU_ITEM__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_ENTRY_MENU_ITEM \
  (mdm_entry_menu_item_get_type ())
#define MDM_ENTRY_MENU_ITEM(object) \
  (G_TYPE_CHECK_INSTANCE_CAST ((object), MDM_TYPE_ENTRY_MENU_ITEM, MdmEntryMenuItem))
#define MDM_ENTRY_MENU_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_ENTRY_MENU_ITEM, MdmEntryMenuItemClass))
#define MDM_IS_ENTRY_MENU_ITEM(object) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((object), MDM_TYPE_ENTRY_MENU_ITEM))
#define MDM_IS_ENTRY_MENU_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_ENTRY_MENU_ITEM))
#define MDM_ENTRY_MENU_ITEM_GET_CLASS(object) \
  (G_TYPE_INSTANCE_GET_CLASS ((object), MDM_TYPE_ENTRY_MENU_ITEM, MdmEntryMenuItemClass))

typedef struct _MdmEntryMenuItem MdmEntryMenuItem;
typedef struct _MdmEntryMenuItemClass MdmEntryMenuItemClass;

GType      mdm_entry_menu_item_get_type      (void) G_GNUC_CONST;

GtkWidget *mdm_entry_menu_item_new           (void);
GtkWidget *mdm_entry_menu_item_get_entry     (MdmEntryMenuItem *item);
GtkWidget *mdm_entry_menu_item_get_image     (MdmEntryMenuItem *item);

G_END_DECLS

#endif /* __MDM_ENTRY_MENU_ITEM__ */
