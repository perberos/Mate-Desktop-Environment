/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <unistd.h>
#include <sys/types.h>

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mdm-entry-menu-item.h"

/* same as twitter */
#define TEXT_BUFFER_MAX_CHARS 64

enum
{
        PROP_0,
};

struct _MdmEntryMenuItem
{
        GtkMenuItem      parent;

        GtkWidget       *hbox;
        GtkWidget       *image;
        GtkWidget       *entry;
};

struct _MdmEntryMenuItemClass
{
        GtkMenuItemClass parent_class;
};

G_DEFINE_TYPE (MdmEntryMenuItem, mdm_entry_menu_item, GTK_TYPE_MENU_ITEM)

static void
mdm_entry_menu_item_set_property (GObject      *object,
                                  guint         param_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        MdmEntryMenuItem *item;

        item = MDM_ENTRY_MENU_ITEM (object);

        switch (param_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }
}

static void
mdm_entry_menu_item_get_property (GObject    *object,
                                  guint       param_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        MdmEntryMenuItem *item;

        item = MDM_ENTRY_MENU_ITEM (object);

        switch (param_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }
}

static void
mdm_entry_menu_item_dispose (GObject *object)
{

        (*G_OBJECT_CLASS (mdm_entry_menu_item_parent_class)->dispose) (object);
}

static gboolean
mdm_entry_menu_item_button_release (GtkWidget      *widget,
                                    GdkEventButton *event)
{
        return TRUE;
}

/* Cut and paste from gtkwindow.c */
static void
send_focus_change (GtkWidget *widget,
                   gboolean   in)
{
        GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

        g_object_ref (widget);

        gtk_widget_set_can_focus (widget, in);

        fevent->focus_change.type = GDK_FOCUS_CHANGE;
        fevent->focus_change.window = g_object_ref (gtk_widget_get_window (widget));
        fevent->focus_change.in = in;

        gtk_widget_event (widget, fevent);

        g_object_notify (G_OBJECT (widget), "has-focus");

        g_object_unref (widget);
        gdk_event_free (fevent);
}

static gboolean
mdm_entry_menu_item_button_press (GtkWidget      *widget,
                                  GdkEventButton *event)
{
        GtkWidget *entry;

        entry = MDM_ENTRY_MENU_ITEM (widget)->entry;

        if (gtk_widget_get_window (entry) != NULL) {
                gdk_window_raise (gtk_widget_get_window (entry));
        }

        if (!gtk_widget_has_focus (entry)) {
                gtk_widget_grab_focus (entry);
        }

        return FALSE;
}

static void
mdm_entry_menu_item_realize (GtkWidget *widget)
{
        if (GTK_WIDGET_CLASS (mdm_entry_menu_item_parent_class)->realize) {
                GTK_WIDGET_CLASS (mdm_entry_menu_item_parent_class)->realize (widget);
        }
}

static void
mdm_entry_menu_item_select (GtkItem *item)
{
        g_return_if_fail (MDM_IS_ENTRY_MENU_ITEM (item));
        send_focus_change (GTK_WIDGET (MDM_ENTRY_MENU_ITEM (item)->entry), TRUE);
}

static void
mdm_entry_menu_item_deselect (GtkItem *item)
{
        g_return_if_fail (MDM_IS_ENTRY_MENU_ITEM (item));

        send_focus_change (GTK_WIDGET (MDM_ENTRY_MENU_ITEM (item)->entry), FALSE);
}

static void
mdm_entry_menu_item_class_init (MdmEntryMenuItemClass *klass)
{
        GObjectClass     *gobject_class;
        GtkWidgetClass   *widget_class;
        GtkMenuItemClass *menu_item_class;
        GtkItemClass     *item_class;

        gobject_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);
        menu_item_class = GTK_MENU_ITEM_CLASS (klass);
        item_class = GTK_ITEM_CLASS (klass);

        gobject_class->set_property = mdm_entry_menu_item_set_property;
        gobject_class->get_property = mdm_entry_menu_item_get_property;
        gobject_class->dispose = mdm_entry_menu_item_dispose;

        widget_class->button_release_event = mdm_entry_menu_item_button_release;
        widget_class->button_press_event = mdm_entry_menu_item_button_press;
        widget_class->realize = mdm_entry_menu_item_realize;

        item_class->select = mdm_entry_menu_item_select;
        item_class->deselect = mdm_entry_menu_item_deselect;

        menu_item_class->hide_on_activate = FALSE;
}

static void
on_entry_show (GtkWidget        *widget,
               MdmEntryMenuItem *item)
{
        if (gtk_widget_get_window (widget) != NULL) {
                gdk_window_raise (gtk_widget_get_window (widget));
        }
        send_focus_change (widget, TRUE);
}

static void
on_text_buffer_changed (GtkTextBuffer    *buffer,
                        MdmEntryMenuItem *item)
{
        int len;

        len = gtk_text_buffer_get_char_count (buffer);
        if (len > TEXT_BUFFER_MAX_CHARS) {
                gdk_window_beep (gtk_widget_get_window (GTK_WIDGET (item)));
        }
}

static void
on_entry_move_focus (GtkWidget        *widget,
                     GtkDirectionType  direction,
                     MdmEntryMenuItem *item)
{
        g_debug ("focus move");
        send_focus_change (GTK_WIDGET (MDM_ENTRY_MENU_ITEM (item)->entry), FALSE);
        g_signal_emit_by_name (item,
                               "move-focus",
                               GTK_DIR_TAB_FORWARD);
}

static void
mdm_entry_menu_item_init (MdmEntryMenuItem *item)
{
        PangoFontDescription *fontdesc;
        PangoFontMetrics     *metrics;
        PangoContext         *context;
        PangoLanguage        *lang;
        int                   ascent;
        GtkTextBuffer        *buffer;

        item->hbox = gtk_hbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (item), item->hbox);

        item->image = gtk_image_new ();
        gtk_box_pack_start (GTK_BOX (item->hbox), item->image, FALSE, FALSE, 0);

        item->entry = gtk_text_view_new ();
        gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (item->entry), FALSE);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (item->entry), TRUE);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (item->entry),
                                     GTK_WRAP_WORD);
        g_signal_connect (item->entry,
                          "show",
                          G_CALLBACK (on_entry_show),
                          item);
        g_signal_connect (item->entry,
                          "move-focus",
                          G_CALLBACK (on_entry_move_focus),
                          item);

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (item->entry));
        g_signal_connect (buffer,
                          "changed",
                          G_CALLBACK (on_text_buffer_changed),
                          item);

        /* get the font ascent for the current font and language */
        context = gtk_widget_get_pango_context (item->entry);
        fontdesc = pango_context_get_font_description (context);
        lang = pango_context_get_language (context);
        metrics = pango_context_get_metrics (context, fontdesc, lang);
        ascent = pango_font_metrics_get_ascent (metrics) * 1.5 / PANGO_SCALE;
        pango_font_metrics_unref (metrics);

        /* size our progress bar to be five ascents long */
        gtk_widget_set_size_request (item->entry, ascent * 5, -1);

        gtk_box_pack_start (GTK_BOX (item->hbox), item->entry, TRUE, TRUE, 0);

        gtk_widget_show (item->hbox);
        gtk_widget_show (item->image);
        gtk_widget_show (item->entry);
}

GtkWidget *
mdm_entry_menu_item_new (void)
{
        return g_object_new (MDM_TYPE_ENTRY_MENU_ITEM, NULL);
}

GtkWidget *
mdm_entry_menu_item_get_entry (MdmEntryMenuItem *item)
{
        g_return_val_if_fail (MDM_IS_ENTRY_MENU_ITEM (item), NULL);

        return item->entry;
}

GtkWidget *
mdm_entry_menu_item_get_image (MdmEntryMenuItem *item)
{
        g_return_val_if_fail (MDM_IS_ENTRY_MENU_ITEM (item), NULL);

        return item->image;
}
