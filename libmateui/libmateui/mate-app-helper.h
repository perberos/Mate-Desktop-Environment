/*
 * Copyright (C) 1998, 1999, 2000 Red Hat, Inc.
 * Copyright (C) 1998, 1999 Miguel de Icaza, Federico Mena, Chris Toshok.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/*
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data,
 * Marc Ewing added menu support, toggle and radio support, and I
 * don't know what you other people did :) menu insertion/removal
 * functions by Jaka Mocnik.  Standard menu items by George Lebl and
 * Nat Friedman.
 *
 * Some subtree hackage (and possibly various justification hacks) by Justin 
 * Maurer.
 *
 * Major cleanups and rearrangements by Federico Mena and Justin Maurer.  */

#ifndef MATE_APP_HELPER_H
#define MATE_APP_HELPER_H

#include <gtk/gtk.h>

#include <libmateui/mate-appbar.h>
#include <libmateui/mate-app.h>

G_BEGIN_DECLS

/* This module lets you easily create menus and toolbars for your 
 * applications. You basically define a hierarchy of arrays of MateUIInfo 
 * structures, and you later call the provided functions to create menu bars 
 * or tool bars.
 */

/* These values identify the item type that a particular MateUIInfo structure 
 * specifies */

/**
 * MateUIInfoType:
 * @MATE_APP_UI_ENDOFINFO: No more items, use it at the end of an array.
 * @MATE_APP_UI_ITEM: Normal item, or radio item if it is inside a radioitems
 * group.
 * @MATE_APP_UI_TOGGLEITEM: Toggle (check box) item.
 * @MATE_APP_UI_RADIOITEMS: Radio item group.
 * @MATE_APP_UI_SUBTREE: Item that defines a subtree/submenu.
 * @MATE_APP_UI_SEPARATOR: Separator line (menus) or blank space (toolbars).
 * @MATE_APP_UI_HELP: Create a list of help topics, used in the Help menu.
 * @MATE_APP_UI_BUILDER_DATA: Specifies the builder data for the following
 * entries, see code for further info.
 * @MATE_APP_UI_ITEM_CONFIGURABLE: A configurable menu item.
 * @MATE_APP_UI_SUBTREE_STOCK: Item that defines a subtree/submenu, same as
 * #MATE_APP_UI_SUBTREE, but the texts should be looked up in the libmate
 * catalog.
 * @MATE_APP_UI_INCLUDE: Almost like @MATE_APP_UI_SUBTREE, but inserts items
 * into the current menu or whatever, instead of making a submenu.
 *
 * These values identify the item type that a particular MateUIInfo structure
 * specifies.
 *
 * One should be careful when using mate_app_create_[custom|interp|with_data]
 * functions with #MateUIInfo arrays containing #MATE_APP_UI_BUILDER_DATA
 * items, since their #MateUIBuilderData structures completely override the
 * ones generated or supplied by the above functions.
 */
typedef enum {
	MATE_APP_UI_ENDOFINFO,
	MATE_APP_UI_ITEM,
	MATE_APP_UI_TOGGLEITEM,
	MATE_APP_UI_RADIOITEMS,
	MATE_APP_UI_SUBTREE,
	MATE_APP_UI_SEPARATOR,
	MATE_APP_UI_HELP,
	MATE_APP_UI_BUILDER_DATA,
	MATE_APP_UI_ITEM_CONFIGURABLE,
	MATE_APP_UI_SUBTREE_STOCK,
	MATE_APP_UI_INCLUDE
} MateUIInfoType;

/* If you insert a value into this enum it'll break configurations all
   over the place.  Only append.  You should also append a matching
   item in the default types near the top of mate-app-helper.c */

/**
 * MateUIInfoConfigurableTypes:
 * @MATE_APP_CONFIGURABLE_ITEM_NEW: The "New" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_OPEN: The "Open" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_SAVE: The "Save" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_SAVE_AS: The "Save as..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_REVERT: The "Revert" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PRINT: The "Print" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PRINT_SETUP: The "Print setup..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_CLOSE: The "Close" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_QUIT: The "Quit" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_CUT: The "Cut" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_COPY: The "Copy" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PASTE: The "Paste" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_CLEAR: The "Clear" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_UNDO: The "Undo" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_REDO: The "Redo" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_FIND: The "Find..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_FIND_AGAIN: The "Find again" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_REPLACE: The "Replace..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PROPERTIES: The "Properties..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PREFERENCES: The "Preferences..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_ABOUT: The "About..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_SELECT_ALL: The "Select all" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_NEW_WINDOW: The "New window" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_CLOSE_WINDOW: The "Close window" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_NEW_GAME: The "New game" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_PAUSE_GAME: The "Pause game" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_RESTART_GAME: The "Restart game" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_UNDO_MOVE: The "Undo move" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_REDO_MOVE: The "Redo move" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_HINT: The "Hint" menu.
 * @MATE_APP_CONFIGURABLE_ITEM_SCORES: The "Scores..." menu.
 * @MATE_APP_CONFIGURABLE_ITEM_END_GAME: The "End game" menu.
 *
 * A user can redefine the accelerator keys for each menu item (if the
 * application supports this). This enum gives an identifier for each menu
 * shortcut that can possible be redefined. If an application is not using one
 * of these accelerators, then no shortcut redefinition is possible unless the
 * application specifically implements it (moral: use standard menu items).
 */
typedef enum {
        /* 0 */
        MATE_APP_CONFIGURABLE_ITEM_NEW,
        MATE_APP_CONFIGURABLE_ITEM_OPEN,
        MATE_APP_CONFIGURABLE_ITEM_SAVE,
        MATE_APP_CONFIGURABLE_ITEM_SAVE_AS,
        MATE_APP_CONFIGURABLE_ITEM_REVERT,
        MATE_APP_CONFIGURABLE_ITEM_PRINT,
        MATE_APP_CONFIGURABLE_ITEM_PRINT_SETUP,
        MATE_APP_CONFIGURABLE_ITEM_CLOSE,
        MATE_APP_CONFIGURABLE_ITEM_QUIT,
        MATE_APP_CONFIGURABLE_ITEM_CUT,
	/* 10 */
        MATE_APP_CONFIGURABLE_ITEM_COPY,
        MATE_APP_CONFIGURABLE_ITEM_PASTE,
        MATE_APP_CONFIGURABLE_ITEM_CLEAR,
        MATE_APP_CONFIGURABLE_ITEM_UNDO,
        MATE_APP_CONFIGURABLE_ITEM_REDO,
        MATE_APP_CONFIGURABLE_ITEM_FIND,
        MATE_APP_CONFIGURABLE_ITEM_FIND_AGAIN,
        MATE_APP_CONFIGURABLE_ITEM_REPLACE,
        MATE_APP_CONFIGURABLE_ITEM_PROPERTIES,
        MATE_APP_CONFIGURABLE_ITEM_PREFERENCES,
	/* 20 */
        MATE_APP_CONFIGURABLE_ITEM_ABOUT,
	MATE_APP_CONFIGURABLE_ITEM_SELECT_ALL,
	MATE_APP_CONFIGURABLE_ITEM_NEW_WINDOW,
	MATE_APP_CONFIGURABLE_ITEM_CLOSE_WINDOW,
	MATE_APP_CONFIGURABLE_ITEM_NEW_GAME,
	MATE_APP_CONFIGURABLE_ITEM_PAUSE_GAME,
	MATE_APP_CONFIGURABLE_ITEM_RESTART_GAME,
	MATE_APP_CONFIGURABLE_ITEM_UNDO_MOVE,
	MATE_APP_CONFIGURABLE_ITEM_REDO_MOVE,
	MATE_APP_CONFIGURABLE_ITEM_HINT,
	/* 30 */
	MATE_APP_CONFIGURABLE_ITEM_SCORES,
	MATE_APP_CONFIGURABLE_ITEM_END_GAME
} MateUIInfoConfigurableTypes;

#define MATE_APP_CONFIGURABLE_ITEM_EXIT	MATE_APP_CONFIGURABLE_ITEM_QUIT

/**
 * MateUIPixmapType:
 * @MATE_APP_PIXMAP_NONE: No pixmap specified.
 * @MATE_APP_PIXMAP_STOCK: Use a stock pixmap (#MateStock).
 * @MATE_APP_PIXMAP_DATA: Use a pixmap from inline xpm data.
 * @MATE_APP_PIXMAP_FILENAME: Use a pixmap from the specified filename.
 *
 * These values identify the type of pixmap used in an item.
 */
typedef enum {
	MATE_APP_PIXMAP_NONE,
	MATE_APP_PIXMAP_STOCK,
	MATE_APP_PIXMAP_DATA,
	MATE_APP_PIXMAP_FILENAME
} MateUIPixmapType;

typedef struct {
	MateUIInfoType type;		/* Type of item */
	gchar const *label;		/* String to use in item's label */
	gchar const *hint;		/* Tooltip for toolbar items, status 
					   bar message for menu items. */
	gpointer moreinfo;		/* Extra information; depends on the
					   type. */
	gpointer user_data;		/* User data sent to the callback. */
	gpointer unused_data;		/* Should be NULL (reserved). */
	MateUIPixmapType pixmap_type;	/* Type of pixmap for this item. */
	gconstpointer pixmap_info;	/* Pointer to pixmap information. */
	guint accelerator_key;		/* Accelerator key, or 0 for none. */
	GdkModifierType ac_mods;	/* Mask of modifier keys for the 
					   accelerator. */
	GtkWidget *widget;		/* Filled in by the mate_app_create* 
					   functions. */
} MateUIInfo;

/* Callback data */

#define MATEUIINFO_KEY_UIDATA		"uidata"
#define MATEUIINFO_KEY_UIBDATA		"uibdata"

/* Handy MateUIInfo macros */

/* Used to terminate an array of MateUIInfo structures */
#define MATEUIINFO_END			{ MATE_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,		\
					  (MateUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert a separator line (on a menu) or a blank space (on a toolbar) */
#define MATEUIINFO_SEPARATOR		{ MATE_APP_UI_SEPARATOR, NULL, NULL, NULL, NULL, NULL,		\
					  (MateUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert an item with an inline xpm icon */
#define MATEUIINFO_ITEM(label, tooltip, callback, xpm_data) \
	{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback, NULL, NULL, \
		MATE_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL}

/* Insert an item with a stock icon */
#define MATEUIINFO_ITEM_STOCK(label, tooltip, callback, stock_id) \
	{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback, NULL, NULL, \
		MATE_APP_PIXMAP_STOCK, stock_id, 0, (GdkModifierType) 0, NULL }

/* Insert an item with no icon */
#define MATEUIINFO_ITEM_NONE(label, tooltip, callback) \
	{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback, NULL, NULL, \
		MATE_APP_PIXMAP_NONE, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert an item with an inline xpm icon and a user data pointer */
#define MATEUIINFO_ITEM_DATA(label, tooltip, callback, user_data, xpm_data) \
	{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback, user_data, NULL, \
		MATE_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL }

/* Insert a toggle item (check box) with an inline xpm icon */
#define MATEUIINFO_TOGGLEITEM(label, tooltip, callback, xpm_data) \
	{ MATE_APP_UI_TOGGLEITEM, label, tooltip, (gpointer)callback, NULL, NULL, \
		MATE_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL }

/* Insert a toggle item (check box) with an inline xpm icon and a user data pointer */
#define MATEUIINFO_TOGGLEITEM_DATA(label, tooltip, callback, user_data, xpm_data)		\
					{ MATE_APP_UI_TOGGLEITEM, label, tooltip, (gpointer)callback,	\
					  user_data, NULL, MATE_APP_PIXMAP_DATA, xpm_data,	\
					  0, (GdkModifierType) 0, NULL }

/* Insert all the help topics based on the application's id */
#define MATEUIINFO_HELP(app_name) \
	{ MATE_APP_UI_HELP, NULL, NULL, (void *)app_name, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a subtree (submenu) */
#define MATEUIINFO_SUBTREE(label, tree) \
	{ MATE_APP_UI_SUBTREE, label, NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a subtree with a hint */
#define MATEUIINFO_SUBTREE_HINT(label, hint, tree) \
	{ MATE_APP_UI_SUBTREE, label, hint, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a subtree (submenu) with a stock icon */
#define MATEUIINFO_SUBTREE_STOCK(label, tree, stock_id) \
	{ MATE_APP_UI_SUBTREE, label, NULL, tree, NULL, NULL, \
		MATE_APP_PIXMAP_STOCK, stock_id, 0, (GdkModifierType) 0, NULL }

#define MATEUIINFO_INCLUDE(tree) \
	{ MATE_APP_UI_INCLUDE, NULL, NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a list of radio items */
#define MATEUIINFO_RADIOLIST(list) \
	{ MATE_APP_UI_RADIOITEMS, NULL, NULL, list, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a radio item with an inline xpm icon */
#define MATEUIINFO_RADIOITEM(label, tooltip, callback, xpm_data) \
	{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback, NULL, NULL, \
		MATE_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL }

/* Insert a radio item with an inline xpm icon and a user data pointer */
#define MATEUIINFO_RADIOITEM_DATA(label, tooltip, callback, user_data, xpm_data)		\
					{ MATE_APP_UI_ITEM, label, tooltip, (gpointer)callback,	\
					  user_data, NULL, MATE_APP_PIXMAP_DATA, xpm_data,	\
					  0, (GdkModifierType) 0, NULL }
					  
/*
 * Stock menu item macros for some common menu items.  Please see
 * mate-libs/devel-docs/suggestions.txt about MATE menu standards.
 */

/*
 * The 'File' menu
 */

/*
 * Note: New item requires to to specify what is new, so you need
 * to specify the document type, so you need to supply the label
 * as well (it should start with "_New ")
 */
#define MATEUIINFO_MENU_NEW_ITEM(label, tip, cb, data)                     \
        { MATE_APP_UI_ITEM_CONFIGURABLE, label, tip,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_NEW, (GdkModifierType) 0, NULL }

/* If you have more then one New type, use this tree */
#define MATEUIINFO_MENU_NEW_SUBTREE(tree)                                  \
        { MATE_APP_UI_SUBTREE_STOCK, N_("_New"), NULL, tree, NULL, NULL,   \
          MATE_APP_PIXMAP_STOCK, MATE_STOCK_MENU_NEW,                     \
          MATE_KEY_NAME_NEW, MATE_KEY_MOD_NEW, NULL }

#define MATEUIINFO_MENU_OPEN_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_OPEN, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_SAVE_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_SAVE, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_SAVE_AS_ITEM(cb, data)                             \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_SAVE_AS, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_REVERT_ITEM(cb, data)                              \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_REVERT, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_PRINT_ITEM(cb, data)                               \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PRINT, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_PRINT_SETUP_ITEM(cb, data)                         \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PRINT_SETUP, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_CLOSE_ITEM(cb, data)                               \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_CLOSE, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_EXIT_ITEM(cb, data)	MATEUIINFO_MENU_QUIT_ITEM(cb, data)
#define MATEUIINFO_MENU_QUIT_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_QUIT, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_QUIT_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_QUIT, (GdkModifierType) 0, NULL }
/*
 * The "Edit" menu
 */

#define MATEUIINFO_MENU_CUT_ITEM(cb, data)                                 \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_CUT, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_COPY_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_COPY, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_PASTE_ITEM(cb, data)                               \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PASTE, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_SELECT_ALL_ITEM(cb, data)                          \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_SELECT_ALL, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_CLEAR_ITEM(cb, data)                               \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_CLEAR, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_UNDO_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_UNDO, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_REDO_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_REDO, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_FIND_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_FIND, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_FIND_AGAIN_ITEM(cb, data)                          \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_FIND_AGAIN, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_REPLACE_ITEM(cb, data)                             \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_REPLACE, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_PROPERTIES_ITEM(cb, data)                          \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PROPERTIES, (GdkModifierType) 0, NULL }

/*
 * The Settings menu
 */
#define MATEUIINFO_MENU_PREFERENCES_ITEM(cb, data)                         \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PREFERENCES, (GdkModifierType) 0, NULL }

/*
 * The Windows menu
 */
#define MATEUIINFO_MENU_NEW_WINDOW_ITEM(cb, data)                          \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_NEW_WINDOW, (GdkModifierType) 0, NULL }

#define MATEUIINFO_MENU_CLOSE_WINDOW_ITEM(cb, data)                        \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_CLOSE_WINDOW, (GdkModifierType) 0, NULL }

/*
 * And the "Help" menu
 */
#define MATEUIINFO_MENU_ABOUT_ITEM(cb, data)                               \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_ABOUT, (GdkModifierType) 0, NULL }

/* 
 * The "Game" menu
 */

#define MATEUIINFO_MENU_NEW_GAME_ITEM(cb, data)                            \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_NEW_GAME, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_PAUSE_GAME_ITEM(cb, data)                          \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_PAUSE_GAME, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_RESTART_GAME_ITEM(cb, data)                        \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_RESTART_GAME, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_UNDO_MOVE_ITEM(cb, data)                           \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_UNDO_MOVE, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_REDO_MOVE_ITEM(cb, data)                           \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_REDO_MOVE, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_HINT_ITEM(cb, data)                                \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_HINT, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_SCORES_ITEM(cb, data)                              \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_SCORES, (GdkModifierType) 0, NULL }
	  
#define MATEUIINFO_MENU_END_GAME_ITEM(cb, data)                            \
        { MATE_APP_UI_ITEM_CONFIGURABLE, NULL, NULL,                       \
          (gpointer)cb, (gpointer)(data), NULL,                             \
          MATE_APP_PIXMAP_NONE, NULL,                                      \
          MATE_APP_CONFIGURABLE_ITEM_END_GAME, (GdkModifierType) 0, NULL }

const gchar * mate_app_helper_gettext (const gchar *string);

#ifdef ENABLE_NLS
#define D_(x) dgettext (GETTEXT_PACKAGE, x)
#define L_(x) mate_app_helper_gettext(x)
#else
#define D_(x) x
#define L_(x) x
#endif

/* Some standard menus */
#define MATEUIINFO_MENU_FILE_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_File"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_EDIT_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_Edit"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_VIEW_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_View"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_SETTINGS_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_Settings"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_FILES_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("Fi_les"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_WINDOWS_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_Windows"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_HELP_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_Help"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
#define MATEUIINFO_MENU_GAME_TREE(tree) \
	{ MATE_APP_UI_SUBTREE_STOCK, N_("_Game"), NULL, tree, NULL, NULL, \
		(MateUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }
		
/*these are strings to be used for paths when working with the menus stuff*/
#define MATE_MENU_FILE_STRING D_("_File")
#define MATE_MENU_FILE_PATH D_("_File/")
#define MATE_MENU_EDIT_STRING D_("_Edit")
#define MATE_MENU_EDIT_PATH D_("_Edit/")
#define MATE_MENU_VIEW_STRING D_("_View")
#define MATE_MENU_VIEW_PATH D_("_View/")
#define MATE_MENU_SETTINGS_STRING D_("_Settings")
#define MATE_MENU_SETTINGS_PATH D_("_Settings/")
#define MATE_MENU_NEW_STRING D_("_New")
#define MATE_MENU_NEW_PATH D_("_New/")
#define MATE_MENU_FILES_STRING D_("Fi_les")
#define MATE_MENU_FILES_PATH D_("Fi_les/")
#define MATE_MENU_WINDOWS_STRING D_("_Windows")
#define MATE_MENU_WINDOWS_PATH D_("_Windows/")


/* Types useful to language bindings */
    
typedef struct _MateUIBuilderData MateUIBuilderData;

typedef void (* MateUISignalConnectFunc) (MateUIInfo        *uiinfo,
					   const char         *signal_name,
					   MateUIBuilderData *uibdata);

/**
 * MateUIBuilderData:
 * @connect_func: Function that connects to the item's signals.
 * @data: User data pointer for the signal callback.
 * @is_interp: If %TRUE, the signal should be connected with
 * g_signal_connect_closure_by_id(), otherwise, g_signal_connect() is used.
 * @relay_func: Marshaller function for language bindings.
 * @destroy_func: Destroy notification function for language bindings.
 *
 * This structure defines how the relevant menu items are to have their signals
 * connected. This includes the activations signals, as well as the destroy
 * notifications. The affected menu items are either the items following aA
 * #MATE_APP_UI_BUILDER_DATA item in an array of #MateUIInfo structures or
 * all   of the menu items that are connected as a result of a call to
 * mate_app_create_menu_custom() and similar functions.
 */
struct _MateUIBuilderData {
	MateUISignalConnectFunc connect_func;
	gpointer data;
	gboolean is_interp;
	GtkCallbackMarshal relay_func;
	GDestroyNotify destroy_func;
};

/* Flush the accelerator definitions into the application specific
 * configuration file ~/.mate/accels/<app-id>.
 */
void mate_accelerators_sync (void);
     

/* Fills the specified menu shell with items created from the specified
 * info, inserting them from the item no. pos on.
 * The accel group will be used as the accel group for all newly created
 * sub menus and serves as the global accel group for all menu item
 * hotkeys. If it is passed as NULL, global hotkeys will be disabled.
 * The uline_accels argument determines whether underline accelerators
 * will be featured from the menu item labels.
 */
void mate_app_fill_menu (GtkMenuShell	*menu_shell,
			  MateUIInfo	*uiinfo,
			  GtkAccelGroup	*accel_group,
			  gboolean	 uline_accels,
			  gint		 pos);

/* Same as mate_app_fill_menu, but sets all the user data pointers to
 * the specified value.
 */
void mate_app_fill_menu_with_data (GtkMenuShell	*menu_shell,
				    MateUIInfo		*uiinfo,
				    GtkAccelGroup	*accel_group,
				    gboolean		 uline_accels,
				    gint		 pos,
				    gpointer		 user_data);

/* Fills the specified menu shell with items created from the specified
 * info, inserting them from item no. pos on and using the specified
 * builder data -- this is intended for language bindings.
 * The accel group will be used as the accel group for all newly created
 * sub menus and serves as the global accel group for all menu item
 * hotkeys. If it is passed as NULL, global hotkeys will be disabled.
 * The uline_accels argument determines whether underline accelerators
 * will be featured from the menu item labels.
 */
void mate_app_fill_menu_custom (GtkMenuShell	    *menu_shell,
				 MateUIInfo	    *uiinfo,
				 MateUIBuilderData *uibdata,
				 GtkAccelGroup	    *accel_group,
				 gboolean	     uline_accels,
				 gint		     pos);

/* Converts MATE_APP_UI_ITEM_CONFIGURABLE menu MateUIInfos to the
   corresponding standard MATE_APP_UI_ITEMs.  mate_app_create_menus
   calls this for you, but if you need to alter something afterwards,
   you can call this function first.  The main reason for this being a
   public interface is so that it can be called from
   mate_popup_menu_new which clears a copy of the passed
   MateUIInofs. */
void mate_app_ui_configure_configurable (MateUIInfo* uiinfo);


/* Constructs a menu bar and attaches it to the specified application window */
void mate_app_create_menus (MateApp *app, MateUIInfo *uiinfo);

/* Constructs a menu bar and attaches it to the specified application window -- this version is
 * intended for language bindings.
 */
void mate_app_create_menus_interp (MateApp *app, MateUIInfo *uiinfo,
				    GtkCallbackMarshal relay_func, gpointer data,
				    GDestroyNotify destroy_func);

/* Constructs a menu bar, sets all the user data pointers to the specified value, and attaches it to
 * the specified application window.
 */
void mate_app_create_menus_with_data (MateApp *app, MateUIInfo *uiinfo, gpointer user_data);

/* Constructs a menu bar and attaches it to the specified application window, using the
 * specified builder data -- intended for language bindings.
 */
void mate_app_create_menus_custom (MateApp *app, MateUIInfo *uiinfo, MateUIBuilderData *uibdata);

/* Fills the specified toolbar with buttons created from the specified info.  If accel_group is not
 * NULL, then the items' accelerator keys are put into it.
 */
void mate_app_fill_toolbar (GtkToolbar *toolbar, MateUIInfo *uiinfo, GtkAccelGroup *accel_group);

/* Same as mate_app_fill_toolbar, but sets all the user data pointers
 * to the specified value.
 */
void mate_app_fill_toolbar_with_data (GtkToolbar *toolbar, MateUIInfo *uiinfo,
				       GtkAccelGroup *accel_group, gpointer user_data);

/* Fills the specified toolbar with buttons created from the specified info, using the specified
 * builder data -- this is intended for language bindings.  If accel_group is not NULL, then the
 * items' accelerator keys are put into it.
 */
void mate_app_fill_toolbar_custom (GtkToolbar *toolbar, MateUIInfo *uiinfo, MateUIBuilderData *uibdata,
				    GtkAccelGroup *accel_group);

/* Constructs a toolbar and attaches it to the specified application window */
void mate_app_create_toolbar (MateApp *app, MateUIInfo *uiinfo);

/* Constructs a toolbar and attaches it to the specified application window -- this version is
 * intended for language bindings.
 */
void mate_app_create_toolbar_interp (MateApp *app, MateUIInfo *uiinfo,
				      GtkCallbackMarshal relay_func, gpointer data,
				      GDestroyNotify destroy_func);

/* Constructs a toolbar, sets all the user data pointers to the specified value, and attaches it to
 * the specified application window.
 */
void mate_app_create_toolbar_with_data (MateApp *app, MateUIInfo *uiinfo, gpointer user_data);

/* Constructs a toolbar and attaches it to the specified application window, using the specified
 * builder data -- intended for language bindings.
 */
void mate_app_create_toolbar_custom (MateApp *app, MateUIInfo *uiinfo, MateUIBuilderData *uibdata);

/* finds menu item described by path (see below for details) starting in the GtkMenuShell top
 * and returns its parent GtkMenuShell and the position after this item in pos:
 * gtk_menu_shell_insert(p, w, pos) would then insert widget w in GtkMenuShell p right after
 * the menu item described by path.
 * the path argument should be in the form "File/.../.../Something".
 * "" will insert the item as the first one in the menubar
 * "File/" will insert it as the first one in the File menu
 * "File/Settings" will insert it after the Setting item in the File menu
 * use of  "File/<Separator>" should be obvious. however this stops after the first separator.
 */
GtkWidget *mate_app_find_menu_pos (GtkWidget *parent, const gchar *path, gint *pos);

/* removes num items from the existing app's menu structure begining with item described
 * by path
 */
void mate_app_remove_menus (MateApp *app, const gchar *path, gint items);

/* Same as the above, except it removes the specified number of items 
 * from the existing app's menu structure begining with item described by path,
 * plus the number specified by start - very useful for adding and removing Recent
 * document items in the File menu.
 */
void mate_app_remove_menu_range (MateApp *app, const gchar *path, gint start, gint items);

/* inserts menus described by uiinfo in existing app's menu structure right after the item described by path.
 */
void mate_app_insert_menus_custom (MateApp *app, const gchar *path, MateUIInfo *uiinfo, MateUIBuilderData *uibdata);

void mate_app_insert_menus (MateApp *app, const gchar *path, MateUIInfo *menuinfo);

void mate_app_insert_menus_with_data (MateApp *app, const gchar *path, MateUIInfo *menuinfo, gpointer data);

void mate_app_insert_menus_interp (MateApp *app, const gchar *path, MateUIInfo *menuinfo,
				    GtkCallbackMarshal relay_func, gpointer data,
				    GDestroyNotify destroy_func);


/* Activate the menu item hints, displaying in the given appbar.
   This can't be automatic since we can't reliably find the 
   appbar. */
void mate_app_install_appbar_menu_hints    (MateAppBar* appbar,
                                             MateUIInfo* uiinfo);

void mate_app_install_statusbar_menu_hints (GtkStatusbar* bar,
                                             MateUIInfo* uiinfo);

/* really? why can't it be automatic? */
void mate_app_install_menu_hints           (MateApp *app,
                                             MateUIInfo *uiinfo);

void mate_app_setup_toolbar                (GtkToolbar *toolbar,
                                             MateComponentDockItem *dock_item);

G_END_DECLS

#endif
