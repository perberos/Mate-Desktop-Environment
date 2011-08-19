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
 * Written by: Ray Strode <rstrode@redhat.com>
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
#include "gdm-languages.h"
#include "gdm-language-option-widget.h"
#include "gdm-recent-option-widget.h"
#include "gdm-language-chooser-dialog.h"

#define GDM_LANGUAGE_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_LANGUAGE_OPTION_WIDGET, GdmLanguageOptionWidgetPrivate))

struct GdmLanguageOptionWidgetPrivate
{
        GtkWidget *dialog;
};

enum {
        LANGUAGE_ACTIVATED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     gdm_language_option_widget_class_init  (GdmLanguageOptionWidgetClass *klass);
static void     gdm_language_option_widget_init        (GdmLanguageOptionWidget      *language_option_widget);
static void     gdm_language_option_widget_finalize    (GObject                      *object);
static void     gdm_language_option_widget_hide_dialog (GdmLanguageOptionWidget      *widget);

G_DEFINE_TYPE (GdmLanguageOptionWidget, gdm_language_option_widget, GDM_TYPE_RECENT_OPTION_WIDGET)

static void
gdm_language_option_widget_set_language_from_dialog (GdmLanguageOptionWidget *widget)
{
        char *language_name;

        language_name = gdm_language_chooser_dialog_get_current_language_name (GDM_LANGUAGE_CHOOSER_DIALOG (widget->priv->dialog));
        gdm_language_option_widget_set_current_language_name (widget, language_name);
        g_free (language_name);
}

static void
on_dialog_response (GtkDialog               *dialog,
                    int                      response_id,
                    GdmLanguageOptionWidget *widget)
{
        switch (response_id) {
                case GTK_RESPONSE_OK:
                        gdm_language_option_widget_set_language_from_dialog (widget);
                        break;

                default:
                        break;
        }

        gdm_language_option_widget_hide_dialog (widget);
}

static void
gdm_language_option_widget_hide_dialog (GdmLanguageOptionWidget *widget)
{
        gtk_widget_destroy (widget->priv->dialog);
        widget->priv->dialog = NULL;
}

static void
create_dialog (GdmLanguageOptionWidget *widget)
{
        gdm_profile_start (NULL);

        g_assert (widget->priv->dialog == NULL);

        widget->priv->dialog = gdm_language_chooser_dialog_new ();

        gdm_profile_end (NULL);
}

static void
gdm_language_option_widget_show_dialog (GdmLanguageOptionWidget *widget,
                                        const char              *active_item_id)
{
        if (widget->priv->dialog == NULL) {
                create_dialog (widget);
        }

        g_signal_connect (GTK_DIALOG (widget->priv->dialog),
                          "response",
                          G_CALLBACK (on_dialog_response),
                          widget);

        gtk_widget_show_all (GTK_WIDGET (widget->priv->dialog));

        gdm_language_chooser_dialog_set_current_language_name (GDM_LANGUAGE_CHOOSER_DIALOG (GDM_LANGUAGE_OPTION_WIDGET (widget)->priv->dialog),
                                                               active_item_id);
}

static void
gdm_language_option_widget_activated (GdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = gdm_option_widget_get_active_item (GDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        if (strcmp (active_item_id, "__other") == 0) {
                g_free (active_item_id);

                active_item_id = gdm_option_widget_get_default_item (widget);
                gdm_language_option_widget_set_current_language_name (GDM_LANGUAGE_OPTION_WIDGET (widget), active_item_id);
                gdm_language_option_widget_show_dialog (GDM_LANGUAGE_OPTION_WIDGET (widget), active_item_id);
        }

        g_signal_emit (G_OBJECT (widget), signals[LANGUAGE_ACTIVATED], 0);

        g_free (active_item_id);
}

static void
gdm_language_option_widget_class_init (GdmLanguageOptionWidgetClass *klass)
{
        GObjectClass         *object_class = G_OBJECT_CLASS (klass);
        GdmOptionWidgetClass *option_widget_class = GDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = gdm_language_option_widget_finalize;

        option_widget_class->activated = gdm_language_option_widget_activated;

        signals[LANGUAGE_ACTIVATED] = g_signal_new ("language-activated",
                                                     G_TYPE_FROM_CLASS (object_class),
                                                     G_SIGNAL_RUN_FIRST,
                                                     G_STRUCT_OFFSET (GdmLanguageOptionWidgetClass, language_activated),
                                                     NULL,
                                                     NULL,
                                                     g_cclosure_marshal_VOID__VOID,
                                                     G_TYPE_NONE,
                                                     0);

        g_type_class_add_private (klass, sizeof (GdmLanguageOptionWidgetPrivate));
}

static char *
gdm_language_option_widget_lookup_item (GdmRecentOptionWidget *widget,
                                        const char            *locale,
                                        char                 **name,
                                        char                 **comment)
{
        char *language;
        char *readable_language;
        char *lang_tag;
        char *normalized_locale;

        normalized_locale = gdm_normalize_language_name (locale);

        language = gdm_get_language_from_name (locale, normalized_locale);

        if (language == NULL) {
                g_free (normalized_locale);
                return NULL;
        }

        readable_language = gdm_get_language_from_name (locale, NULL);
        gdm_parse_language_name (locale, &lang_tag, NULL, NULL, NULL);
        *name = g_strdup_printf ("<span lang=\"%s\">%s</span>", lang_tag, language);
        *comment = readable_language;

        g_free (language);
        g_free (lang_tag);

        return normalized_locale;
}

static void
gdm_language_option_widget_init (GdmLanguageOptionWidget *widget)
{
        GError *error;

        widget->priv = GDM_LANGUAGE_OPTION_WIDGET_GET_PRIVATE (widget);

        error = NULL;
        gdm_recent_option_widget_set_mateconf_key (GDM_RECENT_OPTION_WIDGET (widget),
                                                "/apps/gdm/simple-greeter/recent-languages",
                                                gdm_language_option_widget_lookup_item,
                                                &error);

        if (error != NULL) {
                g_warning ("Could not read recent languages from mateconf: %s",
                           error->message);
                g_error_free (error);
        }

        gdm_option_widget_add_item (GDM_OPTION_WIDGET (widget),
                                    "__other",
                                    /* translators: This brings up a dialog
                                     * with a list of languages to choose from
                                     */
                                    C_("language", "Other…"),
                                    _("Choose a language from the "
                                      "full list of available languages."),
                                    GDM_OPTION_WIDGET_POSITION_BOTTOM);
}

static void
gdm_language_option_widget_finalize (GObject *object)
{
        GdmLanguageOptionWidget *language_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_LANGUAGE_OPTION_WIDGET (object));

        language_option_widget = GDM_LANGUAGE_OPTION_WIDGET (object);

        g_return_if_fail (language_option_widget->priv != NULL);

        if (language_option_widget->priv->dialog != NULL) {
                gtk_widget_destroy (language_option_widget->priv->dialog);
        }

        G_OBJECT_CLASS (gdm_language_option_widget_parent_class)->finalize (object);
}

GtkWidget *
gdm_language_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_LANGUAGE_OPTION_WIDGET,
                               "label-text", _("Language"),
                               "icon-name", "preferences-desktop-locale",
                               "max-item-count", 8,
                               NULL);

        return GTK_WIDGET (object);
}

char *
gdm_language_option_widget_get_current_language_name (GdmLanguageOptionWidget *widget)
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
gdm_language_option_widget_set_current_language_name (GdmLanguageOptionWidget *widget,
                                                      const char              *language_name)
{
        char *normalized_language_name;

        g_return_if_fail (GDM_IS_LANGUAGE_OPTION_WIDGET (widget));

        if (language_name != NULL) {
                normalized_language_name = gdm_normalize_language_name (language_name);
        } else {
                normalized_language_name = NULL;
        }

        if (normalized_language_name != NULL &&
            !gdm_option_widget_lookup_item (GDM_OPTION_WIDGET (widget),
                                            normalized_language_name, NULL, NULL, NULL)) {
                gdm_recent_option_widget_add_item (GDM_RECENT_OPTION_WIDGET (widget),
                                                   normalized_language_name);
        }

        gdm_option_widget_set_active_item (GDM_OPTION_WIDGET (widget), normalized_language_name);
        g_free (normalized_language_name);
}
