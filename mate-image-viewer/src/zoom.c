/* Eye of Mate image viewer - utility functions for computing zoom factors
 *
 * Copyright (C) 2000 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include "zoom.h"



/**
 * zoom_fit_size:
 * @dest_width: Width of destination area.
 * @dest_height: Height of destination area.
 * @src_width: Width of source image.
 * @src_height: Height of source image.
 * @upscale_smaller: Whether to scale up images smaller than the destination size.
 * @width: Return value for image width.
 * @height: Return value for image height.
 *
 * Computes the final dimensions of an image that is to be scaled to fit to a
 * certain size.  If @upscale_smaller is TRUE, then images smaller than the
 * destination size will be scaled up; otherwise, they will be left at their
 * original size.
 **/
void
zoom_fit_size (guint dest_width, guint dest_height,
	       guint src_width, guint src_height,
	       gboolean upscale_smaller,
	       guint *width, guint *height)
{
	guint w, h;

	g_return_if_fail (width != NULL);
	g_return_if_fail (height != NULL);

	if (src_width == 0 || src_height == 0) {
		*width = 0;
		*height = 0;
		return;
	}

	if (src_width <= dest_width && src_height <= dest_height && !upscale_smaller) {
		*width = src_width;
		*height = src_height;
		return;
	}

	w = dest_width;
	h = floor ((double) (src_height * w) / src_width + 0.5);

	if (h > dest_height) {
		h = dest_height;
		w = floor ((double) (src_width * h) / src_height + 0.5);
	}

	g_assert (w <= dest_width);
	g_assert (h <= dest_height);

	*width = w;
	*height = h;
}

/**
 * zoom_fit_scale:
 * @dest_width: Width of destination area.
 * @dest_height: Height of destination area.
 * @src_width: Width of source image.
 * @src_height: Height of source image.
 * @upscale_smaller: Whether to scale up images smaller than the destination size.
 *
 * Similar to zoom_fit_size(), but returns the zoom factor of the final image
 * with respect to the original image's size.
 *
 * Return value: Zoom factor with respect to the original size.
 **/
double
zoom_fit_scale (guint dest_width, guint dest_height,
		guint src_width, guint src_height,
		gboolean upscale_smaller)
{
	guint w, h;
	double wfactor, hfactor;
	double factor;

	if (src_width == 0 || src_height == 0)
		return 1.0;

	if (dest_width == 0 || dest_height == 0)
		return 0.0;

	zoom_fit_size (dest_width, dest_height, src_width, src_height, upscale_smaller, &w, &h);

	wfactor = (double) w / src_width;
	hfactor = (double) h / src_height;

	factor = MIN (wfactor, hfactor);

	return factor;
}

