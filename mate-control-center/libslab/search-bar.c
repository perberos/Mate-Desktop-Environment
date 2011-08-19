/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "search-bar.h"
#include "config.h"

#include "search-entry.h"
#include "search-context-picker.h"
#include "nld-marshal.h"

#include <glib/gi18n-lib.h>

typedef struct
{
	GtkWidget *hbox;
	NldSearchContextPicker *context_picker;
	GtkEntry *entry;
	GtkWidget *button;

	int search_timeout;
	guint timeout_id;

	gboolean block_signal;
} NldSearchBarPrivate;

#define NLD_SEARCH_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NLD_TYPE_SEARCH_BAR, NldSearchBarPrivate))

static void nld_search_bar_class_init (NldSearchBarClass *);
static void nld_search_bar_init (NldSearchBar *);
static void nld_search_bar_finalize (GObject *);

static gboolean nld_search_bar_focus (GtkWidget *, GtkDirectionType);
static void nld_search_bar_grab_focus (GtkWidget *);

enum
{
	SEARCH,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NldSearchBar, nld_search_bar, GTK_TYPE_VBOX)

static void emit_search (NldSearchBar * search_bar);
static void emit_search_callback (GtkWidget * widget, gpointer search_bar);

static void nld_search_bar_class_init (NldSearchBarClass * nld_search_bar_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (nld_search_bar_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (nld_search_bar_class);

	object_class->finalize = nld_search_bar_finalize;
	widget_class->focus = nld_search_bar_focus;
	widget_class->grab_focus = nld_search_bar_grab_focus;

	g_type_class_add_private (nld_search_bar_class, sizeof (NldSearchBarPrivate));

	signals[SEARCH] =
		g_signal_new ("search", G_TYPE_FROM_CLASS (nld_search_bar_class),
		G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, G_STRUCT_OFFSET (NldSearchBarClass, search),
		NULL, NULL, nld_marshal_VOID__INT_STRING, G_TYPE_NONE, 2, G_TYPE_INT,
		G_TYPE_STRING);
}

static void
nld_search_bar_init (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);
	GtkWidget *alignment;
	GtkWidget *entry;

	GTK_WIDGET_SET_FLAGS (search_bar, GTK_CAN_FOCUS);

	priv->hbox = gtk_hbox_new (FALSE, 3);
	gtk_box_pack_start (GTK_BOX (search_bar), priv->hbox, TRUE, FALSE, 0);

	alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
	gtk_box_pack_start (GTK_BOX (priv->hbox), alignment, TRUE, TRUE, 0);

	entry = nld_search_entry_new ();
	priv->entry = GTK_ENTRY (entry);
	gtk_widget_show (entry);
	gtk_container_add (GTK_CONTAINER (alignment), entry);

	g_signal_connect (entry, "activate", G_CALLBACK (emit_search_callback), search_bar);

	priv->search_timeout = -1;
}

static void
nld_search_bar_finalize (GObject * object)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (object);

	if (priv->timeout_id)
		g_source_remove (priv->timeout_id);

	G_OBJECT_CLASS (nld_search_bar_parent_class)->finalize (object);
}

static gboolean
nld_search_bar_focus (GtkWidget * widget, GtkDirectionType dir)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (widget);

	return gtk_widget_child_focus (priv->hbox, dir);
}

gboolean
nld_search_bar_has_focus (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	return GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (priv->entry));
}

static void
nld_search_bar_grab_focus (GtkWidget * widget)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (widget);

	gtk_widget_grab_focus (GTK_WIDGET (priv->entry));
}

GtkWidget *
nld_search_bar_new (void)
{
	return g_object_new (NLD_TYPE_SEARCH_BAR, NULL);
}

void
nld_search_bar_clear (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	priv->block_signal = TRUE;
	gtk_entry_set_text (priv->entry, "");
	if (priv->context_picker)
		nld_search_context_picker_set_context (priv->context_picker, 0);
	priv->block_signal = FALSE;
}

static void
emit_search (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (priv->block_signal)
		return;

	if (priv->timeout_id)
	{
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	g_signal_emit (search_bar, signals[SEARCH], 0, nld_search_bar_get_context_id (search_bar),
		nld_search_bar_get_text (search_bar));
}

static void
emit_search_callback (GtkWidget * widget, gpointer search_bar)
{
	emit_search (search_bar);
}

gboolean
nld_search_bar_get_show_contexts (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	return priv->context_picker && GTK_WIDGET_VISIBLE (priv->context_picker);
}

static NldSearchContextPicker *
build_context_picker (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);
	GtkWidget *picker;

	picker = nld_search_context_picker_new ();
	g_signal_connect (picker, "context_changed", G_CALLBACK (emit_search_callback), search_bar);

	gtk_box_pack_start (GTK_BOX (priv->hbox), picker, 0, 0, FALSE);
	gtk_box_reorder_child (GTK_BOX (priv->hbox), picker, 0);

	return NLD_SEARCH_CONTEXT_PICKER (picker);
}

void
nld_search_bar_set_show_contexts (NldSearchBar * search_bar, gboolean show_contexts)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (show_contexts)
	{
		if (!priv->context_picker)
			priv->context_picker = build_context_picker (search_bar);
		gtk_widget_show (GTK_WIDGET (priv->context_picker));
	}
	else if (priv->context_picker)
		gtk_widget_hide (GTK_WIDGET (priv->context_picker));
}

void
nld_search_bar_add_context (NldSearchBar * search_bar, const char *label, const char *icon_name,
	int context_id)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (!priv->context_picker)
		priv->context_picker = build_context_picker (search_bar);

	nld_search_context_picker_add_context (priv->context_picker, label, icon_name, context_id);
}

gboolean
nld_search_bar_get_show_button (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	return priv->button != NULL;
}

void
nld_search_bar_set_show_button (NldSearchBar * search_bar, gboolean show_button)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (show_button)
	{
		GtkWidget *image;

		if (priv->button)
			return;

		priv->button = gtk_button_new_with_label (_("Find Now"));
		image = gtk_image_new_from_icon_name ("system-search", GTK_ICON_SIZE_MENU);
		gtk_button_set_image (GTK_BUTTON (priv->button), image);
		gtk_widget_show (priv->button);

		g_signal_connect (priv->button, "clicked", G_CALLBACK (emit_search_callback),
			search_bar);

		gtk_box_pack_end (GTK_BOX (priv->hbox), priv->button, FALSE, FALSE, 0);
	}
	else
	{
		if (!priv->button)
			return;

		gtk_widget_destroy (priv->button);
		priv->button = NULL;
	}
}

static gboolean
search_timeout (gpointer search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	priv->timeout_id = 0;
	emit_search (search_bar);
	return FALSE;
}

static void
entry_changed (GtkWidget * entry, gpointer search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (priv->search_timeout == 0)
		emit_search (search_bar);
	else if (priv->search_timeout > 0)
	{
		if (priv->timeout_id != 0)
			g_source_remove (priv->timeout_id);
		priv->timeout_id =
			g_timeout_add (priv->search_timeout * 1000, search_timeout, search_bar);
	}
}

int
nld_search_bar_get_search_timeout (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	return priv->search_timeout;
}

void
nld_search_bar_set_search_timeout (NldSearchBar * search_bar, int search_timeout)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (priv->search_timeout != -1 && search_timeout == -1)
		g_signal_handlers_disconnect_by_func (priv->entry, entry_changed, search_bar);
	else if (search_timeout != -1)
	{
		g_signal_connect (priv->entry, "changed", G_CALLBACK (entry_changed), search_bar);
	}

	priv->search_timeout = search_timeout;
}

const char *
nld_search_bar_get_text (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	return gtk_entry_get_text (priv->entry);
}

void
nld_search_bar_set_text (NldSearchBar * search_bar, const char *text, gboolean activate)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	gtk_entry_set_text (priv->entry, text);
	if (activate)
		emit_search (search_bar);
}

int
nld_search_bar_get_context_id (NldSearchBar * search_bar)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	if (priv->context_picker && GTK_WIDGET_VISIBLE (priv->context_picker))
		return nld_search_context_picker_get_context (priv->context_picker);
	else
		return -1;
}

void
nld_search_bar_set_context_id (NldSearchBar * search_bar, int context_id)
{
	NldSearchBarPrivate *priv = NLD_SEARCH_BAR_GET_PRIVATE (search_bar);

	g_return_if_fail (priv->context_picker != NULL);

	nld_search_context_picker_set_context (priv->context_picker, context_id);
}
