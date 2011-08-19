/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Red Hat, Inc.
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

#ifndef __GVC_SPEAKER_TEST_H
#define __GVC_SPEAKER_TEST_H

#include <glib-object.h>
#include <gvc-mixer-card.h>
#include <gvc-mixer-control.h>

G_BEGIN_DECLS

#define GVC_TYPE_SPEAKER_TEST         (gvc_speaker_test_get_type ())
#define GVC_SPEAKER_TEST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_SPEAKER_TEST, GvcSpeakerTest))
#define GVC_SPEAKER_TEST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_SPEAKER_TEST, GvcSpeakerTestClass))
#define GVC_IS_SPEAKER_TEST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_SPEAKER_TEST))
#define GVC_IS_SPEAKER_TEST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_SPEAKER_TEST))
#define GVC_SPEAKER_TEST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_SPEAKER_TEST, GvcSpeakerTestClass))

typedef struct GvcSpeakerTestPrivate GvcSpeakerTestPrivate;

typedef struct
{
        GtkNotebook               parent;
        GvcSpeakerTestPrivate *priv;
} GvcSpeakerTest;

typedef struct
{
        GtkNotebookClass        parent_class;
} GvcSpeakerTestClass;

GType               gvc_speaker_test_get_type            (void);

GtkWidget *         gvc_speaker_test_new                 (GvcMixerControl *control,
                                                          GvcMixerCard *card);

G_END_DECLS

#endif /* __GVC_SPEAKER_TEST_H */
