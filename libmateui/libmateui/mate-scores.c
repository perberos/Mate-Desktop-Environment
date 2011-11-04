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
 * G(NOME|TK) Scores Widget by Horacio J. Peña <horape@compendium.com.ar>
 *
 */

#include <config.h>

#include <string.h>
#include <time.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libmate/mate-macros.h>
#include <libmate/mate-util.h>
#include <libmate/mate-config.h>
#include <libmate/mate-score.h>

#include "mate-scores.h"

struct _MateScoresPrivate {
	GtkWidget* but_clear;
	GtkTable* table;

	GtkWidget* logo_container;
	GtkWidget* logo;
	GtkWidget** label_names;
	GtkWidget** label_scores;
	GtkWidget** label_times;

	guint n_scores;
};

static void mate_scores_finalize(GObject* object);

/**
 * mate_scores_get_type:
 *
 * Returns the GType for the MateScores widget
 */
MATE_CLASS_BOILERPLATE(MateScores, mate_scores, GtkDialog, GTK_TYPE_DIALOG)

static void mate_scores_instance_init(MateScores* gs)
{
	GtkWidget *label;

	gs->_priv = g_new0(MateScoresPrivate, 1);

	gtk_window_set_title (GTK_WINDOW (gs), _("Top Ten"));

	gtk_dialog_add_button (GTK_DIALOG (gs),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	gs->_priv->logo = NULL;
	gs->_priv->but_clear = NULL;

	gs->_priv->table    = GTK_TABLE( gtk_table_new (0, 3, FALSE) );
	gtk_table_set_col_spacings (gs->_priv->table, 30);
	gtk_table_set_row_spacings (gs->_priv->table,  5);

	label = gtk_label_new ( _("User") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 0, 1, 0, 1);
	label = gtk_label_new ( _("Score") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 1, 2, 0, 1);
	label = gtk_label_new ( _("Date") );
	gtk_widget_show (label);
	gtk_table_attach_defaults ( gs->_priv->table, label, 2, 3, 0, 1);

	gtk_widget_show(GTK_WIDGET(gs->_priv->table));

	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (gs)->vbox),
			  GTK_WIDGET (gs->_priv->table),
			  TRUE, TRUE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (gs), 5);

	g_signal_connect_swapped (gs, "response",
				  G_CALLBACK (gtk_widget_destroy),
				  gs);

	gs->_priv->logo_container = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(gs->_priv->logo_container);

	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (gs)->vbox),
			  gs->_priv->logo_container,
			  FALSE, FALSE, 0);
}

static void mate_scores_class_init(MateScoresClass* class)
{
	GObjectClass *gobject_class;

	gobject_class = (GObjectClass *) class;

	gobject_class->finalize = mate_scores_finalize;
}

static void mate_scores_finalize(GObject* object)
{
	MateScores *gs = MATE_SCORES(object);

	g_free (gs->_priv->label_names);
	gs->_priv->label_names = NULL;
	g_free (gs->_priv->label_scores);
	gs->_priv->label_scores = NULL;
	g_free (gs->_priv->label_times);
	gs->_priv->label_times = NULL;

	g_free (gs->_priv);
	gs->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/**
 * mate_scores_construct:
 * @gs:
 * @n_scores: Number of positions.
 * @names: Names of the players.
 * @scores: Scores
 * @times: Time in which the scores were done
 * @clear: Add a "Clear" Button?
 *
 * Description: useful for language bindings and subclassing, not to be used by
 * itself, see #mate_scores_new.
 */
void mate_scores_construct(MateScores* gs, guint n_scores, gchar** names, gfloat* scores, time_t* times, gboolean clear)
{
	gchar     	tmp[10];
	gchar     	tmp2[256];
	guint           i;

	gs->_priv->n_scores = n_scores;

	gs->_priv->label_names  = g_malloc(sizeof(GtkWidget*) * n_scores);
	gs->_priv->label_scores = g_malloc(sizeof(GtkWidget*) * n_scores);
	gs->_priv->label_times  = g_malloc(sizeof(GtkWidget*) * n_scores);

	for(i=0; i < n_scores; i++) {
		char *str_utf8;

		gs->_priv->label_names[i] = gtk_label_new ( names[i] );
		gtk_widget_show ( gs->_priv->label_names[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_names[i], 0, 1, i+1, i+2);

		g_snprintf(tmp,sizeof(tmp),"%5.2f", scores[i]);
		gs->_priv->label_scores[i] = gtk_label_new ( tmp );
		gtk_widget_show ( gs->_priv->label_scores[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_scores[i], 1, 2, i+1, i+2);

		/* TRANSLATORS NOTE: Date format used when showing scores in games.
		 * Please refer to manpage of strftime(3) for complete reference.
		 */
		if(strftime(tmp2, sizeof(tmp2), _("%a %b %d %T %Y"),
			    localtime( &(times[i]) )) == 0) {
			/* according to docs, if the string does not fit, the
			 * contents of tmp2 are undefined, thus just use
			 * ??? */
			strcpy(tmp2, "???");
		}
		tmp2[sizeof(tmp2)-1] = '\0'; /* just for sanity */

		str_utf8 = g_locale_to_utf8 (tmp2, -1, NULL, NULL, NULL);
		gs->_priv->label_times[i] = gtk_label_new (str_utf8 ? str_utf8 : NULL);
		g_free (str_utf8);
		gtk_widget_show ( gs->_priv->label_times[i] );
		gtk_table_attach_defaults ( gs->_priv->table, gs->_priv->label_times[i], 2, 3, i+1, i+2);
  	}

	/*
	if(clear) {
	  gs->_priv->but_clear = gtk_button_new_with_label ( _("Clear") );
	  gtk_widget_show (gs->_priv->but_clear);
	  gtk_table_attach_defaults ( GTK_TABLE(hor_table), gs->_priv->but_clear, 3, 4, 0, 1);
  	}
	*/
}

/**
 * mate_scores_new:
 * @n_scores: Number of positions.
 * @names: Names of the players.
 * @scores: Scores
 * @times: Time in which the scores were done
 * @clear: Add a "Clear" Button?
 *
 * Description: Creates the high-scores window.
 *
 * Returns: A new #MateScores widget
 */
GtkWidget* mate_scores_new(guint n_scores, gchar** names, gfloat* scores, time_t* times, gboolean clear)
{
	GtkWidget *retval = g_object_new (MATE_TYPE_SCORES, NULL);

	mate_scores_construct(MATE_SCORES(retval), n_scores, names, scores, times, clear);

	return retval;
}

/**
 * mate_scores_set_color:
 * @gs: A #MateScores widget
 * @n: Entry to be changed.
 * @col: Color.
 *
 * Description: Set the color of one entry.
 */
void mate_scores_set_color(MateScores* gs, guint n, GdkColor* col)
{
	GtkStyle *s = gtk_style_new();

	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(col != NULL);
	g_return_if_fail(n < gs->_priv->n_scores);

	/* i believe that i should copy the default style
	   and change only the fg field, how? */

	memcpy((void *) &s->fg[0], col, sizeof(GdkColor) );
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_names[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_scores[n]), s);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->label_times[n]), s);

	g_object_unref (G_OBJECT (s));
}

/**
 * mate_scores_set_def_color:
 * @gs: A #MateScores widget
 * @col: Color
 *
 * Description: Set the default color of the entries.
 */
void mate_scores_set_def_color(MateScores* gs, GdkColor* col)
{
	unsigned int i;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(col != NULL);

	for(i=0;i<gs->_priv->n_scores;i++) {
		mate_scores_set_color(gs, i, col);
	}
}

/**
 * mate_scores_set_colors:
 * @gs: A #MateScores widget
 * @col: Array of colors.
 *
 * Description: Set the color of all the entries.
 */
void mate_scores_set_colors(MateScores* gs, GdkColor* col)
{
	unsigned int i;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(col != NULL);

	for(i=0;i<gs->_priv->n_scores;i++) {
		mate_scores_set_color(gs, i, col+i);
	}
}

/**
 * mate_scores_set_current_player:
 * @gs: A #MateScores widget
 * @i: Index of the current(from 0 to 9).
 *
 * Description: Set the index of the current player in top ten.
 */
void mate_scores_set_current_player(MateScores* gs, gint i)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(i < gs->_priv->n_scores);

	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_names[i]), "CurrentPlayer");
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_scores[i]), "CurrentPlayer");
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->label_times[i]), "CurrentPlayer");
}

/**
 * mate_scores_set_logo_label_title:
 * @gs: A #MateScores widget
 * @txt: Name of the logo.
 *
 * Description: Creates a label to be the logo
 */
void mate_scores_set_logo_label_title(MateScores* gs, const gchar* txt)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(txt != NULL);

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = gtk_label_new(txt);
	gtk_widget_set_name(GTK_WIDGET(gs->_priv->logo), "Logo");
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);
	gtk_widget_show (gs->_priv->logo);
}


/**
 * mate_scores_set_logo_label:
 * @gs: A #MateScores widget
 * @txt: Text in the label.
 * @font: Font to use in the label.
 * @col: Color to use in the label.
 *
 * Description: Creates a label to be the logo
 */
void mate_scores_set_logo_label(MateScores* gs, const gchar* txt, const gchar* font, GdkColor* col)
{
	GtkStyle *s = gtk_style_new(); /* i believe that i should copy the default style
					  and change only the fg & font fields, how? */
	PangoFontDescription *font_desc;

	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	if(col)
		s->fg[0] = *col;

	font_desc = NULL;
	if (font != NULL)
		font_desc = pango_font_description_from_string (font);
	if (font_desc == NULL)
		font_desc = pango_font_description_from_string (_("Sans 14"));
	if (font_desc != NULL)
		s->font_desc = font_desc;

	gs->_priv->logo = gtk_label_new(txt);
	gtk_widget_set_style(GTK_WIDGET(gs->_priv->logo), s);
	g_object_unref (G_OBJECT (s));
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);
	gtk_widget_show (gs->_priv->logo);
}

/**
 * mate_scores_set_logo_widget:
 * @gs: A #MateScores widget
 * @w: Widget to be used as logo.
 *
 * Description:  Set an arbitrary widget to be the logo.
 */
void mate_scores_set_logo_widget(MateScores* gs, GtkWidget* w)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(w != NULL);
	g_return_if_fail(GTK_IS_WIDGET(w));

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = w;
	gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container), gs->_priv->logo);

	gtk_widget_show(w);
}

/**
 * mate_scores_set_logo_pixmap:
 * @gs: A #MateScores widget
 * @pix_name: filename of a pixmap
 *
 * Description:  Sets the logo on the scores dialog box to a pixmap
 */
void mate_scores_set_logo_pixmap(MateScores* gs, const gchar* pix_name)
{
	g_return_if_fail(gs != NULL);
	g_return_if_fail(MATE_IS_SCORES(gs));
	g_return_if_fail(pix_name != NULL);

	if(gs->_priv->logo) {
		gtk_widget_destroy(gs->_priv->logo);
		gs->_priv->logo = NULL;
	}

	gs->_priv->logo = gtk_image_new_from_file (pix_name);
	if (gs->_priv->logo != NULL) {
		gtk_container_add(GTK_CONTAINER(gs->_priv->logo_container),
				  gs->_priv->logo);
		gtk_widget_show (gs->_priv->logo);
	}
}

/**
 * mate_scores_display:
 * @title: Title of the app to be displayed as logo
 * @app_name: Name of the application, as in mate_score_init.
 * @level: Level of the game or %NULL.
 * @pos: Position in the top ten of the current player, as returned by mate_score_log.
 *
 * Description:  Does all the work of displaying the best scores.
 * It calls mate_score_get_notables to retrieve the info, creates the window,
 * and show it. If there are no scores nothing is displayed and the recommended practise
 * is to disable the scores menu entry in this case.
 *
 * USAGE:
 *
 * pos = mate_score_log(score, level, TRUE);
 * mate_scores_display (_("Mi game"), "migame", level, pos);
 *
 * Returns:  If a dialog is displayed return it's pointer.  It can also
 * be %NULL if no dialog is displayed
 */
GtkWidget* mate_scores_display(const gchar* title, const gchar* app_name, const gchar* level, int pos)
{
	GtkWidget *hs = NULL;
	gchar **names = NULL;
	gfloat *scores = NULL;
	time_t *scoretimes = NULL;
	gint top;

	top = mate_score_get_notable(app_name, level, &names, &scores, &scoretimes);
	if (top > 0) {
		hs = mate_scores_new(top, names, scores, scoretimes, 0);
		mate_scores_set_logo_label_title (MATE_SCORES(hs), title);
		if(pos)
 			mate_scores_set_current_player(MATE_SCORES(hs), pos-1);

		gtk_widget_show (hs);
	}

	g_strfreev(names);
	g_free(scores);
	g_free(scoretimes);

	return hs;
}

/**
 * mate_scores_display_with_pixmap:
 * @pixmap_logo: Filename of a logo pixmap to display
 * @app_name: Name of the application, as in mate_score_init.
 * @level: Level of the game or %NULL.
 * @pos: Position in the top ten of the current player, as returned by mate_score_log.
 *
 * Description:  Does all the work of displaying the best scores.
 * It calls mate_score_get_notables to retrieve the info, creates the window,
 * and show it.
 *
 * Returns:  If a dialog is displayed return it's pointer.  It can also
 * be %NULL if no dialog is displayed
 */
GtkWidget* mate_scores_display_with_pixmap(const gchar* pixmap_logo, const gchar* app_name, const gchar* level, int pos)
{
	GtkWidget *hs = NULL;
	gchar **names = NULL;
	gfloat *scores = NULL;
	time_t *scoretimes = NULL;
	gint top;

	top = mate_score_get_notable(app_name, level, &names, &scores, &scoretimes);
	if (top > 0) {
		hs = mate_scores_new(top, names, scores, scoretimes, 0);
		mate_scores_set_logo_pixmap (MATE_SCORES(hs), pixmap_logo);
		if(pos)
 			mate_scores_set_current_player(MATE_SCORES(hs), pos-1);

		gtk_widget_show (hs);
	}

	g_strfreev(names);
	g_free(scores);
	g_free(scoretimes);

	return hs;
}
