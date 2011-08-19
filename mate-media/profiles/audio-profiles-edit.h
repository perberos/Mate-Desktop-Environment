/* audio-profiles-edit.h: widget for a profiles edit dialog */

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

#ifndef GM_AUDIO_PROFILES_EDIT_H
#define GM_AUDIO_PROFILES_EDIT_H

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

G_BEGIN_DECLS

#define GM_AUDIO_TYPE_PROFILES_EDIT              (gm_audio_profiles_edit_get_type ())
#define GM_AUDIO_PROFILES_EDIT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GM_AUDIO_TYPE_PROFILES_EDIT, GMAudioProfilesEdit))
#define GM_AUDIO_PROFILES_EDIT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GM_AUDIO_TYPE_PROFILES_EDIT, GMAudioProfilesEditClass))
#define GM_AUDIO_IS_PROFILES_EDIT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GM_AUDIO_TYPE_PROFILES_EDIT))
#define GM_AUDIO_IS_PROFILES_EDIT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_AUDIO_TYPE_PROFILES_EDIT))
#define GM_AUDIO_PROFILES_EDIT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_AUDIO_TYPE_PROFILES_EDIT, GMAudioProfilesEditClass))

typedef struct _GMAudioProfilesEditClass   GMAudioProfilesEditClass;
typedef struct _GMAudioProfilesEditPrivate GMAudioProfilesEditPrivate;
/* FIXME: this might have to be moved higher up in the hierarchy so it
   can be referenced from other places */
typedef struct _GMAudioProfilesEdit		GMAudioProfilesEdit;

struct _GMAudioProfilesEdit
{
  GtkDialog parent_instance;

  GMAudioProfilesEditPrivate *priv;
};

struct _GMAudioProfilesEditClass
{
  GtkDialogClass parent_class;

};

GType gm_audio_profiles_edit_get_type (void) G_GNUC_CONST;

GtkWidget*		gm_audio_profiles_edit_new	(MateConfClient *conf,
                                                 GtkWindow *transient_parent);

void			gm_audio_profiles_edit_new_profile (GMAudioProfilesEdit *dialog,
						 GtkWindow *transient_parent);


G_END_DECLS

#endif /* GM_AUDIO_PROFILES_EDIT_H */
