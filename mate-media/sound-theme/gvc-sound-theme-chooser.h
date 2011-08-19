/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#ifndef __GVC_SOUND_THEME_CHOOSER_H
#define __GVC_SOUND_THEME_CHOOSER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GVC_TYPE_SOUND_THEME_CHOOSER         (gvc_sound_theme_chooser_get_type ())
#define GVC_SOUND_THEME_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_SOUND_THEME_CHOOSER, GvcSoundThemeChooser))
#define GVC_SOUND_THEME_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_SOUND_THEME_CHOOSER, GvcSoundThemeChooserClass))
#define GVC_IS_SOUND_THEME_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_SOUND_THEME_CHOOSER))
#define GVC_IS_SOUND_THEME_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_SOUND_THEME_CHOOSER))
#define GVC_SOUND_THEME_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_SOUND_THEME_CHOOSER, GvcSoundThemeChooserClass))

typedef struct GvcSoundThemeChooserPrivate GvcSoundThemeChooserPrivate;

typedef struct
{
        GtkVBox                      parent;
        GvcSoundThemeChooserPrivate *priv;
} GvcSoundThemeChooser;

typedef struct
{
        GtkVBoxClass          parent_class;
} GvcSoundThemeChooserClass;

GType               gvc_sound_theme_chooser_get_type            (void);

GtkWidget *         gvc_sound_theme_chooser_new                 (void);

G_END_DECLS

#endif /* __GVC_SOUND_THEME_CHOOSER_H */
