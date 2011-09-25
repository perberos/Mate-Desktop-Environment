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

#ifndef MDM_TIMER_H
#define MDM_TIMER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define MDM_TYPE_TIMER         (mdm_timer_get_type ())
#define MDM_TIMER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_TIMER, MdmTimer))
#define MDM_TIMER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_TIMER, MdmTimerClass))
#define MDM_IS_TIMER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_TIMER))
#define MDM_IS_TIMER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_TIMER))
#define MDM_TIMER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_TIMER, MdmTimerClass))

typedef struct MdmTimerPrivate MdmTimerPrivate;

typedef struct
{
        GObject          parent;
        MdmTimerPrivate *priv;
} MdmTimer;

typedef struct
{
        GObjectClass   parent_class;

        void (* tick)  (MdmTimer *timer, double progress);
        void (* stop)  (MdmTimer *timer);
} MdmTimerClass;

GType     mdm_timer_get_type       (void);
MdmTimer *mdm_timer_new            (void);
#if 0
GObject *mdm_timer_new_for_source (MdmTimerSource *source);
#endif
void      mdm_timer_start          (MdmTimer *timer,
                                   double    number_of_seconds);
void      mdm_timer_stop           (MdmTimer *timer);
gboolean  mdm_timer_is_started     (MdmTimer *timer);

#endif /* MDM_TIMER_H */
