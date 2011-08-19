/* gsdtimeline.c
 *
 * Copyright (C) 2008 Carlos Garnacho  <carlos@imendio.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GSD_TIMELINE_H__
#define __GSD_TIMELINE_H__

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define GSD_TYPE_TIMELINE_DIRECTION       (gsd_timeline_direction_get_type ())
#define GSD_TYPE_TIMELINE_PROGRESS_TYPE   (gsd_timeline_progress_type_get_type ())
#define GSD_TYPE_TIMELINE                 (gsd_timeline_get_type ())
#define GSD_TIMELINE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSD_TYPE_TIMELINE, GsdTimeline))
#define GSD_TIMELINE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  GSD_TYPE_TIMELINE, GsdTimelineClass))
#define GSD_IS_TIMELINE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSD_TYPE_TIMELINE))
#define GSD_IS_TIMELINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  GSD_TYPE_TIMELINE))
#define GSD_TIMELINE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  GSD_TYPE_TIMELINE, GsdTimelineClass))

typedef enum {
  GSD_TIMELINE_DIRECTION_FORWARD,
  GSD_TIMELINE_DIRECTION_BACKWARD
} GsdTimelineDirection;

typedef enum {
  GSD_TIMELINE_PROGRESS_LINEAR,
  GSD_TIMELINE_PROGRESS_SINUSOIDAL,
  GSD_TIMELINE_PROGRESS_EXPONENTIAL
} GsdTimelineProgressType;

typedef struct GsdTimeline      GsdTimeline;
typedef struct GsdTimelineClass GsdTimelineClass;

struct GsdTimeline
{
  GObject parent_instance;
};

struct GsdTimelineClass
{
  GObjectClass parent_class;

  void (* started)           (GsdTimeline *timeline);
  void (* finished)          (GsdTimeline *timeline);
  void (* paused)            (GsdTimeline *timeline);

  void (* frame)             (GsdTimeline *timeline,
			      gdouble      progress);

  void (* __gsd_reserved1) (void);
  void (* __gsd_reserved2) (void);
  void (* __gsd_reserved3) (void);
  void (* __gsd_reserved4) (void);
};

typedef gdouble (*GsdTimelineProgressFunc) (gdouble progress);


GType                   gsd_timeline_get_type           (void) G_GNUC_CONST;
GType                   gsd_timeline_direction_get_type (void) G_GNUC_CONST;
GType                   gsd_timeline_progress_type_get_type (void) G_GNUC_CONST;

GsdTimeline            *gsd_timeline_new                (guint                    duration);
GsdTimeline            *gsd_timeline_new_for_screen     (guint                    duration,
							 GdkScreen               *screen);

void                    gsd_timeline_start              (GsdTimeline             *timeline);
void                    gsd_timeline_pause              (GsdTimeline             *timeline);
void                    gsd_timeline_rewind             (GsdTimeline             *timeline);

gboolean                gsd_timeline_is_running         (GsdTimeline             *timeline);

guint                   gsd_timeline_get_fps            (GsdTimeline             *timeline);
void                    gsd_timeline_set_fps            (GsdTimeline             *timeline,
							 guint                    fps);

gboolean                gsd_timeline_get_loop           (GsdTimeline             *timeline);
void                    gsd_timeline_set_loop           (GsdTimeline             *timeline,
							 gboolean                 loop);

guint                   gsd_timeline_get_duration       (GsdTimeline             *timeline);
void                    gsd_timeline_set_duration       (GsdTimeline             *timeline,
							 guint                    duration);

GdkScreen              *gsd_timeline_get_screen         (GsdTimeline             *timeline);
void                    gsd_timeline_set_screen         (GsdTimeline             *timeline,
							 GdkScreen               *screen);

GsdTimelineDirection    gsd_timeline_get_direction      (GsdTimeline             *timeline);
void                    gsd_timeline_set_direction      (GsdTimeline             *timeline,
							 GsdTimelineDirection     direction);

GsdTimelineProgressType gsd_timeline_get_progress_type  (GsdTimeline             *timeline);
void                    gsd_timeline_set_progress_type  (GsdTimeline             *timeline,
							 GsdTimelineProgressType  type);
void                    gsd_timeline_get_progress_func  (GsdTimeline             *timeline);

void                    gsd_timeline_set_progress_func  (GsdTimeline             *timeline,
							 GsdTimelineProgressFunc  progress_func);

gdouble                 gsd_timeline_get_progress       (GsdTimeline             *timeline);


G_END_DECLS

#endif /* __GSD_TIMELINE_H__ */
