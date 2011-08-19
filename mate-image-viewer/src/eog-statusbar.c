/* Eye of Mate - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnu.org>
 *	   Jens Finke <jens@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eog-statusbar.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define EOG_STATUSBAR_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_STATUSBAR, EogStatusbarPrivate))

G_DEFINE_TYPE (EogStatusbar, eog_statusbar, GTK_TYPE_STATUSBAR)

struct _EogStatusbarPrivate
{
	GtkWidget *progressbar;
	GtkWidget *img_num_statusbar;
};

static void
eog_statusbar_class_init (EogStatusbarClass *klass)
{
	GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (g_object_class, sizeof (EogStatusbarPrivate));
}

static void
eog_statusbar_init (EogStatusbar *statusbar)
{
	EogStatusbarPrivate *priv;
	GtkWidget *vbox;

	statusbar->priv = EOG_STATUSBAR_GET_PRIVATE (statusbar);
	priv = statusbar->priv;

	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar), TRUE);

	priv->img_num_statusbar = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (priv->img_num_statusbar), FALSE);
	gtk_widget_set_size_request (priv->img_num_statusbar, 100, 10);
	gtk_widget_show (priv->img_num_statusbar);

	gtk_box_pack_end (GTK_BOX (statusbar),
			  priv->img_num_statusbar,
			  FALSE,
			  TRUE,
			  0);

	vbox = gtk_vbox_new (FALSE, 0);

	gtk_box_pack_end (GTK_BOX (statusbar),
			  vbox,
			  FALSE,
			  FALSE,
			  2);

	statusbar->priv->progressbar = gtk_progress_bar_new ();

	gtk_box_pack_end (GTK_BOX (vbox),
			  priv->progressbar,
			  TRUE,
			  TRUE,
			  2);

	gtk_widget_set_size_request (priv->progressbar, -1, 10);

	gtk_widget_show (vbox);

	gtk_widget_hide (statusbar->priv->progressbar);

}

GtkWidget *
eog_statusbar_new (void)
{
	return GTK_WIDGET (g_object_new (EOG_TYPE_STATUSBAR, NULL));
}

void
eog_statusbar_set_image_number (EogStatusbar *statusbar,
                                gint          num,
				gint          tot)
{
	gchar *msg;

	g_return_if_fail (EOG_IS_STATUSBAR (statusbar));

	gtk_statusbar_pop (GTK_STATUSBAR (statusbar->priv->img_num_statusbar), 0);

	/* Translators: This string is displayed in the statusbar.
	 * The first token is the image number, the second is total image
	 * count.
	 *
	 * Translate to "%Id" if you want to use localized digits, or
	 * translate to "%d" otherwise.
	 *
	 * Note that translating this doesn't guarantee that you get localized
	 * digits. That needs support from your system and locale definition
	 * too.*/
	msg = g_strdup_printf (_("%d / %d"), num, tot);

	gtk_statusbar_push (GTK_STATUSBAR (statusbar->priv->img_num_statusbar), 0, msg);

      	g_free (msg);
}

void
eog_statusbar_set_progress (EogStatusbar *statusbar,
			    gdouble       progress)
{
	g_return_if_fail (EOG_IS_STATUSBAR (statusbar));

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (statusbar->priv->progressbar),
				       progress);

	if (progress > 0 && progress < 1) {
		gtk_widget_show (statusbar->priv->progressbar);
		gtk_widget_hide (statusbar->priv->img_num_statusbar);
	} else {
		gtk_widget_hide (statusbar->priv->progressbar);
		gtk_widget_show (statusbar->priv->img_num_statusbar);
	}
}

void
eog_statusbar_set_has_resize_grip (EogStatusbar *statusbar, gboolean has_resize_grip)
{
	g_return_if_fail (EOG_IS_STATUSBAR (statusbar));

	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar),
					   has_resize_grip);
}
