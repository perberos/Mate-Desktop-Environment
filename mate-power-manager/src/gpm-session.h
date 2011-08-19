/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#ifndef __GPM_SESSION_H
#define __GPM_SESSION_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GPM_TYPE_SESSION		(gpm_session_get_type ())
#define GPM_SESSION(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_SESSION, GpmSession))
#define GPM_SESSION_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_SESSION, GpmSessionClass))
#define GPM_IS_SESSION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_SESSION))
#define GPM_IS_SESSION_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_SESSION))
#define GPM_SESSION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_SESSION, GpmSessionClass))

typedef struct GpmSessionPrivate GpmSessionPrivate;

typedef struct
{
	GObject			 parent;
	GpmSessionPrivate	*priv;
} GpmSession;

typedef struct
{
	GObjectClass	parent_class;
	void		(* idle_changed)		(GpmSession	*session,
							 gboolean	 is_idle);
	void		(* inhibited_changed)		(GpmSession	*session,
							 gboolean	 is_idle_inhibited,
							 gboolean        is_suspend_inhibited);
	/* just exit */
	void		(* stop)			(GpmSession	*session);
	/* reply with EndSessionResponse */
	void		(* query_end_session)		(GpmSession	*session,
							 guint		 flags);
	/* reply with EndSessionResponse */
	void		(* end_session)			(GpmSession	*session,
							 guint		 flags);
	void		(* cancel_end_session)		(GpmSession	*session);
} GpmSessionClass;

GType		 gpm_session_get_type			(void);
GpmSession	*gpm_session_new			(void);

gboolean	 gpm_session_logout			(GpmSession	*session);
gboolean	 gpm_session_get_idle			(GpmSession	*session);
gboolean	 gpm_session_get_idle_inhibited		(GpmSession	*session);
gboolean	 gpm_session_get_suspend_inhibited	(GpmSession	*session);
gboolean	 gpm_session_register_client		(GpmSession	*session,
							 const gchar	*app_id,
							 const gchar	*client_startup_id);
gboolean	 gpm_session_end_session_response	(GpmSession	*session,
							 gboolean	 is_okay,
							 const gchar	*reason);

G_END_DECLS

#endif	/* __GPM_SESSION_H */
