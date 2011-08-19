/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2006-2009 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPM_DPMS_H
#define __GPM_DPMS_H

G_BEGIN_DECLS

#define GPM_TYPE_DPMS		(gpm_dpms_get_type ())
#define GPM_DPMS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_DPMS, GpmDpms))
#define GPM_DPMS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_DPMS, GpmDpmsClass))
#define GPM_IS_DPMS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_DPMS))
#define GPM_IS_DPMS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_DPMS))
#define GPM_DPMS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_DPMS, GpmDpmsClass))

typedef enum {
	GPM_DPMS_MODE_ON,
	GPM_DPMS_MODE_STANDBY,
	GPM_DPMS_MODE_SUSPEND,
	GPM_DPMS_MODE_OFF,
	GPM_DPMS_MODE_UNKNOWN
} GpmDpmsMode;

typedef struct GpmDpmsPrivate GpmDpmsPrivate;

typedef struct
{
	GObject	 	parent;
	GpmDpmsPrivate *priv;
} GpmDpms;

typedef struct
{
	GObjectClass	parent_class;
	void		(* mode_changed)	(GpmDpms 	*dpms,
						 GpmDpmsMode	 mode);
} GpmDpmsClass;

typedef enum
{
	GPM_DPMS_ERROR_GENERAL
} GpmDpmsError;

#define GPM_DPMS_ERROR gpm_dpms_error_quark ()

GQuark		 gpm_dpms_error_quark		(void);
GType		 gpm_dpms_get_type		(void);
GpmDpms		*gpm_dpms_new			(void);
gboolean	 gpm_dpms_get_mode	 	(GpmDpms	*dpms,
						 GpmDpmsMode	*mode,
						 GError		**error);
gboolean	 gpm_dpms_set_mode	 	(GpmDpms	*dpms,
						 GpmDpmsMode	 mode,
						 GError		**error);
const gchar	*gpm_dpms_mode_to_string	(GpmDpmsMode	 mode);
GpmDpmsMode	 gpm_dpms_mode_from_string	(const gchar	*mode);
void		 gpm_dpms_test			(gpointer	 data);

G_END_DECLS

#endif /* __GPM_DPMS_H */
