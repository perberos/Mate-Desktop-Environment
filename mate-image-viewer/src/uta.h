/* Eye of Mate image viewer - Microtile array utilities
 *
 * Copyright (C) 2000-2009 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *
 * Portions based on code from libart_lgpl by Raph Levien.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef UTA_H
#define UTA_H

#define EOG_UTILE_SHIFT 5
#define EOG_UTILE_SIZE (1 << EOG_UTILE_SHIFT)

typedef guint32 EogUtaBbox;

struct _EogIRect {
  int x0, y0, x1, y1;
};

struct _EogUta {
  int x0;
  int y0;
  int width;
  int height;
  EogUtaBbox *utiles;
};

typedef struct _EogIRect EogIRect;
typedef struct _EogUta EogUta;



G_GNUC_INTERNAL
void	eog_uta_free 		(EogUta *uta);

G_GNUC_INTERNAL
void	eog_irect_intersect 	(EogIRect *dest,
				 const EogIRect *src1, const EogIRect *src2);
G_GNUC_INTERNAL
int	eog_irect_empty 	(const EogIRect *src);

G_GNUC_INTERNAL
EogUta *uta_ensure_size (EogUta *uta, int x1, int y1, int x2, int y2);

G_GNUC_INTERNAL
EogUta *uta_add_rect (EogUta *uta, int x1, int y1, int x2, int y2);

G_GNUC_INTERNAL
void uta_remove_rect (EogUta *uta, int x1, int y1, int x2, int y2);

G_GNUC_INTERNAL
void uta_find_first_glom_rect (EogUta *uta, EogIRect *rect, int max_width, int max_height);

G_GNUC_INTERNAL
void uta_copy_area (EogUta *uta, int src_x, int src_y, int dest_x, int dest_y, int width, int height);



#endif
