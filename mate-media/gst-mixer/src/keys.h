/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * keys.h: MateConf key macros
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

#ifndef __GVC_KEYS_H__
#define __GVC_KEYS_H__

G_BEGIN_DECLS

#define MATE_VOLUME_CONTROL_KEY_DIR \
  "/apps/mate-volume-control"
#define MATE_VOLUME_CONTROL_KEY(key) \
  MATE_VOLUME_CONTROL_KEY_DIR "/" key

#define MATE_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT \
  MATE_VOLUME_CONTROL_KEY ("active-element")
#define PREF_UI_WINDOW_WIDTH   MATE_VOLUME_CONTROL_KEY ("ui/window_width")
#define PREF_UI_WINDOW_HEIGHT  MATE_VOLUME_CONTROL_KEY ("ui/window_height")

G_END_DECLS

#endif /* __GVC_KEYS_H__ */
