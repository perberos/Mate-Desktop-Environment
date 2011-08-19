/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _MATE_DA_ITEM_H_
#define _MATE_DA_ITEM_H_

#include <glib.h>

typedef struct _MateDAItem MateDAItem;

typedef struct _MateDAWebItem MateDAWebItem;
typedef struct _MateDATermItem MateDATermItem;
typedef struct _MateDASimpleItem MateDASimpleItem;
typedef struct _MateDAVisualItem MateDAVisualItem;
typedef struct _MateDAMobilityItem MateDAMobilityItem;

struct _MateDAItem {
    gchar *name;
    gchar *executable;
    gchar *command;
    gchar *icon_name;
    gchar *icon_path;
};

struct _MateDAWebItem {
    MateDAItem generic;
    gboolean run_in_terminal;
    gboolean netscape_remote;
    gchar *tab_command;
    gchar *win_command;
};

struct _MateDASimpleItem {
    MateDAItem generic;
    gboolean run_in_terminal;
};

struct _MateDATermItem {
    MateDAItem generic;
    gchar *exec_flag;
};

struct _MateDAVisualItem {
    MateDAItem generic;
    gboolean run_at_startup;
};

struct _MateDAMobilityItem {
    MateDAItem generic;
    gboolean run_at_startup;
};

MateDAWebItem* mate_da_web_item_new (void);
MateDATermItem* mate_da_term_item_new (void);
MateDASimpleItem* mate_da_simple_item_new (void);
MateDAVisualItem* mate_da_visual_item_new (void);
MateDAMobilityItem* mate_da_mobility_item_new (void);
void mate_da_web_item_free (MateDAWebItem *item);
void mate_da_term_item_free (MateDATermItem *item);
void mate_da_simple_item_free (MateDASimpleItem *item);
void mate_da_visual_item_free (MateDAVisualItem *item);
void mate_da_mobility_item_free (MateDAMobilityItem *item);

#endif
