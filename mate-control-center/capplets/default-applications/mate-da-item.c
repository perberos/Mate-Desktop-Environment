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

#include "mate-da-capplet.h"
#include "mate-da-item.h"

MateDAWebItem*
mate_da_web_item_new (void)
{
    MateDAWebItem *item = NULL;

    item = g_new0 (MateDAWebItem, 1);

    return item;
}

MateDASimpleItem*
mate_da_simple_item_new (void)
{
    MateDASimpleItem *item = NULL;

    item = g_new0 (MateDASimpleItem, 1);

    return item;
}

MateDATermItem*
mate_da_term_item_new (void)
{
    MateDATermItem *item = NULL;

    item = g_new0 (MateDATermItem, 1);

    return item;
}

MateDAVisualItem*
mate_da_visual_item_new (void)
{
    MateDAVisualItem *item = NULL;

    item = g_new0 (MateDAVisualItem, 1);

    return item;
}

MateDAMobilityItem*
mate_da_mobility_item_new (void)
{
    MateDAMobilityItem *item = NULL;

    item = g_new0 (MateDAMobilityItem, 1);

    return item;
}

void
mate_da_web_item_free (MateDAWebItem *item)
{
    g_return_if_fail (item != NULL);

    g_free (item->generic.name);
    g_free (item->generic.executable);
    g_free (item->generic.command);
    g_free (item->generic.icon_name);
    g_free (item->generic.icon_path);

    g_free (item->tab_command);
    g_free (item->win_command);

    g_free (item);
}

void
mate_da_simple_item_free (MateDASimpleItem *item)
{
    g_return_if_fail (item != NULL);

    g_free (item->generic.name);
    g_free (item->generic.executable);
    g_free (item->generic.command);
    g_free (item->generic.icon_name);
    g_free (item->generic.icon_path);

    g_free (item);
}

void
mate_da_term_item_free (MateDATermItem *item)
{
    g_return_if_fail (item != NULL);

    g_free (item->generic.name);
    g_free (item->generic.executable);
    g_free (item->generic.command);
    g_free (item->generic.icon_name);
    g_free (item->generic.icon_path);

    g_free (item->exec_flag);

    g_free (item);
}

void
mate_da_visual_item_free (MateDAVisualItem *item)
{
    g_return_if_fail (item != NULL);

    g_free (item->generic.name);
    g_free (item->generic.executable);
    g_free (item->generic.command);
    g_free (item->generic.icon_name);
    g_free (item->generic.icon_path);

    g_free (item);
}

void
mate_da_mobility_item_free (MateDAMobilityItem *item)
{
    g_return_if_fail (item != NULL);

    g_free (item->generic.name);
    g_free (item->generic.executable);
    g_free (item->generic.command);
    g_free (item->generic.icon_name);
    g_free (item->generic.icon_path);

    g_free (item);
}

