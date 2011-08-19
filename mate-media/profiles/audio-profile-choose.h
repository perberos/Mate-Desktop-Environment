/* audio-profile-choose.h: combo box to choose a specific profile */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
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
 * Boston, MA 02111-1307, USA.
 */

#ifndef GM_AUDIO_PROFILE_CHOOSE_H
#define GM_AUDIO_PROFILE_CHOOSE_H

#include "audio-profile.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Standard macros */
#define GM_AUDIO_TYPE_PROFILE_CHOOSE            (gm_audio_profile_choose_get_type ())
#define GM_AUDIO_PROFILE_CHOOSE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GM_AUDIO_TYPE_PROFILE_CHOOSE, GMAudioProfileChoose))
#define GM_AUDIO_PROFILE_CHOOSE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST  ((klass), GM_AUDIO_TYPE_PROFILE_CHOOSE, GMAudioProfileChooseClass))
#define GM_AUDIO_IS_PROFILE_CHOOSE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GM_AUDIO_TYPE_PROFILE_CHOOSE))
#define GM_AUDIO_IS_PROFILE_CHOOSE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE  ((klass), GM_AUDIO_TYPE_PROFILE_CHOOSE))
#define GM_AUDIO_PROFILE_CHOOSE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj), GM_AUDIO_TYPE_PROFILE_CHOOSE, GMAudioProfileChooseClass))

/* Structs */
typedef struct _GMAudioProfileChoose        GMAudioProfileChoose;
typedef struct _GMAudioProfileChooseClass   GMAudioProfileChooseClass;
typedef struct _GMAudioProfileChoosePrivate GMAudioProfileChoosePrivate;

struct _GMAudioProfileChoose
{
  GtkComboBox parent;

  /*< Private >*/
  GMAudioProfileChoosePrivate *priv;
};

struct _GMAudioProfileChooseClass
{
  GtkComboBoxClass parent_class;

  /* Signals */
  void (*profile_changed) (GMAudioProfileChoose *choose,
			   GMAudioProfile       *profile);
};

/* Public API */
GType           gm_audio_profile_choose_get_type           (void) G_GNUC_CONST;
GtkWidget      *gm_audio_profile_choose_new                (void);
GMAudioProfile *gm_audio_profile_choose_get_active_profile (GMAudioProfileChoose *choose);
gboolean        gm_audio_profile_choose_set_active_profile (GMAudioProfileChoose *choose,
							    const gchar          *id);

/* Deprecated API */
GMAudioProfile *gm_audio_profile_choose_get_active         (GtkWidget            *choose);
gboolean        gm_audio_profile_choose_set_active         (GtkWidget            *choose,
							    const char           *id);

G_END_DECLS

#endif /* GM_AUDIO_PROFILE_CHOOSE_H */
