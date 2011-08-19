/*
 * Copyright © 2004 Noah Levitt
 * Copyright © 2007 Christian Persch
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

/* block means unicode block */

#if !defined (__GUCHARMAP_GUCHARMAP_H_INSIDE__) && !defined (GUCHARMAP_COMPILATION)
#error "Only <gucharmap/gucharmap.h> can be included directly."
#endif

#ifndef GUCHARMAP_BLOCK_CHAPTERS_MODEL_H
#define GUCHARMAP_BLOCK_CHAPTERS_MODEL_H

#include <gucharmap/gucharmap-chapters-model.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL             (gucharmap_block_chapters_model_get_type ())
#define GUCHARMAP_BLOCK_CHAPTERS_MODEL(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL, GucharmapBlockChaptersModel))
#define GUCHARMAP_BLOCK_CHAPTERS_MODEL_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL, GucharmapBlockChaptersModelClass))
#define GUCHARMAP_IS_BLOCK_CHAPTERS_MODEL(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL))
#define GUCHARMAP_IS_BLOCK_CHAPTERS_MODEL_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL))
#define GUCHARMAP_BLOCK_CHAPTERS_MODEL_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_BLOCK_CHAPTERS_MODEL, GucharmapBlockChaptersModelClass))

typedef struct _GucharmapBlockChaptersModel GucharmapBlockChaptersModel;
typedef struct _GucharmapBlockChaptersModelPrivate GucharmapBlockChaptersModelPrivate;
typedef struct _GucharmapBlockChaptersModelClass GucharmapBlockChaptersModelClass;

struct _GucharmapBlockChaptersModel
{
  GucharmapChaptersModel parent;

  /*< private >*/
  GucharmapBlockChaptersModelPrivate *priv;
};

struct _GucharmapBlockChaptersModelClass
{
  GucharmapChaptersModelClass parent_class;
};

GType                   gucharmap_block_chapters_model_get_type (void);
GucharmapChaptersModel* gucharmap_block_chapters_model_new      (void);

G_END_DECLS

#endif /* #ifndef GUCHARMAP_BLOCK_CHAPTERS_MODEL_H */
