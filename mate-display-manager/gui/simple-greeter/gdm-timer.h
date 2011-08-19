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
 *  Written by: Ray Strode <rstrode@redhat.com>
 */

#ifndef GDM_TIMER_H
#define GDM_TIMER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_TIMER         (gdm_timer_get_type ())
#define GDM_TIMER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_TIMER, GdmTimer))
#define GDM_TIMER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_TIMER, GdmTimerClass))
#define GDM_IS_TIMER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_TIMER))
#define GDM_IS_TIMER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_TIMER))
#define GDM_TIMER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_TIMER, GdmTimerClass))

typedef struct GdmTimerPrivate GdmTimerPrivate;

typedef struct
{
        GObject          parent;
        GdmTimerPrivate *priv;
} GdmTimer;

typedef struct
{
        GObjectClass   parent_class;

        void (* tick)  (GdmTimer *timer, double progress);
        void (* stop)  (GdmTimer *timer);
} GdmTimerClass;

GType     gdm_timer_get_type       (void);
GdmTimer *gdm_timer_new            (void);
#if 0
GObject *gdm_timer_new_for_source (GdmTimerSource *source);
#endif
void      gdm_timer_start          (GdmTimer *timer,
                                   double    number_of_seconds);
void      gdm_timer_stop           (GdmTimer *timer);
gboolean  gdm_timer_is_started     (GdmTimer *timer);

#endif /* GDM_TIMER_H */
