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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "mdm-user-chooser-widget.h"
#include "mdm-user-chooser-dialog.h"

#define MDM_USER_CHOOSER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_USER_CHOOSER_DIALOG, MdmUserChooserDialogPrivate))

struct MdmUserChooserDialogPrivate
{
        GtkWidget *chooser_widget;
};

enum {
        PROP_0,
};

static void     mdm_user_chooser_dialog_class_init  (MdmUserChooserDialogClass *klass);
static void     mdm_user_chooser_dialog_init        (MdmUserChooserDialog      *user_chooser_dialog);
static void     mdm_user_chooser_dialog_finalize    (GObject                       *object);

G_DEFINE_TYPE (MdmUserChooserDialog, mdm_user_chooser_dialog, GTK_TYPE_DIALOG)

char *
mdm_user_chooser_dialog_get_chosen_user_name (MdmUserChooserDialog *dialog)
{
        char *user_name;

        g_return_val_if_fail (MDM_IS_USER_CHOOSER_DIALOG (dialog), NULL);

        user_name = mdm_user_chooser_widget_get_chosen_user_name (MDM_USER_CHOOSER_WIDGET (dialog->priv->chooser_widget));

        return user_name;
}

void
mdm_user_chooser_dialog_set_show_user_guest (MdmUserChooserDialog *dialog,
                                             gboolean              show_user)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_DIALOG (dialog));

        mdm_user_chooser_widget_set_show_user_guest (MDM_USER_CHOOSER_WIDGET (dialog->priv->chooser_widget), show_user);
}

void
mdm_user_chooser_dialog_set_show_user_auto (MdmUserChooserDialog *dialog,
                                            gboolean              show_user)
{
        g_return_if_fail (MDM_IS_USER_CHOOSER_DIALOG (dialog));

        mdm_user_chooser_widget_set_show_user_auto (MDM_USER_CHOOSER_WIDGET (dialog->priv->chooser_widget), show_user);
}

static void
mdm_user_chooser_dialog_set_property (GObject        *object,
                                      guint           prop_id,
                                      const GValue   *value,
                                      GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_user_chooser_dialog_get_property (GObject        *object,
                                      guint           prop_id,
                                      GValue         *value,
                                      GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_user_chooser_dialog_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MdmUserChooserDialog      *user_chooser_dialog;

        user_chooser_dialog = MDM_USER_CHOOSER_DIALOG (G_OBJECT_CLASS (mdm_user_chooser_dialog_parent_class)->constructor (type,
                                                                                                                           n_construct_properties,
                                                                                                                           construct_properties));

        return G_OBJECT (user_chooser_dialog);
}

static void
mdm_user_chooser_dialog_dispose (GObject *object)
{
        G_OBJECT_CLASS (mdm_user_chooser_dialog_parent_class)->dispose (object);
}

static void
mdm_user_chooser_dialog_class_init (MdmUserChooserDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_user_chooser_dialog_get_property;
        object_class->set_property = mdm_user_chooser_dialog_set_property;
        object_class->constructor = mdm_user_chooser_dialog_constructor;
        object_class->dispose = mdm_user_chooser_dialog_dispose;
        object_class->finalize = mdm_user_chooser_dialog_finalize;

        g_type_class_add_private (klass, sizeof (MdmUserChooserDialogPrivate));
}

static void
on_response (MdmUserChooserDialog *dialog,
             gint                      response_id)
{
        switch (response_id) {
        default:
                break;
        }
}

static void
mdm_user_chooser_dialog_init (MdmUserChooserDialog *dialog)
{

        dialog->priv = MDM_USER_CHOOSER_DIALOG_GET_PRIVATE (dialog);

        dialog->priv->chooser_widget = mdm_user_chooser_widget_new ();

        gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), dialog->priv->chooser_widget);

        gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                GTK_STOCK_OK, GTK_RESPONSE_OK,
                                NULL);
        g_signal_connect (dialog,
                          "response",
                          G_CALLBACK (on_response),
                          dialog);

        gtk_widget_show_all (GTK_WIDGET (dialog));
}

static void
mdm_user_chooser_dialog_finalize (GObject *object)
{
        MdmUserChooserDialog *user_chooser_dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_USER_CHOOSER_DIALOG (object));

        user_chooser_dialog = MDM_USER_CHOOSER_DIALOG (object);

        g_return_if_fail (user_chooser_dialog->priv != NULL);

        G_OBJECT_CLASS (mdm_user_chooser_dialog_parent_class)->finalize (object);
}

GtkWidget *
mdm_user_chooser_dialog_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_USER_CHOOSER_DIALOG,
                               NULL);

        return GTK_WIDGET (object);
}
