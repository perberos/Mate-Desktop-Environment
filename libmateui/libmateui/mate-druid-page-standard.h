/* mate-druid-page-standard.h
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2001  James M. Cape <jcape@ignore-your.tv>
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
#ifndef __MATE_DRUID_PAGE_STANDARD_H__
#define __MATE_DRUID_PAGE_STANDARD_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include "mate-druid-page.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_DRUID_PAGE_STANDARD            (mate_druid_page_standard_get_type ())
#define MATE_DRUID_PAGE_STANDARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DRUID_PAGE_STANDARD, MateDruidPageStandard))
#define MATE_DRUID_PAGE_STANDARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DRUID_PAGE_STANDARD, MateDruidPageStandardClass))
#define MATE_IS_DRUID_PAGE_STANDARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DRUID_PAGE_STANDARD))
#define MATE_IS_DRUID_PAGE_STANDARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DRUID_PAGE_STANDARD))
#define MATE_DRUID_PAGE_STANDARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DRUID_PAGE_STANDARD, MateDruidPageStandardClass))


typedef struct _MateDruidPageStandard        MateDruidPageStandard;
typedef struct _MateDruidPageStandardPrivate MateDruidPageStandardPrivate;
typedef struct _MateDruidPageStandardClass   MateDruidPageStandardClass;

/**
 * MateDruidPageStandard
 * @vbox: A packing widget that holds the contents of the page.
 * @title: The title of the displayed page.
 * @logo: The logo of the displayed page.
 * @top_watermark: The watermark at the top of the displated page.
 * @title_foreground: The color of the title text.
 * @background: The color of the background of the top section and title.
 * @logo_background: The background color of the logo.
 * @contents_background: The background color of the contents section.
 *
 * A widget representing pages that are not initial or terminal pages of a
 * druid.
 */
struct _MateDruidPageStandard
{
	MateDruidPage parent;

	/*< public >*/
	GtkWidget *vbox;
	gchar *title;
	GdkPixbuf *logo;
	GdkPixbuf *top_watermark;
	GdkColor title_foreground;
	GdkColor background;
	GdkColor logo_background;
	GdkColor contents_background;

	/*< private >*/
	MateDruidPageStandardPrivate *_priv;
};
struct _MateDruidPageStandardClass
{
	MateDruidPageClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

#define mate_druid_page_standard_set_bg_color      mate_druid_page_standard_set_background
#define mate_druid_page_standard_set_logo_bg_color mate_druid_page_standard_set_logo_background
#define mate_druid_page_standard_set_title_color   mate_druid_page_standard_set_title_foreground


GType      mate_druid_page_standard_get_type                (void) G_GNUC_CONST;
GtkWidget *mate_druid_page_standard_new                     (void);
GtkWidget *mate_druid_page_standard_new_with_vals           (const gchar            *title,
							      GdkPixbuf              *logo,
							      GdkPixbuf              *top_watermark);

void       mate_druid_page_standard_set_title               (MateDruidPageStandard *druid_page_standard,
							      const gchar            *title);
void       mate_druid_page_standard_set_logo                (MateDruidPageStandard *druid_page_standard,
							      GdkPixbuf              *logo_image);
void       mate_druid_page_standard_set_top_watermark       (MateDruidPageStandard *druid_page_standard,
							      GdkPixbuf              *top_watermark_image);
void       mate_druid_page_standard_set_title_foreground    (MateDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       mate_druid_page_standard_set_background          (MateDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       mate_druid_page_standard_set_logo_background     (MateDruidPageStandard *druid_page_standard,
							      GdkColor               *color);
void       mate_druid_page_standard_set_contents_background (MateDruidPageStandard *druid_page_standard,
							      GdkColor               *color);

/* Convenience Function */
void       mate_druid_page_standard_append_item             (MateDruidPageStandard *druid_page_standard,
							      const gchar            *question,
							      GtkWidget              *item,
							      const gchar            *additional_info);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_DRUID_PAGE_STANDARD_H__ */

