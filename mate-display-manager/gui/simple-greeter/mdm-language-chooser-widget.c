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
#include <errno.h>
#include <dirent.h>
#include <locale.h>
#include <sys/stat.h>

#include <fontconfig/fontconfig.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mdm-language-chooser-widget.h"
#include "mdm-chooser-widget.h"
#include "mdm-languages.h"

#define MDM_LANGUAGE_CHOOSER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LANGUAGE_CHOOSER_WIDGET, MdmLanguageChooserWidgetPrivate))

struct MdmLanguageChooserWidgetPrivate
{
        guint               languages_added : 1;
};

static void     mdm_language_chooser_widget_class_init  (MdmLanguageChooserWidgetClass *klass);
static void     mdm_language_chooser_widget_init        (MdmLanguageChooserWidget      *language_chooser_widget);
static void     mdm_language_chooser_widget_finalize    (GObject                       *object);

G_DEFINE_TYPE (MdmLanguageChooserWidget, mdm_language_chooser_widget, MDM_TYPE_CHOOSER_WIDGET)

enum {
        CHOOSER_LIST_TITLE_COLUMN = 0,
        CHOOSER_LIST_TRANSLATED_COLUMN,
        CHOOSER_LIST_LOCALE_COLUMN
};

char *
mdm_language_chooser_widget_get_current_language_name (MdmLanguageChooserWidget *widget)
{
        char *language_name;

        g_return_val_if_fail (MDM_IS_LANGUAGE_CHOOSER_WIDGET (widget), NULL);

        language_name = mdm_chooser_widget_get_selected_item (MDM_CHOOSER_WIDGET (widget));

        if (language_name == NULL) {
                language_name = g_strdup (setlocale (LC_MESSAGES, NULL));
        }

        return language_name;
}

void
mdm_language_chooser_widget_set_current_language_name (MdmLanguageChooserWidget *widget,
                                                       const char               *lang_name)
{
        char *name;

        g_return_if_fail (MDM_IS_LANGUAGE_CHOOSER_WIDGET (widget));

        if (lang_name == NULL) {
                mdm_chooser_widget_set_selected_item (MDM_CHOOSER_WIDGET (widget),
                                                      NULL);
                return;
        }

        name = mdm_normalize_language_name (lang_name);
        mdm_chooser_widget_set_selected_item (MDM_CHOOSER_WIDGET (widget),
                                              name);

        g_free (name);
}

static void
mdm_language_chooser_widget_add_language (MdmLanguageChooserWidget *widget,
                                          const char               *name)
{
        char *language;
        char *normalized_name;
        char *readable_language;
        char *lang_tag;
        char *tagged;

        normalized_name = mdm_normalize_language_name (name);
        mdm_parse_language_name (name, &lang_tag, NULL, NULL, NULL);
        language = mdm_get_language_from_name (normalized_name, normalized_name);
        readable_language = mdm_get_language_from_name (normalized_name, NULL);

        tagged = g_strdup_printf ("<span lang=\"%s\">%s</span>", lang_tag, language);

        if (language != NULL) {
                mdm_chooser_widget_add_item (MDM_CHOOSER_WIDGET (widget),
                                             normalized_name,
                                             NULL,
                                             tagged,
                                             readable_language,
                                             0,
                                             FALSE,
                                             FALSE,
                                             NULL,
                                             NULL);
                g_free (language);
        }
        g_free (readable_language);
        g_free (tagged);
        g_free (lang_tag);
        g_free (normalized_name);
}

static gboolean
language_has_font (const char *locale)
{
        const FcCharSet *charset;
        FcPattern       *pattern;
        FcObjectSet     *object_set;
        FcFontSet       *font_set;
        char            *language_code;
        gboolean         is_displayable;

        is_displayable = FALSE;
        pattern = NULL;
        object_set = NULL;
        font_set = NULL;

        mdm_parse_language_name (locale, &language_code, NULL, NULL, NULL);

        charset = FcLangGetCharSet ((FcChar8 *) language_code);
        if (!charset) {
                /* fontconfig does not know about this language */
                is_displayable = TRUE;
        } else {
                /* see if any fonts support rendering it */
                pattern = FcPatternBuild (NULL, FC_LANG, FcTypeString, language_code, NULL);

                if (pattern == NULL)
                        goto done;

                object_set = FcObjectSetCreate ();

                if (object_set == NULL)
                        goto done;

                font_set = FcFontList (NULL, pattern, object_set);

                if (font_set == NULL)
                        goto done;

                is_displayable = (font_set->nfont > 0);
        }

 done:

        if (font_set != NULL)
                FcFontSetDestroy (font_set);

        if (object_set != NULL)
                FcObjectSetDestroy (object_set);

        if (pattern != NULL)
                FcPatternDestroy (pattern);

        g_free (language_code);
        return is_displayable;
}

static void
add_available_languages (MdmLanguageChooserWidget *widget)
{
        char **language_names;
        int    i;

        language_names = mdm_get_all_language_names ();

        for (i = 0; language_names[i] != NULL; i++) {
                if (!language_has_font (language_names[i])) {
                        g_debug ("There is insufficient font coverage to display messages in the '%s' locale",
                                 language_names[i]);
                        continue;
                }
                mdm_language_chooser_widget_add_language (widget,
                                                          language_names[i]);
        }

        g_strfreev (language_names);
}

static void
mdm_language_chooser_widget_dispose (GObject *object)
{
        G_OBJECT_CLASS (mdm_language_chooser_widget_parent_class)->dispose (object);
}

static void
mdm_language_chooser_widget_realize (GtkWidget *widget)
{
        MdmLanguageChooserWidget *chooser;

        chooser = MDM_LANGUAGE_CHOOSER_WIDGET (widget);

        GTK_WIDGET_CLASS (mdm_language_chooser_widget_parent_class)->realize (widget);

        if (!chooser->priv->languages_added) {
                add_available_languages (chooser);
                chooser->priv->languages_added = TRUE;
        }
}

static void
mdm_language_chooser_widget_class_init (MdmLanguageChooserWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->dispose = mdm_language_chooser_widget_dispose;
        object_class->finalize = mdm_language_chooser_widget_finalize;
        widget_class->realize = mdm_language_chooser_widget_realize;

        g_type_class_add_private (klass, sizeof (MdmLanguageChooserWidgetPrivate));
}

static void
mdm_language_chooser_widget_init (MdmLanguageChooserWidget *widget)
{
        widget->priv = MDM_LANGUAGE_CHOOSER_WIDGET_GET_PRIVATE (widget);

        mdm_chooser_widget_set_separator_position (MDM_CHOOSER_WIDGET (widget),
                                                   MDM_CHOOSER_WIDGET_POSITION_TOP);
}

static void
mdm_language_chooser_widget_finalize (GObject *object)
{
        MdmLanguageChooserWidget *language_chooser_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LANGUAGE_CHOOSER_WIDGET (object));

        language_chooser_widget = MDM_LANGUAGE_CHOOSER_WIDGET (object);

        g_return_if_fail (language_chooser_widget->priv != NULL);

        G_OBJECT_CLASS (mdm_language_chooser_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_language_chooser_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_LANGUAGE_CHOOSER_WIDGET,
                               "inactive-text", _("_Languages:"),
                               "active-text", _("_Language:"),
                               NULL);

        return GTK_WIDGET (object);
}
