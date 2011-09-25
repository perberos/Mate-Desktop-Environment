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

#include "mdm-profile.h"
#include "mdm-languages.h"
#include "mdm-language-option-widget.h"
#include "mdm-recent-option-widget.h"
#include "mdm-language-chooser-dialog.h"

#define MDM_LANGUAGE_OPTION_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LANGUAGE_OPTION_WIDGET, MdmLanguageOptionWidgetPrivate))

struct MdmLanguageOptionWidgetPrivate
{
        GtkWidget *dialog;
};

enum {
        LANGUAGE_ACTIVATED,
        NUMBER_OF_SIGNALS
};

static guint signals [NUMBER_OF_SIGNALS] = { 0, };

static void     mdm_language_option_widget_class_init  (MdmLanguageOptionWidgetClass *klass);
static void     mdm_language_option_widget_init        (MdmLanguageOptionWidget      *language_option_widget);
static void     mdm_language_option_widget_finalize    (GObject                      *object);
static void     mdm_language_option_widget_hide_dialog (MdmLanguageOptionWidget      *widget);

G_DEFINE_TYPE (MdmLanguageOptionWidget, mdm_language_option_widget, MDM_TYPE_RECENT_OPTION_WIDGET)

static void
mdm_language_option_widget_set_language_from_dialog (MdmLanguageOptionWidget *widget)
{
        char *language_name;

        language_name = mdm_language_chooser_dialog_get_current_language_name (MDM_LANGUAGE_CHOOSER_DIALOG (widget->priv->dialog));
        mdm_language_option_widget_set_current_language_name (widget, language_name);
        g_free (language_name);
}

static void
on_dialog_response (GtkDialog               *dialog,
                    int                      response_id,
                    MdmLanguageOptionWidget *widget)
{
        switch (response_id) {
                case GTK_RESPONSE_OK:
                        mdm_language_option_widget_set_language_from_dialog (widget);
                        break;

                default:
                        break;
        }

        mdm_language_option_widget_hide_dialog (widget);
}

static void
mdm_language_option_widget_hide_dialog (MdmLanguageOptionWidget *widget)
{
        gtk_widget_destroy (widget->priv->dialog);
        widget->priv->dialog = NULL;
}

static void
create_dialog (MdmLanguageOptionWidget *widget)
{
        mdm_profile_start (NULL);

        g_assert (widget->priv->dialog == NULL);

        widget->priv->dialog = mdm_language_chooser_dialog_new ();

        mdm_profile_end (NULL);
}

static void
mdm_language_option_widget_show_dialog (MdmLanguageOptionWidget *widget,
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

        mdm_language_chooser_dialog_set_current_language_name (MDM_LANGUAGE_CHOOSER_DIALOG (MDM_LANGUAGE_OPTION_WIDGET (widget)->priv->dialog),
                                                               active_item_id);
}

static void
mdm_language_option_widget_activated (MdmOptionWidget *widget)
{
        char *active_item_id;

        active_item_id = mdm_option_widget_get_active_item (MDM_OPTION_WIDGET (widget));
        if (active_item_id == NULL) {
                return;
        }

        if (strcmp (active_item_id, "__other") == 0) {
                g_free (active_item_id);

                active_item_id = mdm_option_widget_get_default_item (widget);
                mdm_language_option_widget_set_current_language_name (MDM_LANGUAGE_OPTION_WIDGET (widget), active_item_id);
                mdm_language_option_widget_show_dialog (MDM_LANGUAGE_OPTION_WIDGET (widget), active_item_id);
        }

        g_signal_emit (G_OBJECT (widget), signals[LANGUAGE_ACTIVATED], 0);

        g_free (active_item_id);
}

static void
mdm_language_option_widget_class_init (MdmLanguageOptionWidgetClass *klass)
{
        GObjectClass         *object_class = G_OBJECT_CLASS (klass);
        MdmOptionWidgetClass *option_widget_class = MDM_OPTION_WIDGET_CLASS (klass);

        object_class->finalize = mdm_language_option_widget_finalize;

        option_widget_class->activated = mdm_language_option_widget_activated;

        signals[LANGUAGE_ACTIVATED] = g_signal_new ("language-activated",
                                                     G_TYPE_FROM_CLASS (object_class),
                                                     G_SIGNAL_RUN_FIRST,
                                                     G_STRUCT_OFFSET (MdmLanguageOptionWidgetClass, language_activated),
                                                     NULL,
                                                     NULL,
                                                     g_cclosure_marshal_VOID__VOID,
                                                     G_TYPE_NONE,
                                                     0);

        g_type_class_add_private (klass, sizeof (MdmLanguageOptionWidgetPrivate));
}

static char *
mdm_language_option_widget_lookup_item (MdmRecentOptionWidget *widget,
                                        const char            *locale,
                                        char                 **name,
                                        char                 **comment)
{
        char *language;
        char *readable_language;
        char *lang_tag;
        char *normalized_locale;

        normalized_locale = mdm_normalize_language_name (locale);

        language = mdm_get_language_from_name (locale, normalized_locale);

        if (language == NULL) {
                g_free (normalized_locale);
                return NULL;
        }

        readable_language = mdm_get_language_from_name (locale, NULL);
        mdm_parse_language_name (locale, &lang_tag, NULL, NULL, NULL);
        *name = g_strdup_printf ("<span lang=\"%s\">%s</span>", lang_tag, language);
        *comment = readable_language;

        g_free (language);
        g_free (lang_tag);

        return normalized_locale;
}

static void
mdm_language_option_widget_init (MdmLanguageOptionWidget *widget)
{
        GError *error;

        widget->priv = MDM_LANGUAGE_OPTION_WIDGET_GET_PRIVATE (widget);

        error = NULL;
        mdm_recent_option_widget_set_mateconf_key (MDM_RECENT_OPTION_WIDGET (widget),
                                                "/apps/mdm/simple-greeter/recent-languages",
                                                mdm_language_option_widget_lookup_item,
                                                &error);

        if (error != NULL) {
                g_warning ("Could not read recent languages from mateconf: %s",
                           error->message);
                g_error_free (error);
        }

        mdm_option_widget_add_item (MDM_OPTION_WIDGET (widget),
                                    "__other",
                                    /* translators: This brings up a dialog
                                     * with a list of languages to choose from
                                     */
                                    C_("language", "Other…"),
                                    _("Choose a language from the "
                                      "full list of available languages."),
                                    MDM_OPTION_WIDGET_POSITION_BOTTOM);
}

static void
mdm_language_option_widget_finalize (GObject *object)
{
        MdmLanguageOptionWidget *language_option_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LANGUAGE_OPTION_WIDGET (object));

        language_option_widget = MDM_LANGUAGE_OPTION_WIDGET (object);

        g_return_if_fail (language_option_widget->priv != NULL);

        if (language_option_widget->priv->dialog != NULL) {
                gtk_widget_destroy (language_option_widget->priv->dialog);
        }

        G_OBJECT_CLASS (mdm_language_option_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_language_option_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_LANGUAGE_OPTION_WIDGET,
                               "label-text", _("Language"),
                               "icon-name", "preferences-desktop-locale",
                               "max-item-count", 8,
                               NULL);

        return GTK_WIDGET (object);
}

char *
mdm_language_option_widget_get_current_language_name (MdmLanguageOptionWidget *widget)
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
mdm_language_option_widget_set_current_language_name (MdmLanguageOptionWidget *widget,
                                                      const char              *language_name)
{
        char *normalized_language_name;

        g_return_if_fail (MDM_IS_LANGUAGE_OPTION_WIDGET (widget));

        if (language_name != NULL) {
                normalized_language_name = mdm_normalize_language_name (language_name);
        } else {
                normalized_language_name = NULL;
        }

        if (normalized_language_name != NULL &&
            !mdm_option_widget_lookup_item (MDM_OPTION_WIDGET (widget),
                                            normalized_language_name, NULL, NULL, NULL)) {
                mdm_recent_option_widget_add_item (MDM_RECENT_OPTION_WIDGET (widget),
                                                   normalized_language_name);
        }

        mdm_option_widget_set_active_item (MDM_OPTION_WIDGET (widget), normalized_language_name);
        g_free (normalized_language_name);
}
