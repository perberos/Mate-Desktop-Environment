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

#ifndef __MATE_DATE_EDIT_H_
#define __MATE_DATE_EDIT_H_

#include <time.h>
#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	MATE_DATE_EDIT_SHOW_TIME             = 1 << 0,
	MATE_DATE_EDIT_24_HR                 = 1 << 1,
	MATE_DATE_EDIT_WEEK_STARTS_ON_MONDAY = 1 << 2,
	MATE_DATE_EDIT_DISPLAY_SECONDS       = 1 << 3
} MateDateEditFlags;


#define MATE_TYPE_DATE_EDIT            (mate_date_edit_get_type ())
#define MATE_DATE_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DATE_EDIT, MateDateEdit))
#define MATE_DATE_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DATE_EDIT, MateDateEditClass))
#define MATE_IS_DATE_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DATE_EDIT))
#define MATE_IS_DATE_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DATE_EDIT))
#define MATE_DATE_EDIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DATE_EDIT, MateDateEditClass))

typedef struct _MateDateEdit        MateDateEdit;
typedef struct _MateDateEditPrivate MateDateEditPrivate;
typedef struct _MateDateEditClass   MateDateEditClass;

struct _MateDateEdit {
	GtkHBox hbox;

	/*< private >*/
	MateDateEditPrivate *_priv;
};

struct _MateDateEditClass {
	GtkHBoxClass parent_class;

	void (*date_changed) (MateDateEdit *gde);
	void (*time_changed) (MateDateEdit *gde);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

GType      mate_date_edit_get_type        (void) G_GNUC_CONST;
GtkWidget *mate_date_edit_new            (time_t the_time,
					   gboolean show_time,
					   gboolean use_24_format);
GtkWidget *mate_date_edit_new_flags      (time_t the_time,
					   MateDateEditFlags flags);

/* Note that everything that can be achieved with mate_date_edit_new can
 * be achieved with mate_date_edit_new_flags, so that's why this call
 * is like the _new_flags call */
void      mate_date_edit_construct	  (MateDateEdit *gde,
					   time_t the_time,
					   MateDateEditFlags flags);

void      mate_date_edit_set_time        (MateDateEdit *gde, time_t the_time);
time_t    mate_date_edit_get_time        (MateDateEdit *gde);
void      mate_date_edit_set_popup_range (MateDateEdit *gde, int low_hour, int up_hour);
void      mate_date_edit_set_flags       (MateDateEdit *gde, MateDateEditFlags flags);
int       mate_date_edit_get_flags       (MateDateEdit *gde);

time_t    mate_date_edit_get_initial_time(MateDateEdit *gde);

#ifndef MATE_DISABLE_DEPRECATED
time_t    mate_date_edit_get_date        (MateDateEdit *gde);
#endif /* MATE_DISABLE_DEPRECATED */

G_END_DECLS

#endif
