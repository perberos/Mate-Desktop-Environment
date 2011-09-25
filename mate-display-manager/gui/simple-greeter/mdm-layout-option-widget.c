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

#include "mdm-profile.h"
#include "mdm-layouts.h"
#include "mdm-layout-option-widget.h"
#include "mdm-recent-option-widget.h"
#include "mdm-layout-chooser-dialog.h"

#define MDM_LAYOUT_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LAYOUT_OPTION_WIDGET, MdmLayoutOptionWidgetPrivate))

struct MdmLayoutOptionWidgetPrivate
{
        GtkWidget *dialog;
};

enum {
        LAYOUT_ACTIVATED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     mdm_layout_option_widget_class_init  (MdmLayoutOptionWidgetClass *klass);
static void     mdm_layout_option_widget_init        (MdmLayoutOptionWidget      *layout_option_widget);
static void     mdm_layout_option_widget_finalize    (GObject                     *object);
static void     mdm_layout_option_widget_hide_dialog (MdmLayoutOptionWidget       *widget);

G_DEFINE_TYPE (MdmLayoutOptionWidget, mdm_layout_option_widget, MDM_TYPE_RECENT_OPTION_WIDGET)

static void
mdm_layout_option_widget_set_layout_from_dialog (MdmLayoutOptionWidget *widget)
{
        char *layout_name;

        layout_name = mdm_layout_chooser_dialog_get_current_layout_name (MDM_LAYOUT_CHOOSER_DIALOG (widget->priv->dialog));
        g_debug ("MdmLayoutOptionWidget: Setting layout from dialog: '%s'", layout_name);

        mdm_layout_option_widget_set_current_layout_name (widget, layout_name);
        g_free (layout_name);
}

static void
on_dialog_response (GtkDialog             *dialog,
                    int                    response_id,
                    MdmLayoutOptionWidget *widget)
{
        g_debug ("MdmLayoutOptionWidget: Got response from dialog: '%d'", response_id);

        switch (response_id) {
                case GTK_RESPONSE_OK:
                        mdm_layout_option_widget_set_layout_from_dialog (widget);
                        break;
                default:
                        break;
        }

        mdm_layout_option_widget_hide_dialog (widget);
}

static void
mdm_layout_option_widget_hide_dialog (MdmLayoutOptionWidget *widget)
{
        gtk_widget_destroy (widget->priv->dialog);
        widget->priv->dialog = NULL;
}

static void
create_dialog (MdmLayoutOptionWidget *widget)
{
        mdm_profile_start (NULL);

        g_assert (widget->priv->dialog == NULL);

        widget->priv->dialog = mdm_layout_chooser_dialog_new ();

        mdm_profile_end (NULL);
}

static void
mdm_layout_option_widget_show_dialog (MdmLayoutOptionWidget *widget,
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

        mdm_layout_chooser_dialog_set_current_layout_name (MDM_LAYOUT_CHOOSER_DIALOG (MDM_LAYOUT_OPTION_WIDGET (widget)->priv->dialog),
                                                           active_item_id);
}

static void
mdm_layout_option_widget_activated (MdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = mdm_option_widget_get_active_item (MDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        if (strcmp (active_item_id, "__other") == 0) {
                g_free (active_item_id);

                active_item_id = mdm_option_widget_get_default_item (widget);
                mdm_layout_option_widget_set_current_layout_name (MDM_LAYOUT_OPTION_WIDGET (widget), active_item_id);
                mdm_layout_option_widget_show_dialog (MDM_LAYOUT_OPTION_WIDGET (widget), active_item_id);
        }

        g_signal_emit (G_OBJECT (widget), signals[LAYOUT_ACTIVATED], 0);

        g_free (active_item_id);
}

static void
mdm_layout_option_widget_class_init (MdmLayoutOptionWidgetClass *klass)
{
        GObjectClass         *object_class = G_OBJECT_CLASS (klass);
        MdmOptionWidgetClass *option_widget_class = MDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = mdm_layout_option_widget_finalize;

        option_widget_class->activated = mdm_layout_option_widget_activated;

        signals[LAYOUT_ACTIVATED] = g_signal_new ("layout-activated",
                                                  G_TYPE_FROM_CLASS (object_class),
                                                  G_SIGNAL_RUN_FIRST,
                                                  G_STRUCT_OFFSET (MdmLayoutOptionWidgetClass, layout_activated),
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);

        g_type_class_add_private (klass, sizeof (MdmLayoutOptionWidgetPrivate));
}

static char *
mdm_layout_option_widget_lookup_item (MdmRecentOptionWidget *widget,
                                      const char            *key,
                                      char                 **name,
                                      char                 **comment)
{
        char *layout;

        layout = mdm_get_layout_from_name (key);

        if (layout == NULL) {
                return NULL;
        }

        *name = layout;
        *comment = NULL;

        return g_strdup (key);
}

static void
mdm_layout_option_widget_init (MdmLayoutOptionWidget *widget)
{
        GError *error;

        widget->priv = MDM_LAYOUT_OPTION_WIDGET_GET_PRIVATE (widget);

        error = NULL;
        mdm_recent_option_widget_set_mateconf_key (MDM_RECENT_OPTION_WIDGET (widget),
                                                "/apps/mdm/simple-greeter/recent-layouts",
                                                mdm_layout_option_widget_lookup_item,
                                                &error);

        if (error != NULL) {
                g_warning ("Could not read recent layouts from mateconf: %s",
                           error->message);
                g_error_free (error);
        }

        mdm_option_widget_add_item (MDM_OPTION_WIDGET (widget),
                                    "__other",
                                    /* translators: This brings up a dialog of
                                     * available keyboard layouts
                                     */
                                    C_("keyboard", "Other…"),
                                    _("Choose a keyboard layout from the "
                                      "full list of available layouts."),
                                    MDM_OPTION_WIDGET_POSITION_BOTTOM);
}

static void
mdm_layout_option_widget_finalize (GObject *object)
{
        MdmLayoutOptionWidget *layout_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LAYOUT_OPTION_WIDGET (object));

        layout_option_widget = MDM_LAYOUT_OPTION_WIDGET (object);

        g_return_if_fail (layout_option_widget->priv != NULL);

        if (layout_option_widget->priv->dialog != NULL) {
                gtk_widget_destroy (layout_option_widget->priv->dialog);
        }

        G_OBJECT_CLASS (mdm_layout_option_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_layout_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_LAYOUT_OPTION_WIDGET,
                               "label-text", _("Keyboard"),
                               "icon-name", "preferences-desktop-keyboard",
                               "max-item-count", 8,
                               NULL);

        return GTK_WIDGET (object);
}

char *
mdm_layout_option_widget_get_current_layout_name (MdmLayoutOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = mdm_option_widget_get_active_item (MDM_OPTION_WIDGET (widget));
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
mdm_layout_option_widget_set_current_layout_name (MdmLayoutOptionWidget *widget,
                                                  const char            *id)
{
        g_return_if_fail (MDM_IS_LAYOUT_OPTION_WIDGET (widget));

        if (id != NULL &&
            !mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (widget),
                                            id, NULL, NULL, NULL)) {
                mdm_recent_option_widget_add_item (MDM_RECENT_OPTION_WIDGET (widget),
                                                   id);
        }
        g_debug ("MdmLayoutOptionWidget: Setting active item: '%s'", id);
        mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (widget), id);
}
