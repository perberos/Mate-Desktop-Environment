/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef LIBMATECANVAS_H
#define LIBMATECANVAS_H

#include <libmatecanvas/mate-canvas.h>
#include <libmatecanvas/mate-canvas-line.h>
#include <libmatecanvas/mate-canvas-text.h>
#include <libmatecanvas/mate-canvas-rich-text.h>
#include <libmatecanvas/mate-canvas-polygon.h>
#include <libmatecanvas/mate-canvas-pixbuf.h>
#include <libmatecanvas/mate-canvas-widget.h>
#include <libmatecanvas/mate-canvas-rect-ellipse.h>
#include <libmatecanvas/mate-canvas-bpath.h>
#include <libmatecanvas/mate-canvas-util.h>
#include <libmatecanvas/mate-canvas-clipgroup.h>

G_BEGIN_DECLS

GType mate_canvas_points_get_type (void);
#define MATE_TYPE_CANVAS_POINTS mate_canvas_points_get_type()

G_END_DECLS

#endif /* LIBMATECANVAS_H */
