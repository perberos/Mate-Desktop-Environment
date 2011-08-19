/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPMSCREENSAVER_H
#define __GPMSCREENSAVER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_SCREENSAVER		(gpm_screensaver_get_type ())
#define GPM_SCREENSAVER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_SCREENSAVER, GpmScreensaver))
#define GPM_SCREENSAVER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_SCREENSAVER, GpmScreensaverClass))
#define GPM_IS_SCREENSAVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_SCREENSAVER))
#define GPM_IS_SCREENSAVER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_SCREENSAVER))
#define GPM_SCREENSAVER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_SCREENSAVER, GpmScreensaverClass))

typedef struct GpmScreensaverPrivate GpmScreensaverPrivate;

typedef struct
{
	GObject		       parent;
	GpmScreensaverPrivate *priv;
} GpmScreensaver;

typedef struct
{
	GObjectClass	parent_class;
#if 0
	void		(* auth_request)		(GpmScreensaver	*screensaver,
					    		 gboolean	 auth);
#endif
} GpmScreensaverClass;

GType		 gpm_screensaver_get_type		(void);
GpmScreensaver	*gpm_screensaver_new			(void);
void		 gpm_screensaver_test			(gpointer	 data);

gboolean	 gpm_screensaver_lock			(GpmScreensaver	*screensaver);
gboolean	 gpm_screensaver_lock_enabled		(GpmScreensaver	*screensaver);
guint32 	 gpm_screensaver_add_throttle    	(GpmScreensaver	*screensaver,
							 const gchar	*reason);
gboolean 	 gpm_screensaver_remove_throttle    	(GpmScreensaver	*screensaver,
							 guint32         cookie);
gboolean	 gpm_screensaver_check_running		(GpmScreensaver	*screensaver);
gboolean	 gpm_screensaver_poke			(GpmScreensaver	*screensaver);

G_END_DECLS

#endif	/* __GPMSCREENSAVER_H */
