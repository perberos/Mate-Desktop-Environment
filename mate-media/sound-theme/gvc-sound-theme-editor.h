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

#ifndef __GVC_SOUND_THEME_EDITOR_H
#define __GVC_SOUND_THEME_EDITOR_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GVC_TYPE_SOUND_THEME_EDITOR         (gvc_sound_theme_editor_get_type ())
#define GVC_SOUND_THEME_EDITOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_SOUND_THEME_EDITOR, GvcSoundThemeEditor))
#define GVC_SOUND_THEME_EDITOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_SOUND_THEME_EDITOR, GvcSoundThemeEditorClass))
#define GVC_IS_SOUND_THEME_EDITOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_SOUND_THEME_EDITOR))
#define GVC_IS_SOUND_THEME_EDITOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_SOUND_THEME_EDITOR))
#define GVC_SOUND_THEME_EDITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_SOUND_THEME_EDITOR, GvcSoundThemeEditorClass))

typedef struct GvcSoundThemeEditorPrivate GvcSoundThemeEditorPrivate;

typedef struct
{
        GtkVBox                     parent;
        GvcSoundThemeEditorPrivate *priv;
} GvcSoundThemeEditor;

typedef struct
{
        GtkVBoxClass          parent_class;
} GvcSoundThemeEditorClass;

GType               gvc_sound_theme_editor_get_type            (void);

GtkWidget *         gvc_sound_theme_editor_new                 (void);

G_END_DECLS

#endif /* __GVC_SOUND_THEME_EDITOR_H */
