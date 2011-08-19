/* audio-profile-edit.h: dialog to edit a specific profile */

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

#ifndef GM_AUDIO_PROFILE_EDIT_H
#define GM_AUDIO_PROFILE_EDIT_H

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

G_BEGIN_DECLS

#define GM_AUDIO_TYPE_PROFILE_EDIT              (gm_audio_profile_edit_get_type ())
#define GM_AUDIO_PROFILE_EDIT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GM_AUDIO_TYPE_PROFILE_EDIT, GMAudioProfileEdit))
#define GM_AUDIO_PROFILE_EDIT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GM_AUDIO_TYPE_PROFILE_EDIT, GMAudioProfileEditClass))
#define GM_AUDIO_IS_PROFILE_EDIT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GM_AUDIO_TYPE_PROFILE_EDIT))
#define GM_AUDIO_IS_PROFILE_EDIT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GM_AUDIO_TYPE_PROFILE_EDIT))
#define GM_AUDIO_PROFILE_EDIT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GM_AUDIO_TYPE_PROFILE_EDIT, GMAudioProfileEditClass))

typedef struct _GMAudioProfileEditClass   GMAudioProfileEditClass;
typedef struct _GMAudioProfileEditPrivate GMAudioProfileEditPrivate;
/* FIXME: this might have to be moved higher up in the hierarchy so it
   can be referenced from other places */
typedef struct _GMAudioProfileEdit		GMAudioProfileEdit;

struct _GMAudioProfileEdit
{
  GtkDialog parent_instance;

  GMAudioProfileEditPrivate *priv;
};

struct _GMAudioProfileEditClass
{
  GtkDialogClass parent_class;

};

GType gm_audio_profile_edit_get_type (void) G_GNUC_CONST;

/* create a new Profile Edit Dialog */
GtkWidget* gm_audio_profile_edit_new (MateConfClient *conf, const char *name);

G_END_DECLS

#endif /* GM_AUDIO_PROFILE_EDIT_H */
