/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "mdm-language-chooser-widget.h"
#include "mdm-language-chooser-dialog.h"

#define MDM_LANGUAGE_CHOOSER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LANGUAGE_CHOOSER_DIALOG, MdmLanguageChooserDialogPrivate))

struct MdmLanguageChooserDialogPrivate
{
        GtkWidget *chooser_widget;
};


static void     mdm_language_chooser_dialog_class_init  (MdmLanguageChooserDialogClass *klass);
static void     mdm_language_chooser_dialog_init        (MdmLanguageChooserDialog      *language_chooser_dialog);
static void     mdm_language_chooser_dialog_finalize    (GObject                       *object);

G_DEFINE_TYPE (MdmLanguageChooserDialog, mdm_language_chooser_dialog, GTK_TYPE_DIALOG)

char *
mdm_language_chooser_dialog_get_current_language_name (MdmLanguageChooserDialog *dialog)
{
        char *language_name;

        g_return_val_if_fail (MDM_IS_LANGUAGE_CHOOSER_DIALOG (dialog), NULL);

        language_name = mdm_language_chooser_widget_get_current_language_name (MDM_LANGUAGE_CHOOSER_WIDGET (dialog->priv->chooser_widget));

        return language_name;
}

void
mdm_language_chooser_dialog_set_current_language_name (MdmLanguageChooserDialog *dialog,
                                                       const char               *language_name)
{

        g_return_if_fail (MDM_IS_LANGUAGE_CHOOSER_DIALOG (dialog));

        mdm_language_chooser_widget_set_current_language_name (MDM_LANGUAGE_CHOOSER_WIDGET (dialog->priv->chooser_widget), language_name);
}

static void
mdm_language_chooser_dialog_size_request (GtkWidget      *widget,
                                       GtkRequisition *requisition)
{
        int            screen_w;
        int            screen_h;
        GtkRequisition child_requisition;

        if (GTK_WIDGET_CLASS (mdm_language_chooser_dialog_parent_class)->size_request) {
                GTK_WIDGET_CLASS (mdm_language_chooser_dialog_parent_class)->size_request (widget, requisition);
        }

        screen_w = gdk_screen_get_width (gtk_widget_get_screen (widget));
        screen_h = gdk_screen_get_height (gtk_widget_get_screen (widget));

        gtk_widget_get_child_requisition (gtk_bin_get_child (GTK_BIN (widget)), &child_requisition);
        *requisition = child_requisition;

        requisition->width += 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));
        requisition->height += 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        requisition->width = MIN (requisition->width, .50 * screen_w);
        requisition->height = MIN (requisition->height, .80 * screen_h);
}

static void
mdm_language_chooser_dialog_realize (GtkWidget *widget)
{
        MdmLanguageChooserDialog *chooser_dialog;
        GdkWindow *root_window;
        GdkCursor *cursor;

        root_window = gdk_screen_get_root_window (gdk_screen_get_default ());
        cursor = gdk_cursor_new (GDK_WATCH);
        gdk_window_set_cursor (root_window, cursor);
        gdk_cursor_unref (cursor);

        chooser_dialog = MDM_LANGUAGE_CHOOSER_DIALOG (widget);

        gtk_widget_show (chooser_dialog->priv->chooser_widget);

        GTK_WIDGET_CLASS (mdm_language_chooser_dialog_parent_class)->realize (widget);

        cursor = gdk_cursor_new (GDK_LEFT_PTR);
        gdk_window_set_cursor (root_window, cursor);
        gdk_cursor_unref (cursor);
}

static void
mdm_language_chooser_dialog_class_init (MdmLanguageChooserDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->finalize = mdm_language_chooser_dialog_finalize;
        widget_class->size_request = mdm_language_chooser_dialog_size_request;
        widget_class->realize = mdm_language_chooser_dialog_realize;

        g_type_class_add_private (klass, sizeof (MdmLanguageChooserDialogPrivate));
}

static gboolean
respond (MdmLanguageChooserDialog *dialog)
{
        gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
        return FALSE;
}

static void
queue_response (MdmLanguageChooserDialog *dialog)
{
        g_idle_add ((GSourceFunc) respond, dialog);
}

static void
mdm_language_chooser_dialog_init (MdmLanguageChooserDialog *dialog)
{

        dialog->priv = MDM_LANGUAGE_CHOOSER_DIALOG_GET_PRIVATE (dialog);

        dialog->priv->chooser_widget = mdm_language_chooser_widget_new ();
        mdm_chooser_widget_set_hide_inactive_items (MDM_CHOOSER_WIDGET (dialog->priv->chooser_widget),
                                                    FALSE);

        mdm_language_chooser_widget_set_current_language_name (MDM_LANGUAGE_CHOOSER_WIDGET (dialog->priv->chooser_widget),
                                                               setlocale (LC_MESSAGES, NULL));
        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), dialog->priv->chooser_widget);

        g_signal_connect_swapped (G_OBJECT (dialog->priv->chooser_widget),
                                  "activated", G_CALLBACK (queue_response),
                                  dialog);

        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OK, GTK_RESPONSE_OK,
                                NULL);

        gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
        gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);
        gtk_container_set_border_width (GTK_CONTAINER (dialog->priv->chooser_widget), 5);
        gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ALWAYS);
        gtk_window_set_default_size (GTK_WINDOW (dialog), 512, 440);
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}

static void
mdm_language_chooser_dialog_finalize (GObject *object)
{
        MdmLanguageChooserDialog *language_chooser_dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LANGUAGE_CHOOSER_DIALOG (object));

        language_chooser_dialog = MDM_LANGUAGE_CHOOSER_DIALOG (object);

        g_return_if_fail (language_chooser_dialog->priv != NULL);

        G_OBJECT_CLASS (mdm_language_chooser_dialog_parent_class)->finalize (object);
}

GtkWidget *
mdm_language_chooser_dialog_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_LANGUAGE_CHOOSER_DIALOG,
                               "icon-name", "preferences-desktop-locale",
                               "title", _("Languages"),
                               "border-width", 8,
                               "modal", TRUE,
                               NULL);

        return GTK_WIDGET (object);
}
