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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Written by: Matthias Clasen <mclasen@redhat.com>
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "gdm-profile.h"
#include "gdm-layouts.h"
#include "gdm-layout-option-widget.h"
#include "gdm-recent-option-widget.h"
#include "gdm-layout-chooser-dialog.h"

#define GDM_LAYOUT_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_LAYOUT_OPTION_WIDGET, GdmLayoutOptionWidgetPrivate))

struct GdmLayoutOptionWidgetPrivate
{
        GtkWidget *dialog;
};

enum {
        LAYOUT_ACTIVATED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     gdm_layout_option_widget_class_init  (GdmLayoutOptionWidgetClass *klass);
static void     gdm_layout_option_widget_init        (GdmLayoutOptionWidget      *layout_option_widget);
static void     gdm_layout_option_widget_finalize    (GObject                     *object);
static void     gdm_layout_option_widget_hide_dialog (GdmLayoutOptionWidget       *widget);

G_DEFINE_TYPE (GdmLayoutOptionWidget, gdm_layout_option_widget, GDM_TYPE_RECENT_OPTION_WIDGET)

static void
gdm_layout_option_widget_set_layout_from_dialog (GdmLayoutOptionWidget *widget)
{
        char *layout_name;

        layout_name = gdm_layout_chooser_dialog_get_current_layout_name (GDM_LAYOUT_CHOOSER_DIALOG (widget->priv->dialog));
        g_debug ("GdmLayoutOptionWidget: Setting layout from dialog: '%s'", layout_name);

        gdm_layout_option_widget_set_current_layout_name (widget, layout_name);
        g_free (layout_name);
}

static void
on_dialog_response (GtkDialog             *dialog,
                    int                    response_id,
                    GdmLayoutOptionWidget *widget)
{
        g_debug ("GdmLayoutOptionWidget: Got response from dialog: '%d'", response_id);

        switch (response_id) {
                case GTK_RESPONSE_OK:
                        gdm_layout_option_widget_set_layout_from_dialog (widget);
                        break;
                default:
                        break;
        }

        gdm_layout_option_widget_hide_dialog (widget);
}

static void
gdm_layout_option_widget_hide_dialog (GdmLayoutOptionWidget *widget)
{
        gtk_widget_destroy (widget->priv->dialog);
        widget->priv->dialog = NULL;
}

static void
create_dialog (GdmLayoutOptionWidget *widget)
{
        gdm_profile_start (NULL);

        g_assert (widget->priv->dialog == NULL);

        widget->priv->dialog = gdm_layout_chooser_dialog_new ();

        gdm_profile_end (NULL);
}

static void
gdm_layout_option_widget_show_dialog (GdmLayoutOptionWidget *widget,
                                      const char            *active_item_id)
{
        if (widget->priv->dialog == NULL) {
                create_dialog (widget);
        }

        g_signal_connect (GTK_DIALOG (widget->priv->dialog),
                          "response",
                          G_CALLBACK (on_dialog_response),
                          widget);

        gtk_widget_show_all (GTK_WIDGET (widget->priv->dialog));

        gdm_layout_chooser_dialog_set_current_layout_name (GDM_LAYOUT_CHOOSER_DIALOG (GDM_LAYOUT_OPTION_WIDGET (widget)->priv->dialog),
                                                           active_item_id);
}

static void
gdm_layout_option_widget_activated (GdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = gdm_option_widget_get_active_item (GDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        if (strcmp (active_item_id, "__other") == 0) {
                g_free (active_item_id);

                active_item_id = gdm_option_widget_get_default_item (widget);
                gdm_layout_option_widget_set_current_layout_name (GDM_LAYOUT_OPTION_WIDGET (widget), active_item_id);
                gdm_layout_option_widget_show_dialog (GDM_LAYOUT_OPTION_WIDGET (widget), active_item_id);
        }

        g_signal_emit (G_OBJECT (widget), signals[LAYOUT_ACTIVATED], 0);

        g_free (active_item_id);
}

static void
gdm_layout_option_widget_class_init (GdmLayoutOptionWidgetClass *klass)
{
        GObjectClass         *object_class = G_OBJECT_CLASS (klass);
        GdmOptionWidgetClass *option_widget_class = GDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = gdm_layout_option_widget_finalize;

        option_widget_class->activated = gdm_layout_option_widget_activated;

        signals[LAYOUT_ACTIVATED] = g_signal_new ("layout-activated",
                                                  G_TYPE_FROM_CLASS (object_class),
                                                  G_SIGNAL_RUN_FIRST,
                                                  G_STRUCT_OFFSET (GdmLayoutOptionWidgetClass, layout_activated),
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);

        g_type_class_add_private (klass, sizeof (GdmLayoutOptionWidgetPrivate));
}

static char *
gdm_layout_option_widget_lookup_item (GdmRecentOptionWidget *widget,
                                      const char            *key,
                                      char                 **name,
                                      char                 **comment)
{
        char *layout;

        layout = gdm_get_layout_from_name (key);

        if (layout == NULL) {
                return NULL;
        }

        *name = layout;
        *comment = NULL;

        return g_strdup (key);
}

static void
gdm_layout_option_widget_init (GdmLayoutOptionWidget *widget)
{
        GError *error;

        widget->priv = GDM_LAYOUT_OPTION_WIDGET_GET_PRIVATE (widget);

        error = NULL;
        gdm_recent_option_widget_set_mateconf_key (GDM_RECENT_OPTION_WIDGET (widget),
                                                "/apps/gdm/simple-greeter/recent-layouts",
                                                gdm_layout_option_widget_lookup_item,
                                                &error);

        if (error != NULL) {
                g_warning ("Could not read recent layouts from mateconf: %s",
                           error->message);
                g_error_free (error);
        }

        gdm_option_widget_add_item (GDM_OPTION_WIDGET (widget),
                                    "__other",
                                    /* translators: This brings up a dialog of
                                     * available keyboard layouts
                                     */
                                    C_("keyboard", "Other…"),
                                    _("Choose a keyboard layout from the "
                                      "full list of available layouts."),
                                    GDM_OPTION_WIDGET_POSITION_BOTTOM);
}

static void
gdm_layout_option_widget_finalize (GObject *object)
{
        GdmLayoutOptionWidget *layout_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_LAYOUT_OPTION_WIDGET (object));

        layout_option_widget = GDM_LAYOUT_OPTION_WIDGET (object);

        g_return_if_fail (layout_option_widget->priv != NULL);

        if (layout_option_widget->priv->dialog != NULL) {
                gtk_widget_destroy (layout_option_widget->priv->dialog);
        }

        G_OBJECT_CLASS (gdm_layout_option_widget_parent_class)->finalize (object);
}

GtkWidget *
gdm_layout_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_LAYOUT_OPTION_WIDGET,
                               "label-text", _("Keyboard"),
                               "icon-name", "preferences-desktop-keyboard",
                               "max-item-count", 8,
                               NULL);

        return GTK_WIDGET (object);
}

char *
gdm_layout_option_widget_get_current_layout_name (GdmLayoutOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = gdm_option_widget_get_active_item (GDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return NULL;
        }

        if (strcmp (active_item_id, "__other") == 0) {
                g_free (active_item_id);
                return NULL;
        }

        return active_item_id;
}

void
gdm_layout_option_widget_set_current_layout_name (GdmLayoutOptionWidget *widget,
                                                  const char            *id)
{
        g_return_if_fail (GDM_IS_LAYOUT_OPTION_WIDGET (widget));

        if (id != NULL &&
            !gdm_option_widget_lookup_item (GDM_OPTION_WIDGET (widget),
                                            id, NULL, NULL, NULL)) {
                gdm_recent_option_widget_add_item (GDM_RECENT_OPTION_WIDGET (widget),
                                                   id);
        }
        g_debug ("GdmLayoutOptionWidget: Setting active item: '%s'", id);
        gdm_option_widget_set_active_item (GDM_OPTION_WIDGET (widget), id);
}
