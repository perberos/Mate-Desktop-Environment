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

/*
 * "High Scores" Widget
 *
 * AUTHOR:
 * Horacio J. Peña <horape@compendium.com.ar>
 *
 * This is free software (under the terms of the GNU LGPL)
 *
 * USAGE:
 * Use the mate_scores_display. The other functions are going to be
 * discontinued... (ok, i should add pixmap support to *_display
 * before)
 *
 * DESCRIPTION:
 * A specialized widget to display "High Scores" for games. It's
 * very integrated with the mate-score stuff so you only need to
 * call one function to do all the work...
 *
 */

#ifndef MATE_SCORES_H
#define MATE_SCORES_H

#ifndef MATE_DISABLE_DEPRECATED

#include <time.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_SCORES \
	(mate_scores_get_type())
#define MATE_SCORES(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), MATE_TYPE_SCORES, MateScores))
#define MATE_SCORES_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), MATE_TYPE_SCORES, MateScoresClass))
#define MATE_IS_SCORES(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), MATE_TYPE_SCORES))
#define MATE_IS_SCORES_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), MATE_TYPE_SCORES))
#define MATE_SCORES_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), MATE_TYPE_SCORES, MateScoresClass))

typedef struct _MateScores MateScores;
typedef struct _MateScoresPrivate MateScoresPrivate;
typedef struct _MateScoresClass MateScoresClass;

struct _MateScores {
	GtkDialog dialog;

	/*< private >*/
	MateScoresPrivate* _priv;
};

struct _MateScoresClass {
	GtkDialogClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

GType mate_scores_get_type(void) G_GNUC_CONST;
GtkWidget* mate_scores_display(const gchar* title, const gchar* app_name, const gchar* level, int pos);
GtkWidget* mate_scores_display_with_pixmap(const gchar* pixmap_logo, const gchar* app_name, const gchar* level, int pos);
GtkWidget* mate_scores_new(guint n_scores, gchar** names, gfloat* scores, time_t* times, gboolean clear);
void mate_scores_construct(MateScores* gs, guint n_scores, gchar** names, gfloat* scores, time_t* times, gboolean clear);
void mate_scores_set_logo_label(MateScores* gs, const gchar* txt, const gchar* font, GdkColor* col);
void mate_scores_set_logo_pixmap(MateScores* gs, const gchar* pix_name);
void mate_scores_set_logo_widget(MateScores* gs, GtkWidget* w);
void mate_scores_set_color(MateScores* gs, guint n, GdkColor* col);
void mate_scores_set_def_color(MateScores* gs, GdkColor* col);
void mate_scores_set_colors(MateScores* gs, GdkColor* col);
void mate_scores_set_logo_label_title(MateScores* gs, const gchar* txt);
void mate_scores_set_current_player(MateScores* gs, gint i);

#ifdef __cplusplus
}
#endif


#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_SCORES_H */

