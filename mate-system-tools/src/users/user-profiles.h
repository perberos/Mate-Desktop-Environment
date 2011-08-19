/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#ifndef __USER_PROFILES_H__
#define __USER_PROFILES_H__

G_BEGIN_DECLS

#include <glib.h>
#include <glib-object.h>
#include <oobs/oobs-user.h>
#include <oobs/oobs-group.h>

#define GST_TYPE_USER_PROFILES           (gst_user_profiles_get_type ())
#define GST_USER_PROFILES(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_USER_PROFILES, GstUserProfiles))
#define GST_USER_PROFILES_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_USER_PROFILES, GstUserProfilesClass))
#define GST_IS_USER_PROFILES(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_USER_PROFILES))
#define GST_IS_USER_PROFILES_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_USER_PROFILES))
#define GST_USER_PROFILES_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_USER_PROFILES, GstUserProfilesClass))

typedef struct _GstUserProfiles GstUserProfiles;
typedef struct _GstUserProfilesClass GstUserProfilesClass;
typedef struct _GstUserProfile GstUserProfile;

struct _GstUserProfiles
{
	GObject parent;
};

struct _GstUserProfilesClass
{
	GObjectClass parent_class;
};

struct _GstUserProfile
{
	gchar *name;
	gchar  *description;
	gboolean is_default;

	/* profile data */
	gchar  *shell;
	gchar  *home_prefix;
	uid_t   uid_min;
	uid_t   uid_max;
	gchar **groups;
};

GType            gst_user_profiles_get_type            (void);
GstUserProfiles* gst_user_profiles_get                 (void);

GstUserProfile*  gst_user_profiles_get_from_name       (GstUserProfiles *profiles,
                                                        const gchar *name);
GList*           gst_user_profiles_get_list            (GstUserProfiles *profiles);
GstUserProfile*  gst_user_profiles_get_default_profile (GstUserProfiles *profiles);
GstUserProfile*  gst_user_profiles_get_for_user        (GstUserProfiles *profiles,
                                                        OobsUser        *user,
                                                        gboolean         strict);
void             gst_user_profiles_apply               (GstUserProfiles *profiles,
                                                        GstUserProfile  *profile,
                                                        OobsUser        *user,
                                                        gboolean         new_user);


G_END_DECLS

#endif /* __USER_PROFILES_H__ */
