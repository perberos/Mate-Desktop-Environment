/* mate-druid.h
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
/* TODO: allow setting bgcolor for all pages globally */
#ifndef __MATE_DRUID_H__
#define __MATE_DRUID_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include "mate-druid-page.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_DRUID            (mate_druid_get_type ())
#define MATE_DRUID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DRUID, MateDruid))
#define MATE_DRUID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DRUID, MateDruidClass))
#define MATE_IS_DRUID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DRUID))
#define MATE_IS_DRUID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DRUID))
#define MATE_DRUID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DRUID, MateDruidClass))


typedef struct _MateDruid        MateDruid;
typedef struct _MateDruidPrivate MateDruidPrivate;
typedef struct _MateDruidClass   MateDruidClass;

struct _MateDruid
{
	GtkContainer parent;
	GtkWidget *help;
	GtkWidget *back;
	GtkWidget *next;
	GtkWidget *cancel;
	GtkWidget *finish;

	/*< private >*/
	MateDruidPrivate *_priv;
};
struct _MateDruidClass
{
	GtkContainerClass parent_class;

	void     (*cancel)	(MateDruid *druid);
	void     (*help)	(MateDruid *druid);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


GType      mate_druid_get_type              (void) G_GNUC_CONST;
GtkWidget *mate_druid_new                   (void);
void	   mate_druid_set_buttons_sensitive (MateDruid *druid,
					      gboolean back_sensitive,
					      gboolean next_sensitive,
					      gboolean cancel_sensitive,
					      gboolean help_sensitive);
void	   mate_druid_set_show_finish       (MateDruid *druid, gboolean show_finish);
void	   mate_druid_set_show_help         (MateDruid *druid, gboolean show_help);
void       mate_druid_prepend_page          (MateDruid *druid, MateDruidPage *page);
void       mate_druid_insert_page           (MateDruid *druid, MateDruidPage *back_page, MateDruidPage *page);
void       mate_druid_append_page           (MateDruid *druid, MateDruidPage *page);
void	   mate_druid_set_page              (MateDruid *druid, MateDruidPage *page);

/* Pure sugar, methods for making new druids with a window already */
GtkWidget *mate_druid_new_with_window       (const char *title,
					      GtkWindow *parent,
					      gboolean close_on_cancel,
					      GtkWidget **window);
void       mate_druid_construct_with_window (MateDruid *druid,
					      const char *title,
					      GtkWindow *parent,
					      gboolean close_on_cancel,
					      GtkWidget **window);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_DRUID_H__ */
