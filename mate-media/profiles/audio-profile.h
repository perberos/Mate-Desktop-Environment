/* audio-profile.h: public header for audio profiles */
/*
 * Copyright (C) 2003,2004 Thomas Vander Stichele
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */

#ifndef AUDIO_PROFILE_H
#define AUDIO_PROFILE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GM_AUDIO_TYPE_PROFILE              (gm_audio_profile_get_type ())
#define GM_AUDIO_PROFILE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GM_AUDIO_TYPE_PROFILE, GMAudioProfile))
#define GM_AUDIO_PROFILE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GM_AUDIO_TYPE_PROFILE, GMAudioProfileClass))
#define GM_AUDIO_IS_PROFILE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GM_AUDIO_TYPE_PROFILE))
#define GM_AUDIO_IS_PROFILE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_AUDIO_TYPE_PROFILE))
#define GM_AUDIO_PROFILE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_AUDIO_TYPE_PROFILE, GMAudioProfileClass))

/* mask for lockedness of settings */
typedef struct
{
  unsigned int name : 1;
  unsigned int description : 1;
  unsigned int pipeline : 1;
  unsigned int extension : 1;
  unsigned int active : 1;
} GMAudioSettingMask;

typedef struct _GMAudioProfile        GMAudioProfile;
typedef struct _GMAudioProfileClass   GMAudioProfileClass;
typedef struct _GMAudioProfilePrivate GMAudioProfilePrivate;

struct _GMAudioProfile
{
  GObject parent_instance;
  GMAudioProfilePrivate *priv;
};

struct _GMAudioProfileClass
{
  GObjectClass parent_class;

  void (* changed)   (GMAudioProfile           *profile,
                      const GMAudioSettingMask *mask);
  void (* forgotten) (GMAudioProfile           *profile);
};


GType gm_audio_profile_get_type (void) G_GNUC_CONST;

const char*     gm_audio_profile_get_id	        (GMAudioProfile *profile);
const char*     gm_audio_profile_get_name	(GMAudioProfile *profile);
const char*     gm_audio_profile_get_description(GMAudioProfile *profile);
const char*     gm_audio_profile_get_pipeline	(GMAudioProfile *profile);
const char*     gm_audio_profile_get_extension	(GMAudioProfile *profile);
gboolean        gm_audio_profile_get_active     (GMAudioProfile *profile);

GList*		gm_audio_profile_get_active_list(void);
GMAudioProfile*	gm_audio_profile_lookup		(const char *id);

void gm_audio_profile_set_name (GMAudioProfile *profile, const char *name);
void gm_audio_profile_set_description (GMAudioProfile *profile, const char *name);
void gm_audio_profile_set_pipeline (GMAudioProfile *profile, const char *name);
void gm_audio_profile_set_extension (GMAudioProfile *profile, const char *name);
void gm_audio_profile_set_active (GMAudioProfile *profile, gboolean active);

G_END_DECLS

#endif /* AUDIO_PROFILE_H */
