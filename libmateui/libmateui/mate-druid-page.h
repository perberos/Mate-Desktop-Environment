/* mate-druid-page.h
 * Copyright (C) 1999  Red Hat, Inc.
 * All rights reserved.
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
/*
  @NOTATION@
*/
#ifndef __MATE_DRUID_PAGE_H__
#define __MATE_DRUID_PAGE_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libmatecanvas/mate-canvas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_DRUID_PAGE            (mate_druid_page_get_type ())
#define MATE_DRUID_PAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DRUID_PAGE, MateDruidPage))
#define MATE_DRUID_PAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DRUID_PAGE, MateDruidPageClass))
#define MATE_IS_DRUID_PAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DRUID_PAGE))
#define MATE_IS_DRUID_PAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DRUID_PAGE))
#define MATE_DRUID_PAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DRUID_PAGE, MateDruidPageClass))


typedef struct _MateDruidPage        MateDruidPage;
typedef struct _MateDruidPagePrivate MateDruidPagePrivate;
typedef struct _MateDruidPageClass   MateDruidPageClass;

struct _MateDruidPage
{
	GtkBin parent;

	/*< private >*/
	MateDruidPagePrivate *_priv;
};
struct _MateDruidPageClass
{
	GtkBinClass parent_class;

	gboolean (*next)	(MateDruidPage *druid_page, GtkWidget *druid);
	void     (*prepare)	(MateDruidPage *druid_page, GtkWidget *druid);
	gboolean (*back)	(MateDruidPage *druid_page, GtkWidget *druid);
	void     (*finish)	(MateDruidPage *druid_page, GtkWidget *druid);
	gboolean (*cancel)	(MateDruidPage *druid_page, GtkWidget *druid);

	/* Signal used for relaying out the canvas */
	void     (*configure_canvas) (MateDruidPage *druid_page);

	/* virtual */
	void	 (*set_sidebar_shown) (MateDruidPage *druid_page,
				       gboolean sidebar_shown);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      mate_druid_page_get_type            (void) G_GNUC_CONST;
GtkWidget *mate_druid_page_new                 (void);
/* These are really to be only called from MateDruid */
gboolean   mate_druid_page_next                (MateDruidPage *druid_page);
void       mate_druid_page_prepare             (MateDruidPage *druid_page);
gboolean   mate_druid_page_back                (MateDruidPage *druid_page);
gboolean   mate_druid_page_cancel              (MateDruidPage *druid_page);
void       mate_druid_page_finish              (MateDruidPage *druid_page);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_DRUID_PAGE_H__ */




