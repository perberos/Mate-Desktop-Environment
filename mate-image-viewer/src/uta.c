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

#include <config.h>
#include <glib.h>
#include "uta.h"

#define EOG_UTA_BBOX_CONS(x0, y0, x1, y1) (((x0) << 24) | ((y0) << 16) | \
				       ((x1) << 8) | (y1))

#define EOG_UTA_BBOX_X0(ub) ((ub) >> 24)
#define EOG_UTA_BBOX_Y0(ub) (((ub) >> 16) & 0xff)
#define EOG_UTA_BBOX_X1(ub) (((ub) >> 8) & 0xff)
#define EOG_UTA_BBOX_Y1(ub) ((ub) & 0xff)

#define eog_renew(p, type, n) ((type *)g_realloc (p, (n) * sizeof(type)))
/* This one must be used carefully - in particular, p and max should
   be variables. They can also be pstruct->el lvalues. */
#define eog_expand(p, type, max) do { if(max) { p = eog_renew (p, type, max <<= 1); } else { max = 1; p = g_new(type, 1); } } while (0)



/**
 * eog_uta_new: Allocate a new uta.
 * @x0: Left coordinate of uta.
 * @y0: Top coordinate of uta.
 * @x1: Right coordinate of uta.
 * @y1: Bottom coordinate of uta.
 *
 * Allocates a new microtile array. The arguments are in units of
 * tiles, not pixels.
 *
 * Returns: the newly allocated #EogUta.
 **/
static EogUta *
eog_uta_new (int x0, int y0, int x1, int y1)
{
  EogUta *uta;

  uta = g_new (EogUta, 1);
  uta->x0 = x0;
  uta->y0 = y0;
  uta->width = x1 - x0;
  uta->height = y1 - y0;

  uta->utiles = g_new0 (EogUtaBbox, uta->width * uta->height);

  return uta;
}

/**
 * eog_uta_free: Free a uta.
 * @uta: The uta to free.
 *
 * Frees the microtile array structure, including the actual microtile
 * data.
 **/
void
eog_uta_free (EogUta *uta)
{
  g_free (uta->utiles);
  g_free (uta);
}

/**
 * eog_irect_intersect: Find intersection of two integer rectangles.
 * @dest: Where the result is stored.
 * @src1: A source rectangle.
 * @src2: Another source rectangle.
 *
 * Finds the intersection of @src1 and @src2.
 **/
void
eog_irect_intersect (EogIRect *dest, const EogIRect *src1, const EogIRect *src2) {
  dest->x0 = MAX (src1->x0, src2->x0);
  dest->y0 = MAX (src1->y0, src2->y0);
  dest->x1 = MIN (src1->x1, src2->x1);
  dest->y1 = MIN (src1->y1, src2->y1);
}

/**
 * eog_irect_empty: Determine whether integer rectangle is empty.
 * @src: The source rectangle.
 *
 * Return value: TRUE if @src is an empty rectangle, FALSE otherwise.
 **/
int
eog_irect_empty (const EogIRect *src) {
  return (src->x1 <= src->x0 || src->y1 <= src->y0);
}

/**
 * eog_uta_from_irect: Generate uta covering a rectangle.
 * @bbox: The source rectangle.
 *
 * Generates a uta exactly covering @bbox. Please do not call this
 * function with a @bbox with zero height or width.
 *
 * Return value: the new uta.
 **/
static EogUta *
eog_uta_from_irect (EogIRect *bbox)
{
  EogUta *uta;
  EogUtaBbox *utiles;
  EogUtaBbox bb;
  int width, height;
  int x, y;
  int xf0, yf0, xf1, yf1;
  int ix;

  uta = g_new (EogUta, 1);
  uta->x0 = bbox->x0 >> EOG_UTILE_SHIFT;
  uta->y0 = bbox->y0 >> EOG_UTILE_SHIFT;
  width = ((bbox->x1 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT) - uta->x0;
  height = ((bbox->y1 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT) - uta->y0;
  utiles = g_new (EogUtaBbox, width * height);

  uta->width = width;
  uta->height = height;
  uta->utiles = utiles;

  xf0 = bbox->x0 & (EOG_UTILE_SIZE - 1);
  yf0 = bbox->y0 & (EOG_UTILE_SIZE - 1);
  xf1 = ((bbox->x1 - 1) & (EOG_UTILE_SIZE - 1)) + 1;
  yf1 = ((bbox->y1 - 1) & (EOG_UTILE_SIZE - 1)) + 1;
  if (height == 1)
    {
      if (width == 1)
	utiles[0] = EOG_UTA_BBOX_CONS (xf0, yf0, xf1, yf1);
      else
	{
	  utiles[0] = EOG_UTA_BBOX_CONS (xf0, yf0, EOG_UTILE_SIZE, yf1);
	  bb = EOG_UTA_BBOX_CONS (0, yf0, EOG_UTILE_SIZE, yf1);
	  for (x = 1; x < width - 1; x++)
	    utiles[x] = bb;
	  utiles[x] = EOG_UTA_BBOX_CONS (0, yf0, xf1, yf1);
	}
    }
  else
    {
      if (width == 1)
	{
	  utiles[0] = EOG_UTA_BBOX_CONS (xf0, yf0, xf1, EOG_UTILE_SIZE);
	  bb = EOG_UTA_BBOX_CONS (xf0, 0, xf1, EOG_UTILE_SIZE);
	  for (y = 1; y < height - 1; y++)
	    utiles[y] = bb;
	  utiles[y] = EOG_UTA_BBOX_CONS (xf0, 0, xf1, yf1);
	}
      else
	{
	  utiles[0] =
	    EOG_UTA_BBOX_CONS (xf0, yf0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
	  bb = EOG_UTA_BBOX_CONS (0, yf0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
	  for (x = 1; x < width - 1; x++)
	    utiles[x] = bb;
	  utiles[x] = EOG_UTA_BBOX_CONS (0, yf0, xf1, EOG_UTILE_SIZE);
	  ix = width;
	  for (y = 1; y < height - 1; y++)
	    {
	      utiles[ix++] =
		EOG_UTA_BBOX_CONS (xf0, 0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
	      bb = EOG_UTA_BBOX_CONS (0, 0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
	      for (x = 1; x < width - 1; x++)
		utiles[ix++] = bb;
	      utiles[ix++] = EOG_UTA_BBOX_CONS (0, 0, xf1, EOG_UTILE_SIZE);
	    }
	  utiles[ix++] = EOG_UTA_BBOX_CONS (xf0, 0, EOG_UTILE_SIZE, yf1);
	  bb = EOG_UTA_BBOX_CONS (0, 0, EOG_UTILE_SIZE, yf1);
	  for (x = 1; x < width - 1; x++)
	    utiles[ix++] = bb;
	  utiles[ix++] = EOG_UTA_BBOX_CONS (0, 0, xf1, yf1);
	}
    }
  return uta;
}


/**
 * uta_ensure_size:
 * @uta: A microtile array.
 * @x1: Left microtile coordinate that must fit in new array.
 * @y1: Top microtile coordinate that must fit in new array.
 * @x2: Right microtile coordinate that must fit in new array.
 * @y2: Bottom microtile coordinate that must fit in new array.
 *
 * Ensures that the size of a microtile array is big enough to fit the specified
 * microtile coordinates.  If it is not big enough, the specified @uta will be
 * freed and a new one will be returned.  Otherwise, the original @uta will be
 * returned.  If a new microtile array needs to be created, this function will
 * copy the @uta's contents to the new array.
 *
 * Note that the specified coordinates must have already been scaled down by the
 * ART_UTILE_SHIFT factor.
 *
 * Return value: The same value as @uta if the original microtile array was
 * big enough to fit the specified microtile coordinates, or a new array if
 * it needed to be grown.  In the second case, the original @uta will be
 * freed automatically.
 **/
EogUta *
uta_ensure_size (EogUta *uta, int x1, int y1, int x2, int y2)
{
	EogUta *new_uta;
	EogUtaBbox *utiles, *new_utiles;
	int new_ofs, ofs;
	int x, y;

	g_return_val_if_fail (x1 < x2, NULL);
	g_return_val_if_fail (y1 < y2, NULL);

	if (!uta)
		return eog_uta_new (x1, y1, x2, y2);

	if (x1 >= uta->x0
	    && y1 >= uta->y0
	    && x2 <= uta->x0 + uta->width
	    && y2 <= uta->y0 + uta->height)
		return uta;

	new_uta = g_new (EogUta, 1);

	new_uta->x0 = MIN (uta->x0, x1);
	new_uta->y0 = MIN (uta->y0, y1);
	new_uta->width = MAX (uta->x0 + uta->width, x2) - new_uta->x0;
	new_uta->height = MAX (uta->y0 + uta->height, y2) - new_uta->y0;
	new_uta->utiles = g_new (EogUtaBbox, new_uta->width * new_uta->height);

	utiles = uta->utiles;
	new_utiles = new_uta->utiles;

	new_ofs = 0;

	for (y = new_uta->y0; y < new_uta->y0 + new_uta->height; y++) {
		if (y < uta->y0 || y >= uta->y0 + uta->height)
			for (x = 0; x < new_uta->width; x++)
				new_utiles[new_ofs++] = 0;
		else {
			ofs = (y - uta->y0) * uta->width;

			for (x = new_uta->x0; x < new_uta->x0 + new_uta->width; x++)
				if (x < uta->x0 || x >= uta->x0 + uta->width)
					new_utiles[new_ofs++] = 0;
				else
					new_utiles[new_ofs++] = utiles[ofs++];
		}
	}

	eog_uta_free (uta);
	return new_uta;
}

/**
 * uta_add_rect:
 * @uta: A microtile array, or NULL if a new array should be created.
 * @x1: Left coordinate of rectangle.
 * @y1: Top coordinate of rectangle.
 * @x2: Right coordinate of rectangle.
 * @y2: Bottom coordinate of rectangle.
 *
 * Adds the specified rectangle to a microtile array.  The array is
 * grown to fit the rectangle if necessary.
 *
 * Return value: The original @uta, or a new microtile array if the original one
 * needed to be grown to fit the specified rectangle.  In the second case, the
 * original @uta will be freed automatically.
 **/
EogUta *
uta_add_rect (EogUta *uta, int x1, int y1, int x2, int y2)
{
	EogUtaBbox *utiles;
	EogUtaBbox bb;
	int rect_x1, rect_y1, rect_x2, rect_y2;
	int xf1, yf1, xf2, yf2;
	int x, y;
	int ofs;

	g_return_val_if_fail (x1 < x2, NULL);
	g_return_val_if_fail (y1 < y2, NULL);

	/* Empty uta */

	if (!uta) {
		EogIRect r;

		r.x0 = x1;
		r.y0 = y1;
		r.x1 = x2;
		r.y1 = y2;

		return eog_uta_from_irect (&r);
	}

	/* Grow the uta if necessary */

	rect_x1 = x1 >> EOG_UTILE_SHIFT;
	rect_y1 = y1 >> EOG_UTILE_SHIFT;
	rect_x2 = (x2 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;
	rect_y2 = (y2 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;

	uta = uta_ensure_size (uta, rect_x1, rect_y1, rect_x2, rect_y2);

	/* Add the rectangle */

	xf1 = x1 & (EOG_UTILE_SIZE - 1);
	yf1 = y1 & (EOG_UTILE_SIZE - 1);
	xf2 = ((x2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;
	yf2 = ((y2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;

	utiles = uta->utiles;

	ofs = (rect_y1 - uta->y0) * uta->width + rect_x1 - uta->x0;

	if (rect_y2 - rect_y1 == 1) {
		if (rect_x2 - rect_x1 == 1) {
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					xf1, yf1, xf2, yf2);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
		} else {
			/* Leftmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					xf1, yf1, EOG_UTILE_SIZE, yf2);
			else
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					EOG_UTILE_SIZE,
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));

			/* Tiles in between */
			for (x = rect_x1 + 1; x < rect_x2 - 1; x++) {
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0, yf1, EOG_UTILE_SIZE, yf2);
				else
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0,
						MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
						EOG_UTILE_SIZE,
						MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
			}

			/* Rightmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0, yf1, xf2, yf2);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0,
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
		}
	} else {
		if (rect_x2 - rect_x1 == 1) {
			/* Topmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					xf1, yf1, xf2, EOG_UTILE_SIZE);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					EOG_UTILE_SIZE);
			ofs += uta->width;

			/* Tiles in between */
			for (y = rect_y1 + 1; y < rect_y2 - 1; y++) {
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs] = EOG_UTA_BBOX_CONS (
						xf1, 0, xf2, EOG_UTILE_SIZE);
				else
					utiles[ofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (bb), xf1),
						0,
						MAX (EOG_UTA_BBOX_X1 (bb), xf2),
						EOG_UTILE_SIZE);
				ofs += uta->width;
			}

			/* Bottommost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					xf1, 0, xf2, yf2);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					0,
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
		} else {
			/* Top row, leftmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					xf1, yf1, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
			else
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					EOG_UTILE_SIZE,
					EOG_UTILE_SIZE);

			/* Top row, in between */
			for (x = rect_x1 + 1; x < rect_x2 - 1; x++) {
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0, yf1, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
				else
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0,
						MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
						EOG_UTILE_SIZE,
						EOG_UTILE_SIZE);
			}

			/* Top row, rightmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0, yf1, xf2, EOG_UTILE_SIZE);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0,
					MIN (EOG_UTA_BBOX_Y0 (bb), yf1),
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					EOG_UTILE_SIZE);

			ofs += uta->width - (rect_x2 - rect_x1 - 1);

			/* Rows in between */
			for (y = rect_y1 + 1; y < rect_y2 - 1; y++) {
				/* Leftmost tile */
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						xf1, 0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
				else
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (bb), xf1),
						0,
						EOG_UTILE_SIZE,
						EOG_UTILE_SIZE);

				/* Tiles in between */
				bb = EOG_UTA_BBOX_CONS (0, 0, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
				for (x = rect_x1 + 1; x < rect_x2 - 1; x++)
					utiles[ofs++] = bb;

				/* Rightmost tile */
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs] = EOG_UTA_BBOX_CONS (
						0, 0, xf2, EOG_UTILE_SIZE);
				else
					utiles[ofs] = EOG_UTA_BBOX_CONS (
						0,
						0,
						MAX (EOG_UTA_BBOX_X1 (bb), xf2),
						EOG_UTILE_SIZE);

				ofs += uta->width - (rect_x2 - rect_x1 - 1);
			}

			/* Bottom row, leftmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					xf1, 0, EOG_UTILE_SIZE, yf2);
			else
				utiles[ofs++] = EOG_UTA_BBOX_CONS (
					MIN (EOG_UTA_BBOX_X0 (bb), xf1),
					0,
					EOG_UTILE_SIZE,
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));

			/* Bottom row, tiles in between */
			for (x = rect_x1 + 1; x < rect_x2 - 1; x++) {
				bb = utiles[ofs];
				if (bb == 0)
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0, 0, EOG_UTILE_SIZE, yf2);
				else
					utiles[ofs++] = EOG_UTA_BBOX_CONS (
						0,
						0,
						EOG_UTILE_SIZE,
						MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
			}

			/* Bottom row, rightmost tile */
			bb = utiles[ofs];
			if (bb == 0)
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0, 0, xf2, yf2);
			else
				utiles[ofs] = EOG_UTA_BBOX_CONS (
					0,
					0,
					MAX (EOG_UTA_BBOX_X1 (bb), xf2),
					MAX (EOG_UTA_BBOX_Y1 (bb), yf2));
		}
	}

	return uta;
}

/**
 * uta_remove_rect:
 * @uta: A microtile array.
 * @x1: Left coordinate of rectangle.
 * @y1: Top coordinate of rectangle.
 * @x2: Right coordinate of rectangle.
 * @y2: Bottom coordinate of rectangle.
 *
 * Removes a rectangular region from the specified microtile array.  Due to the
 * way microtile arrays represent regions, the tiles at the edge of the
 * rectangle may not be clipped exactly.
 **/
void
uta_remove_rect (EogUta *uta, int x1, int y1, int x2, int y2)
{
	EogUtaBbox *utiles;
	int rect_x1, rect_y1, rect_x2, rect_y2;
	int clip_x1, clip_y1, clip_x2, clip_y2;
	int xf1, yf1, xf2, yf2;
	int ofs;
	int x, y;

	g_return_if_fail (uta != NULL);
	g_return_if_fail (x1 <= x2);
	g_return_if_fail (y1 <= y2);

	if (x1 == x2 || y1 == y2)
		return;

	rect_x1 = x1 >> EOG_UTILE_SHIFT;
	rect_y1 = y1 >> EOG_UTILE_SHIFT;
	rect_x2 = (x2 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;
	rect_y2 = (y2 + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;

	clip_x1 = MAX (rect_x1, uta->x0);
	clip_y1 = MAX (rect_y1, uta->y0);
	clip_x2 = MIN (rect_x2, uta->x0 + uta->width);
	clip_y2 = MIN (rect_y2, uta->y0 + uta->height);

	if (clip_x1 >= clip_x2 || clip_y1 >= clip_y2)
		return;

	xf1 = x1 & (EOG_UTILE_SIZE - 1);
	yf1 = y1 & (EOG_UTILE_SIZE - 1);
	xf2 = ((x2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;
	yf2 = ((y2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;

	utiles = uta->utiles;

	ofs = (clip_y1 - uta->y0) * uta->width + clip_x1 - uta->x0;

	for (y = clip_y1; y < clip_y2; y++) {
		int cy1, cy2;

		if (y == rect_y1)
			cy1 = yf1;
		else
			cy1 = 0;

		if (y == rect_y2 - 1)
			cy2 = yf2;
		else
			cy2 = EOG_UTILE_SIZE;

		for (x = clip_x1; x < clip_x2; x++) {
			int cx1, cx2;
			EogUtaBbox bb;
			int bb_x1, bb_y1, bb_x2, bb_y2;
			int bb_cx1, bb_cy1, bb_cx2, bb_cy2;

			bb = utiles[ofs];
			bb_x1 = EOG_UTA_BBOX_X0 (bb);
			bb_y1 = EOG_UTA_BBOX_Y0 (bb);
			bb_x2 = EOG_UTA_BBOX_X1 (bb);
			bb_y2 = EOG_UTA_BBOX_Y1 (bb);

			if (x == rect_x1)
				cx1 = xf1;
			else
				cx1 = 0;

			if (x == rect_x2 - 1)
				cx2 = xf2;
			else
				cx2 = EOG_UTILE_SIZE;

			/* Clip top and bottom */

			if (cx1 <= bb_x1 && cx2 >= bb_x2) {
				if (cy1 <= bb_y1 && cy2 > bb_y1)
					bb_cy1 = cy2;
				else
					bb_cy1 = bb_y1;

				if (cy1 < bb_y2 && cy2 >= bb_y2)
					bb_cy2 = cy1;
				else
					bb_cy2 = bb_y2;
			} else {
				bb_cy1 = bb_y1;
				bb_cy2 = bb_y2;
			}

			/* Clip left and right */

			if (cy1 <= bb_y1 && cy2 >= bb_y2) {
				if (cx1 <= bb_x1 && cx2 > bb_x1)
					bb_cx1 = cx2;
				else
					bb_cx1 = bb_x1;

				if (cx1 < bb_x2 && cx2 >= bb_x2)
					bb_cx2 = cx1;
				else
					bb_cx2 = bb_x2;
			} else {
				bb_cx1 = bb_x1;
				bb_cx2 = bb_x2;
			}

			if (bb_cx1 < bb_cx2 && bb_cy1 < bb_cy2)
				utiles[ofs] = EOG_UTA_BBOX_CONS (bb_cx1, bb_cy1,
								 bb_cx2, bb_cy2);
			else
				utiles[ofs] = 0;

			ofs++;
		}

		ofs += uta->width - (clip_x2 - clip_x1);
	}
}

void
uta_find_first_glom_rect (EogUta *uta, EogIRect *rect, int max_width, int max_height)
{
  EogIRect *rects;
  int n_rects, n_rects_max;
  int x, y;
  int width, height;
  int ix;
  int left_ix;
  EogUtaBbox *utiles;
  EogUtaBbox bb;
  int x0, y0, x1, y1;
  int *glom;
  int glom_rect;

  n_rects = 0;
  n_rects_max = 1;
  rects = g_new (EogIRect, n_rects_max);

  width = uta->width;
  height = uta->height;
  utiles = uta->utiles;

  glom = g_new (int, width * height);
  for (ix = 0; ix < width * height; ix++)
    glom[ix] = -1;

  ix = 0;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	bb = utiles[ix];
	if (bb)
	  {
	    x0 = ((uta->x0 + x) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_X0(bb);
	    y0 = ((uta->y0 + y) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_Y0(bb);
	    y1 = ((uta->y0 + y) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_Y1(bb);

	    left_ix = ix;
	    /* now try to extend to the right */
	    while (x != width - 1 &&
		   EOG_UTA_BBOX_X1(bb) == EOG_UTILE_SIZE &&
		   (((bb & 0xffffff) ^ utiles[ix + 1]) & 0xffff00ff) == 0 &&
		   (((uta->x0 + x + 1) << EOG_UTILE_SHIFT) +
		    EOG_UTA_BBOX_X1(utiles[ix + 1]) -
		    x0) <= max_width)
	      {
		bb = utiles[ix + 1];
		ix++;
		x++;
	      }
	    x1 = ((uta->x0 + x) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_X1(bb);


	    /* if rectangle nonempty */
	    if ((x1 ^ x0) || (y1 ^ y0))
	      {
		/* try to glom onto an existing rectangle */
		glom_rect = glom[left_ix];
		if (glom_rect != -1 &&
		    x0 == rects[glom_rect].x0 &&
		    x1 == rects[glom_rect].x1 &&
		    y0 == rects[glom_rect].y1 &&
		    y1 - rects[glom_rect].y0 <= max_height)
		  {
		    rects[glom_rect].y1 = y1;
		  }
		else
		  {
		    if (n_rects == n_rects_max)
		      eog_expand (rects, EogIRect, n_rects_max);
		    rects[n_rects].x0 = x0;
		    rects[n_rects].y0 = y0;
		    rects[n_rects].x1 = x1;
		    rects[n_rects].y1 = y1;
		    glom_rect = n_rects;
		    n_rects++;
		  }
		if (y != height - 1)
		  glom[left_ix + width] = glom_rect;
	      }
	  }
	ix++;
      }

  if (n_rects > 0) {
	  rect->x0 = rects[0].x0;
	  rect->y0 = rects[0].y0;
	  rect->x1 = rects[0].x1;
	  rect->y1 = rects[0].y1;
  } else
	  rect->x0 = rect->y0 = rect->x1 = rect->y1 = 0;

  g_free (glom);
  g_free (rects);
}

#if 0

void
uta_find_first_glom_rect (EogUta *uta, EogIRect *rect, int max_width, int max_height)
{
	EogUtaBbox *utiles;
	EogUtaBbox bb;
	int width, height;
	int ofs;
	int x, y;
	int x1, y1, x2, y2;

	g_return_if_fail (uta != NULL);
	g_return_if_fail (rect != NULL);
	g_return_if_fail (max_width > 0 && max_height > 0);

	utiles = uta->utiles;
	width = uta->width;
	height = uta->height;

	ofs = 0;

	/* We find the first nonempty tile, and then grow the rectangle to the
	 * right and then down.
	 */

	x1 = y1 = x2 = y2 = 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			bb = utiles[ofs];

			if (!bb) {
				ofs++;
				continue;
			}

			x1 = ((uta->x0 + x) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_X0 (bb);
			y1 = ((uta->y0 + y) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_Y0 (bb);
			y2 = ((uta->y0 + y) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_Y1 (bb);

			/* Grow to the right */

			while (x != width - 1
			       && EOG_UTA_BBOX_X1 (bb) == EOG_UTILE_SIZE
			       && (((bb & 0xffffff) ^ utiles[ofs + 1]) & 0xffff00ff) == 0
			       && (((uta->x0 + x + 1) << EOG_UTILE_SHIFT)
				   + EOG_UTA_BBOX_X1 (utiles[ofs + 1])
				   - x1) <= max_width) {
				ofs++;
				bb = utiles[ofs];
				x++;
			}

			x2 = ((uta->x0 + x) << EOG_UTILE_SHIFT) + EOG_UTA_BBOX_X1 (bb);
			goto grow_down;
		}
	}

 grow_down:

}

#endif

/* Copies a single microtile to another location in the UTA, offsetted by the
 * specified distance.  A microtile can thus end up being added in a single part
 * to another microtile, in two parts to two horizontally or vertically adjacent
 * microtiles, or in four parts to a 2x2 square of microtiles.
 *
 * This is basically a normal BitBlt but with copying-forwards-to-the-destination
 * instead of fetching-backwards-from-the-source.
 */
static void
copy_tile (EogUta *uta, int x, int y, int xofs, int yofs)
{
	EogUtaBbox *utiles;
	EogUtaBbox bb, dbb;
	int t_x1, t_y1, t_x2, t_y2;
	int d_x1, d_y1, d_x2, d_y2;
	int d_tx1, d_ty1;
	int d_xf1, d_yf1, d_xf2, d_yf2;
	int dofs;

	utiles = uta->utiles;

	bb = utiles[(y - uta->y0) * uta->width + x - uta->x0];

	if (bb == 0)
		return;

	t_x1 = EOG_UTA_BBOX_X0 (bb) + (x << EOG_UTILE_SHIFT);
	t_y1 = EOG_UTA_BBOX_Y0 (bb) + (y << EOG_UTILE_SHIFT);
	t_x2 = EOG_UTA_BBOX_X1 (bb) + (x << EOG_UTILE_SHIFT);
	t_y2 = EOG_UTA_BBOX_Y1 (bb) + (y << EOG_UTILE_SHIFT);

	d_x1 = t_x1 + xofs;
	d_y1 = t_y1 + yofs;
	d_x2 = t_x2 + xofs;
	d_y2 = t_y2 + yofs;

	d_tx1 = d_x1 >> EOG_UTILE_SHIFT;
	d_ty1 = d_y1 >> EOG_UTILE_SHIFT;

	dofs = (d_ty1 - uta->y0) * uta->width + d_tx1 - uta->x0;

	d_xf1 = d_x1 & (EOG_UTILE_SIZE - 1);
	d_yf1 = d_y1 & (EOG_UTILE_SIZE - 1);
	d_xf2 = ((d_x2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;
	d_yf2 = ((d_y2 - 1) & (EOG_UTILE_SIZE - 1)) + 1;

	if (d_x2 - d_x1 <= EOG_UTILE_SIZE - d_xf1) {
		if (d_y2 - d_y1 <= EOG_UTILE_SIZE - d_yf1) {
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, d_yf1, d_xf2, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}
		} else {
			/* Top tile */
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, d_yf1, d_xf2, EOG_UTILE_SIZE);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						EOG_UTILE_SIZE);
			}

			dofs += uta->width;

			/* Bottom tile */
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 + 1 >= uta->y0 && d_ty1 + 1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, 0, d_xf2, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						0,
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}
		}
	} else {
		if (d_y2 - d_y1 <= EOG_UTILE_SIZE - d_yf1) {
			/* Left tile */
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, d_yf1, EOG_UTILE_SIZE, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						EOG_UTILE_SIZE,
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}

			dofs++;

			/* Right tile */
			if (d_tx1 + 1 >= uta->x0 && d_tx1 + 1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0, d_yf1, d_xf2, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0,
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}
		} else {
			/* Top left tile */
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, d_yf1, EOG_UTILE_SIZE, EOG_UTILE_SIZE);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						EOG_UTILE_SIZE,
						EOG_UTILE_SIZE);
			}

			dofs++;

			/* Top right tile */
			if (d_tx1 + 1 >= uta->x0 && d_tx1 + 1 < uta->x0 + uta->width
			    && d_ty1 >= uta->y0 && d_ty1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0, d_yf1, d_xf2, EOG_UTILE_SIZE);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0,
						MIN (EOG_UTA_BBOX_Y0 (dbb), d_yf1),
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						EOG_UTILE_SIZE);
			}

			dofs += uta->width - 1;

			/* Bottom left tile */
			if (d_tx1 >= uta->x0 && d_tx1 < uta->x0 + uta->width
			    && d_ty1 + 1 >= uta->y0 && d_ty1 + 1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						d_xf1, 0, EOG_UTILE_SIZE, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						MIN (EOG_UTA_BBOX_X0 (dbb), d_xf1),
						0,
						EOG_UTILE_SIZE,
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}

			dofs++;

			/* Bottom right tile */
			if (d_tx1 + 1 >= uta->x0 && d_tx1 + 1 < uta->x0 + uta->width
			    && d_ty1 + 1 >= uta->y0 && d_ty1 + 1 < uta->y0 + uta->height) {
				dbb = utiles[dofs];
				if (dbb == 0)
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0, 0, d_xf2, d_yf2);
				else
					utiles[dofs] = EOG_UTA_BBOX_CONS (
						0,
						0,
						MAX (EOG_UTA_BBOX_X1 (dbb), d_xf2),
						MAX (EOG_UTA_BBOX_Y1 (dbb), d_yf2));
			}
		}
	}
}

/**
 * uta_copy_area:
 * @uta: A microtile array.
 * @src_x: Left coordinate of source rectangle.
 * @src_y: Top coordinate of source rectangle.
 * @dest_x: Left coordinate of destination.
 * @dest_y: Top coordinate of destination.
 * @width: Width of region to copy.
 * @height: Height of region to copy.
 *
 * Copies a rectangular region within a microtile array.  The array will not be
 * expanded if the destination area does not fit within it; rather only the area
 * that fits will be copied.  The source rectangle must be completely contained
 * within the microtile array.
 **/
void
uta_copy_area (EogUta *uta, int src_x, int src_y, int dest_x, int dest_y, int width, int height)
{
	int rect_x1, rect_y1, rect_x2, rect_y2;
	gboolean top_to_bottom, left_to_right;
	int xofs, yofs;
	int x, y;

	g_return_if_fail (uta != NULL);
	g_return_if_fail (width >= 0 && height >= 0);
	g_return_if_fail (src_x >= uta->x0 << EOG_UTILE_SHIFT);
	g_return_if_fail (src_y >= uta->y0 << EOG_UTILE_SHIFT);
	g_return_if_fail (src_x + width <= (uta->x0 + uta->width) << EOG_UTILE_SHIFT);
	g_return_if_fail (src_y + height <= (uta->y0 + uta->height) << EOG_UTILE_SHIFT);

	if ((src_x == dest_x && src_y == dest_y) || width == 0 || height == 0)
		return;

	/* FIXME: This function is not perfect.  It *adds* the copied/offsetted
	 * area to the original contents of the microtile array, thus growing
	 * the region more than needed.  The effect should be to "overwrite" the
	 * original contents, just like XCopyArea() does.  Care needs to be
	 * taken when the edges of the rectangle do not fall on microtile
	 * boundaries, because tiles may need to be "split".
	 *
	 * Maybe this will work:
	 *
	 * 1. Copy the rectangular array of tiles that form the region to a
	 *    temporary buffer.
	 *
	 * 2. uta_remove_rect() the *destination* rectangle from the original
	 *    microtile array.
	 *
	 * 3. Copy back the temporary buffer to the original array while
	 *    offsetting it in the same way as copy_tile() does.
	 */

	rect_x1 = src_x >> EOG_UTILE_SHIFT;
	rect_y1 = src_y >> EOG_UTILE_SHIFT;
	rect_x2 = (src_x + width + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;
	rect_y2 = (src_y + height + EOG_UTILE_SIZE - 1) >> EOG_UTILE_SHIFT;

	xofs = dest_x - src_x;
	yofs = dest_y - src_y;

	left_to_right = xofs < 0;
	top_to_bottom = yofs < 0;

	if (top_to_bottom && left_to_right) {
		for (y = rect_y1; y < rect_y2; y++)
			for (x = rect_x1; x < rect_x2; x++)
				copy_tile (uta, x, y, xofs, yofs);
	} else if (top_to_bottom && !left_to_right) {
		for (y = rect_y1; y < rect_y2; y++)
			for (x = rect_x2 - 1; x >= rect_x1; x--)
				copy_tile (uta, x, y, xofs, yofs);
	} else if (!top_to_bottom && left_to_right) {
		for (y = rect_y2 - 1; y >= rect_y1; y--)
			for (x = rect_x1; x < rect_x2; x++)
				copy_tile (uta, x, y, xofs, yofs);
	} else if (!top_to_bottom && !left_to_right) {
		for (y = rect_y2 - 1; y >= rect_y1; y--)
			for (x = rect_x2 - 1; x >= rect_x1; x--)
				copy_tile (uta, x, y, xofs, yofs);
	}
}
