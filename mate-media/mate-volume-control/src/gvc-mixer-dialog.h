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

#ifndef __GVC_MIXER_DIALOG_H
#define __GVC_MIXER_DIALOG_H

#include <glib-object.h>
#include "gvc-mixer-control.h"

G_BEGIN_DECLS

#define GVC_TYPE_MIXER_DIALOG         (gvc_mixer_dialog_get_type ())
#define GVC_MIXER_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialog))
#define GVC_MIXER_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogClass))
#define GVC_IS_MIXER_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_MIXER_DIALOG))
#define GVC_IS_MIXER_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_MIXER_DIALOG))
#define GVC_MIXER_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogClass))

typedef struct GvcMixerDialogPrivate GvcMixerDialogPrivate;

typedef struct
{
        GtkDialog              parent;
        GvcMixerDialogPrivate *priv;
} GvcMixerDialog;

typedef struct
{
        GtkDialogClass         parent_class;
} GvcMixerDialogClass;

GType               gvc_mixer_dialog_get_type            (void);

GvcMixerDialog *    gvc_mixer_dialog_new                 (GvcMixerControl *control);
gboolean            gvc_mixer_dialog_set_page            (GvcMixerDialog *dialog, const gchar* page);

G_END_DECLS

#endif /* __GVC_MIXER_DIALOG_H */
