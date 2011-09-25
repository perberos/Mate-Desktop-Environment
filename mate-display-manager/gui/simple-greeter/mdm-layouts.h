/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2008 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Matthias Clasen
 */

#ifndef __MDM_LAYOUTS_H
#define __MDM_LAYOUTS_H

G_BEGIN_DECLS

char *        mdm_get_layout_from_name   (const char *name);
char **       mdm_get_all_layout_names   (void);
gboolean      mdm_layout_is_valid        (const char *layout);
const char *  mdm_layout_get_default_layout (void);
void          mdm_layout_activate        (const char *layout);

G_END_DECLS

#endif /* __MDM_LAYOUT_CHOOSER_WIDGET_H */
