/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __EGG_COLOR_H
#define __EGG_COLOR_H

#include <glib.h>

G_BEGIN_DECLS

#define	EGG_COLOR_WHITE			0xffffff
#define	EGG_COLOR_BLACK			0x000000
#define	EGG_COLOR_RED			0xff0000
#define	EGG_COLOR_GREEN			0x00ff00
#define	EGG_COLOR_BLUE			0x0000ff
#define	EGG_COLOR_CYAN			0x00ffff
#define	EGG_COLOR_MAGENTA		0xff00ff
#define	EGG_COLOR_YELLOW		0xffff00
#define	EGG_COLOR_GREY			0xcccccc
#define	EGG_COLOR_DARK_RED		0x600000
#define	EGG_COLOR_DARK_GREEN		0x006000
#define	EGG_COLOR_DARK_BLUE		0x000060
#define	EGG_COLOR_DARK_CYAN		0x006060
#define	EGG_COLOR_DARK_MAGENTA		0x600060
#define	EGG_COLOR_DARK_YELLOW		0x606000
#define	EGG_COLOR_DARK_GREY		0x606060

guint32		 egg_color_from_rgb			(guint8		 red,
							 guint8		 green,
							 guint8		 blue);
void		 egg_color_to_rgb			(guint32	 color,
							 guint8		*red,
							 guint8		*green,
							 guint8		*blue);
#ifdef EGG_TEST
void		 egg_color_test				(gpointer	 data);
#endif

G_END_DECLS

#endif /* __EGG_COLOR_H */
