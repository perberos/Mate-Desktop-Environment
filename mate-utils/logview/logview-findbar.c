/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-findbar.c - find toolbar for logview
 *
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#include "logview-findbar.h"

struct _LogviewFindbarPrivate {
  GtkWidget *entry;
  GtkWidget *message;

  GtkToolItem *clear_button;
  GtkToolItem *back_button;
  GtkToolItem *forward_button;
  GtkToolItem *status_item;
  GtkToolItem *separator;
  
  char *string;

  guint status_bold_id;
};

enum {
  PREVIOUS,
  NEXT,
  CLOSE,
  TEXT_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (LogviewFindbar, logview_findbar, GTK_TYPE_TOOLBAR);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_FINDBAR, LogviewFindbarPrivate))

static void
back_button_clicked_cb (GtkToolButton *button,
                        gpointer user_data)
{
  LogviewFindbar *findbar = user_data;

  g_signal_emit (findbar, signals[PREVIOUS], 0);
}

static void
forward_button_clicked_cb (GtkToolButton *button,
                           gpointer user_data)
{
  LogviewFindbar *findbar = user_data;

  g_signal_emit (findbar, signals[NEXT], 0);
}

static void
clear_button_clicked_cb (GtkToolButton *button,
                         gpointer user_data)
{
  LogviewFindbar *findbar = user_data;

  logview_findbar_set_message (findbar, NULL);
  gtk_entry_set_text (GTK_ENTRY (findbar->priv->entry), "");
}

static void
entry_activate_cb (GtkWidget *entry,
                   gpointer user_data)
{
  LogviewFindbar *findbar = user_data;

  g_signal_emit (findbar, signals[NEXT], 0);
}

static void
entry_changed_cb (GtkEditable *editable,
                  gpointer user_data)
{
  LogviewFindbar *findbar = user_data;
  const char *text;

  text = gtk_entry_get_text (GTK_ENTRY (editable));

  if (g_strcmp0 (text, "") == 0) {
    return;
  }

  if (g_strcmp0 (findbar->priv->string, text) != 0) {
    g_free (findbar->priv->string);
    findbar->priv->string = g_strdup (text);

    g_signal_emit (findbar, signals[TEXT_CHANGED], 0);
  }
}

static gboolean
entry_key_press_event_cb (GtkWidget *entry,
                          GdkEventKey *event,
                          gpointer user_data)
{
  LogviewFindbar *findbar = user_data;

  if (event->keyval == GDK_Escape) {
    g_signal_emit (findbar, signals[CLOSE], 0);
    return TRUE;
  }

  return FALSE;
}

static gboolean
unbold_timeout_cb (gpointer user_data)
{
  LogviewFindbar *findbar = user_data;
  PangoFontDescription *desc;

  desc = pango_font_description_new ();
  gtk_widget_modify_font (findbar->priv->message, desc);
  pango_font_description_free (desc);

  findbar->priv->status_bold_id = 0;

  return FALSE;
}

static void 
logview_findbar_init (LogviewFindbar *findbar)
{
  GtkWidget *label, *w, *box;
  GtkToolbar *gtoolbar;
  GtkToolItem *item;
  LogviewFindbarPrivate *priv;
  
  priv = findbar->priv = GET_PRIVATE (findbar);

  gtoolbar = GTK_TOOLBAR (findbar);

  gtk_toolbar_set_style (gtoolbar, GTK_TOOLBAR_BOTH_HORIZ);

  priv->status_bold_id = 0;

  /* Find: |_______| */
  w = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (w), 0, 0, 2, 2);

  box = gtk_hbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (w), box);

  label = gtk_label_new_with_mnemonic (_("_Find:"));
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

  priv->entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (priv->entry), 32);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->entry);
  gtk_box_pack_start (GTK_BOX (box), priv->entry, TRUE, TRUE, 0);

  item = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (item), w);
  gtk_toolbar_insert (gtoolbar, item, -1);
  gtk_widget_show_all (GTK_WIDGET (item));

  /* "Previous" and "Next" buttons */
  w = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  priv->back_button = gtk_tool_button_new (w, _("Find Previous"));
  gtk_tool_item_set_is_important (priv->back_button, TRUE);
  gtk_tool_item_set_tooltip_text (priv->back_button,
                                 _("Find previous occurrence of the search string"));
  gtk_toolbar_insert (gtoolbar, priv->back_button, -1);
  gtk_widget_show_all (GTK_WIDGET (priv->back_button));

  w = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
  priv->forward_button = gtk_tool_button_new (w, _("Find Next"));
  gtk_tool_item_set_is_important (priv->forward_button, TRUE);
  gtk_tool_item_set_tooltip_text (priv->forward_button,
                                 _("Find next occurrence of the search string"));
  gtk_toolbar_insert (gtoolbar, priv->forward_button, -1);
  gtk_widget_show_all (GTK_WIDGET (priv->forward_button));

  /* clear button */
  priv->clear_button = gtk_tool_button_new_from_stock (GTK_STOCK_CLEAR);
  gtk_tool_item_set_tooltip_text (priv->clear_button,
                                 _("Clear the search string"));
  gtk_toolbar_insert (gtoolbar, priv->clear_button, -1);
  gtk_widget_show_all (GTK_WIDGET (priv->clear_button));

  /* separator */
  priv->separator = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (gtoolbar, priv->separator, -1);

  /* message */
  priv->status_item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (priv->status_item, TRUE);
  priv->message = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (priv->message), TRUE);
  gtk_misc_set_alignment (GTK_MISC (priv->message), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (priv->status_item), priv->message);
  gtk_widget_show (priv->message);
  gtk_toolbar_insert (gtoolbar, priv->status_item, -1);

  priv->string = NULL;

  /* signal handlers */
  g_signal_connect (priv->back_button, "clicked",
                    G_CALLBACK (back_button_clicked_cb), findbar);
  g_signal_connect (priv->forward_button, "clicked",
                    G_CALLBACK (forward_button_clicked_cb), findbar);
  g_signal_connect (priv->clear_button, "clicked",
                    G_CALLBACK (clear_button_clicked_cb), findbar);
  g_signal_connect (priv->entry, "activate",
                    G_CALLBACK (entry_activate_cb), findbar);
  g_signal_connect (priv->entry, "changed",
                    G_CALLBACK (entry_changed_cb), findbar);
  g_signal_connect (priv->entry, "key-press-event",
                    G_CALLBACK (entry_key_press_event_cb), findbar);
}

static void
do_grab_focus (GtkWidget *widget)
{
  LogviewFindbar *findbar = LOGVIEW_FINDBAR (widget);

  gtk_widget_grab_focus (findbar->priv->entry);
}

static void
do_finalize (GObject *obj)
{
  LogviewFindbar *findbar = LOGVIEW_FINDBAR (obj);

  g_free (findbar->priv->string);

  G_OBJECT_CLASS (logview_findbar_parent_class)->finalize (obj);
}

static void
logview_findbar_class_init (LogviewFindbarClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);

  oclass->finalize = do_finalize;

  wclass->grab_focus = do_grab_focus;

  signals[PREVIOUS] = g_signal_new ("previous",
                                    G_OBJECT_CLASS_TYPE (oclass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (LogviewFindbarClass, previous),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);

  signals[NEXT] = g_signal_new ("next",
                                G_OBJECT_CLASS_TYPE (oclass),
                                G_SIGNAL_RUN_LAST,
                                G_STRUCT_OFFSET (LogviewFindbarClass, next),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__VOID,
                                G_TYPE_NONE, 0);

  signals[CLOSE] = g_signal_new ("close",
                                 G_OBJECT_CLASS_TYPE (oclass),
                                 G_SIGNAL_RUN_LAST,
                                 G_STRUCT_OFFSET (LogviewFindbarClass, close),
                                 NULL, NULL,
                                 g_cclosure_marshal_VOID__VOID,
                                 G_TYPE_NONE, 0);

  signals[TEXT_CHANGED] = g_signal_new ("text-changed",
                                        G_OBJECT_CLASS_TYPE (oclass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (LogviewFindbarClass, text_changed),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewFindbarPrivate));
}

/* public methods */

GtkWidget *
logview_findbar_new (void)
{
  GtkWidget *widget;
  widget = g_object_new (LOGVIEW_TYPE_FINDBAR, NULL);
  return widget;
}

void
logview_findbar_open (LogviewFindbar *findbar)
{
  g_assert (LOGVIEW_IS_FINDBAR (findbar));

  gtk_widget_show (GTK_WIDGET (findbar));
  gtk_widget_grab_focus (GTK_WIDGET (findbar));
}

const char *
logview_findbar_get_text (LogviewFindbar *findbar)
{
  g_assert (LOGVIEW_IS_FINDBAR (findbar));

  return findbar->priv->string;
}

void
logview_findbar_set_message (LogviewFindbar *findbar,
                             const char *text)
{
  PangoFontDescription *desc;

  g_assert (LOGVIEW_IS_FINDBAR (findbar));

  if (text) {
    desc = pango_font_description_new ();
    pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    gtk_widget_modify_font (findbar->priv->message, desc);
    pango_font_description_free (desc);
    
    findbar->priv->status_bold_id = g_timeout_add (600, unbold_timeout_cb, findbar);
  }

  gtk_label_set_text (GTK_LABEL (findbar->priv->message), 
                      text != NULL ? text : "");
  g_object_set (findbar->priv->separator, "visible", text != NULL, NULL);
  g_object_set (findbar->priv->status_item, "visible", text != NULL, NULL);
}
